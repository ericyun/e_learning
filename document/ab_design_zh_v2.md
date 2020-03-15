#Audiobox_v2 设计文档

### 适用产品

| 类别 | 适用对象 |
| --- |
| 软件版本 | `QSDK-V2.4.0` |
| 芯片型号 | `Apollo` `Apollo-2` `Apollo-ECO` |

### 修订记录

| 修订说明 | 日期 | 作者 | 说明 |
| --- |
| 初版 | 2019/02/22 | 员清观 | audiobox升级到v2.0, 新建文档 |
| v2.1 | 2019/07/28 | 员清观 | audiobox增加多个api接口,并强化测试和调试,更新文档 |

### 术语解释

| 术语 | 解释 |
| --- |
| QSDK | 盈方微Apollo系列芯片软件开发套件 |
| Audiobox | QSDK中音频处理模块 |
| API | Aplication Program Interface，应用程序接口 |
| AEC | Acoustic Echo Cancellation，回声消除 |
| AEC1.0 | 通过配置codec寄存器或者硬件连线环回的方式，将1路音频输出环回为1路音频输入，作为aec算法的参考音的方式 |
| AEC2.0 | 软件环回的方式，在dma回调函数中将音频输出添加到音频输入数据流中 |
| Eventhub  | QSDK中负责audiobox和videobox消息转发机制的进程 |

----
## 1. Audiobox 基础
Audiobox 是QSDK的音频子系统，提供音频的录音和放音功能支持，同时支持静音(Mute)控制、音量(Volume)控制、音频流格式(Format)控制、回声消除(AEC)控制等音频通道参数管理功能。另外还提供 abctrl 工具用来测试 Audiobox 各项功能

此文档针对Audiobox子系统的开发人员，对Audiobox的内部机制进行了简要介绍

<img src="image/ab_system_structure.dot.svg"/>

----
## 2. 基本功能架构
### 2.1 基本需求
初版的 Audiobox 版本功能比较单一，通过音频通道只能获取到PCM格式音频，录音和放音通道的参数也必须保持一致，而且实际上只能支持一路录音，无法满足复杂应用场景的开发需求。因此，我们以新的框架重构了 Audiobox 模块。本文后续描述中，将以`audiobox_v1`表示初版，以`audiobox_v2`表示新版本。`audiobox_v2`保持完全向前兼容，原有的上层应用程序，无需任何修改即可从`audiobox_v1`升级到`audiobox_v2`

`audiobox_v2`除了基本的录音和放音需求外，还需要满足如下设计要求：
- 集成多种codec，可以申请带编解码的通道
- 录音通道和放音通道格式可以不同，通道参数可以和设备参数不一致，可以支持多个通道同时录音放音，并且各个通道可以采用不同参数
- 相同参数的录音通道可共享编码处理，以节省开销
- 支持AEC功能，支持基于DSP的alango算法和基于ARM的speex算法，支持AEC1.0和可配置延时的AEC2.0
- 申请录音通道时可以指定从最老帧还是最新帧开始获取音频流
- 可以申请4声道(包含两个录音声道和两个放音声道)格式的录音通道

要升级到最新的`audiobox_v2`，system、kernel和buildroot都需要同步更新。system部分除Audiobox模块版本升级外，还删除了对vcp7g模块的依赖，并调整了hlibdsp和hlibunitrace模块；buildroot/package中增加了speexdsp支持；kernel中infotm驱动优化了ceva-dsp和sound模块

----
### 2.2 基于Eventhub进程实现的API接口
Audiobox提供了一整套API接口给其他进程调用，这些API调用基于进程间消息队列机制实现，主要函数如下，具体请查看`audiobox.h`头文件
```cpp
int audio_start_service(void);
int audio_stop_service(void);
int audio_get_format(const char *dev, audio_fmt_t *fmt);
int audio_set_format(const char *dev, audio_fmt_t *fmt);
int audio_get_master_volume(const char *dev, ...);//ignore origional parameter: int dir
int audio_set_master_volume(const char *dev, int volume, ...);//ignore origional parameter: int dir
int audio_get_channel_ex(const char *dev, audio_chn_fmt_t *chn_fmt, int flag);
int audio_enhance_mode(void);
int audio_get_channel(const char *dev, audio_fmt_t *fmt, int flag);
int audio_put_channel(int handle);
int audio_get_mute(int handle);
int audio_set_mute(int handle, int mute);
int audio_get_volume(int handle);
int audio_set_volume(int handle, int volume);
int audio_get_frame(int handle, struct fr_buf_info *buf);
int audio_put_frame(int handle, struct fr_buf_info *buf);
int audio_read_frame(int handle, char *buf, int size);
int audio_write_frame(int handle, char *buf, int size);
int audio_enable_aec(int handle, int enable);

/* v2.1文档对应的新增api接口*/
int audio_get_master_volume_ex(const char *dev, int index);
int audio_set_master_volume_ex(const char *dev, int index, int volume);
int audio_pause_channel(int handle, int flag);
int audio_pause_device(const char *dev, int enable);
int audio_get_statistics(int handle, struct channel_statistic *statistic);
int audio_get_delay(int handle);
int audio_playbuf_empty(int handle);
int audio_query_channel(int handle, audio_chan_info_t *chn_info);
int audio_get_dbginfo(audio_dbg_info_t* devinfo);
int audio_show_devinfo(const char *dev);
int audio_show_chninfo(int handle);
int audio_get_initial_devpara(dev_attr_setting_t *para);
int audio_set_audioboxpara(const char *dev, audiobox_setting_t* para, int setting_type);
int audio_set_devpara(const char *dev, dev_attr_setting_t *para);
int audio_set_tracepara(const char *dev, trace_setting_t *para);
int audio_set_aecfreq(const char *dev, int freq);
int audio_set_alsadepth(const char *dev, int depth);
```

典型的应用中，应用进程、Audiobox进程、Eventhub进程在启动过程中会分别以自身进程的pid为key创建消息队列。启动阶段，Audiobox进程在`audiobox_listener_init（）`函数中调用`event_register_handler()`子函数向eventhub进程注册，声明自己需要eventhub转发消息，并创建一个线程侦听自身消息队列。当应用进程调用Audiobox的API接口时，最终会发送一个消息到eventhub消息队列，eventhub把这个消息转发到audiobox消息队列。audiobox从消息队列获取消息后，根据消息类型调用合适的回调函数处理。应用API的回调函数是`audiobox_listener()`，接着它会为不同的命令调用不同的功能函数

例如，应用进程调用`audio_get_channel()`函数，最终将调用`event_rpc_call()`函数发送`AB_GET_CHANNEL`命令到eventhub消息队列；eventhub转发此消息到audiobox消息队列；audiobox从消息队列中读取到消息后，调用`audiobox_listener()`函数，其中调用`audiobox_get_channel()`子函数处理`AB_GET_CHANNEL`命令，创建音频通道

----
## 3. 音频通道管理
### 3.1 基本数据结构
本小节详细描述了三个基本的数据结构`audio_dev_t`、`audio_codec_t`、`audio_chn_t`，以及它们是如何实现音频通道管理的

<img src="image/ab_devlist_graph.dot.svg"/>

上图是依次创建如下音频通道时内部数据结构图：1. G711A录音通道；2. G711A录音通道(参数同1)；3. AAC录音通道；4. AAC录音通道(参数同3)；5. G711A放音通道；6. G711A放音通道。从图中可以看出，参数相同的两个录音通道(chan_0和chan_1，chan_2和chan_3)共享一个codec，而参数相同的放音通道(chan_4和chan_5)不会共享codec

**audio_dev_t结构体**

`audio_dev_t`定义对应一个音频设备：
- `node`成员 接入`audiobox_devlist`链表；添加设备节点的操作由`devctrl_create_dev()`函数中`list_add_tail(&dev->node, &audiobox_devlist);`完成
- `head`成员 维护一个本音频设备的通道链表，每个节点对应一个`audio_chn_t`定义的音频通道；添加通道节点的操作由`audiobox_get_channel()`函数中`list_add_tail(&chn->node, &dev->head);`完成
- `apulist`成员 维护一个本音频设备的`codec apu`链表，每个节点对应一个`apu_codec_t`定义的`code apu`；添加节点的操作由`__apu_codec_create()`函数中`list_add_tail(&p_codec->node, &dev->apulist);`完成

| 成员变量 | 描述 |
| --- |
| char devname[32]; | 设备名称 |
| snd_pcm_t *handle; | PCM handle |
| snd_ctl_t *ctl; | CTL handle |
| audio_dev_attr_t attr; | 设备参数 |
| pthread_mutex_t dev_mutex; | 设备通道和codec链表同步保护 |
| pthread_mutex_t dev_state_mutex; | 主线程与录音放音线程间state切换同步保护 |
| pthread_cond_t  dev_cond; |  |
| int direct; | 两个取值：DEVICE_OUT_ALL-放音，DEVICE_IN_ALL-录音 |
| int m_volume; | 设备master-volume主音量 |
| int foreground_chan_counter; | 放音设备上前景播放的通道的个数 |
| pthread_t hal_server_id; | 音频底层server线程的id，录音时从alsa读音频流，放音时向alsa写音频流 |
| int hal_state; | 0: pthread not created 1:working 2: delayed destroy. |
| int dev_fr_size; | dev_fr buffer长度 |
| int dev_fr_num; | dev fr buffer个数 |
| int fr_total_size; | 当前设备已申请的fr缓冲区总大小 |
| struct _apu_frnode_ dev_fr_para; | dev_fr 参数 |
| struct _apu_frnode_ dev_fr_para_mapped; | dev_fr_mapped 参数 |
| struct _apu_frnode_ dev_preprocfr_para; | dev_preprocfr 参数 |
| apu_aec_T preproc; | aec管理结构体 |
| pthread_t preproc_pid; | aec线程id |
| int aecapu_state; | aec线程控制 |
| int preproc_enabled; | aec使能控制 |
| int preproc_actived; | At least one channel have request to read frame |
| int aecvx_select; | AEC_VERSION_V0:none, AEC_VERSION_V1:aecv1, AEC_VERSION_V2:aecv2 |
| pthread_t decoder_pid; | 放音时解码线程id |
| int decoder_state; | 放音时解码线程状态控制 |
| int trace_mode;  | trace控制模式 |
| int trace_size; | 已经保存的trace的长度 |
| int trace_fd;  | 当前trace文件的fd |
| int devfr_counter;  | 和alsa交互的period计数 |
| int aecfr_counter;  | aec模块处理次数 |
| int devfr_counter_mapped;  | chan_remap模块处理次数 |
|  |  |
| **struct list_head node;** | dev链表节点 |
| **struct list_head head;** | 通道链表头 |
| **struct list_head apulist;** | codec链表头 |

**apu_codec_t结构体**

`apu_codec_t`结构体定义对应一个`codec apu`：
- `node`成员 接入音频设备的`apulist`链表
- `vchanlist`成员 维护一个引用本`codec apu`的通道链表，每个节点对应一个`audio_chn_t`定义的音频通道，这个链表上的所有通道具有相同的通道参数；添加通道节点的操作由`__apu_codec_create()`函数中`list_add_tail(&chn->chn_node, &p_codec->vchanlist);`完成

| 成员变量 | 描述 |
| --- |
| int		apu_state; | apu线程状态管理，0:init 1:running 2:pause  -1:stop |
| pthread_t apu_pid; | apu线程id |
| int		ref_counter; | number of capture channels sharing this codec. |
| int		apu_activated; | at least one referring channel had request to read a frame. |
| char *infrname; | codec输入fr名称 |
| char *outfrname; | codec输出fr名称 |
| void	*softvol_handle; | softvol handle |
| int 	softvol_enabled; | 0: no softvol 1: set capture/playback master volume, set playback channel volume |
| struct fr_buf_info softvol_frinfo; | softvol process out buffer, virtual fr |
| int insize; | recombine处理输入音频帧大小 |
| int outsize; | recombine处理输出音频帧大小 |
| int recombine_enabled; | 0: no recombine mudule. 1: recombine mudule enabled |
| struct fr_buf_info recombine_frinfo; |  |
| void 	*codec_handle; | codec handle |
| struct _apu_frnode_ codec_fr_para; | encoder and decoder out fr parameter |
| struct _apu_frnode_ *infr_para; | refer to input fr: channel's channel_fr |
| struct _apu_frnode_ *outfr_para; | refer to output fr: codec_fr_para |
| struct fr_buf_info infr_ref; | for read from input channel fr |
| struct fr_buf_info outfr_buf; | for write to output codec fr |
| struct fr_buf_info outfr_ref; | for read from output codec fr |
| audio_chn_fmt_t fmt; | codec对应通道参数 |
| audio_dev_t dev; | codec归属设备 |
| int chan_id; | 创建此codec的通道id |
| int apu_vol; | caculated softvol volume value. |
| pthread_mutex_t vchan_mutex; | |
| int is_prepared; | code initialized |
|  |  |
| **struct list_head node;** | 设备codec链表中节点 |
| **struct list_head vchanlist;** | list of capture channels referring to this codec |

**audio_chn_t结构体定义**

`audio_chn_t`定义对应一个音频通道：
- `node`成员 接入音频通道对应音频设备的`head`链表
- `chn_node`成员 接入本通道对应的`codec apu`的`vchanlist`链表

| 成员变量 | 描述 |
| --- |
| char chnname[32]; | 通道名 |
| audio_chn_fmt_t fmt; | 通道参数 |
| int flag; | 通道flag，计算得到下面timeout和priority成员 |
| int timeout; | 计算：(flag & TIMEOUT_MASK)? PLAYBACK_TIMEOUT : 0; |
| int priority; | 计算：(flag & PRIORITY_MASK) >> PRIORITY_BIT; |
| int direct; | 两个取值：DEVICE_OUT_ALL-放音，DEVICE_IN_ALL-录音 |
| int id; | 通道唯一id(0~31) |
| int	chn_pid; | 申请通道的线程的pid |
| int req_stop_play; | 放音通道销毁过程step id |
| audio_dev_t dev; | 通道所在设备 |
| int chan_activated; | 录音通道激活标记，上层真实读取音频时设置为1 |
| void *apu_handle; | codec handle |
| struct _apu_frnode_ channel_fr; | channel_fr |
| struct fr_buf_info chn_fr_buf; | for writing to record channel channel_fr  |
| int chn_fr_size; | 通道fr buffer长度 |
| int chn_fr_flag; | 通道fr flag |
| int chn_fr_num; | 通道fr buffer个数 |
|  |  |
| **struct list_head node;** | 通道在设备通道链表中的节点 |
| **struct list_head chn_node;** | 通道在设备codec链表中的节点 |

### 3.2 fr子系统
Audiobox的音频流处理基于fr子系统，每个处理子模块都是从一个输入fr中取得音频帧，处理之后输出到另一个输出fr中。Audiobox定义了如下fr:
- `chan_fr` 录音时，通道dispatch子模块输出fr，应用线程调用`audio_read_frame()`从此fr读取音频帧
- `chan_fr` 放音时，通道codec子模块输入fr，应用线程调用`audio_write_frame()`函数将音频帧写入此fr
- `codec_fr` 通道codec子模块输出fr
- `recombine_fr` 通道recombine子模块输出fr
- `softvol_fr` 通道软音量处理子模块输出fr
- `aec_fr` 设备aec子模块输出fr；放音时不定义此fr
- `dev_fr` 录音时读取alsa音频流输出到此fr；放音时不定义此fr
- `dev_fr_mapped` 使能AEC2.0录音时设备chan_remap子模块输出fr；放音时不定义此fr

其中`recombiner_fr`和`softvol_fr`只是普通的buffer，而不是调用`apu_alloc_fr()`申请的实际fr。为了将几个子模块设计为基于fr子系统的统一模型，将这两个buffer抽象为虚拟的fr，并在audiobox实现中使用下面统一接口访问这几个真实的或虚拟的fr：
```cpp
int _apu_fr_get_ref(void *apu_handle, struct fr_buf_info* ref)
int _apu_fr_put_ref(struct fr_buf_info* ref)
int _apu_fr_get_buf(void *apu_handle, struct fr_buf_info* buf)
int _apu_fr_put_buf(struct fr_buf_info* buf)
```

### 3.3 子模块定义
`audiobox_v2`定义若干音频流处理子模块来实现上述的基本需求：

| 子模块 | API | 功能描述 |
| --- |
| softvol | 新建：softvol_apu_open()<br />关闭：softvol_apu_close()<br />处理：softvol_apu_framer() | 实现设备master-volume和通道volume设置的处理 |
| recombine | 新建：recombiner_apu_open()<br />关闭：recombiner_apu_close()<br />处理：recombiner_apu_framer() | 设备与通道不同sample_size之间的转换 |
| codec | 新建：codec_apu_open()<br />关闭：codec_apu_close()<br />处理：codec_apu_framer() | 实现通道音频流编解码功能 |
| AEC | 新建：aec_lib_activate_preproc()<br />关闭：aec_lib_deactivate_preproc()<br />处理：apu_preproc_framer() | 多aec-lib支持，增加3个新的lib接口以挂载当前有效aec-lib |
| dispatch | 处理：dispatch_apu_framer() | 录音通道有效，将codec模块输出分发到1个或多个chan <br />这是同参数录音通道共享codec apu的实现基础 |
| chan-remap | 处理：get_capture_raw_stream() | AEC2.0模式有效，将4声道音频流转换为2声道音频流 |
| capture | 处理：audio_hal_read() | 从alsa缓冲区读取音频数据流 |
| playback  | 处理：audio_hal_write()  | 将音频数据流写入alsa缓冲区|

现在通道参数可以与设备参数不一致，上述的子模块可以将设备参数的音频流转换为通道格式的音频流。对于不同线程中申请的通道参数相同的多个录音通道，可以共享各个子模块以节省系统资源，音频流可以从同一个`codec_fr`被`dispatch`不同通道的`chan_fr`

----
### 3.4 AEC功能
**AEC2.0实现**

对于录音通道，如果没有使能AEC2.0，调用`audio_hal_read()`函数从ALSA读取到`dev_fr`的原始音频流为"MIC左声道+MIC右声道"的2声道模式；如果audiobox使能AEC2.0，那么audiobox启动时会调用`audiobox_set_aec_version()`函数通知内核，之后调用`audio_hal_read()`函数从ALSA读取到`dev_fr`的原始音频流将为"MIC左声道+MIC右声道+SPK左声道+SPK右声道"的4声道模式，增加的2个SPK声道为本地放音的环回，通过驱动层的控制，保证SPK和MIC之间的相对延时满足AEC处理的要求

使能AEC2.0时，非AEC通道从4声道格式的原始音频流中提取"MIC左声道+MIC右声道"，AEC通道从4声道格式的原始音频流中提取"MIC左声道+SPK左声道"

**多个aec算法库的管理**

AEC模块可以通过`aec_lib_register()`函数注册当前使用哪一个aec-lib，当前提供了基于arm的speexdsp和基于dsp的alango两种aec算法支持

## 4 通道申请释放流程
上层应用调用`audio_get_channel()`函数创建通道，调用`audio_put_channel()`函数释放通道，本章通过图例描述录音和放音通道的基本流程

### 4.1 录音通道申请和释放
本节通过图例描述下面场景的流程：申请两个相同通道参数的录音通道，然后依次释放。基于此场景来解析录音通道申请流程，以及同通道参数的两个通道如何共享codec子模块

**申请第一个录音通道**

<img src="image/ab_get_mic_0.dot.svg"/>

**申请第二个录音通道，同通道参数**

<img src="image/ab_get_mic_1.dot.svg"/>

NOTE: 第二录音通道和第一录音通道参数相同，共享之前创建codec即可，无需新建

**释放第二个录音通道**

<img src="image/ab_put_mic_1.dot.svg"/>

NOTE: 此时只释放通道，不释放共享的codec和音频设备

**释放第一个录音通道**

<img src="image/ab_put_mic_0.dot.svg"/>

NOTE: 此时释放通道、共享的codec和音频设备

### 4.2 放音通道申请和释放
本节通过图例描述下面场景的流程：申请两个相同通道参数的放音通道，然后依次释放

**申请第一个放音通道**

<img src="image/ab_get_spk_0.dot.svg"/>

**申请第二个放音通道**

<img src="image/ab_get_spk_1.dot.svg"/>

NOTE: 放音通道即使通道参数相同，也无法共享codec

**释放第二个放音通道**

<img src="image/ab_put_spk_1.dot.svg"/>

NOTE: 此时只释放通道和codec，不释放音频设备

**释放第一个放音通道**

<img src="image/ab_put_spk_0.dot.svg"/>

NOTE: 此时释放通道、codec和音频设备

NOTE: 因为不可能一个以上放音通道同时放音，所以设备所有放音通道共享同一个`thread_decoder_workloop()`线程，处理当前处于激活状态的通道的音频流。只在设备启动时启动线程，设备停止时退出线程

----
## 5 音频数据流
本章以流程图显示几种典型的通道音频数据流走向。`softvol`、`recombine`、`codec`三个子模块处理在实际简单点的通道配置中，可能会被跳过，这里不再绘图描述

### 5.1 aec使能的录音流程
<img src="image/ab_capture_enable_aec.dot.svg"/>

audiobox从`alsa buffer`中读取录音数据，处理后分发到音频通道的`chan_fr`，上层app调用`audio_read_frame()`函数从`chan_fr`读取数据流
### 5.2 aec禁止的录音流程(AEC2.0)
<img src="image/ab_capture_disable_aec_2.0.dot.svg"/>

audiobox从`alsa buffer`中读取录音数据，处理后分发到音频通道的`chan_fr`，上层app调用`audio_read_frame()`函数从`chan_fr`读取数据流
### 5.3 aec禁止的录音流程(no AEC2.0)
<img src="image/ab_capture_disable_aec.dot.svg"/>

audiobox从`alsa buffer`中读取录音数据，处理后分发到音频通道的`chan_fr`，上层app调用`audio_read_frame()`函数从`chan_fr`读取数据流
### 5.4 放音流程
<img src="image/ab_playback.dot.svg"/>

上层app调用`audio_write_frame()`函数，将音频流数据写入`chan_fr`，经过处理最终进入`alsa buffer`被硬件播放
