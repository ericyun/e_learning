# ALSA驱动

整个Alsa体系分为用户空间和内核空间两部分，实现了很多强大的功能。从驱动开发的角度来看，基本只需要把注意力集中在跟硬件相关的部分就可以了。其它内容是Alsa体系维护者的事情。在内核中，Alsa为不同的类型的硬件分别提供了不同的封装形式，这里仅介绍主框架（不同类型硬件的实现就是对主框架的封装）和SOC框架。
１．基本概念
  帧（frame）：全部声道采样一次所得到的数据。以16位立体声来说，一帧的大小是2（声道） * 16（位/采样点） / 8（位/字节）＝ 4 （字节）。
  片断（period）：数据块，一般对应于一次中断所处理的数据。
  子码流（snd_pcm_substream）：通常对应回放（playback）或录音（record）。
  码流（snd_pcm）：可以包含很多子码流(snd_pcm_substream)，通常一个回放SNDRV_PCM_STREAM_PLAYBACK，一个录音SNDRV_PCM_STREAM_CAPTURE
  控制（snd_kcontrol）：通常用于音量调整等作用。
  声卡（snd_card）： 等同于硬件声卡的概念，它包含了码流及控制方面的信息。
  回调函数（snd_pcm_ops）：一个子码流在运行过程中所需要进行处理。
  运行时参数（snd_pcm_runtime）：保存运行过程中（指hw_params回调和hw_free回调之间）中的相关参数，如缓冲区（包括大小，位置，帧大小，片断大小）、通道数，采样精度，采样频率等。
  硬件能力参数（snd_pcm_hardware）：子码流硬件能力方面的相关参数，如所能支持的采样频率、采样精度，通道数量，缓冲大小等。
２．初始化
（1）、创建一块声卡（snd_card），调用snd_card_new，然后向该声卡中添加相关的码流和控制。
（2）、添加码流（snd_pcm），调用snd_pcm_new，注册码流回调函数（snd_pcm_ops）（回放和录音的回调函数是分别注册的），为码流预分配缓冲区，可以使用snd_pcm_lib_preallocate_pages_for_all，该函数会为所有子码流独立预分配缓冲区。
（3）、添加控制，就是调用snd_ctl_add注册snd_kcontrol_new数据结构。
（4）、注册该声卡，调用snd_card_register，成功后，该声卡的alsa驱动就可用了。
（5）、卸载声卡时，调用snd_card_free，alsa会自动删除码流（snd_pcm），控制（snd_kcontrol）和声卡（snd_card）。
３．运行
在运行过程中，Alsa会调用码流中注册的回调函数，这些回调函数使用的主要参数是子码流（snd_pcm_substream），及运行时参数（snd_pcm_runtime）。
可以通过substream->stream的值来区分是回放还是录音。除pointer回调外，其它回调都是通过返回0表示成功，非0表示失败。中断服务程序需要调用snd_pcm_period_elapsed函数，其作用相当于通知缓冲区空闲（对应回放）或者有效（对应录音）。
（1）、打开子码码流，对应回调函数中的open。该函数最主要的工作向运行时参数（snd_pcm_runtime）提供该子码流的硬件能力（snd_pcm_hardware），比如所能支持的采样率，采样精度等，通道数，缓冲区大小等。
（2）、设置参数，对应回调函数中的hw_params，在该函数内，所有目标参数如采样率、采样精度，通道数，缓冲区总大小等都有效，要求按照这些参数来配置硬件，分配缓冲区（当然也可以只记录下相关硬件参数，等到perpare时才进行实际配置，但其实没有必要）。实际缓冲区分配可以调用snd_pcm_lib_malloc_pages函数。在该函数被调用之后，运行时配置（snd_pcm_runtime）中的相关参数值才有效。
（3）、运行前准备，对应回调函数中的prepare，意味着该函数调用后，所有设置都已经就绪。
（4）、启动、停止，对应回调函数trigger，该函数的命令参数（cmd）表明是启动、停止、暂停、挂起还是恢复等。
（5）、读写位置反馈，对回调函数pointer，该函数返回数据的当前读取位置（对应回放）或数据的当前写入位置（对应录音），该位置是反复回绕的（环形缓冲区的用法）。
（6）、停止后清理，对应回调函数hw_free，在该函数中可以释放分配的缓冲区，可以调用snd_pcm_lib_free_pages函数。
（7）、关闭，对应回调函数close。
（8）、ioctl回调，目前的alsa版本（1.0.18a），要求必须给该回调赋值，其值可以使用snd_pcm_lib_ioctl函数。
（9）、ack回调，当alsa把数据写入缓冲区或从缓冲区读走后，会回调该函数，用于特殊的维护操作。

用户通过open打开子码流并获得该子码流的能力参数，从而确定出一套可用的参数。调用hw_params设置这些参数。调用prepare进球最后的准备工作，比如清除fifo。调用trigger启动或停止工作。通过pointer了解工作进展。工作停止后，调用hw_free释放资源，使用close关闭子码流。

目前的android会出现open --> hw_free --> close过程，用于处理音量设置等控制（因此在hw_freek 需要检查所需释放的资源是否有效）。
４．SOC框架
　　这种框架主要用于I2S接口，其特点是两个独立的芯片通过传输总线连接起来构成音频系统，一端称为CPU端，负责发送和接收数字信号，另一端称为CODEC端，负责AD或DA转换，该框架的主要目的是使两端相互独立:
　　　　--- 数字音频接口（snd_soc_dai）：用于描述CPU端或CODEC端的硬件能力。在运行过程中，SOC框架会综合两端的硬件能力，生成两端都可接收硬件能力参数（snd_pcm_hardware）。
　　　　--- 音频接口连接（snd_soc_dai_link）：通过该数据结构，可以描述两端的实际硬件连接。即哪个CPU端跟哪个CODEC端连在一起。
　　　　--- 平台（snd_soc_platform）：这个数据结构跟alsa框架中的码流（snd_pcm）对应，主要字义了相关的回调函数。
　　　　--- 卡片（snd_soc_card）：该结构可能是SOC中声卡的概念吧，这里译为卡片。该结构包含了音频接口连接（snd_soc_dai_link）和平台（snd_soc_platform）。
　　　　--- 设备（snd_soc_device）：该数据结构包含卡片（snd_soc_card），codec设备（snd_soc_codec_device），以及设备配置数据。

dev_qsdk/kernel/drivers/infotm/common/sound/configuration/imapx_ip6205.c
static struct snd_soc_ops imapx_ip6205_ops = {
     .hw_params = imapx_ip6205_hw_params,     .hw_free = imapx_ip6205_hw_free,     };

static struct snd_soc_dai_link imapx_ip6205_dai[] = {
[0] = {
     .name = "IP6205",.stream_name = "soc-audio ip6205 hifi",.codec_dai_name = "ip6205_dai",
     .codec_name = "ip6205.0",.ops = &imapx_ip6205_ops,},
};
static struct snd_soc_card ip6205 = {
     .name = "ip6205-iis",     .owner = THIS_MODULE,     .dai_link = imapx_ip6205_dai,
     .num_links = ARRAY_SIZE(imapx_ip6205_dai),
};
int imapx_ip6205_init(char *codec_name,char *cpu_dai_name, enum data_mode data, int enable, int id)
     imapx_ip6205_device = platform_device_alloc("soc-audio", -1);
     imapx_ip6205_dai[0].cpu_dai_name = cpu_dai_name;   imapx_ip6205_dai[0].platform_name = cpu_dai_name;
     platform_set_drvdata(imapx_ip6205_device, &ip6205);
     platform_device_add(imapx_ip6205_device);
static int imapx_ip6205_hw_params(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *params)
     ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |SND_SOC_DAIFMT_CBS_CFS);
          return dai->driver->ops->set_fmt(dai, fmt);
     ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |SND_SOC_DAIFMT_NB_IF | SND_SOC_DAIFMT_CBM_CFM);
     cpu_dai->driver->ops->set_sysclk(cpu_dai, iisdiv,params_rate(params), params_format(params));

void __init q3f_init_devices(void)
     imapx_hwcfg_fix_to_device(imapx_module_parse_status,ARRAY_SIZE(imapx_module_parse_status));
          if (!strcmp(audio[j].codec_name,imapx_audio_cfg[k].name)){
               imapx_audio_ctrl_register(&audio[j], k);
                    ctrl->codec_device->dev.platform_data = subdata;
                    platform_device_register(ctrl->codec_device/*imap_ip6205_device*/);
               imapx_audio_data_register(&audio[j], k);
          }

一。加载驱动，申请电源和时钟等资源，注册codec和dai的driver
     static int imapx_ip6205_probe(struct platform_device *pdev)　//ip6205.c
          snd_soc_register_codec(&pdev->dev, &imap_ip6205,&ip6205_dai, 1);
     static int __init imapx_ip6205_modinit(void)
          return platform_driver_register(&imapx_ip6205_driver);
     module_init(imapx_ip6205_modinit);
二。codec初始化，从devices.c中，item配置选中的模块会调用init函数
static struct snd_soc_ops imapx_ip6205_ops = {
     .hw_params = imapx_ip6205_hw_params,     .hw_free = imapx_ip6205_hw_free,};//设置hw参数
static struct snd_soc_dai_link imapx_ip6205_dai[] = {//范例数据结构
     [0] = {.name = "IP6205",.stream_name = "soc-audio ip6205 hifi",.codec_dai_name = "ip6205_dai",
     .codec_name = "ip6205.0",.ops = &imapx_ip6205_ops,},
};
static struct snd_soc_card ip6205 = {
     .name = "ip6205-iis",.owner = THIS_MODULE,.dai_link = imapx_ip6205_dai,.num_links = ARRAY_SIZE(imapx_ip6205_dai),};
//添加platform device,触发probe.对于ip2906来讲，
     ip2906.c文件中只有codec_driver的probe初始化register，其他都是空函数调用，因为它是slave的终端
     imapx_ip2906.c文件，看起来是ip2906的一个dai_link抽象，一个link会包含cpu和codec两端，两端都有snd_soc_dai_driver和snd_soc_dai_ops定义
int imapx_ip2906_init(char *codec_name,char *cpu_dai_name, enum data_mode data, int enable, int id)
     platform_set_drvdata(imapx_ip2906_device, &ip2906);     platform_device_add(imapx_ip2906_device);
int imapx_ip6205_init(char *codec_name,char *cpu_dai_name, enum data_mode data, int enable, int id)
     platform_set_drvdata(imapx_ip6205_device, &ip6205);     platform_device_add(imapx_ip6205_device);
int imapx_bt_init(char *codec_name, char *cpu_dai_name, enum data_mode data, int enable, int id)
     platform_set_drvdata(imapx_bt_device, &bt);     platform_device_add(imapx_bt_device);

二。

static int imapx_bt_codec_probe(struct platform_device *pdev)
     snd_soc_register_codec(&pdev->dev, &soc_codec_dev_bt, &bt_dai, 1);//注册bt的codec和dai，ops都是空的。
static int __init bt_modinit(void)
     return platform_driver_register(&imapx_bt_codec_driver);
module_init(bt_modinit);

soc_core.c中几个关键的list
static LIST_HEAD(dai_list);
static LIST_HEAD(platform_list);
static LIST_HEAD(codec_list);
static LIST_HEAD(component_list);

codec和dai通过dev匹配关联在一起

dev_qsdk/kernel/drivers/infotm/common/sound/imapx/imapx-i2s.c
static struct imapx_dma_client imapx_pch = {.name = "I2S PCM Stereo out"};
static struct imapx_dma_client imapx_pch_in = {.name = "I2S PCM Stereo in"};
static struct imapx_pcm_dma_params imapx_i2s_pcm_stereo_out = {//dma地址为iis的寄存器，16bits
     .client = &imapx_pch,.channel = IMAPX_I2S_MASTER_TX,.dma_addr = IMAP_IIS0_BASE + TXDMA,.dma_size = 2,};
static struct imapx_pcm_dma_params imapx_i2s_pcm_stereo_in = {
     .client = &imapx_pch_in,.channel = IMAPX_I2S_MASTER_RX,.dma_addr = IMAP_IIS0_BASE + RXDMA,.dma_size = 2,};

static int imapx_iis_dev_probe(struct platform_device *pdev)
     //分配clock和reg资源
     snd_soc_register_component(&pdev->dev, &imapx_i2s_component,&imapx_i2s_dai, 1);
     imapx_asoc_platform_probe(&pdev->dev);
          snd_soc_register_platform(dev, &imapx_soc_platform);

dev_qsdk/kernel/drivers/infotm/common/sound/imapx/imapx-pcm.c

static struct imapx_dma_client imapx_pcm_dma_client_out = {.name = "PCM Stereo out"};
static struct imapx_dma_client imapx_pcm_dma_client_in = {.name = "PCM Stereo in"};
static struct imapx_pcm_dma_params imapx_pcm_stereo_out[] = {//dma地址为pcm的寄存器，16bits
[0] = {.client = &imapx_pcm_dma_client_out,.channel = IMAPX_PCM0_TX,
     .dma_addr = IMAP_PCM0_BASE + IMAPX_PCM_TXFIFO,.dma_size = 2,},
[1] = {.client = &imapx_pcm_dma_client_out,     .channel = IMAPX_PCM1_TX,
     .dma_addr = IMAP_PCM1_BASE + IMAPX_PCM_TXFIFO,     .dma_size = 2,},
};
static struct imapx_pcm_dma_params imapx_pcm_stereo_in[] = {
[0] = {.client = &imapx_pcm_dma_client_in,     .channel = IMAPX_PCM0_RX,
     .dma_addr = IMAP_PCM0_BASE + IMAPX_PCM_RXFIFO,     .dma_size = 2,},
[1] = {.client = &imapx_pcm_dma_client_in,     .channel = IMAPX_PCM1_RX,
     .dma_addr = IMAP_PCM1_BASE + IMAPX_PCM_RXFIFO,     .dma_size = 2,},
};
static struct platform_driver imapx_pcm_driver = {
     .probe = imapx_pcm_probe,     .remove = imapx_pcm_remove,
     .driver = {.name = "imapx-pcm",.owner = THIS_MODULE,},
};
static struct snd_soc_dai_ops imapx_pcm_dai_ops = {
.set_sysclk = imapx_pcm_set_sysclk,
.trigger = imapx_pcm_trigger,
.hw_params = imapx_pcm_hw_params,
.set_fmt = imapx_pcm_set_fmt,
};

#define IMAPX_PCM_RATES SNDRV_PCM_RATE_8000_96000

#define IMAPX_PCM_DAI_DECLARE \
.symmetric_rates = 1, \
.ops = &imapx_pcm_dai_ops, \
.playback = { \
.channels_min = 1, \
.channels_max = 2, \
.rates = IMAPX_PCM_RATES, \
.formats = SNDRV_PCM_FMTBIT_S16_LE, \
}, \
.capture = { \
.channels_min = 1, \
.channels_max = 2, \
.rates = IMAPX_PCM_RATES, \
.formats = SNDRV_PCM_FMTBIT_S16_LE, \
}

struct snd_soc_dai_driver imapx_pcm_dai[] = {
[0] = {
.name = "imapx-pcm.0",
IMAPX_PCM_DAI_DECLARE,
},
[1] = {
.name = "imapx-pcm.1",
IMAPX_PCM_DAI_DECLARE,
}
};

static const struct snd_soc_component_driver imapx_pcm_component = {
.name = "imapx-pcm",
};

static int imapx_pcm_probe(struct platform_device *pdev)
     module_power_on(SYSMGR_PCM_BASE);     imapx_pad_init("pcm0/1");
     mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
     ioarea = request_mem_region(mem->start, resource_size(mem), pdev->name);
     pcm->regs = ioremap(mem->start, resource_size(mem));
     ret = snd_soc_register_component(&pdev->dev, &imapx_pcm_component,&imapx_pcm_dai[pdev->id], 1);
     ret = imapx_asoc_platform_probe(&pdev->dev);
     dev_set_drvdata(&pdev->dev, pcm);
     pcm->dma_playback = &imapx_pcm_stereo_out[pdev->id];     pcm->dma_capture = &imapx_pcm_stereo_in[pdev->id];
     imapx_pcm_disable_module(pcm);

module_platform_driver(imapx_pcm_driver);

dev_qsdk/kernel/drivers/infotm/common/sound/imapx/imapx-dma.c

static void imapx_audio_buffdone(void *data)//
     struct snd_pcm_substream *substream = data;
     struct imapx_runtime_data *prtd = substream->runtime->private_data;
     struct snd_card *card = substream->pstr->pcm->card;
     if (prtd->state & ST_RUNNING){
          if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
     }

static struct snd_pcm_ops imapx_pcm_ops = {
     .open = imapx_pcm_open,     .close = imapx_pcm_close,     .ioctl = snd_pcm_lib_ioctl,
     .hw_params = imapx_pcm_hw_params,     .hw_free = imapx_pcm_hw_free,     .prepare = imapx_pcm_prepare,
     .trigger = imapx_pcm_trigger,     .pointer = imapx_pcm_pointer,     .mmap = imapx_pcm_mmap,
};
static struct platform_driver soc_driver = {
     .driver = {  .name = "soc-audio",     .owner = THIS_MODULE,     .pm = &snd_soc_pm_ops, },
     .probe = soc_probe,     .remove = soc_remove,
};

static int soc_probe(struct platform_device *pdev)
     card->dev = &pdev->dev;
     return snd_soc_register_card(card);

static int __init snd_soc_init(void)
     snd_soc_util_init();
     return platform_driver_register(&soc_driver);

static int soc_pcm_open(struct snd_pcm_substream *substream)
     ret = cpu_dai->driver->ops->startup(substream, cpu_dai);
     ret = platform->driver->ops->open(substream);
     ret = codec_dai->driver->ops->startup(substream, codec_dai);
     ret = rtd->dai_link->ops->startup(substream);
static int soc_pcm_close(struct snd_pcm_substream *substream)
     cpu_dai->driver->ops->shutdown(substream, cpu_dai);
     codec_dai->driver->ops->shutdown(substream, codec_dai);
     rtd->dai_link->ops->shutdown(substream);
     platform->driver->ops->close(substream);
static int soc_pcm_prepare(struct snd_pcm_substream *substream)
     ret = rtd->dai_link->ops->prepare(substream);
     ret = platform->driver->ops->prepare(substream);
     ret = codec_dai->driver->ops->prepare(substream, codec_dai);
     ret = cpu_dai->driver->ops->prepare(substream, cpu_dai);
static int soc_pcm_hw_params(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *params)
static int soc_pcm_hw_free(struct snd_pcm_substream *substream)
static int soc_pcm_trigger(struct snd_pcm_substream *substream, int cmd)

ALSA pcm 读写

主要分成两种方式：buffer,和mmap，在这个基础上，又衍生出了根据IO中断和poll轮询来确定什么时候可读写两种不同的方式，他们都使用了注册事件的方法,至于事件的触发问题由内核系统来处理，我们不需要考虑。

int snd_async_add_pcm_handler(snd_async_handler_t **handler, snd_pcm_t *pcm, snd_async_callback_t callback, void *private_data)
这个函数为PCM增加了一个handle，设置了callback函数，安装了IO信号处理函数，IO信号发生时调用callback函数来处理读写。其中snd_async_handler_t 结构体中的glist没搞懂是做什么用的。
这种方式是不会产生Xrun的，所以不需要处理，但是当PCM的状态为SND_PCM_STATE_PREPARED的时候，还是要打开PCM的。

struct ip6205_reg_val ip6205_init_val[] = {}; //codec初始化寄存器列表

static struct snd_soc_codec_driver imap_ip6205 = {
     .probe = ip6205_probe,
     .remove = ip6205_remove,.suspend = ip6205_suspend,.resume = ip6205_resume,
     .set_bias_level = ip6205_set_bias_level,
};

static struct snd_soc_dai_driver ip6205_dai = {
.name = "ip6205_dai",
.playback = {
.stream_name = "Playback",
.channels_min = 1,
.channels_max = 2,
.rates = ip6205_RATES,
.formats = SNDRV_PCM_FMTBIT_S32_LE,
},
.capture = {
.stream_name = "Capture",
.channels_min = 1,
.channels_max = 2,
.rates = ip6205_RATES,
.formats = SNDRV_PCM_FMTBIT_S32_LE,
},
.ops = &ip6205_ops
};
static int ip6205_probe(struct snd_soc_codec *codec)
     ip6205_codec = codec;
     ret = ip6205_codec_init(codec);     //用ip6205_init_val[]寄存器数组初始化codec
     ret = ip6205_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
     ret = snd_soc_add_codec_controls(codec, ip6205_snd_controls,ARRAY_SIZE(ip6205_snd_controls));
     ret = device_create_file(codec->dev, &dev_attr_index_reg);

static int imapx_ip6205_probe(struct platform_device *pdev)　//设备核心函数probe
     ip6205->regulator = regulator_get(&pdev->dev, ip6205_cfg->power_pmu);
     ret = regulator_set_voltage(ip6205->regulator, 3300000, 3300000);
     regulator_enable(ip6205->regulator);
     platform_set_drvdata(pdev, ip6205);
     return snd_soc_register_codec(&pdev->dev, &imap_ip6205, &ip6205_dai, 1);
