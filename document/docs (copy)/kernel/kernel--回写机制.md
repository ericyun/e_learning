# 回写机制
## 文件缓存回写的时机
(1)周期性回写，周期为dirty_writeback_interval，默认5s；
(2)块设备首次出现脏数据；
(3)脏页达到限额，包括dirty_bytes、dirty_background_bytes、dirty_ratio、dirty_background_ratio；
(4)剩余内存过少，唤醒所有回写线程；
(5)syscall sync，同步所有超级块
(6)syscall syncfs，同步某个超级块对应的文件系统
(7)syscall fsync/fdatasync，同步某个文件
(8)laptop模式，完成blk io request之后，启动laptop_mode_wb_timer；
(9)arm_dma_alloc() 导致的强制回写部分page.这个需要继续跟踪．

struct delayed_work {
     struct work_struct work;
     struct timer_list timer;
/* target workqueue and CPU ->timer uses to queue ->work */
     struct workqueue_struct *wq;
     int cpu;
};
struct bdi_writeback {//某个bdi的控制回写的数据结构，包含3个io链表，包含回写cb的work，并指向归属的bdi
     struct backing_dev_info *bdi; /* our parent bdi */
unsigned int nr;
unsigned long last_old_flush; /* last old data flush */
     struct delayed_work dwork; /* work item used for writeback *///实际控制回写timer和cb
     struct list_head b_dirty; /* dirty inodes *///io链表
          //通过inode = wb_inode(p_b_dirty->prev);获得
     struct list_head b_io; /* parked for writeback */ //io链表
     struct list_head b_more_io; /* parked for more writeback */ //io链表
     spinlock_t list_lock; /* protects the b_* lists */
};
struct wb_writeback_work {//怎么没看到具体的文件信息？
long nr_pages;
struct super_block *sb;
unsigned long *older_than_this;//有点疑问，为何指向stack中局部变量
enum writeback_sync_modes sync_mode;
unsigned int tagged_writepages:1;
unsigned int for_kupdate:1;
unsigned int range_cyclic:1;
     unsigned int for_background:1;//ratio下限写
enum wb_reason reason; /* why was writeback initiated? */

     struct list_head list; /* pending work list *///所有的work通过list建立链表加入到bdi的work_list
struct completion *done; /* set if the caller waits */
};
struct backing_dev_info {
     struct list_head bdi_list;//系统中所有bdi通过此node链接到global_bdi_list指针的链表
unsigned long ra_pages; /* max readahead in PAGE_CACHE_SIZE units */
unsigned long state; /* Always use atomic bitops on this */
unsigned int capabilities; /* Device capabilities */
congested_fn *congested_fn; /* Function pointer if device is md/dm */
void *congested_data; /* Pointer to aux data for congested func */

char *name;

struct percpu_counter bdi_stat[NR_BDI_STAT_ITEMS];

unsigned long bw_time_stamp; /* last time write bw is updated */
unsigned long dirtied_stamp;
unsigned long written_stamp; /* pages written at bw_time_stamp */
unsigned long write_bandwidth; /* the estimated write bandwidth */
unsigned long avg_write_bandwidth; /* further smoothed write bw */

/*
* The base dirty throttle rate, re-calculated on every 200ms.
* All the bdi tasks' dirty rate will be curbed under it.
* @dirty_ratelimit tracks the estimated @balanced_dirty_ratelimit
* in small steps and isbdi_init much more smooth/stable than the latter.
*/
unsigned long dirty_ratelimit;
unsigned long balanced_dirty_ratelimit;

struct fprop_local_percpu completions;
int dirty_exceeded;

unsigned int min_ratio;
unsigned int max_ratio, max_prop_frac;

     struct bdi_writeback wb; /* default writeback info for this bdi *///控制回写的结构，定义io链表和回写操作
     spinlock_t wb_lock; /* protects work_list */

     struct list_head work_list;//wb_writeback_work通过list节点链接到链表。
struct device *dev;
struct timer_list laptop_mode_wb_timer;
};

struct request_queue *blk_alloc_queue_node(gfp_t gfp_mask, int node_id)
     q = kmem_cache_alloc_node(blk_requestq_cachep,gfp_mask | __GFP_ZERO, node_id);
     err = bdi_init(&q->backing_dev_info);//backing_dev_info结构体初始化,属于request_queue的成员
          bdi_wb_init(&bdi->wb, bdi);//初始化bdi_writeback成员结构体，它属于backing_dev_info的成员
               wb->bdi = bdi;     wb->last_old_flush = jiffies;
               INIT_LIST_HEAD(&wb->b_dirty/b_io/b_more_io);
               INIT_DELAYED_WORK(&wb->dwork, bdi_writeback_workfn);//这是bdi_wb_init()最重要的工作。
     setup_timer(&q->backing_dev_info.laptop_mode_wb_timer,laptop_mode_timer_fn, (unsigned long) q);
     ... ...后面的和bdi无关了
//请注意request_queue和它的bdi是跟着块设备走的，不是某个分区或文件系统
void add_disk(struct gendisk *disk)
     ... ...
     bdi = &disk->queue->backing_dev_info;
     bdi_register_dev(bdi, disk_devt(disk));-->bdi_register
          bdi->dev = dev;
          list_add_tail_rcu(&bdi->bdi_list, &bdi_list);//进入全局bdi链表
     register_disk(disk);
mmc_bus_probe-->mmc_blk_probe的过程中创建了bdi

struct workqueue_struct *bdi_wq;
struct backing_dev_info default_backing_dev_info = {
     .name = "default",     .ra_pages = VM_MAX_READAHEAD * 1024 / PAGE_CACHE_SIZE,     .state = 0,
     .capabilities = BDI_CAP_MAP_COPY,
};
struct backing_dev_info noop_backing_dev_info = {
     .name = "noop",     .capabilities = BDI_CAP_NO_ACCT_AND_WRITEBACK,
};
int __init default_bdi_init(void)//创建回写工作队列，然后是两个bdi
     bdi_wq = alloc_workqueue("writeback", WQ_MEM_RECLAIM | WQ_FREEZABLE |WQ_UNBOUND | WQ_SYSFS, 0);
     err = bdi_init(&default_backing_dev_info);
     bdi_register(&default_backing_dev_info, NULL, "default");
     err = bdi_init(&noop_backing_dev_info);//没有writeback的需要



跟踪内存swap函数，它们会降低效率，看看是否频繁调用。
void wakeup_flusher_threads(long nr_pages, enum wb_reason reason)//启动回写
     __bdi_start_writeback(bdi, nr_pages, false, reason);
          struct wb_writeback_work *work = kzalloc(sizeof(*work), GFP_ATOMIC);
          work->sync_mode = WB_SYNC_NONE; work->nr_pages = nr_pages; work->range_cyclic = range_cyclic;  work->reason = reason;
          bdi_queue_work(bdi, work);
               list_add_tail(&work->list, &bdi->work_list);
               mod_delayed_work(bdi_wq, &bdi->wb.dwork, 0);//之后立刻启动work
队列多个入口，//反向引用树中，尽量弄清楚传递的参数。
bdi_queue_work 引用树，添加新的work到bdi的work_list
     sync_inodes_sb  //.sync_mode = WB_SYNC_ALL,.sb = sb,.reason = WB_REASON_SYNC,
          //sync()系列接口调用
     writeback_inodes_sb_nr //.sb = sb,.sync_mode = WB_SYNC_NONE, .tagged_writepages = 1,指定sb写入若干page，不等待io完成
          //sync()系列接口调用
          writeback_inodes_sb //写入get_nr_dirty_pages()个page，好像是所有的page
               sync_filesystem-->__sync_filesystem //reason: WB_REASON_SYNC
          try_to_writeback_inodes_sb //ext4调用
     __bdi_start_writeback //sync_mode: WB_SYNC_NONE
          wakeup_flusher_threads
               free_more_memory //要求回写1024个page！！效率？range_cyclic：false
                    alloc_page_buffers
                    __getblk_slow-->__getblk //短期大量写入，导致回写释放的内存无法满足需要，强制flush.
               SYSCALL_DEFINE0(sync)
               do_try_to_free_pages
          bdi_start_writeback
               //不需要关注laptop <--- laptop_mode_timer_fn
bdi_start_background_writeback //立刻启动回写
     balance_dirty_pages
          balance_dirty_pages_ratelimited
               generic_perform_write<---write()//最重要,写入时遇到background阈值，启动回写过程，不额外增加新work。
               set_page_dirty_balance
               ... ...//好多
如果没有写完，如何处理？
background回写，一次自动写多少？
writeback_sb_inodes / __writeback_inodes_wb 函数中有100ms的回写时间限制
     如果这样，那么，反而不是最优化的写入办法，因为每次只能写入100ms
     不过如果是单个文件，那么应该是把大部分数据都更新了吧？不完整的数据呢？是否也更新了？所以上层一定要对齐写入。

fat_write_inode
wb_check_old_data_flush 函数，wb->last_old_flush表示上次回写的时间，不是inode的dirty时间，这是要保证5s至少回写一次，
b_more_io ？？？？？？
wb_check_background_flush 在 wb_check_old_data_flush 之后，而且不会更新last_old_flush，这真的合适么？感觉效率有问题啊
因为这样是每回写时间到，都会强制回写一次。
原有的顺序，设定的是buffer需要足够大，这样5s时间先到，回写，wb_check_background_flush不需要执行了。我们(9,16)的定义，就有问题，5s时间到，

redirty_tail
应用team，写入临时文件，什么流程，会否产生消极影响？
不写入inode，同时，每次写入2048个page; 不知道效率有多少提升。
     中断次数优化不多，但是占用资源情况呢？访问mmc的时候，是否是阻塞的呢？
     q3f上测试看看，改变回写算法，是否有所优化。

## sync.c
SYSCALL_DEFINE1(fsync, unsigned int, fd) --> do_fsync(fd, 0);
SYSCALL_DEFINE1(fdatasync, unsigned int, fd) --> do_fsync(fd, 1);
     struct fd f = fdget(fd);
     if (f.file) ret = vfs_fsync(f.file, datasync); -->vfs_fsync_range(file, 0, LLONG_MAX, datasync);
          file->f_op->fsync(file, start, end, datasync); //fat_file_fsync
               struct inode *inode = filp->f_mapping->host;
               res = generic_file_fsync(filp, start, end, datasync);
               err = sync_mapping_buffers(MSDOS_SB(inode->i_sb)->fat_inode->i_mapping);




