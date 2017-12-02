# fat32 code

### 修订记录
| 修订说明 | 日期 | 作者 | 额外说明 |
| --- |
| 初版 | 2017/09/14 | 员清观 | 常用操作索引，尽量简短方便使用 |

----
## 1 当前整理文档

```bash
移植fsck程序来实现fat32文件系统的分析，学习，改进．
~~~~~~~ 虚拟机调试linux内核 ~~~~~~~
增加风险评估部分．
风险评估，由于时间限制，仍然存在某些部分逻辑理解上的盲区．现在采用的应对办法是，通过长时间测试，确认了这些接口在常用的操作中从没有被调用过．
```

```bash
参考资源
网页链接：
     和firefox的书签项如何分工？作为只是管理这里更加方便，而且也很方便访问。书签更多应该作为常用网址，而不是知识收集。
     能否建立一个快捷链接，直接搜索read()/write()的生命旅程之
     read()/write()的生命旅程
```


```
本文档索引信息：(名称必须唯二)

- void bdi_writeback_workfn(struct work_struct *work) 或 文件系统回写
- genhd.c文件入口

- 可能的优化:
```

```
可能的优化:

简化内存页缓冲机制
不支持buffer_head的方式,现在最主要的开销可能是write_begin和每次copy只有4k和write_begin.这部分如果能够优化,也许cpu的性能可以节省一点点.
generic_perform_write 函数简化为直接copy数据,可以提高性能.

27000中断, 每0.5s写对应225次中断,实际写集中在更短时间内完成,可能一个33ms对应了将近100次中断,开销比较大.
有没测量过,一个帧处理过程有多久呢? 比如,如果需要的时间是可变的,比如i帧产生时间久,那么,回写就避开这个时间点.或者,某个isp比较快,就腾出时间来做更多的事情.
isp处理过程中,需要用到那些系统资源,是否和别的线程竞争.
改成实时的,会否导致回写延后进行,导致内存资源得不到释放而紧张呢?

!!!! 使用延时分配策略,所有的簇分配集中进行,会产生资源的burst. 另外,分配簇需要读取fat表,fat表中一个扇区对应512k的数据区,2m的数据,对应4个扇区,可以考虑一次读取4个扇区应该可以减少一点点时间吧

Writeback机制的好处总结起来主要是两点：
1.加快write()的响应速度。因为media的读写相对于内存访问是较慢的。如果每个write()都访问media，势必很慢。将较慢的media访问交给writeback thread，而write()本身的thread里只在内存里操作数据，将数据交到writeback queue即返回。
2.便于合并和排序 (merge and sort) 多个write，merge是将多个少量数据的write合并成几个大量数据的write，减少访问media的次数；sort是将无序的write按照其访问media上的block的顺序排序，减少磁头在media上的移动距离。
```

buffer相关的大量无法索引到的symbol，实际上是一些宏而已，在buffer_head.h中定义，搜TAS_BUFFER_FNS　BUFFER_FNS
BUFFER_FNS(Uptodate, uptodate)
BUFFER_FNS(Dirty, dirty)
TAS_BUFFER_FNS(Dirty, dirty)
BUFFER_FNS(Lock, locked)
BUFFER_FNS(Req, req)
TAS_BUFFER_FNS(Req, req)
BUFFER_FNS(Mapped, mapped)
BUFFER_FNS(New, new)
BUFFER_FNS(Async_Read, async_read)
BUFFER_FNS(Async_Write, async_write)
BUFFER_FNS(Delay, delay)
BUFFER_FNS(Boundary, boundary)
BUFFER_FNS(Write_EIO, write_io_error)
BUFFER_FNS(Unwritten, unwritten)
BUFFER_FNS(Meta, meta)
BUFFER_FNS(Prio, prio)
linux大部分文件中定义的变量和函数是static的，所以，我们只要关注文件中被ＥＸＰＯＲＴ或者注册的ops就好.再结合数据结构，架构就很清晰了。
比较一下最新版本内核，ＦＡＴ部分有没有大的变化。
多个文件并行写入的过程就是不同文件的元数据和数据交叉写入的过程，而元数据和数据在存储设备上位于不同的位置，优化的目标就是尽量安排好他们的次序，以符合效率两原则。
元数据　： fsinfo扇区(空闲帧数等信息)　目录项信息　ｆａｔ表簇号 数据：　文件内容数据。

有时间的时候，应该整理一下buffer.c中的几个重要函数

fat_add_cluster()函数，如果直接分配４个ｃｌｕｓｔｅｒ，不知道会不会好点呢？

struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping; 这个bd_inode什么玩意

```cpp
自下而上，函数调用关系逆向跟踪：
.write_begin() == fat_write_begin()
  generic_perform_write
  generic_file_buffered_write
  __generic_file_aio_write
  struct file_operations fat_file_operations.aio_write == generic_file_aio_write
  do_sync_write
  blkdev_aio_write
  块设备本身作为文件被访问
  使用　struct address_space_operations def_blk_aops
  pagecache_write_begin
```

```cpp
struct buffer_head {
     unsigned long b_state; /* buffer state bitmap (see above) */
     struct buffer_head *b_this_page;/* circular list of page's buffers */
     struct page *b_page; /* the page this bh is mapped to */

     sector_t b_blocknr; /* start block number 当前buffer head对应的文件中逻辑块号*/
     size_t b_size; /* size of mapping */
     char *b_data; /* pointer to data within the page */

     struct block_device *b_bdev;
     bh_end_io_t *b_end_io; /* I/O completion */
       void *b_private; /* reserved for b_end_io */
     struct list_head b_assoc_buffers; /* associated with another mapping */
     struct address_space *b_assoc_map; /* mapping this buffer is associated with */
     atomic_t b_count; /* users using this buffer_head */
};

struct super_block {
struct list_head s_list; /* Keep this first */
dev_t s_dev; /* search index; _not_ kdev_t */
unsigned char s_blocksize_bits;
unsigned long s_blocksize;
loff_t s_maxbytes; /* Max file size */ //初始化为0xffffffff
struct file_system_type *s_type;
const struct super_operations *s_op;//超级块函数集
const struct dquot_operations *dq_op;
const struct quotactl_ops *s_qcop;
const struct export_operations *s_export_op;
unsigned long s_flags; //|=MS_NODIRATIME，不需要更新目录更新时间
unsigned long s_magic; //MSDOS_SUPER_MAGIC,fat32文件系统
struct dentry *s_root;     ／／根目录的dentry，链接"/"和对应boot扇区0的inode
struct rw_semaphore s_umount;
int s_count;
atomic_t s_active;
const struct xattr_handler **s_xattr;

struct list_head s_inodes; /* all inodes */
struct hlist_bl_head s_anon; /* anonymous dentries for (nfs) exporting */
struct list_head s_files;
struct list_head s_mounts; /* list of mounts; _not_ for fs use */
/* s_dentry_lru, s_nr_dentry_unused protected by dcache.c lru locks */
struct list_head s_dentry_lru; /* unused dentry lru */
int s_nr_dentry_unused; /* # of dentry on lru */

/* s_inode_lru_lock protects s_inode_lru and s_nr_inodes_unused */
spinlock_t s_inode_lru_lock ____cacheline_aligned_in_smp;
struct list_head s_inode_lru; /* unused inode lru */
int s_nr_inodes_unused; /* # of inodes on lru */

struct block_device *s_bdev;
struct backing_dev_info *s_bdi;
struct mtd_info *s_mtd;
struct hlist_node s_instances;
struct quota_info s_dquot; /* Diskquota specific options */

struct sb_writers s_writers;

char s_id[32]; /* Informational name */
u8 s_uuid[16]; /* UUID */

void *s_fs_info; /* Filesystem private info */     //指向struct msdos_sb_info结构，fat32的管理信息
unsigned int s_max_links;
fmode_t s_mode;

/* Granularity of c/m/atime in ns.
Cannot be worse than a second */
u32 s_time_gran;

/*
* The next field is for VFS *only*. No filesystems have any business
* even looking at it. You had been warned.
*/
struct mutex s_vfs_rename_mutex; /* Kludge */

/*
* Filesystem subtype. If non-empty the filesystem type field
* in /proc/mounts will be "type.subtype"
*/
char *s_subtype;

/*
* Saved mount options for lazy filesystems using
* generic_show_options()
*/
char __rcu *s_options;
const struct dentry_operations *s_d_op; /* default d_op for dentries */

/*
* Saved pool identifier for cleancache (-1 means none)
*/
int cleancache_poolid;

struct shrinker s_shrink; /* per-sb shrinker handle */

/* Number of inodes with nlink == 0 but still referenced */
atomic_long_t s_remove_count;

/* Being remounted read-only */
int s_readonly_remount;
};

__fat_write_inode 是否可以合并，同时写入多个文件的msdos_dir_entry信息
fat_generic_ioctl 是否可以用来传递应用相关的文件属性信息，基于此做优化
```

```cpp
fat表和cluster管理
static int fat_get_block(struct inode *inode, sector_t iblock,struct buffer_head *bh_result, int create)
     //建立从文件块号到设备扇区号的映射，实际上就是验证指定的iblock是否对应有效的文件内容,并bh_result结构和设备对应
     //可能两次调用fat_bmap函数
     unsigned long max_blocks = bh_result->b_size >> inode->i_blkbits;
     err = __fat_get_block(inode, iblock, &max_blocks, bh_result, create);
          err = fat_bmap(inode, iblock, &phys, &mapped_blocks, create);//获取文件内偏移iblock对应的物理扇区
          if (phys){map_bh(bh_result, sb, phys);               *max_blocks = min(mapped_blocks, *max_blocks);return 0;}
               set_buffer_mapped(bh); bh->b_bdev = sb->s_bdev;　bh->b_blocknr = block;bh->b_size = sb->s_blocksize;
          //申请的扇区包含在现有文件内，返回物理扇区信息；否则，为文件增加一个簇，再返回对应扇区
          offset = (unsigned long)iblock & (sbi->sec_per_clus - 1);
          if (!offset)　err = fat_add_cluster(inode);     //如果新的扇区是新的簇的开始扇区，就给文件添加一个新的簇再重新建立映射
          err = fat_bmap(inode, iblock/sector, &phys, &mapped_blocks, create);//
               last_block = (i_size_read(inode) + (blocksize - 1)) >> blocksize_bits;//文件实际使用的最后一个块
               //根据文件长度计算文件当前需要占用多少扇区，文件长度是应用实际写入的数据的长度
               //长度为０,sector 0就能触发，所以sector是从０开始计数的．
               if (sector >= last_block){//要写的sector超出当前文件大小,计算是否可以从文件实际分配的簇中找到可用块
                    last_block = (MSDOS_I(inode)->mmu_private + (blocksize - 1))>> blocksize_bits;
                    if (sector >= last_block)     return 0;
               }
               cluster = sector >> (sbi->cluster_bits - sb->s_blocksize_bits);//计算文件cluster id,代表文件内部偏移簇位置
               offset = sector & (sbi->sec_per_clus - 1);                                        //计算cluster内偏移多少个sector
               cluster = fat_bmap_cluster(inode, cluster);    //返回文件逻辑簇号对应的磁盘物理cluster id
                    ret = fat_get_cluster(inode, cluster, &fclus, &dclus);
               *phys = fat_clus_to_blknr(sbi, cluster) + offset;        //返回磁盘sector id
               mapped_blocks = sbi->sec_per_clus - offset;        　//当前簇中在*phys扇区之后还有多少块
          *max_blocks = min(mapped_blocks, *max_blocks);
     bh_result->b_size = max_blocks << sb->s_blocksize_bits;　//计算映射的实际bytes
     if (phys)     map_bh(bh_result, sb, phys);     //搜索到有效的扇区，设置bh->b_bdev为设备

struct fat_cache {     //每个inode(文件)对应一个fat_cache的链表，每个表示一个连续的cluster区块
     struct list_head cache_list; //链表节点，链接到struct msdos_inode_info的struct list_head cache_lru成员
     int nr_contig; /* number of contiguous clusters 连续的cluster的个数，最少也是１,简化cluster链表的访问*/
     int fcluster; /* cluster number in the file. 表示在文件中逻辑簇号，逻辑偏移*/
     int dcluster; /* cluster number on disk. 　表示块设备实际簇号，表示物理位置*/
};

static int fat_cache_lookup(struct inode *inode, int fclus,struct fat_cache_id *cid,int *cached_fclus, int *cached_dclus)
     //函数遍历与inode对应的msdos_inode_info结构体的cache_lru队列。这就是该inode的fat_cache队列
     //如果找到fclus所在的cache就保存在cid中,如果没有找到，就返回距离最近的cached_fclus和cached_dclus。然后只能从磁盘fat表中获取了。
     //如果没有任何cash返回-1

int fat_get_cluster(struct inode *inode, int cluster, int *fclus, int *dclus)//获取文件逻辑簇号(文件第cluster个簇)对应的设备物理簇号.
     //由于文件簇链表结构，只能从０簇开始一个个向后查找
     int limit = sb->s_maxbytes >> MSDOS_SB(sb)->cluster_bits; 　//4k对应12bit，limit为磁盘最大cluster数
     if (fat_cache_lookup(inode, cluster, &cid, fclus, dclus) < 0)      cache_init(&cid, -1, -1);　//如果cach中一个都找不到就从头找
     fatent_init(&fatent);
     while (*fclus < cluster) {
          nr = fat_ent_read(inode, &fatent, *dclus);  //获取当前entry: *dclus位置保存的next cluster的entry
               fatent_set_entry(fatent, entry);                  //fatent赋值entry，设定要计算的entry
               ops->ent_blocknr(sb, entry, &offset, &blocknr);　 //计算磁盘cluster在fat表中4字节的偏移，哪一个block，block内部offset
               如果块没有被缓存，err = ops->ent_bread(sb, fatent, offset, blocknr); //读取blocknr，内部指针指向entry的位置
                    fatent->fat_inode = MSDOS_SB(sb)->fat_inode;
                    fatent->bhs[0] = sb_bread(sb, blocknr); //读取指定block
                    ops->ent_set_ptr(fatent, offset);       //fat32_ent_set_ptr() :
                         fatent->u.ent32_p = (__le32 *)(fatent->bhs[0]->b_data + offset);  //指向cluster的数据，下一个cluster的id.
               fat_ent_update_ptr(sb, fatent, offset, blocknr);
                    ops->ent_set_ptr(fatent, offset);                 //fat32_ent_set_ptr() :
               return ops->ent_get(fatent);          //fat32_ent_get() : 读取entry对应位置保存的下一个cluster的entry.
                    return int next = le32_to_cpu(*fatent->u.ent32_p) & 0x0fffffff;     //返回下个cluster号,最后一个返回FAT_ENT_EOF
          如果找到cluster,或者文件结束(nr == FAT_ENT_EOF)     fat_cache_add(inode, &cid);     //添加cid并且尝试合并
     }
```

```
fatent.c函数解析
EXPORT_SYMBOL_GPL(fat_free_clusters);

static struct fatent_operations fat32_ops = {　// vfat_mount()中
     .ent_blocknr = fat_ent_blocknr,　    //计算fat表中簇entry对应4个byte的物理block号和block内offset
     .ent_set_ptr = fat32_ent_set_ptr,　   //簇对象指针改变块内offset位置
     .ent_bread = fat_ent_bread,          //簇对象根据物理位置信息获得buffer head信息，指针定位到offset
     .ent_get = fat32_ent_get,　          //读取簇对象内容，就是簇对象指针指向的内容
     .ent_put = fat32_ent_put,　          //设定簇对象内容，就是下一个簇的簇号
     .ent_next = fat32_ent_next,          //簇对象重置为下一个簇号
};
struct fat_entry {
     int entry;　                    // 代表当前的簇索引
     union {  　u8 *ent12_p[2];__le16 *ent16_p;__le32 *ent32_p;　　} u;// 簇索引表指针
     int nr_bhs;                    // buffer_head数目，可能是1也可能是2，FAT32是1
     struct buffer_head *bhs[2];          // FAT表的扇区的buffer_head
     struct inode *fat_inode;            //超级块的inode
};

static void fat_ent_blocknr(struct super_block *sb, int entry, int *offset, sector_t *blocknr)
     根据cluster编号entry，计算得到它在fat表中的位置，blocknr为扇区(block)号，offset为扇区内偏移位置
     int bytes = (entry << sbi->fatent_shift);
     *offset = bytes & (sb->s_blocksize - 1);
     *blocknr = sbi->fat_start + (bytes >> sb->s_blocksize_bits);//fat表开始扇区号加偏移扇区
static void fat32_ent_set_ptr(struct fat_entry *fatent, int offset)
     fat表entry项指针指向offset位置
     fatent->u.ent32_p = (__le32 *)(fatent->bhs[0]->b_data + offset);//b_data指向一个扇区
static int fat_ent_bread(struct super_block *sb, struct fat_entry *fatent,int offset, sector_t blocknr)
     //确认读取FAT分配表，读到后保存到fat_entry的bhs，并将其offset开始放到u.ent32_p
     fatent->fat_inode = MSDOS_SB(sb)->fat_inode;
     fatent->bhs[0] = sb_bread(sb, blocknr);-->__bread(sb->s_bdev, block, sb->s_blocksize);
          //好像是从内核文件内存映射中读取，如果没有bock的映射，分配page生成buffer_head并读取磁盘填充数据
          //如果有立刻同步的要求的时候，需要额外手动同步操作
          struct buffer_head *bh = __getblk(bdev, block, size);     //bh指向一个buffer_head和block关联
          bh = __bread_slow(bh);     //buffer.c文件中，同步读取指定块数据
               bh->b_end_io = end_buffer_read_sync;
               submit_bh(READ, bh);
               wait_on_buffer(bh);
     fatent->nr_bhs = 1;     ops->ent_set_ptr(fatent, offset);     //调用fat32_ent_set_ptr()
static int fat32_ent_get(struct fat_entry *fatent)     //返回当前entry保存的next cluster值
static void fat32_ent_put(struct fat_entry *fatent, int new)//当前entry项next cluster设定为new
static int fat32_ent_next(struct fat_entry *fatent)    //fatent和内部指针同步指向entry+1项

int fat_count_free_clusters(struct super_block *sb)
     fatent_init(&fatent);     fatent_set_entry(&fatent, FAT_START_ENT);     //初始化fatent到fat表起始位置
     fat_ent_reada(sb, &fatent, min(reada_blocks, rest));     //一次多读点效率高
          //调用sb_breadahead()函数，提前读取磁盘数据到文件内存映射中，连续读效率更高
     err = fat_ent_read_block(sb, &fatent);//如果fatent没有有效内容信息，就获取fatent.entry对应簇所在扇区
          ops->ent_blocknr(sb, fatent->entry, &offset, &blocknr);
          return ops->ent_bread(sb, fatent, offset, blocknr);
     循环：　fat_ent_next(sbi, &fatent)     //遍历同一个buffer_head管理的块上的fat表簇号区域
                    fat32_ent_nextf
                         fatent->entry++;     fatent->u.ent32_p++;
               if (ops->ent_get(&fatent) == FAT_ENT_FREE)    free++;      //   fat32_ent_get
     sbi->free_clusters = free;
     sbi->free_clus_valid = 1;
     mark_fsinfo_dirty(sb);
          __mark_inode_dirty(sbi->fsinfo_inode, I_DIRTY_SYNC);
               后续如何处理》》》》》
int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster) //找到nr_cluster个空闲cluster,保存在cluster[]数组中
     struct buffer_head *bhs[MAX_BUF_PER_PAGE];
     BUG_ON(nr_cluster > (MAX_BUF_PER_PAGE / 2)); /* fixed limit 限制一次最多分配４个*/
     fatent_init(&prev_ent);     fatent_init(&fatent);     //
     fatent_set_entry(&fatent, sbi->prev_free + 1);     //一个可能不错的开始点
     while (count < sbi->max_cluster) {
          err = fat_ent_read_block(sb, &fatent);//读取entry所在块
          foreach entry on this block:          //fat_ent_next(sbi, &fatent)
               跳过dirty的cluster;     //(ops->ent_get(&fatent) != FAT_ENT_FREE)
               ops->ent_put(&fatent, FAT_ENT_EOF);     //标示当前entry为最后一个
               ops->ent_put(&prev_ent, entry);          //上一个块，指向当前entry
               fat_collect_bhs(bhs, &nr_bhs, &fatent);//bhs[]数组保存dirty的buffer head指针,之后fat_sync_bhs　fat_mirror_bhs brelse使用
               sbi->prev_free = entry;
               cluster[idx_clus++] = entry;                    //cluster[]保存分配的几个entry
               prev_ent = fatent;
     };
     mark_fsinfo_dirty(sb);
     if (inode_needs_sync(inode))   err = fat_sync_bhs(bhs, nr_bhs);
          for (i = 0; i < nr_bhs; i++)     write_dirty_buffer(bhs[i], WRITE);
          for (i = 0; i < nr_bhs; i++)     wait_on_buffer(bhs[i]);
     if (!err)               err = fat_mirror_bhs(sb, bhs, nr_bhs);
int fat_free_clusters(struct inode *inode, int cluster)  //free指定的以及之后的所有cluster
     fatent_init(&fatent);
     释放到文件末尾:
          cluster = fat_ent_read(inode, &fatent, cluster);
          ops->ent_put(&fatent, FAT_ENT_FREE);
          sbi->free_clusters++;
          一些bhs操作：　fat_sync_bhs　fat_mirror_bhs　fat_collect_bhs
     if (dirty_fsinfo)     mark_fsinfo_dirty(sb);
int fat_ent_write(struct inode *inode, struct fat_entry *fatent, int new, int wait)//设定fatent的下一个是new
     ops->ent_put(fatent, new);          //当前entry项next cluster设定为new
     if (wait)     err = fat_sync_bhs(fatent->bhs, fatent->nr_bhs);
     return fat_mirror_bhs(sb, fatent->bhs, fatent->nr_bhs);
```

```
inode.c函数解析
从实际的应用情况看，每次只会分配一个cluster
导出函数列表:
     EXPORT_SYMBOL_GPL(fat_attach);
     EXPORT_SYMBOL_GPL(fat_detach);
     EXPORT_SYMBOL_GPL(fat_build_inode);
     EXPORT_SYMBOL_GPL(fat_sync_inode);
     EXPORT_SYMBOL_GPL(fat_fill_super);
     EXPORT_SYMBOL_GPL(fat_flush_inodes);

module_init(init_fat_fs)     //创建两个memory cache, 导出的inode函数基于这个数据结构。
     err = fat_cache_init();
         fat_cache_cachep = kmem_cache_create("fat_cache",sizeof(struct fat_cache),...);//fat表，定义连续簇的cache
     err = fat_init_inodecache();
         fat_inode_cachep = kmem_cache_create("fat_inode_cache",sizeof(struct msdos_inode_info), ...);
               //struct msdos_inode_info fat32文件inode扩展信息结构体

static const struct address_space_operations fat_aops = {     //inode.c
    .readpage    = fat_readpage,
    .readpages    = fat_readpages,
    .writepage    = fat_writepage,
    .writepages    = fat_writepages,
    .write_begin    = fat_write_begin,
    .write_end    = fat_write_end,
    .direct_IO    = fat_direct_IO,
    .bmap        = _fat_bmap
};

static int fat_add_cluster(struct inode *inode)
     err = fat_alloc_clusters(inode, &cluster, 1);     //找到1个空闲的cluster,保存在cluster变量中
     err = fat_chain_add(inode, cluster, 1);     //把cluster添加到文件的最后一个节点
          ret = fat_get_cluster(inode, FAT_ENT_EOF, &fclus, &dclus);　//找到文件最后一个cluster
          new_fclus = fclus + 1; last = dclus;
          fatent_init(&fatent);
          ret = fat_ent_read(inode, &fatent, last);          //好像没有用
          int wait = inode_needs_sync(inode);
          ret = fat_ent_write(inode, &fatent, new_dclus, wait); //文件cluster chain添加一项
          inode->i_blocks += nr_cluster << (sbi->cluster_bits - 9);
struct inode *fat_iget(struct super_block *sb, loff_t i_pos)
     struct hlist_head *head = sbi->inode_hashtable + fat_hash(i_pos);//用i_pos得到head，然后用来寻找inode
     hlist_for_each_entry(i, head, i_fat_hash)     //head在sbi中定义,包含在并可以直接访问文件的双向链表
          if (i->i_pos == i_pos)　inode = igrab(&i->vfs_inode);     //索引号匹配，返回inode
struct inode *fat_build_inode(struct super_block *sb, struct msdos_dir_entry *de, loff_t i_pos)
     inode = new_inode(sb);
     inode->i_ino = iunique(sb, MSDOS_ROOT_INO);　inode->i_version = 1;
     err = fat_fill_inode(inode, de);
     fat_attach(inode, i_pos);               //建立hash，之后可以通过i_pos快速索引到inode
          struct hlist_head *head = sbi->inode_hashtable+ fat_hash(i_pos);
          MSDOS_I(inode)->i_pos = i_pos;
          hlist_add_head(&MSDOS_I(inode)->i_fat_hash, head);     //引入后门
     insert_inode_hash(inode);          //利用hash，之后可以用inode索引号快速访问inode。hash就像是一个粘合剂，目标是加快索引速度。
          __insert_inode_hash(inode, inode->i_ino);     //attach i_ino到inode
               struct hlist_head *b = inode_hashtable + hash(inode->i_sb, hashval);
               hlist_add_head(&inode->i_hash, b);
static int fat_write_inode(struct inode *inode, struct writeback_control *wbc)
     if (inode->i_ino == MSDOS_FSINFO_INO)     err = fat_clusters_flush(sb);
          修改 bh = sb_bread(sb, sbi->fsinfo_sector);
          mark_buffer_dirty(bh);
     else      err = __fat_write_inode(inode, wbc->sync_mode == WB_SYNC_ALL);     //文件目录项信息更新
          i_pos = fat_i_pos_read(sbi, inode);     //MSDOS_I(inode)->i_pos为文件目录项的偏移id,就是第几个目录项，从分区０地址开始计算。
               i_pos = MSDOS_I(inode)->i_pos;
          fat_get_blknr_offset(sbi, i_pos, &blocknr, &offset);//计算第i_pos个目录项对应的扇区和偏移
                fat_get_blknr_offset(sbi, i_pos, &blocknr, &offset);
                *offset = i_pos & (sbi->dir_per_block - 1);
          bh = sb_bread(sb, blocknr);          //读取指定扇区
          raw_entry = &((struct msdos_dir_entry *) (bh->b_data))[offset];
          raw_entry->size = ...;  raw_entry->attr = fat_make_attrs(inode);
          fat_set_start(raw_entry, MSDOS_I(inode)->i_logstart); //设置文件开始簇位置信息
          然后修改时间信息;mark_buffer_dirty(bh);          if (wait) err = sync_dirty_buffer(bh);  //标记目录项bh需要回写

static const struct super_operations fat_sops = {
    .alloc_inode    = fat_alloc_inode,　  .destroy_inode    = fat_destroy_inode,
    .write_inode    = fat_write_inode,    .evict_inode    = fat_evict_inode,    .put_super    = fat_put_super,
　 .statfs        = fat_statfs,     .remount_fs    = fat_remount,     .show_options    = fat_show_options,
};
const struct export_operations fat_export_ops = {
    .fh_to_dentry   = fat_fh_to_dentry,     .fh_to_parent   = fat_fh_to_parent,     .get_parent     = fat_get_parent,
};
```

```
dir.c函数解析

const struct file_operations fat_dir_operations = {
    .llseek        = generic_file_llseek,
    .read        = generic_read_dir,
    .readdir    = fat_readdir,
    .unlocked_ioctl    = fat_dir_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl    = fat_compat_dir_ioctl,
#endif
    .fsync        = fat_file_fsync,
};

Namei_vfat.c      //加载fat32 目录管理 文件创建删除 属性管理

module_init(init_vfat_fs)
     return register_filesystem(&vfat_fs_type);

d_revalidate:用于VFS使一个dentry重新生效。
d_hash:用于VFS向哈希表中加入一个dentry。
d_compare:dentry的最后一个inode被释放时（d_count等于零），此方法被调用，因为这意味这没有inode再使用此dentry；当然，此dentry仍然有效，并且仍然在dcache中。
d_release: 用于清除一个dentry。
d_iput:用于一个dentry释放它的inode（d_count不等于零）

struct dentry_operations {
    int (*d_revalidate)(struct dentry *, unsigned int);     //用于VFS使一个dentry重新生效。
    int (*d_weak_revalidate)(struct dentry *, unsigned int);
    int (*d_hash)(const struct dentry *, const struct inode *, struct qstr *); //用于VFS向哈希表中加入一个dentry。
    int (*d_compare)(struct dentry *, struct inode *,struct dentry *, struct inode *,unsigned int, char *, struct qstr *);
            //dentry的最后一个inode被释放时（d_count等于零）调用，因为没有inode再使用它；当然，此dentry仍然有效，并且仍然在dcache中。
    int (*d_delete)(const struct dentry *);
    void (*d_release)(struct dentry *);  //用于清除一个dentry
    void (*d_prune)(struct dentry *);
    void (*d_iput)(struct dentry *, struct inode *);  用于一个dentry释放它的inode（d_count不等于零）
    char *(*d_dname)(struct dentry *, char *, int);
    struct vfsmount *(*d_automount)(struct path *);
    int (*d_manage)(struct dentry *, bool);
} ____cacheline_aligned;
static const struct dentry_operations vfat_ci_dentry_ops = {
    .d_revalidate    = vfat_revalidate_ci,     // 8.3格式文件名    .d_hash        = vfat_hashi,    .d_compare    = vfat_cmpi,
};
static const struct dentry_operations vfat_dentry_ops = {
    .d_revalidate    = vfat_revalidate,    .d_hash        = vfat_hash,    .d_compare    = vfat_cmp,
};module_init(init_vfat_fs)

struct dentry *d_lookup(const struct dentry *parent, const struct qstr *name)
     dentry = __d_lookup(parent, name);
     foreach dentry: if (parent->d_op->d_compare()==0) return dentry;

static struct file_system_type vfat_fs_type = {
    .owner        = THIS_MODULE,    .name        = "vfat",    .mount        = vfat_mount,
    .kill_sb    = kill_block_super,    .fs_flags    = FS_REQUIRES_DEV,
};
static const struct inode_operations vfat_dir_inode_operations = {
    .create        = vfat_create,
    .lookup        = vfat_lookup,    .unlink        = vfat_unlink,    .mkdir        = vfat_mkdir,
    .rmdir        = vfat_rmdir,    .rename        = vfat_rename,    .setattr    = fat_setattr,
    .getattr    = fat_getattr,
};
static int vfat_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
     cluster = fat_allocmodule_init(init_vfat_fs)
_new_dir(dir, &ts);
          err = fat_alloc_clusters(dir, &cluster, 1);
          blknr = fat_clus_to_blknr(sbi, cluster);bhs[0] = sb_getblk(sb, blknr);
          基于第一个block创建并初始化两个目录项，包括设置start cluster
          零初始化cluster剩余的空间
     err = vfat_add_entry(dir, &dentry->d_name, 1, cluster, &ts, &sinfo);
     inode = fat_build_inode(sb, sinfo.de, sinfo.i_pos);

static int vfat_create(struct inode *dir, struct dentry *dentry, umode_t mode,bool excl)
     //创建并且初始化inode,更新dentry信息
     err = vfat_add_entry(dir, &dentry->d_name, 0, 0, &ts, &sinfo);
          slots = kmalloc(sizeof(*slots) * MSDOS_SLOTS, GFP_NOFS);
          err = vfat_build_slots(dir, qname->name, len, is_dir, cluster, ts, slots, &nr_slots);
               //初始化若干msdos_dir_slot，如果是短名，nr_slots为１,msdos_dir_entry就够不需要额外的msdos_dir_slot定义
               //初始化新的目录文件的内容，之后写入
               de->attr = ..;de->lcase = lcase;;de->time = de->ctime = time;
               de->date = de->cdate = de->adate = date; de->ctime_cs = time_cs; de->size = 0;
               fat_set_start(de, cluster);     //vfat_build_slots函数对短名只有设置地址这个重要的动作，在slots指针返回一个目录项
          //为目录或文件分配空间，写入目录内容
          err = fat_add_entries(dir, slots, nr_slots, sinfo);//nr_slots这里为１
          //父目录也是文件，把新创建的目录或文件的名称等信息写入到父目录文件中，之后fat_build_inode会为新目录或文件创建inode.
               foreach fat_get_entry(dir, &pos, &bh, &de)//获取足够的目录项，短名只需一个
                    //如果老的扇区的目录项没有遍历到结尾，*pos += sizeof(struct msdos_dir_entry); (*de)++; 否则
                    //优先使用当前目录已经分配的簇中的空闲或者标记为删除的位置，可能会需要找到连续多个位置才能满足需求．
                    -->fat_get_entry(dir, pos, bh, de);
                         err = fat_bmap(dir, iblock, &phys, &mapped_blocks, 0);//逻辑iblock换算成物理phys,称之为block mapping
                         fat_dir_readahead(dir, iblock, phys);     //预读phys所在整个簇的目录项
                         *bh = sb_bread(sb, phys);                    //获取phys扇区的buffer head
                         offset = *pos & (sb->s_blocksize - 1);                    *pos += sizeof(struct msdos_dir_entry);
                         *de = (struct msdos_dir_entry *)((*bh)->b_data + offset);//返回指定目录项信息
                    memcpy(bhs[i]->b_data + offset, slots, copy);     //写入目录项
                    mark_buffer_dirty_inode(bhs[i], dir);
                    if(nr_slots)　//空闲的目录项不够用，分配新的簇
                         cluster = fat_add_new_entries(dir, slots, nr_slots, &nr_cluster,
                              err = fat_alloc_clusters(dir, cluster, *nr_cluster);
                              初始化第一个目录项然后零初始化整个ｃｌｕｓｔｅｒ剩余的空间
                         err = fat_chain_add(dir, cluster, nr_cluster);
                         dir->i_size += nr_cluster << sbi->cluster_bits;
                         MSDOS_I(dir)->mmu_private += nr_cluster << sbi->cluster_bits;
                    sinfo->slot_off = pos;sinfo->de = de;sinfo->bh = bh;
                    sinfo->i_pos = fat_make_i_pos(sb, sinfo->bh, sinfo->de);
                    //计算目录项指针偏移索引，单位为目录项(32byte)；表示这是分区上第几个目录项
          dir->i_ctime = dir->i_mtime = dir->i_atime = *ts;
          如果需要立刻sync : (void)fat_sync_inode(dir);     否则   mark_inode_dirty(dir);
     inode = fat_build_inode(sb, sinfo.de, sinfo.i_pos);
     //为目录文件创建并且初始化inode，主要是几个ops，链接到hash中
         inode = fat_iget(sb, i_pos);  if(inode == null)  inode = new_inode(sb);//找到已有的inode，否则新建一个
         err = fat_fill_inode(inode, de);               //用de目录项的内容填充Ｉｎｏｄｅ信息
                   inode->i_op = &fat_file_inode_operations;
                   inode->i_fop = &fat_file_operations;
                   inode->i_mapping->a_ops = &fat_aops;
                   MSDOS_I(inode)->i_start = fat_get_start(sbi, de); //ｆｉｌｌ文件开始位置和长度以及ｏｐｓ操作函数集
                   //inode并没有实际在磁盘上存放．从磁盘上文件目录项中获取开始簇信息，存在inode中．
                   inode->i_size = le32_to_cpu(de->size);　以及其他信息的填充
         fat_attach(inode, i_pos); insert_inode_hash(inode);
     inode->i_version++; inode->i_mtime = inode->i_atime = inode->i_ctime = ts;
     dentry->d_time = dentry->d_parent->d_inode->i_version;
     d_instantiate(dentry, inode);-->__d_instantiate
          hlist_add_head(&dentry->d_alias, &inode->i_dentry);     //inode通过自己的i_dentry链接到dentry
          dentry->d_inode = inode;     //可以通过dentry索引到inode
```

```
fat32文件系统
const struct inode_operations fat_file_inode_operations = {
    .setattr    = fat_setattr,
    .getattr    = fat_getattr,
};
const struct file_operations fat_file_operations = {                 //file.c
    .llseek        = generic_file_llseek,
    .read        = do_sync_read,
    .write        = do_sync_write,
    .aio_read    = generic_file_aio_read,
    .aio_write    = generic_file_aio_write,
    .mmap        = generic_file_mmap,
    .release    = fat_file_release,
    .unlocked_ioctl    = fat_generic_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl    = fat_generic_compat_ioctl,
#endif
    .fsync        = fat_file_fsync,
    .splice_read    = generic_file_splice_read,
};

void fat_truncate_blocks(struct inode *inode, loff_t offset)
     nr_clusters = (offset + (cluster_size - 1)) >> sbi->cluster_bits;
     fat_free(inode, nr_clusters);
          if (wait)     err = fat_sync_inode(inode);     //
               return __fat_write_inode(inode, 1);

          else            mark_inode_dirty(inode);

          return fat_free_clusters(inode, free_start);
     fat_flush_inodes(inode->i_sb, inode, NULL);
static int fat_file_release(struct inode *inode, struct file *filp)
     fat_flush_inodes(inode->i_sb, inode, NULL);　     //flush元数据和文件数据
          writeback_inode(inode);     //回写元数据和文件数据
               ret = sync_inode_metadata(inode, 0); --> sync_inode　//回写元数据
                    return writeback_single_inode(inode, &inode_to_bdi(inode)->wb, wbc);
               ret = filemap_fdatawrite(inode->i_mapping); --> __filemap_fdatawrite --> __filemap_fdatawrite_range
                    //回写文件内容数据
                    mapping_cap_writeback_dirty(mapping)
                    do_writepages(mapping, &wbc);
          struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping;
          ret = filemap_flush(mapping);     //块设备对应的inode内容,应该是全局数据，文件系统数据。
     congestion_wait(BLK_RW_ASYNC, HZ/10);
struct backing_dev_info *inode_to_bdi(struct inode *inode)
     struct super_block *sb = inode->i_sb;
     if (strcmp(sb->s_type->name, "bdev") == 0)//块设备的bdi保存位置
          return inode->i_mapping->backing_dev_info;
     return sb->s_bdi;
struct inode *wb_inode(struct list_head *head)
     return list_entry(head, struct inode, i_wb_list);
```

```
fat32文件系统mount()
static struct dentry *vfat_mount(struct file_system_type *fs_type,...)     //作用有点像ｐｒｏｂｅ函数
     return mount_bdev(fs_type, flags, dev_name, data, vfat_fill_super);
          struct block_device * bdev = blkdev_get_by_path(dev_name, mode, fs_type);//找到设备
          struct super_block *s= sget(fs_type, test_bdev_super, set_bdev_super, flags | MS_NOSEC, bdev);//分配并初始化超级块
               s = alloc_super(type, flags);  s->s_type = type;     //文件系统struct file_system_type *fs_type
               strlcpy(s->s_id, type->name, sizeof(s->s_id));
               list_add_tail(&s->s_list, &super_blocks);     //添加到超级块链表
               hlist_add_head(&s->s_instances, &type->fs_supers);　//添加到文件系统链表
               get_filesystem(type);
               register_shrinker(&s->s_shrink);
          s->s_mode = mode;　          strlcpy(s->s_id, bdevname(bdev, b), sizeof(s->s_id));
          sb_set_blocksize(s, block_size(bdev));
          //设定ｆａｔ３２文件系统加载超级快的函数为vfat_fill_super
          static int vfat_fill_super(struct super_block *sb, void *data, int silent)
              return fat_fill_super(sb, data, silent, 1, setup);
                   struct msdos_sb_info* sbi = kzalloc(sizeof(struct msdos_sb_info), GFP_KERNEL); //分配超级块扩展fs信息
                   sb->s_fs_info = sbi;　　//超级块私有信息设定为fat32结构体
                   sb->s_op = &fat_sops;　　　//fat32超级块函数集
                   sb->s_export_op = &fat_export_ops;　//fat32 nfs?
                   setup(sb);
                        MSDOS_SB(sb)->dir_ops = &vfat_dir_inode_operations;//设置ｉｎｏｄｅ的ｏｐｓ for vfat
                        sb->s_d_op = &vfat_dentry_ops; or sb->s_d_op = &vfat_ci_dentry_ops;
                   bh = sb_bread(sb, 0);　用ｂｈ的内容初始化sbi结构;
                   fat_hash_init(sb);
                   dir_hash_init(sb);
                   fat_ent_access_init(sb);
                         sbi->fatent_shift = 2;          //表示cluster的index占用４个字节
                         sbi->fatent_ops = &fat32_ops;     //fat表函数集
                   fat_set_state(sb, 1, 0);     //DPB, 分区的ｂｏｏｔ信息只有一个扇区，直接通过buffer机制操作
                         bh = sb_bread(sb, 0);　b = (struct fat_boot_sector *) bh->b_data;
                         b->fat32.state |= FAT_STATE_DIRTY; or b->fat32.state &= ~FAT_STATE_DIRTY;
                         mark_buffer_dirty(bh);     sync_dirty_buffer(bh);
                   root_inode = new_inode(sb);root_inode->i_ino = MSDOS_ROOT_INO;root_inode->i_version = 1;
                   error = fat_read_root(root_inode);//root inode初始化
                   insert_inode_hash(root_inode);fat_attach(root_inode, 0);
                   sb->s_root = d_make_root(root_inode);//生成根目录对应的inode和dentry并初始化，"/"对应１扇区
          s->s_flags |= MS_ACTIVE;     //继续mount_bdev函数
          bdev->bd_super = s;
          return dget(s->s_root);

SYSCALL_DEFINE5(mount, char __user *, dev_name, char __user *, dir_name,
          char __user *, type, unsigned long, flags, void __user *, data)      //sys_mount() namespace.c文件
     ret = do_mount(kernel_dev, kernel_dir->name, kernel_type, flags,(void *) data_page);
          retval = do_new_mount(&path, type_page, flags, mnt_flags,dev_name, data_page);
     struct file_system_type *type = get_fs_type(fstype);
　　　//获取文件系统登记的信息，主要是ops操作函数集,包括vfat_mount()函数
     mnt = vfs_kern_mount(type, flags, name, data); //为设备生成mount节点
         mnt = alloc_vfsmnt(name);     //分配vfs的mount节点
         root = mount_fs(type, flags, name, data);     //读取磁盘上超级块信息，初始化文件系统和超级块
              root = type->mount(type, flags, name, data);   //调用 vfat_mount()
              sb = root->d_sb;
         mnt->mnt_mountpoint = mnt->mnt.mnt_root = root;          mnt->mnt.mnt_sb = root->d_sb;
         mnt->mnt_parent = mnt;
         list_add_tail(&mnt->mnt_instance, &root->d_sb->s_mounts);//添加到超级块的mount节点链表中
     err = do_add_mount(real_mount(mnt), path, mnt_flags);//把新mount节点挂载到路径对应的父节点上
         parent = real_mount(path->mnt);         //获得路径对应的parent挂载点
         newmnt->mnt.mnt_flags = mnt_flags;
         err = graft_tree(newmnt, parent, mp); //把newmnt挂载到parent的树
long sys_mknod(const char __user *filename, umode_t mode,unsigned dev);//SYSCALL_DEFINE3(mknod
     return sys_mknodat(AT_FDCWD, filename, mode, dev);     //SYSCALL_DEFINE4(mknodat
          dentry = user_path_create(dfd, filename, &path, lookup_flags); //创建Ｐａｔｈ和页目录项

```
```

fat32文件系统open()
SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode) 展开为sys_open()-->do_sys_open
long do_sys_open(int dfd, const char __user *filename, int flags, umode_t mode)
     int lookup = build_open_flags(flags, mode, &op);
     struct filename *tmp = getname(filename);
     fd = get_unused_fd_flags(flags);
     struct file *f = do_filp_open(dfd, tmp, &op, lookup);
          filp = path_openat(dfd, pathname, &nd, op, flags | LOOKUP_RCU);
               file = get_empty_filp();
               error = do_last(nd, &path, file, op, &opened, pathname);
     fsnotify_open(f);
     fd_install(fd, f);
     putname(tmp);

     fd = get_unused_fd();
     struct file *filp_open(const char *filename, int flags, umode_t mode)
     struct file *newfile = dentry_open(const struct path *path, int flags,const struct cred *cred)
         struct file *f = get_empty_filp();　　f->f_flags = flags;　　　f->f_path = *path;
         error = do_dentry_open(f, NULL, cred);
              f->f_mode = OPEN_FMODE(f->f_flags) | FMODE_LSEEK |  FMODE_PREAD | FMODE_PWRITE;
              path_get(&f->f_path);
              inode = f->f_inode = f->f_path.dentry->d_inode;
              f->f_mapping = inode->i_mapping;
              f->f_op = fops_get(inode->i_fop);
              if(f->f_op->open) f->f_op->open(inode, f);
              f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);
              file_ra_state_init(&f->f_ra, f->f_mapping->host->i_mapping);
     current->files->fd[fd] = newfile;  return fd;

fat32文件系统read()
SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)//从file指定pos读取count字节到buf
     if(file->f_op->read)     ret = file->f_op->read(file, buf, count, pos);  //fat32 : do_sync_read()
     else   ret = do_sync_read(file, buf, count, pos);
              init_sync_kiocb(&kiocb, filp);
                    struct iovec iov = { .iov_base = buf, .iov_len = len };//只有一个io vector
                   *kiocb = (struct kiocb) { .ki_users = ATOMIC_INIT(1), .ki_ctx = NULL,
                                 .ki_filp = filp, .ki_obj.tsk = current,};
              kiocb.ki_pos = *ppos;    kiocb.ki_left = len;    kiocb.ki_nbytes = len;
              ret = filp->f_op->aio_read(&kiocb, &iov, 1, kiocb.ki_pos);        //fat32:  generic_file_aio_read mm/filemap.c
              if (-EIOCBQUEUED == ret)　ret = wait_on_sync_kiocb(&kiocb);//read请求在队列中，等待结束
ssize_t  generic_file_aio_read(struct kiocb *iocb, const struct iovec *iov,unsigned long nr_segs, loff_t pos)//nr_segs:此为１
     //@iocb: kernel I/O control block @iov: io vector request @nr_segs: number of segments in the iovec @pos: current file position
     retval = generic_segment_checks(iov, &nr_segs, &count, VERIFY_WRITE); if (retval)     return retval;
     if (filp->f_flags & O_DIRECT)//不进入cache,单次读写要求对齐但性能好，多次读写无法利用cache优化
         retval = filp->f_mapping->a_ops->direct_IO(READ, iocb,iov, pos, nr_segs);  //fat_direct_IO
              ret = blockdev_direct_IO(rw, iocb, inode, iov, offset, nr_segs,fat_get_block);
                   -->__blockdev_direct_IO-->do_blockdev_direct_IO
                        blk_start_plug(&plug);//把紧密耦合的数据块先plug，全部request完毕后unplug，再一起加入到块设备的queue唤醒处理thread
                        foreach seg : retval = do_direct_IO(dio, &sdio, &map_bh);-->submit_page_section
                             -->dio_bio_submit-->submit_bio(dio->rw, bio)
                        blk_finish_plug(&plug);
     foreach segment
         read_descriptor_t desc; desc.written = 0; desc.arg.buf = iov[seg].iov_base + offset;desc.count = iov[seg].iov_len - offset;
         do_generic_file_read(filp, ppos, &desc, file_read_actor);
              error = mapping->a_ops->readpage(filp, page);//fat_readpages fat_readpage
                    mpage_bio_submit(READ, bio);
                         bio->bi_end_io = mpage_end_io;
                         submit_bio(rw, bio); -->generic_make_request(bio);
                              //submit_bio()函数已经看到三个地方调用：read, writeback, modify_write,要求阻塞读取完毕
int file_read_actor(read_descriptor_t *desc, struct page *page, unsigned long offset, unsigned long size)ssize_t

fat32文件系统write()
SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf, size_t, count)
     struct fd f = fdget(fd);
     loff_t pos = file_pos_read(f.file);
     ret = vfs_write(f.file, buf, count, &pos);
     file_pos_write(f.file, pos);
     fdput(f);
ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
     if (file->f_op->write)            ret = file->f_op->write(file, buf, count, pos); //fat32:  do_sync_write
     else  ret = do_sync_write(file, buf, count, pos);
         init_sync_kiocb(&kiocb, filp);
         kiocb.ki_pos = *ppos;    kiocb.ki_left = len;    kiocb.ki_nbytes = len;
         ret = filp->f_op->aio_write(&kiocb, &iov, 1, kiocb.ki_pos);     //fat32 : generic_file_aio_write
         if (-EIOCBQUEUED == ret)        ret = wait_on_sync_kiocb(&kiocb);
  generic_file_aio_write(struct kiocb *iocb, const struct iovec *iov,unsigned long nr_segs, loff_t pos)
     ret = __generic_file_aio_write(iocb, iov, nr_segs, &iocb->ki_pos);
         err = generic_segment_checks(iov, &nr_segs, &ocount, VERIFY_READ);
         err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
         err = file_remove_suid(file);     err = file_update_time(file);
         if (unlikely(file->f_flags & O_DIRECT)) {}else
         written = generic_file_buffered_write(iocb, iov, nr_segs,pos, ppos, count, written);
              struct file *file = iocb->ki_filp; struct iov_iter i;
              iov_iter_init(&i, iov, nr_segs, count, written);
              ssize_t status = generic_perform_write(file, &i, pos);
                   status = a_ops->write_begin(file, mapping, pos, bytes, flags, &page, &fsdata);     //fat32: fat_write_begin
                              //写入page前需要确认读取要写入的范围，因为是read-modify-write的流程
                        err = cont_write_begin(file, mapping, pos, len, flags, pagep, fsdata, fat_get_block,
                                                                             &MSDOS_I(mapping->host)->mmu_private);
                             //每次调用强制mmu_private对齐block
                             return block_write_begin(mapping, pos, len, flags, pagep, get_block);
                                  page = grab_cache_page_write_begin(mapping, index, flags);
                                       page = find_lock_page(mapping, index);　if(page) return page;
                                            loop to get: page = find_get_page(mapping, offset);
                                       page = __page_cache_alloc(gfp_mask & ~gfp_notmask);
                                       status = add_to_page_cache_lru(page, mapping, index,GFP_KERNEL & ~gfp_notmask);
                                  status = __block_write_begin(page, pos, len, get_block);
                   copied = iov_iter_copy_from_user_atomic(page, i, offset, bytes); //完成从用户缓冲区到page cache的复制
                        foreach segment :  left = __copy_from_user_inatomic(kaddr + offset, buf, bytes);
                             --> memcpy(to, (const void __force *)from, n);
                   status = a_ops->write_end(file, mapping, pos, bytes, copied,page, fsdata);          //fat32: fat_write_end
                        err = generic_write_end　-->
                              block_write_end()
                                   __block_commit_write(inode, page, start, start+copied);
                                        如果是写入不完整页，mark_buffer_dirty(bh);-->__set_page_dirty(page, mapping, 0);-->
                                             __mark_inode_dirty(mapping->host, I_DIRTY_PAGES);
                                        如果整个页都是最新，SetPageUptodate(page);
                              if (pos+copied > inode->i_size) {i_size_write(inode, pos+copied);i_size_changed = 1;}
                              if (i_size_changed)     mark_inode_dirty(inode);-->__mark_inode_dirty(inode, I_DIRTY);
                   balance_dirty_pages_ratelimited(mapping);
     err = generic_write_sync(file, pos, ret);
int __block_write_begin(struct page *page, loff_t pos, unsigned len,
     struct buffer_head *head = create_page_buffers(page, inode, 0);
     foreach buffer_head:
          if (buffer_new(bh))     clear_buffer_new(bh);
          if (!buffer_mapped(bh)){
               err = get_block(inode, block, bh, 1);//fat_get_block，bh->b_size传入需要的size,被修改为实际映射的size.
                    struct inode *bd_inode = bdev->bd_inode;//设备自己的i_map,应该是dir,fat表之类的信息
                    struct address_space *bd_mapping = bd_inode->i_mapping;
                    index = block >> (PAGE_CACHE_SHIFT - bd_inode->i_blkbits);
                    page = find_get_page(bd_mapping, index);

               if (buffer_new(bh))     unmap_underlying_metadata(bh->b_bdev,bh->b_blocknr);--> __find_get_block_slow
               if(buffer不对齐写入等)    ll_rw_block(READ, 1, &bh); -->submit_bh(WRITE, bh);---->submit_bio(rw, bio);
          }
          foreach waiting_bh wait_on_buffer(*--wait_bh);
void __mark_inode_dirty(struct inode *inode, int flags)
     bdi = inode_to_bdi(inode);
     int was_dirty = inode->i_state & I_DIRTY;//trace中，每writeback_interval，被清除一次，
     if(!was_dirty){
          inode->dirtied_when = jiffies;//inode最早page被写入的时间是inode的dirty时间
          list_move(&inode->i_wb_list, &bdi->wb.b_dirty);/添加到bdi的writeback数据结构脏inode链表中
          if (wakeup_bdi)     bdi_wakeup_thread_delayed(bdi);//5s后第一次启动回写线程。
     }
Inode state bits：
     I_DIRTY_SYNC     I_DIRTY_DATASYNC     I_DIRTY_PAGES     I_NEW     I_WILL_FREE     I_FREEING
     I_CLEAR          I_SYNC               I_REFERENCED     I_DIO_WAKEUP

```

```
文件系统回写
实际上，sync()的优先级比background高，所以图片按时写入，应该使用sync，而不是改变background和expired.
static void bdi_wb_init(struct bdi_writeback *wb, struct backing_dev_info *bdi)
     INIT_DELAYED_WORK(&wb->dwork, bdi_writeback_workfn);
void bdi_writeback_workfn(struct work_struct *work)         //文件系统回写
     while(bdi->work_list非空) pages_written = wb_do_writeback(wb, 0);//多层循环，内层循环写入某类work,从而实现高优先级的work先写入
         //work_list上只有强制行的work,没有ratio和时间回写
         wrote += wb_writeback(wb, work);
              如果worklist非空，暂时不处理for_background和for_kupdate的work
              跳过不满足for_background阈值条件的for_background work
              if (work->for_kupdate) 强制设置dirty_expire_interval使能超时场景//怎么没有计算历史时间
              if (list_empty(&wb->b_io)) {   //b_io完毕，现在处理老龄数据和之前被延迟到b_more_io的inode(之前没有sync要求)
                   list_splice_init(&wb->b_more_io, &wb->b_io); //合并b_more_io到b_io
                   queue_io(wb, work);//如果上个b_io已清空，从b_dirty移动超时回写到b_io链表
                        moved = move_expired_inodes(&wb->b_dirty, &wb->b_io, work);
              }
              if (work->sb) progress = __writeback_inodes_wb(wb, work);//一般情况下，b_io中内容是对应work的，之前刚刚添加。
                   wrote += writeback_sb_inodes(sb, wb, work);//文件系统层面的sync()，sb是有效的
                        foreach inode in b_io链表//如果inode锁定，可能想和其他inode一起回写提高效率，扔进b_more_io.
                             if ((inode->i_state & I_SYNC) && wbc.sync_mode != WB_SYNC_ALL) requeue_io(inode, wb);
                             if ((inode->i_state & I_SYNC) && wbc.sync_mode == WB_SYNC_ALL)
                                  inode_sleep_on_writeback(inode);continue;//解锁后重新检查回写inode
                             inode->i_state |= I_SYNC;
                             wbc.nr_to_write = write_chunk = writeback_chunk_size(wb->bdi, work);
                             wbc.pages_skipped = 0;
                             __writeback_single_inode(inode, &wbc);//回写inode文件所有的脏页。
                                  ret = do_writepages(mapping, wbc);
                                  if (wbc->sync_mode == WB_SYNC_ALL) filemap_fdatawait(mapping);//等待完成，下面判断是否写inode
                                  if (inode->i_state&(I_DIRTY|I_DIRTY_SYNC|I_DIRTY_DATASYNC)) write_inode(inode, wbc);
                                       ret = inode->i_sb->s_op->write_inode(inode, wbc); //fat_write_inode
                              if (time_is_before_jiffies(start_time + HZ / 10UL))    break; //最多写100ms就需要退出
                              if (work->nr_pages <= 0) break;

               else progress = __writeback_inodes_wb(wb, work);//文件层次的操作
          wrote += wb_check_old_data_flush(wb);//
               struct wb_writeback_work work = {
                   .nr_pages = nr_pages,.sync_mode = WB_SYNC_NONE,.for_kupdate = 1,.range_cyclic = 1,
                    .reason = WB_REASON_PERIODIC,
               };//检查如果wb->last_old_flush为5s前的jifflies，启动回写
               return wb_writeback(wb, &work);
          wrote += wb_check_background_flush(wb);
               if (over_bground_thresh(wb->bdi))
                    struct wb_writeback_work work = {
                        .nr_pages = LONG_MAX,.sync_mode = WB_SYNC_NONE,.for_background = 1,.range_cyclic = 1,
                        .reason = WB_REASON_BACKGROUND,};/超过回写下限
                    return wb_writeback(wb, &work);
          clear_bit(BDI_writeback_running, &wb->bdi->state);
     if (!list_empty(&bdi->work_list) ||(wb_has_dirty_io(wb) && dirty_writeback_interval))
          //dirty_writeback_interval可以为0？
          queue_delayed_work(bdi_wq, &wb->dwork,msecs_to_jiffies(dirty_writeback_interval * 10));
     current->flags &= ~PF_SWAPWRITE;
int do_writepages(struct address_space *mapping, struct writeback_control *wbc)
     if (mapping->a_ops->writepages)
          ret = mapping->a_ops->writepages(mapping, wbc); //fat_writepages-->mpage_writepages(mapping, wbc, fat_get_block);
               blk_start_plug(&plug);//耦合的数据块plug，全部request完毕unplug，一起加入到块设备的queue唤醒处理thread
               ret = write_cache_pages(mapping, wbc, __mpage_writepage, &mpd);
                   __mpage_writepage(page, wbc, data);
          　            foreach bio = mpage_bio_submit(WRITE, bio);
                              bio->bi_end_io = mpage_end_io;
                              submit_bio(rw, bio);
               if (mpd.bio) mpage_bio_submit(WRITE, mpd.bio);
               blk_finish_plug(&plug);
static int fat_writepage(struct page *page, struct writeback_control *wbc)
     return block_write_full_page --> block_write_full_page_endio(page, get_block, wbc,end_buffer_async_write);
         --> __block_write_full_page(inode, page, get_block, wbc, handler);
                   submit_bh(write_op, bh); -----> submit_bio(rw, bio);
```
```
fat表的管理
int fat_file_fsync(struct file *filp, loff_t start, loff_t end, int datasync)
fat_write_end函数中mark_inode_dirty(inode);好像会影响ｆａｔ表。
struct fs_struct {
    int users;
    spinlock_t lock;
    seqcount_t seq;
    int umask;
    int in_exec;
    struct path root, pwd;
};
```

