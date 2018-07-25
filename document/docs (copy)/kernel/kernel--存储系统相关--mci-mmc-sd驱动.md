MCI/MMC/SD驱动
驱动阅读过程，应该就是注释驱动关键数据结构的过程，是关联定义的关键变量的过程。

irom主要代码流程
iram : 0x3c000000  96k          irom : 0x04000000  128k     uboot0最大64k     boot image area: 0x0            64MB

     dd if=/dev/zero of=/dev/spiblock0 bs=1M count=1

struct isi_hdr {
     uint32_t magic;
     uint32_t type;
     uint32_t flag;
     uint32_t size;
     uint32_t load;
     uint32_t entry;
     uint32_t time;
     uint32_t hcrc;
     uint8_t dcheck[32];
     uint8_t name[192];
     uint8_t extra[256];
};
struct ius_desc {
     uint32_t sector;
          uint32_t sector1:24;     //sector
          uint32_t sector2:6;       //type: IUS_DESC_STRW / IUS_DESC_EXEC / IUS_DESC_CFG / IUS_DESC_REBOOT / IUS_DESC_BOOT
          uint32_t sector3:1;       //
     uint32_t info0;
     uint32_t info1;
     uint32_t length;
};
struct ius_hdr {
     uint32_t magic;
     uint32_t flag;
     uint8_t hwstr[8];
     uint8_t swstr[8];
     uint32_t hcrc;
     uint32_t count;
     uint8_t data[480];
};

int main(void)
     initialization();     gtc_time();     printf("\n__\n"); //打印　"__" 标记
     try_boot(0);
          set_xom(0);  /* set xom to 0 and get the device count, here is 2 */
          devn = boot_info();/* devcnt = 2 here */
          for(i = 0; i < devn; i++)
               gtc_time();     set_xom(i + 1);//两个设备
               if(try_get_bl(&mask, &id)){     blvalid = 1;     break;     }
                    loff_t offs = BL_DEFAULT_LOCATION;//2mbyte
                    id = boot_device();
                    if(vs_device_boot0(id)) offs = 0;  /* ---> if spiflash, then offset=0 , SDcard is at offset 2MB */
                    bl_load_isi(base, id, &offs, 0); /* ----> load uboots to base and make sure crc pass */

                    其他burn确认动作，判断是否可以执行burn.
               burn(mask);
                    ius = malloc(sizeof(struct ius_hdr));
                    idb = boot_device(); /* current bootdevice */
                    if((vs_is_device(id, "eth") || vs_is_device(id, "udc"))...) return;
                    vs_assign_by_id(id, 1);
                    #define IUS_DEFAULT_LOCATION 0x1000000 /* 16 MB */
                    ret = vs_read((uint8_t *)ius, IUS_DEFAULT_LOCATION,sizeof(struct ius_hdr), 0);//read IUS from SDcard id7
                    cdbg_verify_burn_enable(0, NULL);
                    burn_images(ius, id, 0); /* ius: update info , id=7 sdmmc card */

          cdbg_jump((void *)isi_get_entry(base), id);

int burn_images(struct ius_hdr *hdr, int id, uint32_t channel)
     buf = (uint8_t *)IRAM_BASE_PA;
     desc = ius_get_desc(hdr, i);     type = ius_desc_type(desc);     max = ius_desc_maxsize(desc); /* 64KB align ??? */
     ido = DEV_NONE;     burn_image_loop(desc, id, channel, ido, max,((type == IUS_DESC_EXEC)? load: buf));
     if(type == IUS_DESC_EXEC)          cdbg_jump((void *)ius_desc_entry(desc), 0);
     //现在ixl文件第一条必然是IUS_DESC_EXEC，所以会读取sd卡上uboot0.isi到iram指定位置，然后跳转到此位置执行uboot0.后续动作不会执行。
     if(type == IUS_DESC_CFG)           cdbg_config_isi(buf);

uboot0主要代码流程
struct irom_export *irf = &irf_apollo3;
void boot_main ( void )  //uboot0
     if(get_offset_ius()) boot_dev_id = irf->boot_device();//如果没有sd卡或者sd卡上没有有效的IUS升级包，赋4:DEV_FLASH:spi flash
     else boot_dev_id = DEV_IUS;  //sd卡上存在有效的IUS升级包,初始化offs_image[]各个image的偏移地址。
     init_config_item(); //读取item信息
          void *cfg = (void *)irf->malloc(ITEM_SIZE_NORMAL);
          if (boot_dev_id == DEV_IUS) //sd卡有效升级包
               irf->vs_assign_by_id(DEV_MMC(1), 1);
               irf->vs_read(cfg, offs_image[IMAGE_ITEM], ITEM_SIZE_NORMAL, 0);
          else if (boot_dev_id == DEV_FLASH)//spi flash启动
               irf->vs_assign_by_id(boot_dev_id, 1);
               offs_image[IMAGE_ITEM] = DEFAULT_ITEM_OFFSET;
               irf->vs_read(cfg, offs_image[IMAGE_ITEM], ITEM_SIZE_NORMAL, 0);
          else if (boot_dev_id == DEV_MMC(0))
               emmc_init(boot_dev_id);
               offs_image[IMAGE_ITEM] = DEFAULT_ITEM_OFFSET + EMMC_IMAGE_OFFSET;
               mmc_read(cfg, offs_image[IMAGE_ITEM], ITEM_SIZE_NORMAL, 0);
          else if (boot_dev_id == DEV_SNND )
               irf->vs_assign_by_id(boot_dev_id, 1);
               offs_image[IMAGE_ITEM] = DEFAULT_ITEM_OFFSET ;
               irf->vs_read(cfg, offs_image[IMAGE_ITEM], ITEM_SIZE_NORMAL, 0);
          item_init(cfg, ITEM_SIZE_NORMAL);     irf->free(cfg);
     boot_mode = get_boot_mode();//正常启动，IUS烧录，还是OTA升级
     cpu_freq_config();     init_io();     board_binit();　     bootst = apollo3_bootst();
     dramc_init(irf, !!tmp);     asm ( "dsb" );
     boot();//启动uboot1,或者直接启动kernnel
void start_armboot (void) //uboot1
     int init_mmc() //mmc.c
          INIT_LIST_HEAD (&mmc_devices); //static struct list_head mmc_devices;
          for(i = 0; i < 3; i++) {
               mmc = (struct mmc *)(buf + sizeof(struct mmc) * i);
               mmc->dma_desc_addr = (uint32_t)(buf + 0x400);
               imap_sdhc_initialize(i, mmc);

int imap_sdhc_initialize(uint32_t ch, struct mmc *mmc)
     mmc->sdhc_base_pa = SD0_BASE_ADDR/SD1_BASE_ADDR/SD2_BASE_ADDR;
     mmc->port_num = ch; mmc->voltages = 0xff8000; mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_HS;
     mmc->send_cmd = imap_sdhc_send_cmd;
     mmc->set_ios = imap_sdhc_set_ios;
     mmc->init = imap_sdhc_init;
     mmc->f_min = 400000;     mmc->f_max = 50000000;     mmc->b_max = 0;     mmc->has_init = 0;
     mmc_register(mmc);
          mmc->block_dev.if_type = IF_TYPE_MMC;
          mmc->block_dev.dev = cur_dev_num++;
          mmc->block_dev.removable = 1;
          mmc->block_dev.block_read = mmc_bread;     mmc->block_dev.block_write = mmc_bwrite;
          mmc->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;
          INIT_LIST_HEAD (&mmc->link);
          list_add_tail (&mmc->link, &mmc_devices);
U_BOOT_CMD(mmc, 6, 1, do_mmcops,...}
U_BOOT_CMD(mmcinfo, 6, 1, do_mmcinfo,...}
int do_mmcinfo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
     mmc = find_mmc_device(curr_device);
     mmc_init(mmc);     print_mmcinfo(mmc);
int do_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
     if (strcmp(argv[1], "rescan") == 0)
          struct mmc *mmc = find_mmc_device(curr_device);     mmc->has_init = 0;
          mmc_init(mmc);
     else if (strncmp(argv[1], "part", 4) == 0)
          block_dev_desc_t *mmc_dev;     struct mmc *mmc = find_mmc_device(curr_device);
          mmc_init(mmc);
          mmc_dev = mmc_get_dev(curr_device);     then  print_part(mmc_dev);
     else if (strcmp(argv[1], "list") == 0)
          print_mmc_devices('\n');
     else if (strcmp(argv[1], "dev") == 0)
          ...
     else if (strcmp(argv[1], "read") == 0)
          struct mmc *mmc = find_mmc_device(curr_device);     mmc_init(mmc);
          n = mmc->block_dev.block_read(curr_device, blk, cnt, addr);　//mmc_bread()
          flush_cache((ulong)addr, cnt * 512); /* FIXME */
     else if (strcmp(argv[1], "write") == 0)
          struct mmc *mmc = find_mmc_device(curr_device);     mmc_init(mmc);
          n = mmc->block_dev.block_write(curr_device, blk, cnt, addr);  //mmc_bwrite()
int mmc_init(int dev_num)
     struct mmc *mmc = find_mmc_device(dev_num);
     err = mmc->init(mmc);          //imap_sdhc_init()
          pads_chmod(PADSRANGE(31, 37), PADS_MODE_CTRL, 0);
          if(ecfg_check_flag(ECFG_ENABLE_PULL_MMC))
               pads_pull(PADSRANGE(31, 37), 1, 1);pads_pull(35, 1, 0); /* clk */
          mmc->pull_data = mci_pull_data32;     mmc->push_data = mci_push_data32;
          imap_sdhc_reset(mmc);
          mmc_writel(mmc, 0, SDMMC_INTMASK);     mmc_writel(mmc, 0xffffffff, SDMMC_RINTSTS);
          mmc_writel(mmc, 0xffffffff, SDMMC_TMOUT);          val = mmc_readl(mmc, SDMMC_FIFOTH);
          val = (val>>16)&0x7ff;     mmc_writel(mmc, (0x0<<28)|((val/2)<<16)|(val/2), SDMMC_FIFOTH);
          mmc_writel(mmc, 0, SDMMC_CLKENA); val = mmc_readl(mmc, SDMMC_PWREN);val |= 0x1;mmc_writel(mmc, val, SDMMC_PWREN);
          mmc_writel(mmc, 0x8c2800, SDMMC_GPIO);
     mmc_set_bus_width(mmc, 1);     mmc_set_clock(mmc, 400000);     mmc_go_idle(mmc);//发送cmd0
     mmc->part_num = 0;  mmc_send_if_cond(mmc); //发送cmd8    sd_send_op_cond(mmc); //发送cmd55-41 == ACMD41
     mmc_startup(mmc);//发送cmd2/3/9/7
          sd_change_freq(mmc);//发送cmd55/51/6查询频率/6修改频率
     //发送cmd55/6,改变位宽
     mmc_set_clock(mmc, 25000000/50000000); //应该是data频率，cmd line的应该不受影响。

void imap_sdhc_set_ios(struct mmc *mmc)


list数据结构

INIT_LIST_HEAD(&host->queue);

重要全局变量
static struct dw_mci *g_host;      //可以包含多个struct dw_mci_slot
static struct dw_mci *sdio_host;
static int sd1_cd = 1;
static struct dw_mci *mmc_host;

static inline void *mmc_priv(struct mmc_host *host){
     return (void *)host->private;
}
常见关系：
struct dw_mci_slot *slot = mmc_priv(mmc);
struct dw_mci_board *brd = slot->host->pdata;  ///定义的：　struct dw_mci_board imap_mmc1_platdata

__dw_mci_start_request() 初始化状态机，

mci_send_cmd(slot, SDMMC_CMD_UPD_CLK |SDMMC_CMD_PRV_DAT_WAIT, 0);//mci_send_cmd的唯一用法
     dw_mci_disable_low_power
          dw_mci_enable_sdio_irq
     dw_mci_setup_bus
          dw_mci_set_ios
               //.set_ios = dw_mci_set_ios, //struct mmc_host_ops dw_mci_ops
               dw_mci_resume
               sd1_rescan
          dw_mci_enable_sdio_irq
          dw_mci_resume
          sd1_rescan

inode包含文件的元信息，具体来说有以下内容：
　　* 文件的字节数                     　　* 文件拥有者的User ID
　　* 文件的Group ID               　　* 文件的读、写、执行权限
　　* 文件的时间戳，共有三个：ctime指inode上一次变动的时间，mtime指文件内容上一次变动的时间，atime指文件上一次打开的时间。
　　* 文件数据block的位置               * 链接数，即有多少文件名指向这个inode
       inode中没有包含文件名，大小为128/256byte, 一般个数和ｃｌｕｓｔｅｒ的数量差不多。每个文件都需要一个Ｉｎｏｄｅ,可能出现ｉｎｏｄｅ占用100%导致空闲空间无法使用的问题。
       df -h  df -i
       ls -i wdt_test.c　stat wdt_test.c

Secure Digit Card / MultiMedia Card / Secure Digital I/O
1.SD/MMC控制器的的接口是SDIO接口，工作方式有三种，单线，四线，SPI。
2.在MMC的设备模型中，用struct mmc_host来代表一个mmc卡控制器，用struct mmc_card表示一个卡，如SD卡，TF卡，emmc，wifi模块等。struct sdio_func 表示具有SDIO功能的卡。设备struct mmc_card对应驱动struct mmc_card，总线名："mmc"；设备struct sdio_func对应驱动struct sdio_driver，总线名："sdio"；后者是前者的一种容器，或者是继承。struct mmc_driver 用来描述 mmc卡驱动
struct mmc_host_ops 用来描述卡控制器操作集，用于从主机控制器层向 core 层注册操作函数，从而将core 层与具体的主机控制器隔离。也就是说 core 要操作主机控制器，就用这个 ops 当中给的函数指针操作，不能直接调用具体主控制器的函数。

区块层 Card/ block.c queue.c
主要是按照 LINUX 块设备驱动程序的框架实现一个卡的块设备驱动，这 block.c 当中我们可以看到写一个块设备驱动程序时需要的 block_device_operations 结构体变量的定义，其中有 open/release/request 函数的实现，而queue.c 则是对内核提供的请求队列的封装，我们暂时不用深入理解它，只需要知道一个块设备需要一个请求队列就可以了。
核心层　core/ core.c sd.c sd_ops.c bus.c host.c
核心层封装了 MMC/SD 卡的命令，例如存储卡的识别，设置，读写。例如不管什么卡都应该有一些识别，设置，和读写的命令，这些流程都是必须要有的，只是具体对于不同的卡会有一些各自特有的操作。 Core.c 文件是由sd.c 、 mmc.c 两个文件支撑的， core.c 把 MMC 卡、 SD 卡的共性抽象出来，它们的差别由 sd.c 和 sd_ops.c、 mmc.c 和 mmc_ops.c 来完成。
主机控制器层 host　ｄｗ_mmc.c

几个重要知识点的解析：　     host->tasklet　     host->card_work　　     dma和sg

struct buffer_head *bhs[]-->bio-->request->mmc_request，
相关的驱模块可以在同一个文件夹中，方便search
几个希望的功能：
１．能够建立快捷的函数链接，访问回调函数很方便
主要流程直接在函数中描述，不重要的分支在函数后面增加函数描述
２．更便捷直观的显示函数调用关系和数据结构使用
３．像存储系统这么复杂的模块，需要建立几个专题，层次结构一个，还有dma之类


今天要弄清楚，gendisk和card和blk_data和mmc host和slot和dci和。。之间的关系。
从上到下，上层从那个数据结构开始解析的？根据函数流程来。函数流程为经，数据结构为络。
函数解析方向一定是从上到下，反向的只有中断，而isr只影响最底层。
中断：　要么用于收发数据，要么卡检测。卡检测也是用的
mmc_rescan，好像就是从下到上，影响的是remove等，如何影响上层，流程怎么样。

每个代码块中一个主干函数，只显示主流程，中间的分支函数不需要全部解析，把原有的描述拆开就好。或者，主干流程用特殊颜色标记
一个模块，先看probe，然后整理流程：数据流程，控制流程。

static const struct mmc_host_ops dw_mci_ops = {//dw_mmc.c提供的接口
     void (*request)(struct mmc_host *host, struct mmc_request *req);//dw_mci_request
     void (*pre_req)(struct mmc_host *host, struct mmc_request *req,//dw_mci_pre_req
     void (*post_req)(struct mmc_host *host, struct mmc_request *req,//dw_mci_post_req
     void (*set_ios)(struct mmc_host *host, struct mmc_ios *ios);//dw_mci_set_ios
     int (*get_ro)(struct mmc_host *host);
     int (*get_cd)(struct mmc_host *host);
     void (*enable_sdio_irq)(struct mmc_host *host, int enable);
};

任何一个模块，最先需要了解的永远是，它提供给上层的接口是什么，它的下层提供的接口是什么。最开始就需要理清。
每个probe都对应一个驱动层，都创建一系列的数据结构，定义ops，最需要关注的永远是提供给上层的那个。
     dw_mci_probe到dw_mci_init_slot，构建的数据结构，最需要关注的是提供给上层的struct mmc_host *host
     mmc_blk_probe
          struct block_device *bi_bdev;里面包含struct gendisk * bd_disk;　bd_disk前面有
          struct gendisk * bd_disk
          struct mmc_blk_data *

比较看看，不同驱动的ｂｕｓ配置共同之处
struct bus_type {
     const char *name;     //总线名称
     const char *dev_name;     ／／总线上设备可能会以此为基础命名
     struct device *dev_root;／／缺省ｐａｒｅｎｔ设备
     struct bus_attribute *bus_attrs;
     struct device_attribute *dev_attrs;
     struct driver_attribute *drv_attrs;

     int (*match)(struct device *dev, struct device_driver *drv);／／匹配新驱动和设备
     int (*uevent)(struct device *dev, struct kobj_uevent_env *env);／／
     int (*probe)(struct device *dev);／／调用激活的驱动的ｐｒｏｂｅ函数
     int (*remove)(struct device *dev);
     void (*shutdown)(struct device *dev);

     int (*suspend)(struct device *dev, pm_message_t state);
     int (*resume)(struct device *dev);

     const struct dev_pm_ops *pm;
     struct iommu_ops *iommu_ops;

     struct subsys_private *p;／／ｄｒｉｖｅｒ的私有数据
     struct lock_class_key lock_key;
};
struct platform_device {
     const char *name;
     int       id;
     bool    id_auto;
     struct  device dev;
     u32     num_resources;
     struct resource *resource;
     const struct platform_device_id *id_entry;
     struct mfd_cell *mfd_cell;/* MFD cell pointer */
     struct pdev_archdata archdata;/* arch specific additions */
};
struct device {
     struct device *parent;
     struct device_private *p;     //
     struct kobject kobj;
     const char *init_name; /* initial name of the device */
     const struct device_type *type;

     struct mutex mutex; /* mutex to synchronize calls to its driver.*/

     struct bus_type *bus; /* type of bus device is on */
     struct device_driver *driver; /* which driver has allocated this device */
     void *platform_data; /* Platform specific data, device core doesn't touch it */
          //struct dw_mci_board*, 指向平台相关参数信息imap_mmc1_platdata，同　struct dw_mci的pdata
     struct dev_pm_info power;
     struct dev_pm_domain *pm_domain;

     u64 *dma_mask; /* dma mask (if dma'able device) */
     u64 coherent_dma_mask;/* Like dma_mask, but for alloc_coherent mappings as not all hardware supports
                   64 bit addresses for consistent allocations such descriptors. */

     struct device_dma_parameters *dma_parms;

     struct list_head dma_pools; /* dma pools (if dma'ble) */

     struct dma_coherent_mem *dma_mem; /* internal for coherent mem override */

     struct dev_archdata archdata;

     struct device_node *of_node; /* associated device tree node */
     struct acpi_dev_node acpi_node; /* associated ACPI device node */

     dev_t devt; /* dev_t, creates the sysfs "dev" */
     u32 id; /* device instance */

     spinlock_t devres_lock;
     struct list_head devres_head;

     struct klist_node knode_class;
     struct class *class;
     const struct attribute_group **groups; /* optional groups */

     void (*release)(struct device *dev);
     struct iommu_group *iommu_group;
};
struct dw_mci_slot {
struct mmc_host *mmc;
struct dw_mci *host;

int quirks;
int wp_gpio;

u32 ctype;

struct mmc_request *mrq;
struct list_head queue_node;

unsigned int clock;
     unsigned long flags;//bit0: CARD_PRESENT 检测到卡   bit1: CARD_NEED_INIT卡需要初始化
int id;
int last_detect_state;
};

struct dw_mci {
     spinlock_t lock;
     void __iomem *regs;     //ｍｍｉｏ　寄存器组　指针

     struct scatterlist *sg;     //
     struct sg_mapping_iter sg_miter;//

     struct dw_mci_slot *cur_slot;//
     struct mmc_request *mrq;     //
     struct mmc_command *cmd;//
     struct mmc_data *data;          //
     struct workqueue_struct *card_workqueue;     //

/* DMA interface members*/
     int use_dma;     //
     int using_dma;     //
     int index;     //控制器ｉｎｄｅｘ，当前使用mmc1

     dma_addr_t sg_dma;     //
     void *sg_cpu;
          //指向长４ｋ有１ｋ单元的指针数组，每个单元指向一个struct idmac_desc，sg_dma指向这个总长１６ｋ结构数组
     const struct dw_mci_dma_ops *dma_ops;     //
     u32 cmd_status;     //
     u32 data_status;     //
     u32 stop_cmdr;     //
     u32 dir_status;     //
     struct tasklet_struct tasklet;     //
     struct work_struct card_work;     //
     unsigned long pending_events;     //
     unsigned long completed_events;//
     enum dw_mci_state state;     //
     struct list_head queue;     //

     u32 bus_hz;          //
     u32 current_speed;     //
     u32 num_slots;     //
     u32 fifoth_val;     //
     u16 verid;     // dw 控制器型号ｉｄ，从硬件寄存器读取
     u16 data_offset;     //　根据型号不同，数据寄存器的偏移，也不同
     struct device *dev;     //
     struct dw_mci_board *pdata;  //指向平台相关参数信息imap_mmc1_platdata，同　struct device的platform_data
     const struct dw_mci_drv_data *drv_data;     //mmc1 控制器，drv_data为空
     void *priv;     //
     struct clk *biu_clk;     //
     struct clk *ciu_clk;     //
     struct dw_mci_slot *slot[MAX_MCI_SLOTS];     //

/* FIFO push and pull */
     int fifo_depth;     //
     int data_shift;     //　1/2/3　对应　16/32/64ｂｉｔｓ的数据宽度
     u8 part_buf_start;     //和part_buf_count和part_buf一起，push_data和pull_data实现数据对齐传送和接收的功能。
     u8 part_buf_count;     //
     union {
          u16 part_buf16;
          u32 part_buf32;
          u64 part_buf;
     };mmc_blk_alloc_parts(card, md) //为所有分区分配
     void (*push_data)(struct dw_mci *host, void *buf, int cnt);
     void (*pull_data)(struct dw_mci *host, void *buf, int cnt);

/* Workaround flags */
     u32 quirks;     //结构体中初始为　DW_MCI_QUIRK_HIGHSPEED，BIT(2)

     struct regulator *vmmc; /* Power regulator */
     unsigned long irq_flags; /* IRQ flags */
     int irq;
};
/* Board platform data */
struct dw_mci_board {
     u32 num_slots;          //1, 每个控制器只有一个ｓｌｏｔ，结构体中初始为１

     u32 quirks; /* Workaround / Quirk flags 结构体中初始为　DW_MCI_QUIRK_HIGHSPEED，BIT(2)*/
     unsigned int bus_hz; /* Clock speed at the cclk_in pad ，结构体中初始为50 * 1000 * 1000*/

     u32 caps; /* Capabilities */
     u32 caps2; /* More capabilities */
     u32 pm_caps; /* PM capabilities */
     u8 wifi_en;
/*
* Override fifo depth. If 0, autodetect it from the FIFOTH register,
* but note that this may not be reliable after a bootloader has used
* it.
*/
     unsigned int fifo_depth;

/* delay in mS before detecting cards after interrupt */
     u32 detect_delay_ms;

     int (*init)(u32 slot_id, irq_handler_t , void *);
     int (*get_ro)(u32 slot_id);
     int (*get_cd)(u32 slot_id);
     int (*get_ocr)(u32 slot_id);
     int (*get_bus_wd)(u32 slot_id);
/*
* Enable power to selected slot and set voltage to desired level.
* Voltage levels are specified using MMC_VDD_xxx defines defined
* in linux/mmc/host.h file.
*/
     void (*setpower)(u32 slot_id, u32 volt);
     void (*exit)(u32 slot_id);
     void (*select_slot)(u32 slot_id);     //空，每个控制器只有一个ｓｌｏｔ，不需要选择切换

     struct dw_mci_dma_ops *dma_ops;
     struct dma_pdata *data;
     struct block_settings *blk_settings;

/*
* delayline
*/
     int R_delayline_val;
     int W_delayline_val;
};

struct mmc_host {
     struct device *parent;     //父设备为　struct dw_mci *host
struct device class_dev;
int index;
const struct mmc_host_ops *ops;
unsigned int f_min;
unsigned int f_max;//最大频率为　struct dw_mci *host->bus_hz
unsigned int f_init;
u32 ocr_avail;
u32 ocr_avail_sdio; /* SDIO-specific OCR */
u32 ocr_avail_sd; /* SD-specific OCR */
u32 ocr_avail_mmc; /* MMC-specific OCR */
struct notifier_block pm_notify;
u32 max_current_330;
u32 max_current_300;
u32 max_current_180;

u32 caps; /* Host capabilities */
u32 caps2; /* More host capabilities */

mmc_pm_flag_t pm_caps; /* supported pm features */

#ifdef CONFIG_MMC_CLKGATE
int clk_requests; /* internal reference counter */
unsigned int clk_delay; /* number of MCI clk hold cycles */
bool clk_gated; /* clock gated */
struct delayed_work clk_gate_work; /* delayed clock gate */
unsigned int clk_old; /* old clock value cache */
spinlock_t clk_lock; /* lock for clk fields */
struct mutex clk_gate_mutex; /* mutex for clock gating */
struct device_attribute clkgate_delay_attr;
unsigned long clkgate_delay;
#endif

/* host specific block data */
unsigned int max_seg_size; /* see blk_queue_max_segment_size 512×６４ｋ字节*/
unsigned short max_segs; /* see blk_queue_max_segments 64个ｓｅｇｍｅｎｔ*/
unsigned short unused;
unsigned int max_req_size; /* maximum number of bytes in one req 512×６４ｋ字节*/
unsigned int max_blk_size; /* maximum size of one mmc block      ６４ｋ字节*/
unsigned int max_blk_count; /* maximum number of blocks in one req 512个块*/
unsigned int max_discard_to; /* max. discard timeout in ms */

/* private data */
spinlock_t lock; /* lock for claim and bus ops */

struct mmc_ios ios; /* current io bus settings */
u32 ocr; /* the current OCR setting */

/* group bitfields together to minimize padding */
unsigned int use_spi_crc:1;
unsigned int claimed:1; /* host exclusively claimed */
unsigned int bus_dead:1; /* bus has been released */
#ifdef CONFIG_MMC_DEBUG
unsigned int removed:1; /* host is being removed */
#endif

int rescan_disable; /* disable card detection */
int rescan_entered; /* used with nonremovable devices */

struct mmc_card *card; /* device attached to this host */

wait_queue_head_t wq;
struct task_struct *claimer; /* task that has host claimed */
int claim_cnt; /* "claim" nesting count */

struct delayed_work detect;
struct wake_lock detect_wake_lock;
int detect_change; /* card detect flag */
struct mmc_slot slot;

const struct mmc_bus_ops *bus_ops; /* current bus driver */
unsigned int bus_refs; /* reference counter */

unsigned int bus_resume_flags;
#define MMC_BUSRESUME_MANUAL_RESUME (1 << 0)
#define MMC_BUSRESUME_NEEDS_RESUME (1 << 1)

unsigned int sdio_irqs;
struct task_struct *sdio_irq_thread;
bool sdio_irq_pending;
atomic_t sdio_irq_thread_abort;

mmc_pm_flag_t pm_flags; /* requested pm features */

struct led_trigger *led; /* activity led */

#ifdef CONFIG_REGULATOR
bool regulator_enabled; /* regulator state */
#endif
struct mmc_supply supply;

struct dentry *debugfs_root;

struct mmc_async_req *areq; /* active async req */
struct mmc_context_info context_info; /* async synchronization info */

#ifdef CONFIG_FAIL_MMC_REQUEST
struct fault_attr fail_mmc_request;
#endif

unsigned int actual_clock; /* Actual HC clock rate */

unsigned int slotno; /* used for sdio acpi binding */

#ifdef CONFIG_MMC_EMBEDDED_SDIO
struct {
struct sdio_cis *cis;
struct sdio_cccr *cccr;
struct sdio_embedded_func *funcs;
int num_funcs;
} embedded_sdio_data;
#endif

unsigned long private[0] ____cacheline_aligned;
};

dw_mmc-pltfm.c和devices.c

static int imap_mmc1_get_bus_wd(u32 slot_id){    return 4;}
static int imap_mmc1_init(u32 slot_id, irq_handler_t handler, void *data){//配置ｐｉｎ脚复用开启电源
    imapx_pad_init("sd1");    module_power_on(SYSMGR_MMC1_BASE);    return 0;
}
static struct dw_mci_board imap_mmc1_platdata = {
    .num_slots = 1,    .quirks = DW_MCI_QUIRK_HIGHSPEED,    .bus_hz = 50 * 1000 * 1000,
    .detect_delay_ms = 500,    .init = imap_mmc1_init,    .get_bus_wd = imap_mmc1_get_bus_wd,
    .R_delayline_val = -1,    .W_delayline_val = -1,
};
static struct resource imap_mmc1_resource[] = {
    [0] = {           .start = IMAP_MMC1_BASE,           .end = IMAP_MMC1_BASE + IMAP_MMC1_SIZE - 1,
           .flags = IORESOURCE_MEM,           },
    [1] = {           .start = GIC_MMC1_ID,           .end = GIC_MMC1_ID,           .flags = IORESOURCE_IRQ,           }
};
struct platform_device imap_mmc1_device = {
    .name = "imap-mmc1",    .id = 1,    .num_resources = ARRAY_SIZE(imap_mmc1_resource),
    .resource = imap_mmc1_resource,
    .dev = {        .platform_data = &imap_mmc1_platdata,        .dma_mask = &sdmmc1_dma_mask,
        .coherent_dma_mask = DMA_BIT_MASK(32),        },
};

static struct platform_driver dw_mci_pltfm_driver = {
     .probe = dw_mci_pltfm_probe,
     .remove = dw_mci_pltfm_remove,
     .driver = {
          .name = "imap-mmc1",
          .of_match_table = of_match_ptr(dw_mci_pltfm_match),
          .pm = &dw_mci_pltfm_pmops,
     },
};
static int __init dw_mci_platform_init(void)
     return platform_driver_register(&dw_mci_pltfm_driver);
static void __exit dw_mci_platform_exit(void)
     platform_driver_unregister(&dw_mci_pltfm_driver);

dw_mmc.c 这个模块只是export一些设备控制函数
void mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)//专门发送ｃｍｄ，等待mmc_request完成
     __mmc_start_req(host, mrq);
          init_completion(&mrq->completion);
          mrq->done = mmc_wait_done;　--> complete(&mrq->completion); //request处理完调用，__wake_up_common
          mmc_start_request(host, mrq);
               host->ops->request(host, mrq);     //dw_mci_request
     mmc_wait_for_req_done(host, mrq);

static const struct mmc_host_ops dw_mci_ops = {
    .request        = dw_mci_request,
    .pre_req        = dw_mci_pre_req,
    .post_req        = dw_mci_post_req,
    .set_ios        = dw_mci_set_ios,
    .get_ro            = dw_mci_get_ro,
    .get_cd            = dw_mci_get_cd,
    .enable_sdio_irq    = dw_mci_enable_sdio_irq,
};
static int dw_mci_pltfm_probe(struct platform_device *pdev)
     dw_mci_pltfm_register(pdev, NULL);
         struct dw_mci_board *pdata = pdev->dev.platform_data;
         struct dw_mci *host = devm_kzalloc(&pdev->dev, sizeof(struct dw_mci), GFP_KERNEL);//为mmc1分配struct dw_mci
         request irq resource;     //申请ｉｒｑ资源
         host->drv_data = drv_data;     //host->drv_data = NULL;
         host->pdata->init(0,NULL,NULL);    //---> imap_mmc1_init()     //配置ｐｉｎ脚复用开启电源
         struct resource *regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
         host->regs = devm_ioremap_resource(&pdev->dev, regs);     //注册内存和ｉｏ映射资源
         platform_set_drvdata(pdev, host);
               dev_set_drvdata(&pdev->dev, data);     //设置struct device设备的成员struct device_private *p; 设备私有信息
         ret = dw_mci_probe(host);
int dw_mci_probe(struct dw_mci *host)
     const struct dw_mci_drv_data *drv_data = host->drv_data;     //为空NULL
     host->pdata已经初始化为 imap_mmc1_platdata，下面代码没有执行：
         //host->pdata = dw_mci_parse_dt(host);         分配并初始化 struct dw_mci_board 信息
     g_host = host;
     初始化host->biu_clk和host->ciu_clk两个clock
     host->push_data = dw_mci_push_data16/32/64;     host->pull_data = dw_mci_pull_data16/32/64;
     width = 16/32/64;           host->data_shift = 1/2/3;
     dw_mci_init_dma(host);
          host->sg_cpu = dmam_alloc_coherent(host->dev, PAGE_SIZE,&host->sg_dma, GFP_KERNEL);
               struct dma_devres *dr = devres_alloc(dmam_coherent_release, sizeof(*dr), gfp);//注册内存资源用
               void *vaddr  = dma_alloc_coherent(dev, size, dma_handle, gfp);     //分配实际的地址
               dr->vaddr = vaddr;               dr->dma_handle = *dma_handle;                dr->size = size;
               devres_add(dev, dr);            return vaddr;
               //为ｓｇ传输创建管理数据结构，host->sg_cpu为virtual地址上层访问，host->sg_dma为physical地址给控制器访问
          host->dma_ops = &dw_mci_idmac_ops;
          host->dma_ops->init(host);     //dw_mci_idmac_init
               host->ring_size = PAGE_SIZE / sizeof(struct idmac_desc);// 4096/16=256
               循环初始化host->sg_cpu数组，将所有单元链接为一个链表，前节点指向后节点，最后节点指向第一个节点
               mci_writel(host, DBADDR, host->sg_dma);//设置ｄｍａ描述符基地址为host->sg_dma
          host->use_dma = 1;
     tasklet_init(&host->tasklet, dw_mci_tasklet_func, (unsigned long)host);
     host->card_workqueue = alloc_workqueue("dw-mci-card", WQ_MEM_RECLAIM | WQ_NON_REENTRANT, 1);
     INIT_WORK(&host->card_work, dw_mci_work_routine_card);
     devm_request_irq(host->dev, host->irq, dw_mci_interrupt, host->irq_flags, "dw-mci", host);
     host->num_slots = host->pdata->num_slots; //1个ｓｌｏｔ
     foreach i: dw_mci_init_slot(host, i); //分配并初始化struct mmc_host和struct dw_mci_slot对象
          host->ring_size = PAGE_SIZE / sizeof(struct idmac_desc);//4096/16=256
          struct mmc_host *mmc = mmc_alloc_host(sizeof(struct dw_mci_slot), host->dev);
               struct mmc_host *host = kzalloc(sizeof(struct mmc_host) + extra, GFP_KERNEL);
              //分配空间struct mmc_host在前 + struct dw_mci_slot在后, 后者从前者的private[0]开始
              //mmc_host有成员class_dev，它是作为一个class设备存在的
              //一个mmc_host应该是对应一个dw_mci_slot，属于mmc层次结构的Ｈｏｓｔ层，struct dw_mci *host对应mmc1控制器
              //分配并将host关联struct dw_mci_slot和struct mmc_host
              mmc_host_clk_init(host);
                  INIT_DELAYED_WORK(&host->clk_gate_work, mmc_host_clk_gate_work);
              init_waitqueue_head(&host->wq);  host->parent = dev;  //设置parent为struct dw_mci *host
              INIT_DELAYED_WORK(&host->detect, mmc_rescan);
          slot = mmc_priv(mmc); --> return (void *)host->private;//获得紧跟着mmc的dw_mci_slot结构体指针
          slot->mmc = mmc;　　slot->id = id;　　　slot->host = host;　　　host->slot[id] = slot;
          为上层块设备驱动设定回调函数：mmc->ops = &dw_mci_ops;
          初始化ｍｍｃ重要参数，一个例子：
              mmc->max_segs = mmc->max_blk_count = host->ring_size ; //256;
              mmc->max_blk_size = 65536;                 mmc->max_seg_size =0x1000; // 4096
              mmc->max_req_size = mmc->max_seg_size * mmc->max_blk_count; //1Ｍ
          host->vmmc = devm_regulator_get(mmc_dev(mmc), "vmmc");　//mmc_dev(mmc)表示mmc->parent : 控制器设备
          ret = regulator_enable(host->vmmc);           //给mmc1供电
          根据dw_mci_get_cd(mmc)卡检测结果更新 slot->flags 状态位
          slot->wp_gpio = dw_mci_of_get_wp_gpio(host->dev, slot->id);
          ret = mmc_add_host(mmc);
               err = device_add(&host->class_dev);     //增加class设备，做设备相关处理
               mmc_host_clk_sysfs_init(host);          //sys文件系统处理
               mmc_start_host(host);                   //上下电，启动卡检测
                   mmc_power_up(host);　or 　mmc_power_off(host);
                   mmc_detect_change(host, 0);
                       mmc_schedule_delayed_work(&host->detect, 0);　//当前线程结束立刻执行一次host->detect：　mmc_rescan()
         queue_work(host->card_workqueue, &host->card_work);//延迟启动工作任务
void dw_mci_work_routine_card(struct work_struct *work) //host->card_work
     foreach i: struct dw_mci_slot *slot = host->slot[i];
          struct mmc_host *mmc = slot->mmc;   //这里是通过host得到slot,通过slot得到mmc.
          int present = dw_mci_get_cd(mmc);   //读取硬件寄存器实现卡检测,如果没哟变化直接返回。
          更新slot->flags ;
          struct mmc_request *mrq = slot->mrq;
          if (mrq == host->mrq)//当前slot?
               dw_mci_request_end(host, mrq);
                    如果host->queue中有slot排队非空
                         slot = list_entry(host->queue.next,struct dw_mci_slot, queue_node);
                         list_del(&slot->queue_node);
                         dw_mci_start_request(host, slot);     //启动下一个slot
                    mmc_request_done(prev_mmc, mrq);-->struct mmc_request *mrq->done(mrq);//mmc_wait_data_done
          else
               mmc_request_done(slot->mmc, mrq);
          if (present == 0)
               清除slot->flags标记
               sg_miter_stop(&host->sg_miter);　host->sg = NULL;　然后softreset;  mci_wait_soft_reset(host);
          mmc_detect_change(slot->mmc,msecs_to_jiffies(host->pdata->detect_delay_ms));//每500ms调用一次
               host->detect_change = 1;
               mmc_schedule_delayed_work(&host->detect, delay);

static struct mmc_driver mmc_driver = {
     .drv = {.name = "mmcblk",},     .remove = mmc_blk_remove,.suspend = mmc_blk_suspend,.resume = mmc_blk_resume,
     .probe = mmc_blk_probe,
};
static struct bus_type mmc_bus_type = {
     .name = "mmc",     .dev_attrs = mmc_dev_attrs,     .uevent = mmc_bus_uevent,     .remove = mmc_bus_remove,
     .pm = &mmc_bus_pm_ops,          .match = mmc_bus_match,//总是返回１
     .probe = mmc_bus_probe,
};
static const struct block_device_operations mmc_bdops = {
     .open = mmc_blk_open,.release = mmc_blk_release,.getgeo = mmc_blk_getgeo,
     .owner = THIS_MODULE,.ioctl = mmc_blk_ioctl,
};
static int __init mmc_init(void)
     ret = mmc_register_bus();-->return bus_register(&mmc_bus_type);//注册mmc_bus_type，以后mmc_driver card_dev都使用
     ret = mmc_register_host_class();-->return class_register(&mmc_host_class);
     ret = sdio_register_bus();
static int __init mmc_blk_init(void)
     res = register_blkdev(MMC_BLOCK_MAJOR, "mmc");
     res = mmc_register_driver(&mmc_driver);
          drv->drv.bus = &mmc_bus_type;
          return driver_register(&drv->drv);//挂在通用bus下面的驱动的注册方式，驱动和设备都需要指明自己的方式。
static int mmc_bus_probe(struct device *dev)//详细内容参考blk note

void mmc_rescan(struct work_struct *work)
     struct mmc_host *host =container_of(work, struct mmc_host, detect.work);
     for (i = 0; i < ARRAY_SIZE(freqs); i++)
     mmc_rescan_try_freq(host, max(freqs[i], host->f_min));    //从高到低换频率scan存储卡
          host->f_init = freq;     mmc_power_up(host);     mmc_hw_reset_for_init(host);     sdio_reset(host);
          mmc_go_idle(host);     //发送CMD0 复位SD卡
          mmc_send_if_cond(host, host->ocr_avail);//发送CMD8判别目标是否高容量卡
          mmc_attach_sdio(host);        //sdio的wifi之类芯片会响应
          mmc_attach_sd(host);　　　//检测到sd卡执行完整的初始化过程和获取配置参数,创建和添加绑定了mmc bus的mmc_card设备
               err = mmc_send_app_op_cond(host, 0, &ocr); //cmd41获取支持电压
               mmc_sd_attach_bus_ops(host);
                  static const struct mmc_bus_ops mmc_sd_ops = {
     .remove = mmc_sd_remove,.detect = mmc_sd_detect,
     .power_restore = mmc_sd_power_restore,.alive = mmc_sd_alive,};
mmc_attach_bus(host, &mmc_sd_ops);--host->bus_ops = &mmc_sd_ops;
               err = mmc_sd_init_card(host, host->ocr, NULL);//初始化ｓｄ卡流程
                    err = mmc_sd_get_cid(host, ocr, cid, &rocr);
                         mmc_go_idle(host);   //发送cmd 0       err = mmc_send_if_cond(host, ocr); //发送cmd 8
                         err = mmc_send_app_op_cond(host, ocr, rocr); //发送acmd 41
                         err = mmc_all_send_cid(host, cid); //发送cmd 2
                    struct mmc_card *card = mmc_alloc_card(host, &sd_type);//创建mmc_card并绑定mmc_bus
                         card = kzalloc(sizeof(struct mmc_card), GFP_KERNEL);card->host = host;card->dev.parent = mmc_classdev(host);
                         card->dev.bus = &mmc_bus_type;                  card->dev.release = mmc_release_card;card->dev.type = type;
                    card->type = MMC_TYPE_SD;     memcpy(card->raw_cid, cid, sizeof(card->raw_cid));
                    err = mmc_send_relative_addr(host, &card->rca);//获得卡地址
                    err = mmc_sd_get_csd(host, card);   //发送cmd 9获取csd
                    mmc_decode_cid(card);//解析前面获得的CID并填充到card,这一步为什么不在前面获得cid的时候做? 是因为sd卡协议有不同版本而版本信息放在CSD中,所以需要先得到CSD,获得版本号,在根据版本号解析CID中的数据
                    err = mmc_select_card(card);//发送CMD7使用上面得到的地址选择卡，这个cmd使用struct mmc_request mrq封装struct mmc_command命令了，没有data部分，之后ｃｍｄ都如此发送了。
                         __mmc_start_req(host, mrq);
                              mrq->done = mmc_wait_done; mmc_start_request(host, mrq);     //调用mmc回调dw_mci_request
                         mmc_wait_for_req_done(host, mrq);//等待mmc_wait_done被调用，可尝试重发cmd，尝试完毕退出
                    err = mmc_sd_setup_card(host, card, oldcard != NULL);//发送ACMD51获取SCR寄存器值,发送ACMD13获取SD卡状态信息,解析并填充card结构,SCR寄存器是对CSD的补充
                    err = mmc_sd_init_uhs_card(card);//继续初始化ｃａｒｄ
                    mmc_card_set_uhs(card);//支持高速就发CMD６命令把卡设置到高速
                    ｏｒ
                    host->card = card;
               err = mmc_add_card(host->card);   //添加设备
                    ---ret = device_add(&card->dev);       //接着就是mmc_bus_match--mmc_bus_probe

int sd1_rescan( int flag)　//和dw_mci_resume的过程差不多
     if(flag == 1){
          sd1_cd = 1;     queue_work(g_host->card_workqueue, &g_host->card_work);
          return 0;
     }
     sd1_cd = 0;     module_power_down(SYSMGR_MMC1_BASE);
     g_host->pdata->init(0,NULL,NULL);
     if (g_host->vmmc)     ret = regulator_enable(g_host->vmmc);
     mci_wait_reset(g_host->dev, g_host);
     if (g_host->use_dma && g_host->dma_ops->init)     g_host->dma_ops->init(g_host);
     mci_writel(g_host, FIFOTH, g_host->fifoth_val); ... mci_writel(g_host, CTRL, SDMMC_CTRL_INT_ENABLE);
     foreach i : struct dw_mci_slot *slot = g_host->slot[i];
          if (slot->mmc->pm_flags & MMC_PM_KEEP_POWER){
               dw_mci_set_ios(slot->mmc, &slot->mmc->ios);
               dw_mci_setup_bus(slot, true);
          }
     queue_work(g_host->card_workqueue, &g_host->card_work);
int dw_mci_suspend(struct dw_mci *host)
     foreach i: struct dw_mci_slot *slot = host->slot[i];//所有slot
          ret = mmc_suspend_host(slot->mmc);
     if (host->vmmc)     regulator_disable(host->vmmc);
     host->current_speed = 0;
int dw_mci_resume(struct dw_mci *host)
     clk_prepare_enable(host->ciu_clk　/　host->biu_clk);
     regulator_enable(host->vmmc);
     mci_wait_reset(host->dev, host)//等待５ｓ
          unsigned long timeout = jiffies + msecs_to_jiffies(500);
          do{}while (time_before(jiffies, timeout));
     host->dma_ops->init(host);
     foreach i: struct dw_mci_slot *slot = host->slot[i];//所有slot
         dw_mci_set_ios(slot->mmc, &slot->mmc->ios);　设置时钟和电源等接口参数
               struct dw_mci_slot *slot = mmc_priv(mmc);
               const struct dw_mci_drv_data *drv_data = slot->host->drv_data;　//NULL
               dw_mci_setup_bus(slot, false);//POWER等参数
         dw_mci_setup_bus(slot, true);
         ret = mmc_resume_host(host->slot[i]->mmc);
static void dw_mci_request(struct mmc_host *mmc, struct mmc_request *mrq)
     if (!test_bit(DW_MMC_CARD_PRESENT, &slot->flags)){mrq->cmd->error = -ENOMEDIUM; mmc_request_done(mmc, mrq); return;}
     dw_mci_queue_request(struct dw_mci *host, struct dw_mci_slot *slot,struct mmc_request *mrq);
          slot->mrq = mrq;//设置slot请求，每个slot一个请求么？还是有一个队列。
          if (host->state ！= STATE_IDLE)list_add_tail(&slot->queue_node, &host->queue);//发送中，添加到host的队列中返回；
          host->state = STATE_SENDING_CMD;
          dw_mci_start_request(host, slot);
               struct mmc_request *mrq = slot->mrq;
               struct mmc_command *cmd = mrq->sbc ? mrq->sbc : mrq->cmd;//按照sbc cmd data stop的顺序执行操作
               __dw_mci_start_request(host, slot, cmd);
                    host->cur_slot = slot; host->mrq = slot->mrq;
                    host->pending_events = 0; host->completed_events = 0; host->data_status = 0;
                    cmdflags = dw_mci_prepare_command(slot->mmc, cmd);//对cmd做预处理，得到cmdflags
                         struct dw_mci_slot *slot = mmc_priv(mmc);
                         const struct dw_mci_drv_data *drv_data = slot->host->drv_data; //NULL
                         //cmd处理
                    if(cmd->data) {
                             dw_mci_set_timeout(host);//有数据任务就需要设定超时时间
                             dw_mci_submit_data(host, data);
                                   host->data = data;
                                   if (dw_mci_submit_data_dma(host, data))
                                             确认data数据合法且长度有效，host->using_dma = 1;
                                             禁止mmc发送接收中断，使能dma
                                             host->dma_ops->start(host, sg_len); //dw_mci_idmac_start_dma
                                                       dw_mci_translate_sglist(host, host->data, sg_len);
                                                       //data应该管理着sg_len个不连续的内存块，每块本身是连续的。
                                                       用data管理的ｓｇ内存初始化host->sg_cpu，关联两者
                                                       选择和使能然后启动IDMAC
                                                     mci_writel(host, CTRL, temp);   mci_writel(host, BMOD, temp);  mci_writel(host, PLDMND, 1);
                                        出错：sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);//以后用host->sg_miter索引和访问data
                                        出错：屏蔽dma发送接收中断，禁止dma，isr函数中dw_mci_read_data_pio和dw_mci_write_data_pio收发
                   dw_mci_start_command(host, cmd, cmdflags);
                         host->cmd = cmd;　mci_writel(host, CMDARG, cmd->arg);
                         发送cmd with参数cmdflags
                         mci_writel(host, CMD, cmd_flags | SDMMC_CMD_START);
设置中断中启动的thread:
host->sdio_irq_thread = kthread_run(sdio_irq_thread, host, "ksdioirqd/%s", mmc_hostname(host));

static irqreturn_t dw_mci_interrupt(int irq, void *dev_id)
     pending = mci_readl(host, MINTSTS);
     mci_writel(host, RINTSTS, 中断对应ｂｉｔ);　清除中断
     根据pending位处理中断：
         SDMMC_INT_TXDR: dw_mci_write_data_pio(host);//处理Ｔｘ FIFO中断
              static void dw_mci_write_data_pio(struct dw_mci *host)
                   struct sg_mapping_iter *sg_miter = &host->sg_miter; //待发送的数据结构
                        buf = sg_miter->addr; remain = sg_miter->length;
                        host->push_data(host, (void *)(buf + offset), len);//根据fifo深度每次发送部分
                        sg_miter->consumed = offset;
                   !sg_miter_next(sg_miter)循环取得sg_miter，发送数据，直到结束
              sg_miter_stop(sg_miter);

         SDMMC_INT_RXDR ：　dw_mci_read_data_pio(host, false);//接收到部分数据，继续
         SDMMC_INT_TXDR：　 dw_mci_write_data_pio(host);         //发送完部分数据，继续
         SDMMC_INT_CD:  queue_work(host->card_workqueue, &host->card_work);//检测到卡状态变化
         SDIO Interrupts：　mmc_signal_sdio_irq（slot->mmc）　-> wake_up_process(host->sdio_irq_thread);

         SDMMC_INT_CMD_DONE/DW_MCI_CMD_ERROR_FLAGS/DW_MCI_DATA_ERROR_FLAGS/SDMMC_INT_DATA_OVER:
               //好像是命令／数据的开始结束处理延迟到中断处理函数之后执行。
              host->cmd_status = status; /host->data_status = pending;/ 还有其他的一些信息。
              set_bit(EVENT_CMD_COMPLETE/EVENT_DATA_COMPLETE/EVENT_DATA_ERROR, &host->pending_events);
              tasklet_schedule(&host->tasklet);          //dw_mci_tasklet_func
static void dw_mci_tasklet_func(unsigned long priv)//命令数据状态机，只考虑不出错的情况算了。
     struct mmc_request {
          struct mmc_command *sbc; /* SET_BLOCK_COUNT for multiblock */
          struct mmc_command *cmd;
          struct mmc_data *data;
          struct mmc_command *stop;

          struct completion completion;
          void (*done)(struct mmc_request *);/* completion function */
          struct mmc_host *host;
     };//按照sbc-->cmd-->data-->stop的顺序发送
     STATE_IDLE     break;　//等待上层调用dw_mci_request()函数发送请求
     STATE_SENDING_CMD:  只处理EVENT_CMD_COMPLETE事件；此状态下正在发送sbc或cmd
          set_bit(EVENT_CMD_COMPLETE, &host->completed_events);
          按照sbc-->cmd-->data-->stop的顺序发送
               __dw_mci_start_request(host, host->cur_slot,host->mrq->cmd); //发送cmd
               dw_mci_request_end(host, host->mrq);     //发送结束
     STATE_SENDING_DATA: 只处理EVENT_DATA_ERROR EVENT_XFER_COMPLETE事件；此状态下正在发送data
          if(EVENT_XFER_COMPLETE)
               set_bit(EVENT_XFER_COMPLETE, &host->completed_events);
               prev_state = state = STATE_DATA_BUSY;
     STATE_DATA_BUSY:       只处理EVENT_DATA_COMPLETE事件
          set_bit(EVENT_DATA_COMPLETE, &host->completed_events);
          prev_state = state = STATE_SENDING_STOP;
          send_stop_cmd(host, data);
               dw_mci_start_command(host, data->stop, host->stop_cmdr);
     STATE_SENDING_STOP:  只处理EVENT_CMD_COMPLETE事件
          dw_mci_command_complete(host, host->mrq->stop);
          dw_mci_request_end(host, host->mrq);
     STATE_DATA_ERROR:  只处理EVENT_XFER_COMPLETE事件
          state = STATE_DATA_BUSY;

     host->state = state;

