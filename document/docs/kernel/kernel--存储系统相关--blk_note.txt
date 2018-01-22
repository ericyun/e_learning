BLK NOTE
按照文档标准来定义。大文件关联函数有优势，但手动翻阅困难，需要建立索引。

本文档索引信息：(名称必须唯二)
- blk-core.c解析入口
- genhd.c文件入口
- mmc blk dev_ops入口:
- mpage_writepages解析Plug
- request_queue线程入口
- mmc_bus_probe入口


整个block.c文件，基本上就是mmc_blk_probe()函数相关的调用(包括设置的回调函数)值得看看，文件上半部分可以忽略。
block模块的头文件是blkdev.h

blk_core.c解析入口
     mmc_blk_probe
          大部分其他函数都和probe的过程有关系。
     其他一些和request queue有关的函数
          submit_bio和blk_queue_bio相关进入queue，blk_fetch_request相关出queue,blk_end_request相关request发送end回调
          attempt_plug_merge()等入queue时merge相关的函数
          plug机制
               主线：blk_start_plug blk_flush_plug_list blk_finish_plug
               次要：queue_unplugged flush_plug_callbacks blk_run_queue blk_delay_queue blk_start_queue blk_delay_work
     virtio机制的函数接口：blk_requeue_request blk_make_request　blk_get_request
     Helper function相关的函数
其他文件内容解析
block.c解析
     mmc_blk_probe调用层次
     mmc用的packed相关函数很多
     mmc命令封装和发送
整体上异常处理函数占用大量篇幅
     各种

struct mmc_command {
     u32 opcode;     u32 arg;　u32 resp[4];     //命令和参数和response
     unsigned int flags; /* expected response type */
     unsigned int retries; /* max number of retries 重发次数*/
     unsigned int error; /* command error */
     unsigned int cmd_timeout_ms; /* in milliseconds */
     struct mmc_data *data; /* data segment associated with cmd */
     struct mmc_request *mrq; /* associated request */
};
struct mmc_data {
     unsigned int timeout_ns; /* data timeout (in ns, max 80ms) */
     unsigned int timeout_clks; /* data timeout (in clocks) */
     unsigned int blksz; /* data block size */
     unsigned int blocks; /* number of blocks */
     unsigned int error; /* data error */
     unsigned int flags;
#define MMC_DATA_WRITE (1 << 8)
#define MMC_DATA_READ (1 << 9)
#define MMC_DATA_STREAM (1 << 10)

     unsigned int bytes_xfered;
     struct mmc_command *stop; /* stop command 后面紧跟ｓｔｏｐ部分*/
     struct mmc_request *mrq; /* associated request 关联的ｒｅｑｕｅｓｔ*/

     unsigned int sg_len; /* size of scatter list ｓｇ数据管理*/
     struct scatterlist *sg; /* I/O scatter list */
     s32 host_cookie; /* host private data */
};
struct mmc_request {／／一个完整的ｒｅｑｕｅｓｔ
     struct mmc_command *sbc; /* SET_BLOCK_COUNT for multiblock */
     struct mmc_command *cmd;
     struct mmc_data *data;
     struct mmc_command *stop;

     struct completion completion;
     void (*done)(struct mmc_request *);/* completion function */
     struct mmc_host *host;
};
struct mmc_blk_request {//存放一个完整的mmc_request，包含它所有内容
     struct mmc_request mrq;
     struct mmc_command sbc;
     struct mmc_command cmd;
     struct mmc_command stop;
     struct mmc_data data;
};
struct mmc_async_req {
     struct mmc_request *mrq;/* active mmc request */
     int (*err_check) (struct mmc_card *, struct mmc_async_req *);
};
struct mmc_queue_req {//用来管理curr和prev两个mmc_request,
     struct request *req;     //
     struct mmc_blk_request brq;
     struct scatterlist *sg;
     char *bounce_buf;
     struct scatterlist *bounce_sg;
     unsigned int bounce_sg_len;
     struct mmc_async_req mmc_active;//有效mmc_request，封装了额外err_check函数。
     enum mmc_packed_type cmd_type; struct mmc_packed *packed;//ｍｍｃ卡才会用到，ｓｄ卡没用，不会调用mmc_packed_init
};
struct mmc_queue {
    struct mmc_card        *card;
    struct task_struct    *thread;
    struct semaphore    thread_sem;
    unsigned int        flags;
    int            (*issue_fn)(struct mmc_queue *, struct request *);//从mmc_queue取request准备issue然后发送
    void            *data;
    struct request_queue    *queue;//实际的请求队列，包含自己的ops函数集
    struct mmc_queue_req    mqrq[2];//保存两个完整的request，整个流程中很重要
    struct mmc_queue_req    *mqrq_cur;//当前发送请求
    struct mmc_queue_req    *mqrq_prev;//上一个发送请求
};
struct mmc_card {／／主要是保留卡的硬件属性相关信息
     struct mmc_host *host; /* the host this device belongs to */
     struct device dev; /* the device */
     unsigned int rca; /* relative card address of device 卡地址*/
     unsigned int type; /* card type */　//#define MMC_TYPE_SD 1 /* SD card */
     unsigned int state; /* (our) card state 卡检测，保护，只读等*/
     unsigned int quirks; /* card quirks */

//卡擦除参数
     unsigned int erase_size; /* erase size in sectors */
     unsigned int erase_shift; /* if erase unit is power 2 */
     unsigned int pref_erase; /* in sectors */
     u8 erased_byte; /* value of erased bytes */

//卡硬件参数属性信息
     u32 raw_cid[4]; /* raw card CID */u32 raw_csd[4]; /* raw card CSD */u32 raw_scr[2]; /* raw card SCR */
     struct mmc_cid cid; /* card identification */struct mmc_csd csd; /* card specific */
     struct mmc_ext_csd ext_csd; /* mmc v4 extended card specific */struct sd_scr scr; /* extra SD information */
     struct sd_ssr ssr; /* yet more SD information */struct sd_switch_caps sw_caps; /* switch (CMD6) caps */
//sdio相关信息
     unsigned int sdio_funcs; /* number of SDIO functions */
     struct sdio_cccr cccr; /* common card info */
     struct sdio_cis cis; /* common tuple info */
     struct sdio_func *sdio_func[SDIO_MAX_FUNCS]; /* SDIO functions (devices) */
     struct sdio_func *sdio_single_irq; /* SDIO function when only one IRQ active */
     unsigned num_info; /* number of info strings */
     const char **info; /* info strings */
     struct sdio_func_tuple *tuples; /* unknown common tuples */

     unsigned int sd_bus_speed; /* Bus Speed Mode set for the card */

     struct dentry *debugfs_root;
     struct mmc_part part[MMC_NUM_PHY_PARTITION]; /* physical partitions 物理分区*/
     unsigned int nr_parts;　／／分区的个数
};

struct gendisk系列数据结构：
struct callback_head {
     struct callback_head *next;
     void (*func)(struct callback_head *head);
};
#define rcu_head callback_head
struct hd_struct {
     sector_t start_sect;//分区起始物理扇区号
/** nr_sects is protected by sequence counter. One might extend a* partition while IO is happening to it and update of nr_sects
* can be non-atomic on 32bit machines with 64bit sector_t.*/
     sector_t nr_sects;//分区总扇区个数
     seqcount_t nr_sects_seq;
     sector_t alignment_offset;
     unsigned int discard_alignment;
     struct device __dev;
     struct kobject *holder_dir;
     int policy, partno;
     struct partition_meta_info *info;
#ifdef CONFIG_FAIL_MAKE_REQUEST
int make_it_fail;
#endif
     unsigned long stamp;
     atomic_t in_flight[2];
#ifdef CONFIG_SMP
struct disk_stats __percpu *dkstats;
#else
struct disk_stats dkstats;
#endif
     atomic_t ref;
     struct rcu_head rcu_head;
};
struct disk_part_tbl {
     struct rcu_head rcu_head;
     int len;
     struct hd_struct __rcu *last_lookup;
     struct hd_struct __rcu *part[];//0长度数组，part[0]就是下面定义的struct hd_struct part0;
};
struct gendisk {
/* major, first_minor and minors are input parameters only,* don't use directly. Use disk_devt() and disk_max_parts().*/
     int major; /* major number of driver */
     int first_minor;
     int minors; /* maximum number of minors, =1 for  * disks that can't be partitioned. *///可能的分区个数

     char disk_name[DISK_NAME_LEN]; /* name of major driver *///设备名称，对应主设备号
     char *(*devnode)(struct gendisk *gd, umode_t *mode);

     unsigned int events; /* supported events */
     unsigned int async_events; /* async events, subset of all */

/* Array of pointers to partitions indexed by partno.* Protected with matching bdev lock but stat and other
* non-critical accesses use RCU. Always access through* helpers.*/

     struct disk_part_tbl __rcu *part_tbl;//分区链表，初始化指向下面的part0是第一个，然后链表
     struct hd_struct part0;

     const struct block_device_operations *fops;//块设备ｏｐｓ
     struct request_queue *queue;
     void *private_data;

int flags;
struct device *driverfs_dev; // FIXME: remove
struct kobject *slave_dir;

struct timer_rand_state *random;
atomic_t sync_io; /* RAID */
struct disk_events *ev;
#ifdef CONFIG_BLK_DEV_INTEGRITY
struct blk_integrity *integrity;
#endif
int node_id;
};
struct mmc_blk_data {//gendisk的扩展，支配一个queue，在mmc_blk_alloc_req()函数分配和初始化
     spinlock_t lock;
     struct gendisk *disk;
     struct mmc_queue queue;／／每个分区一个ｑｕｅｕｅ
     struct list_head part;//一个ｓｄ卡所有相关分区通过part连接在一起

     unsigned int flags;
#define MMC_BLK_CMD23 (1 << 0) /* Can do SET_BLOCK_COUNT for multiblock */
#define MMC_BLK_REL_WR (1 << 1) /* MMC Reliable write support */
#define MMC_BLK_PACKED_CMD (1 << 2) /* MMC packed command support */

     unsigned int usage;          //初始化为1,mmc_blk_get/mmc_blk_put,发现为0的时候free?
     unsigned int read_only;      //是否只读
     unsigned int part_type;      //
     unsigned int name_idx;
     unsigned int reset_done;
#define MMC_BLK_READ BIT(0)
#define MMC_BLK_WRITE BIT(1)
#define MMC_BLK_DISCARD BIT(2)
#define MMC_BLK_SECDISCARD BIT(3)

/*
* Only set in main mmc_blk_data associated
* with mmc_card with mmc_set_drvdata, and keeps
* track of the current selected device partition.
*/
     unsigned int part_curr;
     struct device_attribute force_ro;
     struct device_attribute power_ro_lock;
     int area_type;
};

static inline struct page *read_mapping_page(struct address_space *mapping, pgoff_t index, void *data)
     filler = (filler_t *)mapping->a_ops->readpage;
     return read_cache_page(mapping, index, filler, data);
         struct page *page = read_cache_page_async(mapping, index, filler, data)
              page = __read_cache_page(mapping, index, filler, data, gfp);
              err = filler(data, page);    //
         return wait_on_page_read(page);

genhd.c文件入口
subsys_initcall(genhd_device_init);
static int __init genhd_device_init(void)
     block_class.dev_kobj = sysfs_dev_block_kobj;     error = class_register(&block_class);
     bdev_map = kobj_map_init(base_probe, &block_class_lock);
     blk_dev_init();
     register_blkdev(BLOCK_EXT_MAJOR, "blkext");
     block_depr = kobject_create_and_add("block", NULL);/* create top-level block dir */
int __init blk_dev_init(void)
     kblockd_workqueue = alloc_workqueue("kblockd",WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
     request_cachep = kmem_cache_create("blkdev_requests",sizeof(struct request), 0, SLAB_PANIC, NULL);
     blk_requestq_cachep = kmem_cache_create("blkdev_queue",sizeof(struct request_queue), 0, SLAB_PANIC, NULL);

很多disk_event相关的函数，暂时不要关注

mmc blk dev_ops入口:
static const struct block_device_operations mmc_bdops = {//mmc_blk_alloc_req()中注册为gendisk的fops
     .open = mmc_blk_open,.release = mmc_blk_release,.getgeo = mmc_blk_getgeo,
     .owner = THIS_MODULE,.ioctl = mmc_blk_ioctl,
};
int mmc_blk_open(struct block_device *bdev, fmode_t mode)
     struct mmc_blk_data *md = mmc_blk_get(bdev->bd_disk);
          md = disk->private_data;
          md->usage++;
void mmc_blk_release(struct gendisk *disk, fmode_t mode)
     struct mmc_blk_data *md = disk->private_data;
     mmc_blk_put(md);
          md->usage--;
          if (md->usage == 0)     put_disk(md->disk);
int mmc_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
     geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
     geo->heads = 4;
     geo->sectors = 16;
int mmc_blk_ioctl(struct block_device *bdev, fmode_t mode,unsigned int cmd, unsigned long arg)
     ret = mmc_blk_ioctl_cmd(bdev, (struct mmc_ioc_cmd __user *)arg);
          struct mmc_blk_ioc_data *idata = mmc_blk_ioctl_copy_from_user(ic_ptr);
          md = mmc_blk_get(bdev->bd_disk);
          card = md->queue.card;//下面是mmc发送命令和数据的基本方式
          cmd.opcode = idata->ic.opcode; 　 cmd.arg = idata->ic.arg;     cmd.flags = idata->ic.flags;
          mrq.cmd = &cmd;
          if (idata->buf_bytes) {//设置数据请求
               data.sg = &sg;data.sg_len = 1;data.blksz = idata->ic.blksz;data.blocks = idata->ic.blocks;
               sg_init_one(data.sg, idata->buf, idata->buf_bytes);
               mmc_set_data_timeout(&data, card);
               mrq.data = &data;
                         }
          mmc_claim_host(card->host);
          err = mmc_blk_part_switch(card, md);
          if (idata->ic.is_acmd)    err = mmc_app_cmd(card->host, card);//MMC_APP_CMD(55 RCA)
          if (is_rpmb)              err = mmc_set_blockcount(card, data.blocks,idata->ic.write_flag & (1 << 31));
          mmc_wait_for_req(card->host, &mrq);
          copy_to_user(&(ic_ptr->response), cmd.resp, sizeof(cmd.resp));
          if (!idata->ic.write_flag)copy_to_user((void __user *)idata->ic.data_ptr,idata->buf, idata->buf_bytes);
          mmc_release_host(card->host);
          mmc_blk_put(md);kfree(idata->buf);kfree(idata);
struct mmc_blk_ioc_data {
     struct mmc_ioc_cmd ic;
     unsigned char *buf;
     u64 buf_bytes;};//分配一个mmc_blk_ioc_data和其中buf，拷贝ic_ptr用户数据
struct mmc_blk_ioc_data *mmc_blk_ioctl_copy_from_user(ic_ptr);
     idata = kzalloc(sizeof(*idata), GFP_KERNEL);
     copy_from_user(&idata->ic, user, sizeof(idata->ic));//获取命令
     idata->buf_bytes = (u64) idata->ic.blksz * idata->ic.blocks;//计算buf大小
     idata->buf = kzalloc(idata->buf_bytes, GFP_KERNEL);//分配buf
     copy_from_user(idata->buf, (void __user *)(unsigned long)idata->ic.data_ptr, idata->buf_bytes);//copy数据

mpage_writepages解析Plug
int mpage_writepages(struct address_space *mapping, struct writeback_control *wbc, get_block_t get_block)
     struct blk_plug plug;//mpage_writepages一般在回写函数中调用
     blk_start_plug(&plug);
          struct task_struct *tsk = current; tsk->plug = plug;
     //把紧密耦合的数据块先plug，全部request完毕后unplug，再一起加入到块设备的queue唤醒处理thread
     //request_queue线程取request需要spin_lock_irq(q->queue_lock);如果每个request进入queue也都要lock，必然导致频繁的竞争
     //所以通过plug的方式，减少竞争。但是，不同文件之间，好像无法从这个机制中受益。那现在我只需要考虑ｄｉｏ的方式下，不同文件之间如何
     //受益就好了吧。
     if(!get_block)
     ret = generic_writepages(mapping, wbc);
else {
     struct mpage_data mpd = {
         .bio = NULL,.last_block_in_bio = 0,.get_block = get_block,.use_writepage = 1,};
     ret = write_cache_pages(mapping, wbc, __mpage_writepage, &mpd);
     if (mpd.bio)     mpage_bio_submit(WRITE, mpd.bio)
}
     blk_finish_plug(&plug);　//阻塞调用
          blk_flush_plug_list(plug, false); current->plug = NULL;//
               flush_plug_callbacks(plug, from_schedule);//struct blk_plug *plug->cb_list
                    foreach_in_cb_list      cb->callback(cb, from_schedule);
               foreach rq = list_entry_rq(list.next);　q = rq->q;
                    queue_unplugged(q, depth, from_schedule);//
                         if (from_schedule)
                              blk_run_queue_async(q);-->mod_delayed_work(kblockd_workqueue, &q->delay_work, 0);
                         else
                              __blk_run_queue(q);--->q->request_fn(q);//mmc_request_fn

void bdi_start_background_writeback(struct backing_dev_info *bdi)
static void end_bio_bh_io_sync(struct bio *bio, int err)
     struct buffer_head *bh = bio->bi_private;
     bh->b_end_io(bh, test_bit(BIO_UPTODATE, &bio->bi_flags));
     bio_put(bio);
ll_rw_block在buffer.c
struct buffer_head *bhs[]-->bio-->request->mmc_request的数据流过程
应该把write和read两个函数的实现放在这个文档中。
void ll_rw_block(int rw, int nr, struct buffer_head *bhs[])//write发送多个buffer，mapping层到block层
     submit_bh(WRITE, bh);
         bio = bio_alloc(GFP_NOIO, 1);
         bio->bi_end_io = end_bio_bh_io_sync;
         submit_bio(int rw, struct bio *bio);　//blk_core.c内核block层提供给buffer.c的接口
              generic_make_request(struct bio *bio);
                   struct request_queue *q = bdev_get_queue(bio->bi_bdev);
                   q->make_request_fn(q, bio); //blk_queue_bio 回调函数的方式实现分层模型,后续进入queue相关处理
void blk_queue_bio(struct request_queue *q, struct bio *bio)//bio添加到reqeust_queue
     blk_queue_bounce(q, &bio);
     if (attempt_plug_merge(q, bio, &request_count))return;//尝试合并到当前plug的链表，然后尝试其他合并
     //优先后向合并，一般写入地址是递增的，所以hash表以上个buffer的结束地址为索引，新的请求添加到前面buffer的后面，所以成为后向合并。
     req = get_request(q, rw_flags, bio, GFP_NOIO);//获取一个free的struct request
     init_request_from_bio(req, bio);//用bio封装初始化request
     plug = current->plug;
     if (plug){
          list_add_tail(&req->queuelist, &plug->list);     drive_stat_acct(req, 1);
     }else{
          add_acct_request(q, req, where);//新的request添加到request_queue
               __elv_add_request(q, rq, where);
          __blk_run_queue(q);-->__blk_run_queue_uncond(q);
              q->request_fn(q);　//mmc_request_fn//上层发送bio到request_queue，可能唤醒thread处理
     }
static void mmc_request_fn(struct request_queue *q)     //进入回调函数继续解析上面流程
     wake_up_interruptible(&cntx->wait);
     or wake_up_process(mq->thread);--->唤醒线程mmc_queue_thread

request_queue线程入口
static int mmc_queue_thread(void *d)//顺序发送块设备request_queue内容,不会再合并优化
     struct mmc_queue *mq = d;
     struct request_queue *q = mq->queue;
     while(request_queue非空){
          struct request * req = blk_fetch_request(q);
               rq = blk_peek_request(q);
               if (rq)blk_start_request(rq);
                    blk_dequeue_request(req);　blk_add_timer(req);//摘取req，分配发送timer，发送结束停止timer
          mq->mqrq_cur->req = req;
          mq->issue_fn(mq, req); //mmc_blk_issue_rq，发送从request_queue中取出的request
          mq->mqrq_prev->brq.mrq.data = NULL;  mq->mqrq_prev->req = NULL;//清空request和data之后会赋这个空值给mqrq_cur
          tmp = mq->mqrq_prev; mq->mqrq_prev = mq->mqrq_cur;     mq->mqrq_cur = tmp;
          //当前request变成prev的，因为发送没结束，才定义的mq->mqrq_prev，发送完毕会复位它
          if(request_queue空){
               up(&mq->thread_sem);schedule();down(&mq->thread_sem); }//线程standby
     }

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req) //mq->issue_fn
     if (req && !mq->mqrq_prev->req)/* claim host only for the first request */
          mmc_claim_host(card->host);//处理第一个request的时候需要claim host
          mmc_blk_part_switch(card, md);//sd卡并不需要做什么工作，ｍｍｃ调用mmc_switch()好像完成什么mode切换
          struct mmc_blk_data *main_md = mmc_get_drvdata(card);
          main_md->part_curr = md->part_type;return 0;
          //不关注mmc_blk_issue_discard_rq和mmc_blk_issue_flush这两个非正常request
          mmc_blk_issue_rw_rq(mq, req);
               mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
                    //struct mmc_blk_request封装和初始化一个request，准备好mq->mqrq_cur->mmc_active
               areq = &mq->mqrq_cur->mmc_active;
               areq = mmc_start_req(card->host, areq, (int *) &status);//mmc_start_req调用__mmc_start_data_req专门发送数据
                    if (areq) mmc_pre_req(host, areq->mrq, !host->areq);//调用下层驱动dw_mci_pre_req，判断data是否对dma有效。
                    if(host->areq)
                         err = mmc_wait_for_data_req_done(host, host->areq->mrq, areq);//等上个request发送完毕
                              err = host->areq->err_check(host->card,host->areq); //mmc_blk_err_check
                              //mmc_wait_for_req专门发送cmd，mrq->done = mmc_wait_done;
                         start_err = __mmc_start_data_req(host, areq->mrq);
                              mrq->done = mmc_wait_data_done;     mrq->host = host;
                              mmc_start_request(host, mrq);
                                   mrq->cmd->error = 0;           mrq->cmd->mrq = mrq;
                                   host->ops->request(host, mrq); //调用下层驱动dw_mci_request()
                    if(host->areq) mmc_post_req(host, host->areq->mrq, 0);//前后两个request的post处理
                         host->ops->post_req(host, mrq, err);//调用下层驱动dw_mci_post_req
                    if(areq)       mmc_post_req(host, areq->mrq, -EINVAL);
                    host->areq = areq;//host只保存处理中的areq，struct mmc_async_req比mmc_request多了err_check()
               switch(status){
                    case MMC_BLK_SUCCESS://发送成功
                         mmc_blk_reset_success(md, type);
                         ret = blk_end_request(req, 0,brq->data.bytes_xfered);-->blk_end_bidi_request-->blk_finish_request
                              -->struct request *req->end_io(req, error);//回调，一般unlock buffer/page.
void mmc_blk_rw_rq_prep(struct mmc_queue_req *mqrq,struct mmc_card *card,int disable_multi,struct mmc_queue *mq)
     struct mmc_blk_request *brq = &mqrq->brq;  struct request *req = mqrq->req; struct mmc_blk_data *md = mq->data;
     brq->mrq.cmd = &brq->cmd;  brq->mrq.data = &brq->data;
     brq->cmd.arg = blk_rq_pos(req);-->return rq->__sector;
     if (!mmc_card_blockaddr(card))     brq->cmd.arg <<= 9;//地址2次运算
     brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
     brq->data.blksz = 512;     brq->data.blocks = blk_rq_sectors(req);-->(rq->__data_len>>9)
     brq->stop.opcode = MMC_STOP_TRANSMISSION; brq->stop.arg = 0; brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
     if (brq->data.blocks > 1){
          brq->mrq.stop = &brq->stop; readcmd = MMC_READ_MULTIPLE_BLOCK(18);     writecmd = MMC_WRITE_MULTIPLE_BLOCK(25);
     }else{
          brq->mrq.stop = NULL;     readcmd = MMC_READ_SINGLE_BLOCK(17);writecmd = MMC_WRITE_BLOCK(24);
     }
     if (rq_data_dir(req) == READ){brq->cmd.opcode = readcmd;     brq->data.flags |= MMC_DATA_READ;}
     else{brq->cmd.opcode = writecmd;     brq->data.flags |= MMC_DATA_WRITE;}
     if(sbc使能){
          brq->sbc.opcode = MMC_SET_BLOCK_COUNT;  brq->sbc.arg = brq->data.blocks |(do_rel_wr ? (1 << 31) : 0) |(do_data_tag ? (1 << 29) : 0);
          brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;     brq->mrq.sbc = &brq->sbc;
     }
     mmc_set_data_timeout(&brq->data, card);
     brq->data.sg = mqrq->sg;     brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);
     mqrq->mmc_active.mrq = &brq->mrq;     mqrq->mmc_active.err_check = mmc_blk_err_check;
     mmc_queue_bounce_pre(mqrq);

void blk_delay_queue(struct request_queue *q, unsigned long msecs)
     queue_delayed_work(kblockd_workqueue, &q->delay_work, msecs_to_jiffies(msecs));
static void blk_delay_work(struct work_struct *work)
     struct request_queue *q = container_of(work, struct request_queue, delay_work.work);
     __blk_run_queue(q);

## 1. mmc_bus_probe入口:永远放在最下，方便索引
设置bio到request的cb函数blk_queue_bio,request到queue的cb函数mmc_request_fn，queue线程和queue分发request的cb函数mmc_blk_issue_rq
分配和初始化gendisk,添加磁盘设备和各分区设备
为设备分配一个request_queue,
static struct mmc_driver mmc_driver = {//block.c
     .drv = {.name = "mmcblk",},     .remove = mmc_blk_remove,.suspend = mmc_blk_suspend,.resume = mmc_blk_resume,
     .probe = mmc_blk_probe,
};
static struct bus_type mmc_bus_type = {//bus.c
     .name = "mmc",     .dev_attrs = mmc_dev_attrs,     .uevent = mmc_bus_uevent,     .remove = mmc_bus_remove,
     .pm = &mmc_bus_pm_ops,          .match = mmc_bus_match,//总是返回１
     .probe = mmc_bus_probe,
};
### 1.1 static int __init mmc_blk_init(void)     //注册mmc块设备驱动
     res = register_blkdev(MMC_BLOCK_MAJOR, "mmc");
     res = mmc_register_driver(&mmc_driver);
          drv->drv.bus = &mmc_bus_type;     return driver_register(&drv->drv);     //注册mmc总线驱动，带bus的ops
### 1.2 static int mmc_bus_probe(struct device *dev)　//解析mmc块设备probe函数，dev在mmc_rescan中检测为struct mmc_card设备
     struct mmc_driver *drv = to_mmc_driver(dev->driver);
     struct mmc_card *card = mmc_dev_to_card(dev);
     return drv->probe(struct mmc_card * card);     //mmc_blk_probe
          md = mmc_blk_alloc(card);-->md = mmc_blk_alloc_req(card, &card->dev, size, false, NULL,MMC_BLK_DATA_AREA_MAIN);
          mmc_blk_alloc_parts(card, md) //为所有分区分配
               ---part_md = mmc_blk_alloc_req(card, disk_to_dev(md->disk), size, default_ro,....;part_md->part_type = part_type;
          mmc_set_drvdata(card, md);　-- card->dev->p->driver_data = md;//card的device的私有数据中的driver_data
          mmc_add_disk(md)//添加设备
               struct mmc_card *card = md->queue.card;
               add_disk(md->disk); ////添加disk设备，
                    retval = blk_alloc_devt(&disk->part0, &devt);//disk的struct device定义在disk->part0中
                    disk_to_dev(disk)->devt = devt;　disk->major = MAJOR(devt);　disk->first_minor = MINOR(devt);
                    bdi = &disk->queue->backing_dev_info;     bdi_register_dev(bdi, disk_devt(disk)); //注册bdi
                    blk_register_region(disk_devt(disk), disk->minors, NULL...
                    register_disk(disk);
                         struct device *ddev = disk_to_dev(disk);     device_add(ddev);
                    blk_register_queue(disk);//queue也需要注册kobject啊
                         struct request_queue *q = disk->queue;
                         kobject_add(&q->kobj, kobject_get(&dev->kobj), "%s", "queue");
                         kobject_uevent(&q->kobj, KOBJ_ADD);
                         ret = elv_register_queue(q);//是因为可能需要用户空间改变elv算法吗
                    retval = sysfs_create_link(&disk_to_dev(disk)->kobj, &bdi->dev->kobj,"bdi");
                    disk_add_events(disk);
          foreach(part) mmc_add_disk(part_md);//添加分区
### 1.3 struct mmc_blk_data *mmc_blk_alloc_req(struct mmc_card *card,struct device *parent,
     struct mmc_blk_data *md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);//分配一个struct mmc_blk_data
     md->disk = alloc_disk(perdev_minors);-->alloc_disk_node()//分配struct gendisk，mmc_blk_data就是gendisk的扩展
          struct gendisk *disk = kmalloc_node(sizeof(struct gendisk),GFP_KERNEL | __GFP_ZERO, node_id);
          init_part_stats(&disk->part0); disk->node_id = node_id; disk->minors = minors;
          disk_expand_part_tbl(disk, 0);//并不分配分区的新的struct hd_struct,只是增加了(struct hd_struct*)
               int size = sizeof(*new_ptbl) + (partno+1) * sizeof(new_ptbl->part[0]);
               new_ptbl = kzalloc_node(size, GFP_KERNEL, disk->node_id);new_ptbl->len = (partno+1);
               复制老的分区指针数组内容，然后rcu_assign_pointer(disk->part_tbl, new_ptbl);
          disk->part_tbl->part[0] = &disk->part0; //实际的第一个分区的struct hd_struct
     ret = mmc_init_queue(&md->queue, card, &md->lock, subname);//请求队列，实现块读写操作
          mq->card = card;　//struct mmc_queue * mq
          mq->queue = blk_init_queue(mmc_request_fn, lock);-->return blk_init_queue_node(rfn, lock, NUMA_NO_NODE);
               struct request_queue *uninit_q = blk_alloc_queue_node(GFP_KERNEL, node_id);//申请
                    申请struct request_queue*q并简单初始化,初始化q->backing_dev_info
                    setup_timer(&q->backing_dev_info.laptop_mode_wb_timer,laptop_mode_timer_fn, (unsigned long) q);
                    setup_timer(&q->timeout, blk_rq_timed_out_timer, (unsigned long) q);
                    INIT_DELAYED_WORK(&q->delay_work, blk_delay_work);return q;
               q = blk_init_allocated_queue(uninit_q, rfn, lock);//更多初始化，主要是注册上下两层request回调函数
                    q->request_fn = rfn;//mmc_request_fn
                    blk_queue_make_request(q, blk_queue_bio);
                         q->nr_requests = BLKDEV_MAX_RQ;q->nr_batching = BLK_BATCH_REQ;
                         q->make_request_fn = mfn;//blk_queue_bio　向request_queue中添加bio请求
          mq->thread = kthread_run(mmc_queue_thread, mq, ");   //轮询request_queue请求，调用mmc_blk_issue_rq处理
     md->queue.issue_fn = mmc_blk_issue_rq;
     md->disk->queue = md->queue.queue;   //上层看到的是...->disk->queue，上层blk_queue_bio发bio到disk的queue
     md->queue.data = md;     md->disk->private_data = md;
     md->disk->major = MMC_BLOCK_MAJOR;  md->disk->fops = &mmc_bdops;
     set_capacity(md->disk, size);
void blk_delay_work(struct work_struct *work)
     struct request_queue *q = container_of(work, struct request_queue, delay_work.work);
     __blk_run_queue(q);
int device_add(struct device *dev)
     dev_set_name(dev, "%s", dev->init_name);
     parent = get_device(dev->parent);//
     kobj = get_device_parent(dev, parent);
     kobject_add(&dev->kobj, dev->kobj.parent, NULL);//驱动设备模型相关的kobject处理
     device_create_file(dev, &uevent_attr);  device_add_class_symlinks(dev); device_add_attrs(dev);
     error = bus_add_device(dev);
     dpm_sysfs_add(dev);     device_pm_add(dev);     kobject_uevent(&dev->kobj, KOBJ_ADD);
     bus_probe_device(dev);-->device_attach(dev);
          -->bus_for_each_drv(dev->bus, NULL, dev, __device_attach);
               while ((drv = next_driver(&i)) && !error) error = fn(drv, data);//fn:__device_attach
int __device_attach(struct device_driver *drv, void *data)
     struct device *dev = data;
     if (!driver_match_device(drv, dev)) return 0;-->return drv->bus->match ? drv->bus->match(dev, drv):1;
     return driver_probe_device(drv, dev);//
          really_probe(dev, drv);
               dev->driver = drv;
               if (dev->bus->probe)     ret = dev->bus->probe(dev);//先尝试总线probe,mmc就是，但转头调用了driver的probe
               else     ret = drv->probe(dev);

