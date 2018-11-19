##

###  修订记录
| 修订说明 | 日期 | 作者 | 额外说明 |
| --- |
| 初版 | 2018/04/10 | 员清观 |  |

## 1 地址空间
几个基本的地址空间概念：
- 用户虚拟地址：User virtual addresses，用户程序见到的常规地址. 用户地址在长度上是 32 位或者 64 位, 依赖底层的硬件结构, 并且每个进程有它自己的虚拟地址空间.
- 总线地址：Bus addresses,在外设和内存之间使用的地址,但是这不是必要. 一些体系可提供一个 I/O 内存管理单元(IOMMU), 它在总线和主内存之间重映射地址.
- 物理地址：Physical addresses，在处理器和系统内存之间使用的地址. 有32bit或64bit等。
- 内核逻辑地址:Kernel logical addresses,他们是虚拟地址（需要经过MMU转换的地址），这些组成了常规的内核地址空间.这些地址映射了部分(也许全部)主存并且常常被当作物理内存. 在大部分的体系上, 逻辑地址和它们的相关物理地址只差一个常量偏移. 逻辑地址常常存储于 unsigned long 或者 void * 类型的变量中. 从 kmalloc 返回的内存就是内核逻辑地址. 如果有内核逻辑地址, 可通过宏 __pa() ( 在 中定义，但是也可能在它包含的头文件中)返回它关联的物理地址. 同时物理地址也可被映射回逻辑地址使用 __va(), 但是只适用于低端内存（后面会讲到）.  不同的内核函数需要不同类型地址.
- 内核虚拟地址：Kernel virtual addresses，它们都是从内核地址空间到物理地址的映射，但是内核虚拟地址并不必像逻辑地址空间一样具备线性的、一对一到物理地址的映射。所有的逻辑地址是内核虚拟地址, 但是许多内核虚拟地址不是逻辑地址. （也就是说在启动MMU后所有的地址都是内核虚拟地址，但是有一部分可以称为内核逻辑地址，因为他们具有上面介绍的特性：线性且连续、与对应的物理地址只差一个常量偏移）;对于内核虚拟地址，vmalloc 分配的内存是虚拟地址. kmap 函数也返回虚拟地址. 虚拟地址常常存储于指针变量.
- Low memory：低端内存 在内核空间中拥有逻辑地址的内存. 在大部分系统中（ARM构架几乎都是）,几乎所有的内存都是低端内存.
- High memory：高端内存 没有逻辑地址映射的内存,它位于内核逻辑地址范围之外，使用前必须使用vmalloc等内核函数做好映射.

在内核中phys_to_virt只是给地址减去一个固定的偏移; ioremap()的原则就是内核会根据指定的物理地址新建映射页表，物理地址和虚拟地址的关系就由这些页表来搭建; `#define __phys_to_virt(x)	((x) - PHYS_OFFSET + PAGE_OFFSET)`

许多内核数据结构必须放在低端内存; 高端内存主要为用户进程页所保留.

![linux内存管理内核内存地址空间](pic_dir/linux内存管理内核内存地址空间.png)

cat /proc/zoneinfo
cat /proc/buddyinfo
cat /proc/slabinfo
cat /proc/vmallocinfo
cat /proc/pagetypeinfo

CONFIG_CMA_SIZE_SEL_MBYTES :: 32Mbytes 对应CMA的大小定义
CONFIG_DMA_API_DEBUG :: 开启dma_alloc的trace,不知道是否有用.

```cpp


extern int memblock_debug;
//#define memblock_dbg(fmt, ...) \
	if (memblock_debug){ \
		printk(KERN_EMERG "@@@@@@@  %s,%d" , __func__, __LINE__); \
		printk(KERN_EMERG "@@@@@@@@@@@@@@@ [memblock]" pr_fmt(fmt), ##__VA_ARGS__); \
		}\
```
/* __GFP_WAIT表示分配内存的请求可以中断。也就是说，调度器在该请求期间可随意选择另一个过程执行，或者该请求可以被另一个更重要的事件中断. 分配器还可以在返回内存之前, 在队列上等待一个事件(相关进程会进入睡眠状态).
--->某些请求,是否可以多等等呢?


## 1.1 vmlinux.lds.s文件

```cpp
//.xx.xx : {} 段定义
//. = PAGE_OFFSET　定位到指定位置
//. = ALIGN(4); 字节对齐
//xxxx = .;　当前地址赋值到一个kernel中可以访问的变量
//可以使用外部定义的宏和目标文件中定义的符号，可以将全局变量直接设计定位

//include/asm_generic/vmlinux.lds.h 文件部分常用内容：

//对应所有的__setup("xxxx_name", xxxx_setup_func());定义，可以用来解析命令行中的xxxx_name参数，比如下面的定义:
	//Kernel command line: console=ttyAMA3,115200 lpj=567808 mem=62M rootfstype=ext4 root=/dev/mmcblk0p2 rw init=/init	//__setup("root=", root_dev_setup);
	//__setup("init=", init_setup);
	//__setup("rootfstype=", fs_names_setup);
	//__setup("rdinit=", rdinit_setup);
	//__setup("console=", console_setup);
	//__setup("lpj=", lpj_setup);
	//__setup("rw", readwrite);
	//__
//#define INIT_SETUP(initsetup_align)					\
		. = ALIGN(initsetup_align);				\
		VMLINUX_SYMBOL(__setup_start) = .;			\
		*(.init.setup)						\
		VMLINUX_SYMBOL(__setup_end) = .;
//#define INIT_CALLS_LEVEL(level)						\
		VMLINUX_SYMBOL(__initcall##level##_start) = .;		\
		*(.initcall##level##.init)				\
		*(.initcall##level##s.init)				\
//#define INIT_CALLS							\
		VMLINUX_SYMBOL(__initcall_start) = .;			\
		*(.initcallearly.init)					\
		INIT_CALLS_LEVEL(0)					\
		INIT_CALLS_LEVEL(1)					\
		INIT_CALLS_LEVEL(2)					\
		INIT_CALLS_LEVEL(3)					\
		INIT_CALLS_LEVEL(4)					\
		INIT_CALLS_LEVEL(5)					\
		INIT_CALLS_LEVEL(rootfs)				\
		INIT_CALLS_LEVEL(6)					\
		INIT_CALLS_LEVEL(7)					\
		VMLINUX_SYMBOL(__initcall_end) = .;
//#define BSS_SECTION(sbss_align, bss_align, stop_align)			\
	. = ALIGN(sbss_align);						\
	VMLINUX_SYMBOL(__bss_start) = .;				\
	SBSS(sbss_align)						\
	BSS(bss_align)							\
	. = ALIGN(stop_align);						\
	VMLINUX_SYMBOL(__bss_stop) = .;

//arch/arm/kernel/vmlinux.lds.S 文件主体部分．
OUTPUT_ARCH(arm)　//设置输出文件的arm
ENTRY(stext)　//stext的值设定为入口地址
jiffies = jiffies_64;
SECTIONS
{
	. = PAGE_OFFSET + TEXT_OFFSET; //定位，否则从0开始
	.head.text : {
		_text = .;
		HEAD_TEXT
	}
	.text : {			/* Real text segment		*/
		_stext = .;		/* Text and read-only data	*/
			__exception_text_start = .;
			*(.exception.text)
			__exception_text_end = .;
			IRQENTRY_TEXT
			TEXT_TEXT
			SCHED_TEXT
			LOCK_TEXT
			KPROBES_TEXT
			IDMAP_TEXT
			*(.fixup)
			*(.gnu.warning)
			*(.glue_7)
			*(.glue_7t)
		. = ALIGN(4);
		*(.got)			/* Global offset table		*/
			ARM_CPU_KEEP(PROC_INFO)
	}

	RO_DATA(PAGE_SIZE)

	. = ALIGN(4); //4字节对齐
	__ex_table : AT(ADDR(__ex_table) - LOAD_OFFSET) {
		__start___ex_table = .;
		*(__ex_table)
		__stop___ex_table = .;
	}

	/* * Stack unwinding tables */
	. = ALIGN(8);
	.ARM.unwind_idx : {
		__start_unwind_idx = .;
		*(.ARM.exidx*)
		__stop_unwind_idx = .;
	}
	.ARM.unwind_tab : {
		__start_unwind_tab = .;
		*(.ARM.extab*)
		__stop_unwind_tab = .;
	}

	NOTES

	_etext = .;			/* End of text and rodata section */

	. = ALIGN(PAGE_SIZE);
	__init_begin = .;

	INIT_TEXT_SECTION(8)
	.exit.text : {
		ARM_EXIT_KEEP(EXIT_TEXT)
	}
	.init.proc.info : {
		ARM_CPU_DISCARD(PROC_INFO)
	}
	.init.arch.info : {
		__arch_info_begin = .;
		*(.arch.info.init)
		__arch_info_end = .;
	}
	.init.tagtable : {
		__tagtable_begin = .;
		*(.taglist.init)
		__tagtable_end = .;
	}
	.init.pv_table : {
		__pv_table_begin = .;
		*(.pv_table)
		__pv_table_end = .;
	}
	.init.data : {
		INIT_DATA
		INIT_SETUP(16)
		INIT_CALLS
		CON_INITCALL
		SECURITY_INITCALL
		INIT_RAM_FS
	}
	.exit.data : {
		ARM_EXIT_KEEP(EXIT_DATA)
	}

//#ifdef CONFIG_SMP
	PERCPU_SECTION(L1_CACHE_BYTES)
//#endif

	__init_end = .;
	. = ALIGN(THREAD_SIZE);
	__data_loc = .;

	.data : AT(__data_loc) {
		_data = .;		/* address in memory */
		_sdata = .;

		/*
		 * first, the init task union, aligned
		 * to an 8192 byte boundary.
		 */
		INIT_TASK_DATA(THREAD_SIZE)

		NOSAVE_DATA
		CACHELINE_ALIGNED_DATA(L1_CACHE_BYTES)
		READ_MOSTLY_DATA(L1_CACHE_BYTES)

		/*
		 * and the usual data section
		 */
		DATA_DATA
		CONSTRUCTORS

		_edata = .;
	}
	_edata_loc = __data_loc + SIZEOF(.data);

	. = ALIGN(PAGE_SIZE);
	__sram_copy_in_dram = .;
	.sram IMAP_SRAM_VIRTUAL_ADDR : AT(__sram_copy_in_dram) {
		__sram_start = .;
		*(.sram.start);
		. = ALIGN(8);
		*(.sram.text);
		. = ALIGN(8);
		*(.sram.rodata);
		. = ALIGN(8);
		*(.sram.data);
		. = ALIGN(8);
		__sram_bss = .;
		*(.sram.bss);
		. = ALIGN(8);
		__sram_end = .;
	}
	. = __sram_copy_in_dram + SIZEOF(.sram);
	NOTES

	BSS_SECTION(0, 0, 0)
	_end = .;

	STABS_DEBUG
	.comment 0 : { *(.comment) }
}
```

## 2 关键全局数据结构

```cpp
struct bootmem_data;
typedef struct pglist_data {
	struct zone node_zones[MAX_NR_ZONES]; /* 包含了结点中各内存域的数据结构, 可能的区域类型用zone_type表示*/
	struct zonelist node_zonelists[MAX_ZONELISTS];/*  指点了备用结点及其内存域的列表，以便在当前结点没有可用空间时，在备用结点分配内存  我们的SMP系统UMA结构，不存在*/
	int nr_zones;/*  保存结点中不同内存域的数目    */
  struct page *node_mem_map;   /* 指向page实例数组的指针，用于描述结点的所有物理内存页，它包含了结点中所有内存域的页 */
  struct bootmem_data *bdata;　/* 在系统启动boot期间，内存管理子系统初始化之前，内核页需要使用内存（另外，还需要保留部分内存用于初始化内存管理子系统）为解决这个问题，内核使用了自举内存分配器，此结构用于这个阶段的内存管理  */
  unsigned long node_start_pfn; /*起始页面帧号，指出该节点在全局mem_map中的偏移，系统中所有的页帧是依次编号的，每个页帧的号码都是全局唯一的（不只是结点内唯一）在UMA系统中总是0， 因为系统中只有一个内存结点， 因此其第一个页帧编号总是0. */
	unsigned long node_present_pages; /* total number of physical pages 结点中页帧的数目　*/
	unsigned long node_spanned_pages; /* total size of physical page range, including holes */
	int node_id;　/*  全局结点ID，系统中的NUMA结点都从0开始编号 --- 无用 */
	nodemask_t reclaim_nodes;	/* Nodes allowed to reclaim from */
	wait_queue_head_t kswapd_wait;　/*  交换守护进程的等待队列，在将页帧换出结点时会用到。后面的文章会详细讨论 */
	wait_queue_head_t pfmemalloc_wait;
	struct task_struct *kswapd;	/* Protected by lock_memory_hotplug()，指向负责该结点的交换守护进程的task_struct, 在将页帧换出结点时会唤醒该进程，我们的UMA应该没有什么用处 */
	int kswapd_max_order;　/*  定义需要释放的区域的长度  */
	enum zone_type classzone_idx;
} pg_data_t;



```

在内存中，每个簇所对应的node又被分成的称为管理区(zone)的块，它们各自描述在内存中的范围。一个管理区(zone)由struct zone结构体来描述．zone对象用于跟踪诸如页面使用情况的统计数, 空闲区域信息和锁信息．里面保存着内存使用状态信息，如page使用统计, 未使用的内存区域，互斥访问的锁（LOCKS）等.
管理区的类型用zone_type表示,我们的系统中用到了：　<br>
- ZONE_NORMAL 	标记了可直接映射到内存段的普通内存域. 这是在所有体系结构上保证会存在的唯一内存区域, 但无法保证该地址范围对应了实际的物理地址. 例如, 如果AMD64系统只有两2G内存, 那么所有的内存都属于ZONE_DMA32范围, 而ZONE_NORMAL则为空；
- ZONE_MOVABLE 	内核定义了一个伪内存域ZONE_MOVABLE, 在防止物理内存碎片的机制memory migration中需要使用该内存域. 供防止物理内存碎片的极致使用
- ZONE_HIGHMEM 	896MB~物理内存结束，实际没有用到

```cpp
enum {
	MIGRATE_UNMOVABLE,
	MIGRATE_RECLAIMABLE,
	MIGRATE_MOVABLE,
	MIGRATE_PCPTYPES,	/* the number of types on the pcp lists */
	MIGRATE_RESERVE = MIGRATE_PCPTYPES,
	MIGRATE_CMA,
	MIGRATE_TYPES
};//内存迁移类型
static int fallbacks[MIGRATE_TYPES][4] = {
	[MIGRATE_UNMOVABLE]   = { MIGRATE_RECLAIMABLE, MIGRATE_MOVABLE,     MIGRATE_RESERVE },
	[MIGRATE_RECLAIMABLE] = { MIGRATE_UNMOVABLE,   MIGRATE_MOVABLE,     MIGRATE_RESERVE },
	[MIGRATE_MOVABLE]     = { MIGRATE_CMA,         MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE, MIGRATE_RESERVE },
	[MIGRATE_CMA]         = { MIGRATE_RESERVE }, /* Never used */
	[MIGRATE_RESERVE]     = { MIGRATE_RESERVE }, /* Never used */
};
//[MIGRATE_MOVABLE]     = { MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE, MIGRATE_RESERVE }, //如果禁止MOVABLE类型使用CMA
struct free_area {
	struct list_head	free_list[MIGRATE_TYPES];
	unsigned long		nr_free;
};
enum zone_watermarks {
	WMARK_MIN,	WMARK_LOW,	WMARK_HIGH,	NR_WMARK
};
//#define min_wmark_pages(z) (z->watermark[WMARK_MIN])
//#define low_wmark_pages(z) (z->watermark[WMARK_LOW])
//#define high_wmark_pages(z) (z->watermark[WMARK_HIGH])

struct zone {
  unsigned long watermark[NR_WMARK];//每个 zone 在系统启动时会计算出 3 个水位值, 分别为 WMAKR_MIN, WMARK_LOW, WMARK_HIGH 水位, 这在页面分配器和 kswapd 页面回收中会用到
  unsigned long percpu_drift_mark;
  unsigned long		lowmem_reserve[MAX_NR_ZONES];//zone 中预留的内存, 为了防止一些代码必须运行在低地址区域，所以事先保留一些低地址区域的内存
  unsigned long		dirty_balance_reserve;
  //这个数组用于实现每个CPU的热/冷页帧列表。内核使用这些列表来保存可用于满足实现的“新鲜”页。但冷热页帧对应的高速缓存状态不同：有些页帧很可能在高速缓存中，因此可以快速访问，故称之为热的；未缓存的页帧与此相对，称之为冷的，page管理的数据结构对象，内部有一个page的列表(list)来管理。每个CPU维护一个page list，避免自旋锁的冲突。这个数组的大小和NR_CPUS(CPU的数量）有关，这个值是编译的时候确定的
  struct per_cpu_pageset __percpu *pageset;
  spinlock_t		lock;
	int                     all_unreclaimable; /* All pages pinned */
  bool			compact_blockskip_flush;
  unsigned long		compact_cached_free_pfn;
	unsigned long		compact_cached_migrate_pfn;

  /* free areas of different sizes : 页面使用状态的信息，以每个bit标识对应的page是否可以分配，是用于伙伴系统的，每个数组元素指向对应阶也表的数组开头，以下是供页帧回收扫描器(page reclaim scanner)访问的字段，scanner会跟据页帧的活动情况对内存域中使用的页进行编目，如果页帧被频繁访问，则是活动的，相反则是不活动的，在需要换出页帧时，这样的信息是很重要的*/
  struct free_area	free_area[MAX_ORDER];　 //MAX_ORDER : 0 ~~ 10
	unsigned long		*pageblock_flags; //可以跟踪包含pageblock_nr_pages个页的内存区的属性
  spinlock_t		lru_lock; //LRU(最近最少使用算法)的自旋锁
	struct lruvec		lruvec;　//LRU 链表集合

	unsigned long		pages_scanned;	   /* since last reclaim */
	unsigned long		flags;		   /* zone flags, see below */
  atomic_long_t		vm_stat[NR_VM_ZONE_STAT_ITEMS]; //zone 计数
  unsigned int inactive_ratio;

  wait_queue_head_t	* wait_table;  /* 进程等待一个page释放的队列的散列表, 它会被wait_on_page()，unlock_page()函数使用. 用哈希表，而不用一个等待队列的原因，防止进程长期等待资源 */
	unsigned long		wait_table_hash_nr_entries; /*  等待队列散列表中的调度实体数目  */
	unsigned long		wait_table_bits; /*  等待队列散列表数组大小, 值为2^order  */

  struct pglist_data	*zone_pgdat; //指向这个zone所在的pglist_data对象
	/* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
	unsigned long		zone_start_pfn;
  unsigned long		spanned_pages; //zone 中包含的页面数量
	unsigned long		present_pages; //zone 中实际管理的页面数量. 对一些体系结构来说, 其值和 spanned_pages 相等
	unsigned long		managed_pages; //zone 中被伙伴系统管理的页面数量
  const char		*name; //”Normal”
}____cacheline_internodealigned_in_smp;

typedef struct pglist_data pg_data_t;
unsigned long min_low_pfn; //系统可用的第一个pfn, 0, 对应pfn::0x40000
unsigned long max_low_pfn; //系统可用的最后一个PFN,0x3e00, 对应pfn::0x43e00, 0x3e00对应15872个实际的page
unsigned long max_pfn; //系统可用的最后一个PFN,0x3e00,arm_bootmem_free()调用后是0x40000+0x3e00
//PHYS_PFN_OFFSET :: 0x40000 页帧号开始偏移．
bootmem_data_t bootmem_node_data[MAX_NUMNODES] __initdata;
struct pglist_data __refdata contig_page_data = {	.bdata = &bootmem_node_data[0] };

int min_free_kbytes = 1024;
int min_free_order_shift = 1;
int extra_free_kbytes = 0;
static unsigned long __meminitdata nr_kernel_pages; //0x3d84 at end of free_area_init_core()
static unsigned long __meminitdata nr_all_pages;    //0x3d84 at end of free_area_init_core()
static unsigned long __meminitdata dma_reserve;     //0 always

//初始化zone的内存页
//在分配内存时, 如果必须”盗取”不同于预定迁移类型的内存区, 内核在策略上倾向于”盗取”更大的内存区. 由于所有页最初都是可移动的, 那么在内核分配不可移动的内存区时, 则必须”盗取”.实际上, 在启动期间分配可移动内存区的情况较少, 那么分配器有很高的几率分配长度最大的内存区, 并将其从可移动列表转换到不可移动列表. 由于分配的内存区长度是最大的, 因此不会向可移动内存中引入碎片.总而言之, 这种做法避免了启动期间内核分配的内存(经常在系统的整个运行时间都不释放)散布到物理内存各处, 从而使其他类型的内存分配免受碎片的干扰，这也是页可移动性分组框架的最重要的目标之一.
void __meminit memmap_init_zone(unsigned long size, int nid, unsigned long zone,unsigned long start_pfn, enum memmap_context context)

unsigned long max_mapnr;
struct page *mem_map; //页的数据结构对象都保存在mem_map全局数组中，该数组通常被存放在ZONE_NORMAL的首部，或者就在小内存系统中为装入内核映像而预留的区域之后。从载入内核的低地址内存区域的后面内存区域，也就是ZONE_NORMAL开始的地方的内存的页的数据结构对象，都保存在这个全局数组中

int __init kswapd_init(void)
  kswapd_run(nid);   //|--> pgdat->kswapd = kthread_run(kswapd, pgdat, "kswapd%d", nid);
  hotcpu_notifier(cpu_callback, 0);


```
## 2.1 内存初始化主流程

如果能知道当前函数运行时层次,就可以方便的打印trace信息了.

```cpp
//内存管理主干框架
struct meminfo meminfo;
static char __initdata cmd_line[COMMAND_LINE_SIZE];

void __init arm_bootmem_free(unsigned long min, unsigned long max_low, unsigned long max_high)
	zhol_size[0] = max_low - min; 	zhole_size[0] = max_low - min; //只有一个内存区域
  |--> free_area_init_node(0, zone_size, min, zhole_size);//void __paginginit free_area_init_node(int nid, unsigned long *zones_size, unsigned long node_start_pfn, unsigned long *zholes_size)
    pg_data_t *pgdat = NODE_DATA(nid);    pgdat->node_id = nid; pgdat->node_start_pfn = node_start_pfn; //从0开始的
    calculate_node_totalpages(pgdat, zones_size, zholes_size);//单个ZONE, MAX_NR_ZONES::2 pgdat->node_spanned_pages=pgdat->node_present_pages=15872pages对应62M,
    |--> alloc_node_mem_map(pgdat);//void alloc_node_mem_map(struct pglist_data *pgdat)
      start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);//从0开始
			end = ALIGN(pgdat_end_pfn(pgdat), 0x400);//0x400对齐
      size =  (end - start) * sizeof(struct page);//(0x44000-0x4000)*32 = 0x80000 bytes (512k)
      |--> map = alloc_bootmem_node_nopanic(pgdat, size);//__alloc_bootmem_node_nopanic(pgdat, x, SMP_CACHE_BYTES, BOOTMEM_LOW_LIMIT),申请0x80000字节，并且0x40字节对齐(cacheline)
      mem_map = NODE_DATA(0)->node_mem_map = map + (pgdat->node_start_pfn - start);//pgdat:0xc0569724,mem_map：0xc06db000开始保存所有的struct page信息,有效的总15872page;设备映射的1.5M不需要PAGE结构管理.
    |--> free_area_init_core(pgdat, zones_size, zholes_size);//void __paginginit free_area_init_core(struct pglist_data *pgdat, unsigned long *zones_size, unsigned long *zholes_size)
			//初始化swapd线程;初始化zone数据结构.
      int nid = pgdat->node_id; unsigned long zone_start_pfn = pgdat->node_start_pfn;
      init_waitqueue_head(&pgdat->kswapd_wait);   init_waitqueue_head(&pgdat->pfmemalloc_wait);
      struct zone *zone = &pgdat->node_zones[0];//我们只有一个zone，而且内存地址中间没有hole
      zone->managed_pages = zone->present_pages  = zone->spanned_pages = size = zone_spanned_pages_in_node(nid, j, zones_size);//-->return zones_size[0];
			zone->zone_pgdat = pgdat;
      zone_pcp_init(zone); //-->zone->pageset = &boot_pageset;
			set_pageblock_order();
      setup_usemap(pgdat, zone, zone_start_pfn, size);
			ret = init_currently_empty_zone(zone, zone_start_pfn,size, MEMMAP_EARLY);
			|--> memmap_init_zone(size, nid, j, zone_start_pfn);//void __meminit memmap_init_zone(unsigned long size, int nid, unsigned long zone,	unsigned long start_pfn, enum memmap_context context)
				unsigned long end_pfn = start_pfn + size;	highest_memmap_pfn = end_pfn - 1;
				for (pfn = start_pfn; pfn < end_pfn; pfn++) //初步初始化每个struct page,设置迁移类型.
					page = pfn_to_page(pfn);
					set_page_links(page, zone, nid, pfn);//-->set_page_zone(page, zone);set_page_node(page, node);
					init_page_count(page);//-->atomic_set(&page->_count, 1);
					page_mapcount_reset(page);//-->atomic_set(&(page)->_mapcount, -1);
					SetPageReserved(page);
					set_pageblock_migratetype(page, MIGRATE_MOVABLE);//-->__set_bit(bitidx + start_bitidx, &zone->pageblock_flags);
void __init bootmem_init(void)
	find_limits(&min, &max_low, &max_high); //从memblock对应的meminfo结构中
	|--> arm_bootmem_init(min, max_low);//void arm_bootmem_init(unsigned long start_pfn,unsigned long end_pfn)
		boot_pages = bootmem_bootmap_pages(end_pfn - start_pfn);//计算mapping需要的管理页面的个数,每个page对应其中一个bit.
		bitmap = memblock_alloc_base(boot_pages << PAGE_SHIFT, L1_CACHE_BYTES,__pfn_to_phys(end_pfn));//分配管理内存,大概0x1000
		|--> init_bootmem_node(pgdat, __phys_to_pfn(bitmap), start_pfn, end_pfn);//--->init_bootmem_core(pgdat->bdata, freepfn, startpfn, endpfn);		//初始化bootmem,然后将memblock当前mem占用情况转换到bootmem机制::->//unsigned long __init init_bootmem_core(bootmem_data_t *bdata,unsigned long mapstart, unsigned long start, unsigned long end)
			bdata->node_bootmem_map = phys_to_virt(PFN_PHYS(mapstart));
			bdata->node_min_pfn = start;	bdata->node_low_pfn = end;
			link_bootmem(bdata);//void  link_bootmem(bootmem_data_t *bdata) //-->list_add_tail(&bdata->list, &bdata_list);
			mapsize = bootmap_bytes(end - start);
			memset(bdata->node_bootmem_map, 0xff, mapsize);
		for_each_memblock(memory, reg)	free_bootmem(__pfn_to_phys(start), (end - start) << PAGE_SHIFT);
		for_each_memblock(reserved, reg) reserve_bootmem(__pfn_to_phys(start),(end - start) << PAGE_SHIFT, BOOTMEM_DEFAULT);
	//arm_memory_present();		sparse_init();
	arm_bootmem_free(min, max_low, max_high); //max_low:: 0x40000, 页号
	max_low_pfn = max_low - PHYS_PFN_OFFSET;
	max_pfn = max_high - PHYS_PFN_OFFSET;

void __init setup_arch(char **cmdline_p)
	setup_processor();//获取并且显示cpu 类型和cache等信息:　//CPU: ARMv7 Processor [410fc051] revision 1 (ARMv7), cr=50c53c7d	//CPU: PIPT / VIPT nonaliasing data cache, VIPT aliasing instruction cache
	//mdesc = setup_machine_fdt(__atags_pointer); 空函数，所以实际跳转到下面函数：
	|--> machine_desc = mdesc = setup_machine_tags(__atags_pointer, __machine_arch_type);//从__atags_pointer(0x40000100,0x100为偏移地址)中解析参数；然后获取boot_command_line；获取mdesc描述数据结构
		for_each_machine_desc(p)	if (machine_nr == p->nr) {	mdesc = p;	break;}//找到产品mdesc描述数据结构
		tags = phys_to_virt(__atags_pointer);	save_atags(tags);	parse_tags(tags);
		strlcpy(boot_command_line, from, COMMAND_LINE_SIZE);　return mdesc;
	init_mm.start_code = (unsigned long) _text;		init_mm.end_code   = (unsigned long) _etext;
	init_mm.end_data   = (unsigned long) _edata;  init_mm.brk	   = (unsigned long) _end;
	strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);		*cmdline_p = cmd_line;
	parse_early_param(); //解析所有的early_param()定义, 现在看到的只有 : early_param("mem", early_mem);也许还会有early_param("cma", early_cma);early_param("initrd", early_initrd);early_param("loglevel", loglevel);
		|--> early_mem(char*p);//int __init early_mem(char *p)
			start = PHYS_OFFSET;	size  = memparse(p, &endp);
			|--> arm_add_memory(start, size);//int __init arm_add_memory(phys_addr_t start, phys_addr_t size)
				struct membank *bank = &meminfo.bank[meminfo.nr_banks];
				bank->start = PAGE_ALIGN(start); size -= start & ~PAGE_MASK;
				bank->size = size & ~(phys_addr_t)(PAGE_SIZE - 1);	meminfo.nr_banks++;//应该只有而且只需要考虑一个bank
				bank->highmem = 0;
	|--> sanity_check_meminfo();//void __init sanity_check_meminfo(void)
		meminfo.nr_banks = 1; high_memory = __va(arm_lowmem_limit - 1) + 1;　//0x43e00000-->0xc3e00000
		memblock_set_current_limit(arm_lowmem_limit);
	|--> arm_memblock_init(&meminfo, mdesc);//void arm_memblock_init(struct meminfo *mi, struct machine_desc *mdesc)
		memblock_add(mi->bank[0].start, mi->bank[0].size);//bankstart:40000000 banksize:3e00000,添加到 memblock.memory
		memblock_reserve(__pa(_stext), _end - _stext);//reserve代码和数据段
		arm_mm_memblock_reserve();//--> memblock_reserve(__pa(swapper_pg_dir), SWAPPER_PG_DIR_SIZE); 为mm管理页目录信息预留16k内存::0x40004000-->0x40008000
		arm_dt_memblock_reserve();//为设备树管理预留内存
		q3f_reserve();//为dsp预留-->memblock_reserve(arm_lowmem_limit, 0x180000);
		dma_contiguous_reserve(arm_lowmem_limit); //cma内存从0x41800000开始的32M
  |||------>>>paging_init(mdesc); //已经解析， above
	|--> request_standard_resources(mdesc);//void request_standard_resources(struct machine_desc *mdesc)
		kernel_code.start   = virt_to_phys(_text);				kernel_code.end     = virt_to_phys(_etext - 1);
		kernel_data.start   = virt_to_phys(_sdata);				kernel_data.end     = virt_to_phys(_end - 1);
		struct resource *res = alloc_bootmem_low(sizeof(*res)); //分配一个结构体
		res->name  = "System RAM";	res->start = __pfn_to_phys(memblock_region_memory_base_pfn(region));
		res->end = __pfn_to_phys(memblock_region_memory_end_pfn(region)) - 1;	res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
		request_resource(&iomem_resource, res);	request_resource(res, &kernel_code);	request_resource(res, &kernel_data);
	arm_pm_restart = mdesc->restart;
	|--> mdesc->init_early();//void __init q3f_init_early(void)
		item_init(rbget("itemrrtb"), ITEM_SIZE_NORMAL);
		rtcbit_init();

void __init paging_init(struct machine_desc *mdesc)/* paging_init() sets up the page tables, initialises the zone memory maps, and sets up the zero page, bad page and bad page tables. */
	memblock_set_current_limit(arm_lowmem_limit); //设置memblock.current_limit = arm_lowmem_limit;
	build_mem_type_table(); //设置mem_types[]数组
	|--> prepare_page_table();//void prepare_page_table(void) 初始化之前分配的16k PMD页目录区域
		for ( addr = 0; addr < PAGE_OFFSET; addr += PMD_SIZE)	pmd_clear(pmd_off_k(addr));
		for (addr = __phys_to_virt(arm_lowmem_limit);addr < VMALLOC_START; addr += PMD_SIZE) pmd_clear(pmd_off_k(addr));
	|--> map_lowmem();//void __init map_lowmem(void)
		map.pfn = __phys_to_pfn(start::0x40000000);	map.virtual = __phys_to_virt(start);//pfn::0x40000
		map.length = end - start;	//0x3e00000
		map.type = MT_MEMORY;
		|--> create_mapping(&map, false);//void create_mapping(struct map_desc *md, bool force_pages)
			addr = md->virtual & PAGE_MASK;	phys = __pfn_to_phys(md->pfn);
			length = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));	end = addr + length;
			pgd = pgd_offset_k(addr); //计算虚拟地址对应的pgd开始地址, 本系统从0xc0007000开始
			|--> for(31) alloc_init_pud(pgd, addr, next, phys, type, force_pages); //调用了31次,产生31个pgd目录项,每个对应2M空间 //void __init alloc_init_pud(pgd_t *pgd, unsigned long addr,unsigned long end, unsigned long phys, const struct mem_type *type,bool force_pages)
				pud_t *pud = pud_offset(pgd, addr); //pud = pgd; 没有pud
				|--> alloc_init_pmd(pud, addr, next, phys, type, force_pages);//void __init alloc_init_pmd(pud_t *pud, unsigned long addr,unsigned long end, phys_addr_t phys, const struct mem_type *type, bool force_pages)
					pmd_t *pmd = pmd_offset(pud, addr);  //pmd = pud = pgd; 没有pmd
						__map_init_section(pmd, addr, next, phys, type); //force_pages为0 //*pmd = __pmd(phys | type->prot_sect);
						//alloc_init_pte(pmd, addr, next, __phys_to_pfn(phys), type);//force_pages为1
	|--> dma_contiguous_remap();//void dma_contiguous_remap(void)
		map.pfn = __phys_to_pfn(start);		map.virtual = __phys_to_virt(start); //start:0x41800000 end:0x43800000
		map.length = end - start;		map.type = MT_MEMORY_DMA_READY;
		|--> iotable_init(&map, 1);//void __init iotable_init(struct map_desc *io_desc, int nr)
			struct static_vm *svm = early_alloc_aligned(sizeof(*svm) * nr::1, __alignof__(*svm));
			create_mapping(md, false);//最终会实际分配并初始化每个页表 --- > void __init alloc_init_pte(pmd_t *pmd, unsigned long addr, unsigned long end, unsigned long pfn,const struct mem_type *type)
					pte_t *start_pte = early_pte_alloc(pmd);//为页表分配4k空间.
					while() { set_pte_ext(pte, pfn_pte(pfn, __pgprot(type->prot_pte)), 0);pfn++; pte++; addr += PAGE_SIZE; }//512个页的偏移地址填充到页表对应的位置,总占用2k,
					early_pte_install(pmd, start_pte, type->prot_l1); //页表地址和属性信息填充pmd目录项, 每条512个页
			add_static_vm_early(svm);
	|--> devicemaps_init(mdesc);//void __init devicemaps_init(struct machine_desc *mdesc)
		void *vectors = early_alloc(PAGE_SIZE);	early_trap_init(vectors);
		map.pfn = __phys_to_pfn(virt_to_phys(vectors));	map.virtual = 0xffff0000;
		map.length = PAGE_SIZE;	map.type = MT_HIGH_VECTORS; 	create_mapping(&map, false);
		|--> mdesc->map_io();//void __init q3f_map_io(void)
			iotable_init(imap_iodesc, ARRAY_SIZE(imap_iodesc));//struct map_desc imap_iodesc[]定义了芯片的几个寄存器bank区间,分别映射;每一个区间对应一个vm,添加到vmlist.
	//|--> kmap_init();  	|--> tcm_init(); //好像没有做什么
	top_pmd = pmd_off_k(0xffff0000);
	zero_page = early_alloc(PAGE_SIZE);/* allocate the zero page. */
	|--> bootmem_init();
	empty_zero_page = virt_to_page(zero_page);
	__flush_dcache_page(NULL, empty_zero_page); //skip this func

asmlinkage void __init start_kernel(void)
	extern const struct kernel_param __start___param[], __stop___param[]; //这里关键是这两个变量的地址是如何确定的。这两个变量为地址指针，指向内核启动参数处理相关结构体段在内存中的位置（虚拟地址）。这里是外部变量，定义的位置在arch/../../vmlinux.lds.S，而包括ARM的大多数平台是放到kernel\include\asm-generic\vmlinux.lds.h中
	lockdep_init();//    lockdep是一个内核调试模块，用来检查内核互斥机制（尤其是自旋锁）潜在的死锁问题。由于自旋锁以查询方式等待，不释放处理器，比一般互斥机制更容易死锁，故引入lockdep检查以下几种可能的死锁情况：同一个进程递归地加锁同一把锁；一把锁既在中断（或中断下半部）使能的情况下执行过加锁操作， 又在中断（或中断下半部）里执行过加锁操作。这样该锁有可能在锁定时由于中断发生又试图在同一处理器上加锁；加锁后导致依赖图产生成闭环，这是典型的死锁现象
	cgroup_init_early();//cgroup: 它的全称为control group.即一组进程的行为控制.该函数主要是做数据结构和其中链表的初始化
	local_irq_disable();//    关闭系统总中断（底层调用汇编指令）
	early_boot_irqs_disabled = true;//    设置系统中断的关闭标志（bool全局变量）
	boot_cpu_init();//激活当前CPU（在内核全局变量中将当前CPU的状态设为激活状态）
	page_address_init(); //实际是空函数;    高端内存相关，未定义高端内存的话为空函数
  |||------>>>setup_arch(&command_line);//void __init setup_arch(char **cmdline_p)
	mm_init_owner(&init_mm, &init_task); 	mm_init_cpumask(&init_mm);//初始化代表内核本身内存使用的管理结构体init_mm。ps：每一个任务都有一个mm_struct结构以管理内存空间，init_mm是内核的mm_struct，其中：设置成员变量* mmap指向自己，意味着内核只有一个内存管理结构; 设置* pgd=swapper_pg_dir，swapper_pg_dir是内核的页目录(在arm体系结构有16k， 所以init_mm定义了整个kernel的内存空间)。
	setup_command_line(command_line);//对cmdline进行备份和保存：
	setup_nr_cpu_ids();	setup_per_cpu_areas(); 	smp_prepare_boot_cpu();	/* 针对SMP处理器的内存初始化函数，如果不是SMP系统则都为空函数。 */
	build_all_zonelists(NULL, NULL); page_alloc_init();parse_early_param();	parse_args(...); jump_label_init();//设置内存管理相关的node（节点，每个CPU一个内存节点）和其中的zone（内存域，包含于节点中，如）数据结构,以完成内存管理子系统的初始化，并设置bootmem分配器。
	setup_log_buf(0);	//    使用bootmem分配一个记录启动信息的缓存区
	pidhash_init();//    使用bootmem分配并初始化PID散列表，由PID分配器管理空闲和已指派的PID
	vfs_caches_init_early();//    前期VFS缓存初始化
	sort_main_extable();//    对内核异常表（ exception table ）按照异常向量号大小进行排序。
	trap_init();//    对内核陷阱异常进行初始化
	|--> mm_init();//struct mm_struct *mm_init(struct mm_struct *mm, struct task_struct *p)
		page_cgroup_init_flatmem();
    |--> mem_init();//void mem_init(void)
			max_mapnr   = pfn_to_page(max_pfn + PHYS_PFN_OFFSET) - mem_map;
			|--> totalram_pages += free_all_bootmem(); //释放pages -->total_pages += free_all_bootmem_core(bdata);				//unsigned long __init free_all_bootmem_core(bootmem_data_t *bdata)
				start = bdata->node_min_pfn;end = bdata->node_low_pfn;
				for(all page) __free_pages_bootmem(pfn_to_page(start), order);//空闲的page转交给zone管理.
		kmem_cache_init();//释放bootmem到伙伴系统,slab分配器初始化,之后内存分配需要使用slab和buddy的api
		percpu_init_late();		vmalloc_init(); //暂时不看
	sched_init(); //    初始化调度器数据结构，并创建运行队列
	preempt_disable(); 	if (WARN(!irqs_disabled(), "Interrupts were enabled *very* early, fixing it\n")) local_irq_disable();// 在启动的初期关闭抢占和中断。早期启动时的调度是极为脆弱的，直到cpu_idle()的首次运行
	idr_init_cache(); //为IDR机制分配缓存，主要是为struct idr_layer结构体分配空间
	perf_event_init(); //    CPU性能监视机制初始化,此机制包括CPU同一时间执行指令数，cache miss数，分支预测失败次数等性能参数
	rcu_init();	//内核RCU（Read-Copy Update：读取-复制-更新）机制初始化
	tick_nohz_init();	radix_tree_init(); //    内核radix树算法初始化
	/* init some links before init_ISA_irqs() */
	early_irq_init();	init_IRQ();//硬件中断系统初始化：early_irq_init();前期外部中断描述符初始化，主要初始化数据结构。init_IRQ;对应构架特定的中断初始化函数，在ARM构架中会调用machine_desc->init_irq();也就是运行设备描述结构体中的init_irq函数，此函数一般在板级初始化文件（arch/*/mach-*/board-*.c）中定义。
	tick_init();
	init_timers();	hrtimers_init();	softirq_init();	timekeeping_init();	time_init(); //以上几个函数主要是初始化内核的软中断及时钟机制：前面几个函数主要是注册一些内核通知函数到cpu和hotcpu通知链，并开启部分软中断（tasklet等）。最后的time_init是构架相关的，旨在开启一个硬件定时器，开始产生系统时钟。
	profile_init(); //    初始化内核profile子系统，她是内核的性能调试工具。
	call_function_init(); //    初始化所有CPU的call_single_queue（具体作用还没搞明白），并注册CPU热插拔通知函数到CPU通知链中。
	WARN(!irqs_disabled(), "Interrupts were enabled early\n"); 	early_boot_irqs_disabled = false; 	local_irq_enable();//    检测硬件中断是否开启，如果开启了就打印出警告。    设置启动早期IRQ使能标志，允许IRQ使能。   最后开启总中断（ARM构架是这样，其他构架可能也是这个意思）。
	kmem_cache_init_late(); //    slab分配器的后期初始化。如果使用的是slob或slub，则为空函数。
	console_init();
	if (panic_later)
		panic(panic_later, panic_param);

	lockdep_info();    // 打印lockdep调试模块的信息。
	locking_selftest();
	page_cgroup_init(); //mem_cgroup是cgroup体系中提供的用于memory隔离的功能，此处对此功能进行初始化。
	debug_objects_mem_init(); //    debug objects机制的内存分配初始化
	kmemleak_init(); //    内核内存泄漏检测机制初始化；
	setup_per_cpu_pageset(); //    设置每个CPU的页组，并初始化。此前只有启动页组。
	numa_policy_init(); //    非一致性内存访问（NUMA）初始化
	if (late_time_init)
		late_time_init(); //    如果构架存在此函数，就调用后期时间初始化。
	sched_clock_init(); //    对每个CPU，初始化调度时钟。
	calibrate_delay(); //    计算BogoMIPS值，他是衡量一个CPU性能的标志。
	pidmap_init(); //    PID分配映射初始化。
	anon_vma_init(); //    匿名虚拟内存域（ anonymous VMA）初始化
	thread_info_cache_init(); //    获取thread_info缓存空间，大部分构架为空函数（包括ARM）
	cred_init(); //    任务信用系统初始化。详见：Documentation/credentials.txt
	fork_init(totalram_pages); //    进程创建机制初始化。为内核"task_struct"分配空间，计算最大任务数。
	proc_caches_init(); //    初始化进程创建机制所需的其他数据结构，为其申请空间。
	buffer_init(); //    缓存系统初始化，创建缓存头空间，并检查其大小限时。
	key_init(); //    内核密匙管理系统初始化。
	security_init(); //    内核安全框架初始化
	dbg_late_init(); //    内核调试系统后期初始化
	vfs_caches_init(totalram_pages); //虚拟文件系统（VFS）缓存初始化
	signals_init(); //    信号管理系统初始化
	/* rootfs populating might need page-writeback */
	page_writeback_init(); //    页回写机制初始化
	proc_root_init(); //    proc文件系统初始化
	cgroup_init();//control group的正式初始化
	cpuset_init(); //CPUSET初始化
	taskstats_init_early();//    任务状态早期初始化函数：为结构体获取高速缓存，并初始化互斥机制。
	delayacct_init(); //    任务延迟机制初始化
	check_bugs(); //    检查CPU BUG的函数，通过软件规避BUG
	acpi_early_init(); /* before LAPIC and SMP init,     ACPI早期初始化函数。 ACPI - Advanced Configuration and Power Interface高级配置及电源接口 */
	sfi_init_late();//SFI 初始程序晚期设置函数
	ftrace_init();//    功能跟踪调试机制初始化，ftrace 是 function trace 的简称。
	rest_init();

//void __init free_area_init(unsigned long *zones_size) //我们的系统中并没有被调用到
```

## 2.2 页访问相干

```cpp
//#define pgoff_t unsigned long
struct page {
	unsigned long flags; //用来存放页的状态，每一位代表一种状态，所以至少可以同时表示出32中不同的状态,这些状态定义在linux/page-flags.h中
	struct address_space *mapping; //指向与该页相关的address_space对象
	pgoff_t index; //在映射的虚拟空间（vma_area）内的偏移；一个文件可能只映射一部分，假设映射了1M的空间，index指的是在1M空间内的偏移，而不是在整个文件内的偏移
	atomic_t _mapcount; //被页表映射的次数，也就是说该page同时被多少个进程共享。初始值为-1，如果只被一个进程的页表映射了，该值为0. 如果该page处于伙伴系统
	unsigned long private;
};
　　::mapping  指定了页帧所在的地址空间, index是页帧在映射内部的偏移量. 地址空间是一个非常一般的概念. 例如, 可以用在向内存读取文件时. 地址空间用于将文件的内容与装载数据的内存区关联起来. mapping不仅能够保存一个指针, 而且还能包含一些额外的信息, 用于判断页是否属于未关联到地址空间的某个匿名内存区.如果mapping = 0，说明该page属于交换高速缓存页（swap cache）；当需要使用地址空间时会指定交换分区的地址空间swapper_space；如果mapping != 0，第0位bit[0] = 0，说明该page属于页缓存或文件映射，mapping指向文件的地址空间address_space；如果mapping != 0，第0位bit[0] != 0，说明该page为匿名映射，mapping指向struct anon_vma对象。通过mapping恢复anon_vma的方法：anon_vma = (struct anon_vma *)(mapping - PAGE_MAPPING_ANON)。
　　::index   是该页描述结构在地址空间radix树page_tree中的对象索引号即页号, 表示该页在vm_file中的偏移页数
　　::private 私有数据指针, 由应用场景确定其具体的含义：如果设置了PG_private标志，则private字段指向struct buffer_head；如果设置了PG_compound，则指向struct page；如果设置了PG_swapcache标志，private存储了该page在交换分区中对应的位置信息swp_entry_t；如果_mapcount = PAGE_BUDDY_MAPCOUNT_VALUE，说明该page位于伙伴系统，private存储该伙伴的阶

由page获取node和zone，老版本是page中保留指针，现在保留索引，节省内存．
static inline struct zone *page_zone(const struct page *page)
    return &NODE_DATA(page_to_nid(page))->node_zones[page_zonenum(page)];
static inline void set_page_zone(struct page *page, enum zone_type zone)
    page->flags &= ~(ZONES_MASK << ZONES_PGSHIFT);    page->flags |= (zone & ZONES_MASK) << ZONES_PGSHIFT;
static inline void set_page_node(struct page *page, unsigned long node)
    page->flags &= ~(NODES_MASK << NODES_PGSHIFT);    page->flags |= (node & NODES_MASK) << NODES_PGSHIFT;

page的flags标识主要分为4部分，其中标志位flag向高位增长, 其余位字段向低位增长，中间存在空闲位：<br>
- node 	NUMA节点号, 标识该page属于哪一个节点
- zone 	内存域标志，标识该page属于哪一个zone
- flag 	page的状态标识
	- PG_locked 	指定了页是否被锁定, 如果该比特未被置位, 说明有使用者正在操作该page, 则内核的其他部分不允许访问该页， 这可以防止内存管理出现竞态条件
	- PG_error 	如果涉及该page的I/O操作发生了错误, 则该位被设置
	- PG_referenced 	表示page刚刚被访问过
	- PG_uptodate 	表示page的数据已经与后备存储器是同步的, 即页的数据已经从块设备读取，且没有出错,数据是最新的
	- PG_dirty 	与后备存储器中的数据相比，该page的内容已经被修改. 出于性能能的考虑，页并不在每次改变后立即回写, 因此内核需要使用该标识来表明页面中的数据已经改变, 应该在稍后刷出
	- PG_lru 	表示该page处于LRU链表上， 这有助于实现页面的回收和切换. 内核使用两个最近最少使用(least recently used-LRU)链表来区别活动和不活动页. 如果页在其中一个链表中, 则该位被设置
	- PG_active 	page处于inactive LRU链表, PG_active和PG_referenced一起控制该page的活跃程度，这在内存回收时将会非常有用;当位于LRU active_list链表上的页面该位被设置, 并在页面移除时清除该位, 它标记了页面是否处于活动状态
	- PG_slab 	该page属于slab分配器
	- PG_writeback 	page中的数据正在被回写到后备存储器
	- PG_swapcache 	表示该page处于swap cache中
	- PG_reclaim 	表示该page要被回收。当PFRA决定要回收某个page后，需要设置该标志
	- PG_mappedtodisk 	表示page中的数据在后备存储器中有对应
	- PG_swapbacked 	该page的后备存储器是swap
	- PG_unevictable 	该page被锁住，不能交换，并会出现在LRU_UNEVICTABLE链表中，它包括的几种page：ramdisk或ramfs使用的页, shm_locked、mlock锁定的页
	- PG_mlocked 	该page在vma中被锁定，一般是通过系统调用mlock()锁定了一段内存
	- ...

void wait_on_page_locked(struct page *page)
	if (PageLocked(page))
		|--> wait_on_page_bit(page, PG_locked);//void wait_on_page_bit(struct page *page, int bit_nr)
			DEFINE_WAIT_BIT(wait, &page->flags, bit_nr);
			if (test_bit(bit_nr, &page->flags))
				__wait_on_bit(page_waitqueue(page), &wait, sleep_on_page, TASK_UNINTERRUPTIBLE);
void wait_on_page_writeback(struct page *page)
	if (PageWriteback(page))
		wait_on_page_bit(page, PG_writeback);
```

**q3f内存映射空间解析**<br>
```cpp
phys_addr_t arm_lowmem_limit; //0x43e00000 物理地址的上边界
void * high_memory; //0xc3e00000 high_memory = __va(arm_lowmem_limit - 1) + 1;

临时映射线性地址空间   fixmap  : 0xfff00000 - 0xfffe0000   ( 896 kB)
//#define FIXADDR_START		0xfff00000UL
//#define FIXADDR_TOP		0xfffe0000UL
//#define FIXADDR_SIZE		(FIXADDR_TOP - FIXADDR_START)
vmalloc : 0xc4000000 - 0xff000000   ( 944 MB) //当前系统实际对应物理内存映射后面8M对齐的地方//vmalloc()函数用来把不连续的物理地址空间映射到此范围内连续的线性地址空间上
//#define VMALLOC_OFFSET		(8*1024*1024) //防止访问越界．
//#define VMALLOC_START		(((unsigned long)high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1))
//#define VMALLOC_END		0xff000000UL
//mem_map：0xc06db000开始保存页表,有效15872page
	-->kernel部分，大概7Mbytes
	.bss : 0xc056b330 - 0xc06da3c0   (1469 kB)
	//(__bss_start, __bss_stop)
	.data : 0xc04dc000 - 0xc056a260   ( 569 kB)　//struct pglist_data *zone_pgdat:0xc0569724包含在data段中
	//(_sdata, _edata)
	.init : 0xc04c2000 - 0xc04da844   (  99 kB)
	//(__init_begin, __init_end)
	.text : 0xc0008000 - 0xc04c1860   (4839 kB)
	//(_text, _etext)
lowmem  : 0xc0000000 - 0xc3e00000   (  62 MB)
//#define PAGE_OFFSET 0xc0000000
//#high_memory :: 0xc3e00000
modules : 0xbf800000 - 0xc0000000   (   8 MB)
//#define MODULES_VADDR		(PAGE_OFFSET - SZ_8M)
接着需要打印：
物理内存开始：　PHYS_OFFSET

//#define PHYS_OFFSET	PLAT_PHYS_OFFSET
mach-q3f/include/mach/memory.h:14:      #define PLAT_PHYS_OFFSET	UL(0x40000000)
mach-q3f/include/mach/imap-iomap.h:10:  #define IMAP_SDRAM_BASE		0x40000000
//#define ___RBASE   phys_to_virt(IMAP_SDRAM_BASE + 0x3800000) 寄存器地址空间
//#define PHYS_PFN_OFFSET	(PHYS_OFFSET >> PAGE_SHIFT)     ::0x40000
//#define ARCH_PFN_OFFSET		PHYS_PFN_OFFSET								::0x40000

void *page_address(struct page *page);
struct page *pfn_to_page(int pfn);
struct page *virt_to_page(void *kaddr);

virt_to_page(addr)产生线性地址对应的页描述符地址。pfn_to_page(pfn)产生对应页框号的页描述符地址；page_to_pfn(page)产生对应页描述符地址的页框号
//#define __virt_to_phys(x)	((x) - PAGE_OFFSET + PHYS_OFFSET)
//#define __phys_to_virt(x)	((x) - PHYS_OFFSET + PAGE_OFFSET)
//#define __pa(x)			__virt_to_phys((unsigned long)(x))
//#define __va(x)			((void *)__phys_to_virt((unsigned long)(x)))
//#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)

//#if defined(CONFIG_FLATMEM)
//#define __pfn_to_page(pfn)	(mem_map + ((pfn) - ARCH_PFN_OFFSET))
//#define __page_to_pfn(page)	((unsigned long)((page) - mem_map) + ARCH_PFN_OFFSET)
//#define page_to_pfn __page_to_pfn
//#define pfn_to_page __pfn_to_page
//#endif

//#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
//#define virt_addr_valid(kaddr)	((unsigned long)(kaddr) >= PAGE_OFFSET && (unsigned long)(kaddr) < (unsigned long)high_memory)cgroup_init_early
//#define page_to_phys(page)      ((dma_addr_t)page_to_pfn(page) << PAGE_SHIFT)
//#define page_to_virt(page)	pfn_to_virt(page_to_pfn(page))

//启动进程的过程中需要为进程分配内存，sys_execve() --> SYSCALL_DEFINE3(execve...) --> do_execve() --> do_execve_common --> bprm_mm_init()
int bprm_mm_init(struct linux_binprm *bprm)
	struct mm_struct *mm = bprm->mm = mm_alloc();
		struct mm_struct *mm = allocate_mm(); //kmem_cache_alloc(mm_cachep, GFP_KERNEL) 从mm_cachep的slab中分配
		mm_init_cpumask(mm);
		|--> return mm_init(mm, current);//struct mm_struct *mm_init(struct mm_struct *mm, struct task_struct *p)
			|--> mm_alloc_pgd(mm);//int mm_alloc_pgd(struct mm_struct *mm)
  			 mm->pgd = pgd_alloc(mm);
			mmu_notifier_mm_init(mm);	return mm;
	err = init_new_context(current, mm);
	err = __bprm_mm_init(bprm);

pgd_t 用于全局页目录项；pte_t 用于直接页目录项

根据虚拟地址获取物理页的示例代码详见mm/memory.c中的函数　follow_page()

//添加内存条
//#define NR_BANKS	CONFIG_ARM_NR_BANKS linux-menuconfig 中配置内存条个数
struct membank {
	phys_addr_t start;	phys_addr_t size;	unsigned int highmem;
};
struct meminfo meminfo;
int __init early_mem(char *p)
	start = PHYS_OFFSET;	size  = memparse(p, &endp); //地址0x40000000
	|--> arm_add_memory(start, size);//int __init arm_add_memory(phys_addr_t start, phys_addr_t size)
		struct membank *bank = &meminfo.bank[meminfo.nr_banks]; //
		bank->start = PAGE_ALIGN(start);bank->size = size & ~(phys_addr_t)(PAGE_SIZE - 1);
		meminfo.nr_banks++;
early_param("mem", early_mem);
```

**虚拟地址访问和页表操作**<br>
**arm段页机制，图非常不错**<br>
	http://lib.csdn.net/article/operatingsystem/18924
	https://www.aliyun.com/jiaocheng/155843.html
	https://www.cnblogs.com/arnoldlu/p/8087022.html
	https://blog.csdn.net/hnzziafyz/article/details/52201793
	https://blog.csdn.net/qq_25865859/article/details/59105701
	https://blog.csdn.net/stillvxx/article/details/41122999
	https://blog.csdn.net/gatieme/article/details/52403013

```cpp
我们的系统使用的是二级页表，对应的头文件是：　pgtable-2level.h 使用PGD和PTE
//#define PGDIR_SHIFT		21	//确定页全局页目录项能映射的区域大小的位数
//#define PGDIR_SIZE		(1UL << PGDIR_SHIFT) //用于计算页全局目录中一个单独表项所能映射区域的大小
//#define PGDIR_MASK		(~(PGDIR_SIZE-1)) //用于屏蔽Offset, Table，Middle Air及Upper Air的所有位

//#define PMD_SHIFT		21
//#define PMD_SIZE		(1UL << PMD_SHIFT)
//#define PMD_MASK		(~(PMD_SIZE-1))

//1.pgd_offset 	根据当前虚拟地址和当前进程的mm_struct获取pgd项
//#define pgd_index(addr)		((addr) >> PGDIR_SHIFT)
//#define pgd_offset(mm, addr)	((mm)->pgd + pgd_index(addr))
//2.pud_offset 	在两级或三级分页系统中，该宏产生 pgd ，即一个页全局目录项的地址
static inline pud_t * pud_offset(pgd_t * pgd, unsigned long address)
	return (pud_t *)pgd;
//3.pmd_offset 	根据通过pgd_offset获取的pgd 项和虚拟地址，获取相关的pmd项(即pte表的起始地址)
static inline pmd_t * pmd_offset(pud_t * pud, unsigned long address)
	return (pmd_t *)pud;
//4.pte_offset 	根据通过pmd_offset获取的pmd项和虚拟地址，获取相关的pte项(即物理页的起始地址)

//统一的内存分配函数入口
struct page *__alloc_pages(gfp_t gfp_mask, unsigned int order, struct zonelist *zonelist)
//根据虚拟地址获取物理页
struct page *follow_page_mask(struct vm_area_struct *vma,　unsigned long address, unsigned int flags, unsigned int *page_mask)
```

**创建新进程内存映射过程**<br>
首先要为新进程创建一个新的页面目录PGD到task_struct.(struct mm_struct)mm->pgd，并从内核的页面目录swapper_pg_dir中复制内核区间页面目录项至新建进程页面目录PGD的相应位置，具体过程如下：do_fork() –> copy_mm() –> mm_init() –> pgd_alloc() –> set_pgd_fast() –> get_pgd_slow() –> memcpy(&PGD + USER_PTRS_PER_PGD, swapper_pg_dir + USER_PTRS_PER_PGD, (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t))；这样一来，每个进程的页面目录就分成了两部分，第一部分为“用户空间”，用来映射其整个进程空间（0x0000 0000－0xBFFF FFFF）即3G字节的虚拟地址；第二部分为“系统空间”，用来映射（0xC000 0000－0xFFFF FFFF）1G字节的虚拟地址。可以看出Linux系统中每个进程的页面目录的第二部分是相同的，所以从进程的角度来看，每个进程有4G字节的虚拟空间

## 2.6 内核初始化阶段内存管理



memblock_add()函数只被调用一次,参数(base:40000000 size:3e00000),在arm_memblock_init()函数中.

## 2.6 伙伴系统
**伙伴系统文档**<br>
	https://blog.csdn.net/vanbreaker/article/details/7605367

有种场景比较重要,如果当前迁移类型中找不到刚好合适的,而必须切分上一级的area,这时候,寻找fallbacks中,还是切分? 有时候,分的区域太多,也不好,导致更多的外部碎片.

CMA 好像是

```cpp
struct page *get_page_from_freelist(gfp_t gfp_mask, nodemask_t *nodemask, unsigned int order,	struct zonelist *zonelist, int high_zoneidx, int alloc_flags, struct zone *preferred_zone, int migratetype)

bool __zone_watermark_ok(struct zone *z, int order, unsigned long mark, int classzone_idx, int alloc_flags, long free_pages)

struct page *__alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order, struct zonelist *zonelist, nodemask_t *nodemask)
	enum zone_type high_zoneidx = gfp_zone(gfp_mask);//根据gfp_mask确定分配页所处的管理区
	int migratetype = allocflags_to_migratetype(gfp_mask);//根据gfp_mask得到迁移类分配页的型
	int alloc_flags = ALLOC_WMARK_LOW|ALLOC_CPUSET;
	if (allocflags_to_migratetype(gfp_mask) == MIGRATE_MOVABLE)		alloc_flags |= ALLOC_CMA;
  page = get_page_from_freelist(gfp_mask|__GFP_HARDWALL, nodemask, order,	zonelist, high_zoneidx, alloc_flags,	preferred_zone, migratetype);  //从zonelist中找到zone_idx与high_zoneidx相同的管理区，也就是之前认定的管理区
	if (unlikely(!page))
		page = __alloc_pages_slowpath(gfp_mask, order,zonelist, high_zoneidx, nodemask,preferred_zone, migratetype);//第一次分配失败的话则会用通过一条低速路径来进行第二次分配，包括唤醒页换出守护进程等等

void __free_pages(struct page *page, unsigned int order)

struct page *buffered_rmqueue(struct zone *preferred_zone, struct zone *zone, int order, gfp_t gfp_flags, int migratetype)

```

## 2.7 CMA定义
arm_dma_alloc() 函数,如果从dev的cma中分配,应该不需要伙伴机制的加入,单纯的使用bitmap机制管理就好了;如果从全局的cma中分配,需要伙伴机制的配合管理.
```cpp
ret = dma_declare_contiguous(&da8xx_dsp.dev, rproc_size, rproc_base, 0); 为设备分配cma内存
void __init arm_memblock_init(struct meminfo *mi, struct machine_desc *mdesc)
	|--> dma_contiguous_reserve(min(arm_dma_limit, arm_lowmem_limit));//void __init dma_contiguous_reserve(phys_addr_t limit)
		dma_declare_contiguous(NULL, selected_size, 0, limit);

```
可以查询使能 CONFIG_CMA_DEBUG_BITMAP 来打印cma的相关调试内容.

dma_contiguous_default_area

是否CMA机制中可以增加内存碎片合并的算法,这样就算在别的块找不到可以move的目标地点,也可以在CMA内部节省出来.

**命令行中传递大小**<br>

```cpp
struct cma *dma_contiguous_default_area;

static inline struct cma *dev_get_cma_area(struct device *dev)
	if (dev && dev->cma_area)		return dev->cma_area;
	return dma_contiguous_default_area;
static inline void dev_set_cma_area(struct device *dev, struct cma *cma)
	if (dev)		dev->cma_area = cma;
	if (!dev && !dma_contiguous_default_area)		dma_contiguous_default_area = cma;

//#define CMA_SIZE_MBYTES 	CONFIG_CMA_SIZE_MBYTES //CONFIG_CMA_SIZE_MBYTES为.config中配置,当前为32M
static const phys_addr_t size_bytes = CMA_SIZE_MBYTES * SZ_1M;
static phys_addr_t size_cmdline = -1;
int __init early_cma(char *p)
	size_cmdline = memparse(p, &p);
early_param("cma", early_cma);

void __init dma_contiguous_reserve(phys_addr_t limit)//在所有的module init之前调用,预留的系统cma.
	if (size_cmdline != -1) selected_size = size_cmdline;
	else selected_size = size_bytes;
	|--> dma_declare_contiguous(NULL, selected_size, 0, limit);//int __init dma_declare_contiguous(struct device *dev, phys_addr_t size, phys_addr_t base, phys_addr_t limit)
		struct cma_reserved *r = &cma_reserved[cma_reserved_count];
		alignment = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
		base = __memblock_alloc_base(size, alignment, limit); //申请一块32M连续空间,保持8M对齐.
		r->start = base;	r->size = size;	r->dev = dev;
		cma_reserved_count++;
		dma_contiguous_early_fixup(base, size);//没看出来有什么特别的作用.

//#define MAX_CMA_AREAS	(1 + CONFIG_CMA_AREAS)//1+7 = 8
static struct cma_reserved {
	phys_addr_t start;
	unsigned long size;
	struct device *dev;
} cma_reserved[MAX_CMA_AREAS] __initdata;
static unsigned cma_reserved_count __initdata;

static int __init cma_init_reserved_areas(void)
	struct cma_reserved *r = cma_reserved;  unsigned i = cma_reserved_count;
	for (; i; --i, ++r)
		|--> cma = cma_create_area(PFN_DOWN(r->start), r->size >> PAGE_SHIFT);//struct cma *cma_create_area(unsigned long base_pfn, unsigned long count)
			int bitmap_size = BITS_TO_LONGS(count) * sizeof(long);
			cma = kmalloc(sizeof *cma, GFP_KERNEL);cma->base_pfn = base_pfn;cma->count = count;
			cma->bitmap = kzalloc(bitmap_size, GFP_KERNEL);
			ret = cma_activate_area(base_pfn, count);
		dev_set_cma_area(r->dev, cma);
core_initcall(cma_init_reserved_areas);
```

## 2.8 slab分配器

**创建per_cpu**<br>
```cpp
struct per_cpu_pages {
	int count;		/* number of pages in the list，记录了与该列表相关的页的数目　*/
	int high;		/* high watermark, emptying needed，是一个水印. 如果count的值超过了high, 则表明列表中的页太多了 */
	int batch;		/* chunk size for buddy add/remove，如果可能, CPU的高速缓存不是用单个页来填充的, 而是欧诺个多个页组成的块, batch作为每次添加/删除页时多个页组成的块大小的一个参考值 */
	struct list_head lists[MIGRATE_PCPTYPES];/* Lists of pages, one per migrate type stored on the pcp-lists，一个双链表, 保存了当前CPU的冷页或热页, 可使用内核的标准方法处理 */
};
struct per_cpu_pageset {
	struct per_cpu_pages pcp; //这维护了链表中目前已有的一系列页面, 高极值和低极值决定了何时填充该集合或者释放一批页面, 变量决定了一个块中应该分配多少个页面, 并最后决定在页面前的实际链表中分配多少各页面
};
//zone结构体中，struct per_cpu_pageset __percpu *pageset;
static DEFINE_PER_CPU(struct per_cpu_pageset, boot_pageset);
static __meminit void zone_pcp_init(struct zone *zone)
  zone->pageset = &boot_pageset;

```

在内核中只有一个子系统会积极的尝试为任何对象维护per-cpu上的list链表, 这个子系统就是slab分配器. 提供小内存块不是slab分配器的唯一任务. 由于结构上的特点. 它也用作一个缓存. 主要针对经常分配并释放的对象. 通过建立slab缓存, 内核能够储备一些对象, 供后续使用, 即使在初始化状态, 也是如此. 建立和使用缓存的任务不是特别困难. 必须首先用kmem_cache_create建立一个适当的缓存, 接下来即可使用kmem_cache_alloc和kmem_cache_free分配和释放其中包含的对象。slab分配器负责完成与伙伴系统的交互，来分配所需的页. 所有活动缓存的列表保存在/proc/slabinfo

```cpp
int __kmem_cache_create (struct kmem_cache *cachep, unsigned long flags)
void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
void kmem_cache_free(struct kmem_cache *cachep, void *objp)
void kmem_cache_destroy(struct kmem_cache *s)

void *vmalloc(unsigned long size)
void vfree(const void *addr)
```

```cpp
pg_data_t 中　struct zone node_zones[MAX_NR_ZONES];//对应一个内存node, 这个node包含多个zone
enum node_states, 我们系统，node状态应该是  N_NORMAL_MEMORY
```
## 3 重要数据结构
**linux内存管理浅析**<br>
  zone和vma的部分，值得参考
  https://blog.csdn.net/ctthuangcheng/article/details/8915146

**基本概念描述的很好**<br>
  https://blog.csdn.net/goodluckwhh/article/details/9970845
  https://blog.csdn.net/goodluckwhh/article/details/9989695

https://blog.csdn.net/Tommy_wxie/article/details/17122923

**内存管理和进程管理系列文档**<br>
  https://www.cnblogs.com/arnoldlu/p/8060121.html
  http://www.cnblogs.com/arnoldlu/p/8467142.html

**用户空间动态内存管理**<br>
     malloc是libc的库函数，用户程序一般通过它（或类似函数）来分配内存空间。
     libc对内存的分配有两种途径，一是调整堆的大小，二是mmap一个新的虚拟内存区域（堆也是一个vma）。
     在内核中，堆是一个一端固定、一端可伸缩的vma（图：左中）。可伸缩的一端通过系统调用brk来调整。libc管理着堆的空间，用户调用malloc分配内存时，libc尽量从现有的堆中去分配。如果堆空间不够，则通过brk增大堆空间。
     当用户将已分配的空间free时，libc可能会通过brk减小堆空间。但是堆空间增大容易减小却难，考虑这样一种情况，用户空间连续分配了10块内存，前9块已经free。这时，未free的第10块哪怕只有1字节大，libc也不能够去减小堆的大小。因为堆只有一端可伸缩，并且中间不能掏空。而第10块内存就死死地占据着堆可伸缩的那一端，堆的大小没法减小，相关资源也没法归还内核。
     当用户malloc一块很大的内存时，libc会通过mmap系统调用映射一个新的vma。因为对于堆的大小调整和空间管理还是比较麻烦的，重新建一个vma会更方便（上面提到的free的问题也是原因之一）。
     那么为什么不总是在malloc的时候去mmap一个新的vma呢？第一，对于小空间的分配与回收，被libc管理的堆空间已经能够满足需要，不必每次都去进行系统调用。并且vma是以page为单位的，最小就是分配一个页；第二，太多的vma会降低系统性能。缺页异常、vma的新建与销毁、堆空间的大小调整、等等情况下，都需要对vma进行操作，需要在当前进程的所有vma中找到需要被操作的那个（或那些）vma。vma数目太多，必然导致性能下降。（在进程的vma较少时，内核采用链表来管理vma；vma较多时，改用红黑树来管理。）

**用户的栈**<br>


     与堆一样，栈也是一个vma（图：左中），这个vma是一端固定、一端可伸（注意，不能缩）的。这个vma比较特殊，没有类似brk的系统调用让这个vma伸展，它是自动伸展的。
     当用户访问的虚拟地址越过这个vma时，内核会在处理缺页异常的时候将自动将这个vma增大。内核会检查当时的栈寄存器（如：ESP），访问的虚拟地址不能超过ESP加n（n为CPU压栈指令一次性压栈的最大字节数）。也就是说，内核是以ESP为基准来检查访问是否越界。
     但是，ESP的值是可以由用户态程序自由读写的，用户程序如果调整ESP，将栈划得很大很大怎么办呢？内核中有一套关于进程限制的配置，其中就有栈大小的配置，栈只能这么大，再大就出错。
     对于一个进程来说，栈一般是可以被伸展得比较大（如：8MB）。然而对于线程呢？
     首先线程的栈是怎么回事？前面说过，线程的mm是共享其父进程的。虽然栈是mm中的一个vma，但是线程不能与其父进程共用这个vma（两个运行实体显然不用共用一个栈）。于是，在线程创建时，线程库通过mmap新建了一个vma，以此作为线程的栈（大于一般为：2M）。
     可见，线程的栈在某种意义上并不是真正栈，它是一个固定的区域，并且容量很有限。

内核空间分为直接映射区和高端映射区，其中高端映射区又分为动态映射区、KMAP区和固定映射区，如下表所示。
直接映射区        动态映射区        KMAP区        固定映射区
896（max）        120（min）        4M        4M
高端映射区是指内核空间的最高128M的虚拟地址空间；高端内存区是指系统中物理地址大于896MB的所有物理内存

**kmap调用**<br>
kmap 为系统中的任何页返回一个内核虚拟地址. 对于低端内存页,它只返回页的逻辑地址; 对于高端内存页, kmap在内核地址空间的一个专用部分中创建一个特殊的映射. 这样的映射数目是有限, 因此最好不要持有过长的时间. 使用 kmap 创建的映射应当使用 kunmap 来释放; kmap 调用维护一个计数器, 因此若2个或多个函数都在同一个页上调用kmap也是允许的. kmap_atomic是kmap的一种高性能版本. 每个体系都给原子kmap维护一个slot(专用的页表入口); kmap_atomic的调用者必须在type参数中告知系统使用哪个slot. 对驱动有意义的slot是 KM_USER0 和 KM_USER1 (在用户空间的直接运行的代码), 以及 KM_IRQ0 和 KM_IRQ1(对于中断处理).

```cpp
//#include <linux/highmem.h>
void *kmap(struct page *page);
void kunmap(struct page *page);

//#include <linux/highmem.h>
//#include <asm/kmap_types.h>
void *kmap_atomic(struct page *page, enum km_type type);
void kunmap_atomic(void *addr, enum km_type type);
```


**内存管理基本概念**
下面宏可以禁止CMA的共享么？
　　CONFIG_CMA_MIGRATE_OFF

打印  VMALLOC_RESERVE　　VMALLOC_START  VMALLOC_END VMALLOC_OFFSET

内核线性地址空间部分
1. 从PAGE_OFFSET（通常定义为3G）开始，
2. 为了将内核装入内存，从PAGE_OFFSET开始8M线性地址用来映射内核所在的物理内存地址（也可以说是内核所在虚拟地址是从PAGE_OFFSET开始的）；
3. 接下来是mem_map数组，
4. vmalloc()使用从VMALLOC_START到VMALLOC_END

32位的地址空间被分为3部分:
1. 页目录表Directory 	 最高10位
2. 页中间表Table 	     中间10位
3. 页内偏移 	           最低12位
　即页表被划分为页目录表Directory和页中间表Tabl两个部分，内核通常只为进程实际使用的那些虚拟内存区请求页表来减少内存使用量.


Linux内核通过四级页表将虚拟内存空间分为5个部分(4个页表项用于选择页, 1个索引用来表示页内的偏移). 各个体系结构不仅地址长度不同, 而且地址字拆分的方式也不一定相同. 因此内核使用了宏用于将地址分解为各个分量.

![linux页表](pic_dir/linux页表.png)


c20设备，通常只支持ZONE_NORMAL,ZONE_MOVABLE两种ZONE，因为物理内存不超过1G，不需要使用高端内存，硬件上不需要DMA区域

```cpp
//在UMA结构的机器中, 只有一个node结点即contig_page_data
extern bootmem_data_t bootmem_node_data[];
struct pglist_data __refdata contig_page_data = {
	.bdata = &bootmem_node_data[0]
};

//#define NODE_DATA(nid)		(&contig_page_data)
//#define NODE_MEM_MAP(nid)	mem_map

struct bootmem_data;
typedef struct pglist_data {
	struct zone node_zones[MAX_NR_ZONES];//每个Node划分为不同的zone，分别为ZONE_DMA，ZONE_NORMAL，ZONE_HIGHMEM
  int nr_zones;//当前节点中不同内存域zone的数量，1到3个之间

	struct zonelist node_zonelists[MAX_ZONELISTS];
//#ifdef CONFIG_FLAT_NODE_MEM_MAP	/* means !SPARSEMEM */
	struct page *node_mem_map; //node中的第一个page，它可以指向mem_map中的任何一个page，指向page实例数组的指针，用于描述该节点所拥有的的物理内存页，它包含了该页面所有的内存页，被放置在全局mem_map数组中
//#ifdef CONFIG_MEMCG
	struct page_cgroup *node_page_cgroup;
//#endif
//#endif
//#ifndef CONFIG_NO_BOOTMEM
	struct bootmem_data *bdata;//这个仅用于引导程序boot 的内存分配，内存在启动时，也需要使用内存，在这里内存使用了自举内存分配器，这里bdata是指向内存自举分配器的数据结构的实例
//#endif
	unsigned long node_start_pfn;//pfn是page frame number的缩写。这个成员是用于表示node中的开始那个page在物理内存中的位置的。是当前NUMA节点的第一个页帧的编号，系统中所有的页帧是依次进行编号的，这个字段代表的是当前节点的页帧的起始值，对于UMA系统，只有一个节点，所以该值总是0
	unsigned long node_present_pages; //node中的真正可以使用的page数量
	unsigned long node_spanned_pages; //该节点以页帧为单位的总长度，这个不等于前面的node_present_pages,因为这里面包含空洞内存
	int node_id;
	nodemask_t reclaim_nodes;	/* Nodes allowed to reclaim from */
	wait_queue_head_t kswapd_wait; //node的等待队列，交换守护列队进程的等待列表
	wait_queue_head_t pfmemalloc_wait;
	struct task_struct *kswapd;	/* Protected by lock_memory_hotplug() */
	int kswapd_max_order; //需要释放的区域的长度，以页阶为单位
	enum zone_type classzone_idx;
} pg_data_t;

EXPORT_SYMBOL(contig_page_data);

//每个物理的页由一个struct page的数据结构对象来描述。页的数据结构对象都保存在mem_map全局数组中，该数组通常被存放在ZONE_NORMAL的首部，或者就在小内存系统中为装入内核映像而预留的区域之后。从载入内核的低地址内存区域的后面内存区域，也就是ZONE_NORMAL开始的地方的内存的页的数据结构对象，都保存在这个全局数组中。
unsigned long max_mapnr;
struct page *mem_map;

```

## 4 用户空间和内核空间

**为什么要使用copy_from_user**<br>
原理上，内核态是可以直接访问用户态的虚拟地址空间的，所以如果需要在内核态获取用户态地址空间的数据的话，理论上应该是可以直接访问的，但为什么还需要使用copy_from_user接口呢？
因为：直接访问的话，无法保证被访问的用户态虚拟地址是否有对应的页表项，即无法保证该虚拟地址已经分配了相应的物理内存，如果此时没有对应的页表项，那么此时将产生page fault，导致流程混乱，原则上如果没有页表项(即没有物理内存时)，是不应该对齐进行操作的。 所以直接操作有比较大的风险，而copy_from_user本质上也只是做了相关判断和校验，保证不会出现相关异常而已。 page fault是硬件提供的特性，本质为一种“异常”(应该了解“异常”和“中断”的概念吧~)，实际由硬件直接触发，触发的条件为：CPU访问某线性地址时，如果没有找到其对应的页表项，则由硬件直接触发page fault。发生缺页的上下文也有可能位于内核态，但发生缺页的地址只能位于用户态地址空间或者内核态的vmalloc区，否则就会出现oops。对内核来说(以32位为例)，线性映射(直接通过TASK_SIZE偏移映射)的内存(对32位系统来说，就是前896M，即Zone_Normal)相应的页表在内核初始化时就已经建立，所以这部分内存对应的虚拟地址不可能产生page fault。


**内核页表**<br>

内核页表，在内核中其实就是一段内存，存放在主内核页全局目录 init_mm.pgd(swapper_pg_dir) 中，硬件并不直接使用。 进程页表，每个进程自己的页表，放在进程自身的页目录 task_struct.mm->pgd 中。在保护模式下，从硬件角度看，其运行的基本对象为“进程”(或线程)，而寻址则依赖于“进程页表”，在进程调度而进行上下文切换时，会进行页表的切换。从这个角度看，其实是完全没有用到“内核页表”的，那么“内核页表”有什么用呢？跟“进程页表”有什么关系呢？

1、内核页表中的内容为所有进程共享，每个进程都有自己的“进程页表”，“进程页表”中映射的线性地址包括两部分：
用户态
内核态
其中，内核态地址对应的相关页表项，对于所有进程来说都是相同的(因为内核空间对所有进程来说都是共享的)，而这部分页表内容其实就来源于“内核页表”，即每个进程的“进程页表”中内核态地址相关的页表项都是“内核页表”的一个拷贝。
2、“内核页表”由内核自己维护并更新，在vmalloc区发生page fault时，将“内核页表”同步到“进程页表”中。以32位系统为例，内核页表主要包含两部分：
线性映射区
vmalloc区
其中，线性映射区即通过TASK_SIZE偏移进行映射的区域，对32系统来说就是0-896M这部分区域，映射对应的虚拟地址区域为TASK_SIZE-TASK_SIZE+896M。这部分区域在内核初始化时就已经完成映射，并创建好相应的页表，即这部分虚拟内存区域不会发生page fault。
vmalloc区，为896M-896M+128M，这部分区域用于映射高端内存，有三种映射方式：vmalloc、固定、临时，这里就不像述了。。
以vmalloc为例(最常使用)，这部分区域对应的线性地址在内核使用vmalloc分配内存时，其实就已经分配了相应的物理内存，并做了相应的映射，建立了相应的页表项，但相关页表项仅写入了“内核页表”，并没有实时更新到“进程页表中”，内核在这里使用了“延迟更新”的策略，将“进程页表”真正更新推迟到第一次访问相关线性地址，发生page fault时，此时在page fault的处理流程中进行“进程页表”的更新：


缺页地址位于内核空间。并不代表异常发生于内核空间，有可能是用户态访问了内核空间的地址，也就是说访问了vmalloc区的地址，这时候需要将主内核页表向当前进程的内核页表同步

### 4.2
### 4.1 进程栈 线程栈 中断栈 内核栈
	https://blog.csdn.net/yangkuanqaz85988/article/details/52403726

```cpp
/* 虚拟内存管理的操作函数 - 对VMA做打开、关闭和
 * 取消映射操作, (需要保持文件在磁盘上的同步更新等),
 * 当一个缺页或交换页异常发生时，这些函数指针指向实际的函数调用。 */
struct vm_operations_struct {
    void (*open)(struct vm_area_struct * area);
   /* 被内核调用以实现 VMA 的子系统来初始化一个VMA.当对 VMA 产生一个新的引用时( 如fork进程时)，则调用这个方法
    * 唯一的例外是当该 VMA 第一次被 mmap 创建时;在这个情况下, 则需要调用驱动的 mmap 方法.  */

    void (*close)(struct vm_area_struct * area);
   /* 当一个VMA被销毁时, 内核调用此操作.注意：由于没有相应的 VMA使用计数; VMA只被每个使用它的进程打开和关闭一次. */

    int (*fault)(struct vm_area_struct *vma, struct vm_fault *vmf);
   /* 重要的函数调用，当一个进程试图存取使用一个有效的、但当前不在内存中 VMA 的页,自动触发的缺页异常处理程序就调用该方法。
    * 将对应的数据读取到一个映射在用户地址空间的物理内存页中（替代原有的nopage）*/


    /* 通知一个之前只读的页将要变为可写，如果返回错误，将会引发SIGBUS（总线错误） */
    int (*page_mkwrite)(struct vm_area_struct *vma, struct vm_fault *vmf);

    /* 当get_user_pages() 失败，被access_process_vm调用，一般用于特殊的VMA（可以在硬件和内存间交换的VMA）*/
    int (*access)(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write);
		/* called by sys_remap_file_pages() to populate non-linear mapping */
		int (*remap_pages)(struct vm_area_struct *vma, unsigned long addr,
	   		 unsigned long size, pgoff_t pgoff);
};

struct vm_area_struct {
	 struct mm_struct * vm_mm;/* 所属的进程虚拟地址空间的结构体指针，就是我们上面看到的结构体 */
	 /* 被该VMA 覆盖的虚拟地址范围. 也就是在 /proc/*/maps中出现的头 2 个字段 */
   unsigned long vm_start;        /* vm_mm内的起始地址 */
   unsigned long vm_end;          /* 在vm_mm内的结束地址之后的第一个字节的地址 */
   struct vm_area_struct *vm_next, *vm_prev;		/* 用来链接进程的VMA结构体的指针, 按地址排序 */
	 struct rb_node vm_rb; 	 unsigned long rb_subtree_gap;
	 struct mm_struct *vm_mm;	/* The address space we belong to. */
	 pgprot_t vm_page_prot;		/* Access permissions of this VMA. */
	 unsigned long vm_flags;		/* Flags, see mm.h. */
	 union {
		 struct {
			 struct list_head list;
			 void *parent;
			 struct vm_area_struct *head;
		 } vm_set;
		 struct raw_prio_tree_node prio_tree_node;
	 } shared;
	 //上面union shared 则与文件映射页面使用的优先级搜索树相关；union shared 中的 prio_tree_node 结构用于表示优先级搜索树的一个节点；在某些情况下，比如不同的进程的内存区域可能映射到了同一个文件的相同部分，也就是说这些内存区域具有相同的 （radix,size,heap）值，这个时候 Linux 就会在树上相应的节点（树上原来那个具有相同 （radix,size,heap） 值的内存区域）上接一个双向链表用来存放这些内存区域，这个链表用 vm_set.list 来表示；树上那个节点指向的链表中的第一个节点是表头，用 vm_set.head 表示；vm_set.parent 用于表示是否是树结点
	 //与 匿名页面的双向链表相关的字段是 anon_vma_node 和 anon_vma；字段 anon_vma 指向 anon_vma 表；字段 anon_vma_node 将映射该页面的所有虚拟内存区域链接起来
	 struct list_head anon_vma_node;
	 struct anon_vma *anon_vma;
	 const struct vm_operations_struct *vm_ops;//ops回调函数
	 unsigned long vm_pgoff;
	 struct file * vm_file;		/* File we map to (can be NULL). */
	 void * vm_private_data;		/* was vm_pte (shared mem) */
 };

struct mm_struct {
	struct vm_area_struct *mmap; /* 内存区域链表 */
	struct rb_root mm_rb; 			 /* VMA 形成的红黑树 */
	struct vm_area_struct * mmap_cache;    /* 指向最近找到的虚拟区间 */
	unsigned long (*get_unmapped_area) (struct file *filp, unsigned long addr, unsigned long len, unsigned long pgoff, unsigned long flags);
  void (*unmap_area) (struct mm_struct *mm, unsigned long addr);
	unsigned long mmap_base;        /* mmap区域的基地址 */
  unsigned long task_size;        /* 进程虚拟地址空间的大小 */
	unsigned long cached_hole_size;     /* if non-zero, the largest hole below free_area_cache */
	unsigned long free_area_cache;        /* first hole of size cached_hole_size or larger */
	pgd_t * pgd;                   /* 指向进程的页目录 */
	atomic_t mm_users;             /* 用户空间中的有多少用户? */
	atomic_t mm_count;             /* 本数据结构的引用计数 (users count as 1) */
	int map_count;                 /* 虚拟内存区（VMA）的计数 */

	spinlock_t page_table_lock;        /* 页表和计数器的保护自旋锁 */
	struct rw_semaphore mmap_sem;
	struct list_head mmlist; 		 /* 所有 mm_struct 形成的链表 */
	...
	unsigned long total_vm; 		 /* 全部页面数目 */
	unsigned long locked_vm; 		 /* 上锁的页面数据 */
	unsigned long pinned_vm; 		 /* Refcount permanently increased */
	unsigned long shared_vm; 	 	 /* 共享页面数目 Shared pages (files) */
	unsigned long exec_vm; 			 /* 可执行页面数目 VM_EXEC & ~VM_WRITE */
	unsigned long stack_vm; 		 /* 栈区页面数目 VM_GROWSUP/DOWN */
	unsigned long def_flags;
	unsigned long start_code, end_code, start_data, end_data; /* 代码段、数据段 起始地址和结束地址 */
	unsigned long start_brk, brk, start_stack; /* 栈区 的起始地址，堆区 起始地址和结束地址 */
	unsigned long arg_start, arg_end, env_start, env_end; /* 命令行参数 和 环境变量的 起始地址和结束地址 */
	... /* Architecture-specific MM context */
	mm_context_t context; 		   /* 体系结构特殊数据 */
	/* Must use atomic bitops to access the bits */
	unsigned long flags; 				 /* 状态标志位 */
	...
	/* Coredumping and NUMA and HugePage 相关结构体 */
};

struct thread_info {
    unsigned long        flags;        /* low level flags */
    int            preempt_count;    /* 0 => preemptable, <0 => bug */
    mm_segment_t        addr_limit;    /* address limit */
    struct task_struct    *task;        /* main task structure */
    struct exec_domain    *exec_domain;    /* execution domain */
    __u32            cpu;        /* cpu */
    __u32            cpu_domain;    /* cpu domain */
    struct cpu_context_save    cpu_context;    /* cpu context */
    __u32            syscall;    /* syscall number */
    __u8            used_cp[16];    /* thread used copro */
    unsigned long        tp_value;
    struct crunch_state    crunchstate;
    union fp_state        fpstate __attribute__((aligned(8)));
    union vfp_state        vfpstate;
    unsigned long        thumbee_state;    /* ThumbEE Handler Base register */
    struct restart_block    restart_block;
};

//#define THREAD_SIZE		8192
union thread_union {
    struct thread_info thread_info;
    unsigned long stack[THREAD_SIZE/sizeof(long)];
};
//内核栈被放在特殊的段中：__(".data.init_task")))
union thread_union init_thread_union __attribute__((__section__(".data.init_task"))) = { INIT_THREAD_INFO(init_task) };
ENTRY(stack_start)
    .long init_thread_union+THREAD_SIZE
    .long __BOOT_DS
默认跟中断栈共享，可以通过内核配置项修改。它属于进程，即每个进程都有自己的内核栈

```

**内核栈的产生**<br>
在进程被创建的时候，fork族的系统调用中会分别为内核栈和struct task_struct分配空间，调用过程是：fork族的系统调用--->do_fork--->copy_process--->dup_task_struct
在dup_task_struct函数中:

```cpp
static struct task_struct *dup_task_struct(struct task_struct *orig)
	struct task_struct *tsk = alloc_task_struct_node(node); //使用内核的slab分配器去为所要创建的进程分配struct task_struct的空间
	struct thread_info *ti = alloc_thread_info_node(tsk, node);//使用内核的伙伴系统去为所要创建的进程分配内核栈（union thread_union ）空间
	err = arch_dup_task_struct(tsk, orig);//
	tsk->stack = ti;//关联了struct task_struct和内核栈
	|--> setup_thread_stack(tsk, orig);//void setup_thread_stack(struct task_struct *p, struct task_struct *org) 关联了内核栈和struct task_struct
		//#define task_thread_info(task)	((struct thread_info *)(task)->stack)
		//#define task_stack_page(task)	((task)->stack)
		*task_thread_info(p) = *task_thread_info(org);
		task_thread_info(p)->task = p;
	...

static inline struct thread_info *current_thread_info(void)
{
	register unsigned long sp asm ("sp");
	return (struct thread_info *)(sp & ~(THREAD_SIZE - 1));
}
//#define get_current() (current_thread_info()->task)
//#define current get_current()
从上面定义就清楚 current 指针是如何实现的了；另外，由于栈空间的限制，在编写的驱动（特别是被系统调用使用的底层函数）中要注意避免对栈空间消耗较大的代码，比如递归算法、局部自动变量定义的大小等等
```


![进程内存空间结构](pic_dir/进程内存空间结构.png)

**进程栈的动态增长实现**<br>
进程在运行的过程中，通过不断向栈区压入数据，当超出栈区容量时，就会耗尽栈所对应的内存区域，这将触发一个 缺页异常 (page fault)。通过异常陷入内核态后，异常会被内核的 expand_stack() 函数处理，进而调用 acct_stack_growth() 来检查是否还有合适的地方用于栈的增长。如果栈的大小低于 RLIMIT_STACK（通常为8MB），那么一般情况下栈会被加长，程序继续执行，感觉不到发生了什么事情，这是一种将栈扩展到所需大小的常规机制。然而，如果达到了最大栈空间的大小，就会发生 栈溢出（stack overflow），进程将会收到内核发出的 段错误（segmentation fault） 信号。动态栈增长是唯一一种访问未映射内存区域而被允许的情形，其他任何对未映射内存区域的访问都会触发页错误，从而导致段错误。一些被映射的区域是只读的，因此企图写这些区域也会导致段错误。

### 4.2 重要函数解析

 整个系统的内存由一个名为node_data 的struct pglist_data（page_data_t） 指针数组来管理。分析可以开始于此．

```cpp
unsigned long vm_brk(unsigned long addr, unsigned long len)
unsigned long do_mmap_pgoff(struct file *file, unsigned long addr,unsigned long len, unsigned long prot, unsigned long flags, unsigned long pgoff,	unsigned long *populate)

```

## 5 dma直接相关
### 5.1 SMP内存访问一致性
fr操作，spin_lock_t变量在CMA内存中分配的时候，出现crash问题，这是因为spin_lock_t内存必须要开启cache
	https://blog.csdn.net/juS3Ve/article/details/81784688?utm_source=blogxgwz10

使用ARM V7之后的LDREX和STREX指令来实现spin lock和atomic 函数，这篇文章接着探讨ARM架构和总线协议如何来支持的。作为一个爱问为什么的工程师，一定会想到LDXR/ STXR和一般的LDR/STR有什么区别。这个区别就在于LDXR除了向memory发起load请求外，还会记录该memory所在地址的状态（一般ARM处理器在同一个cache line大小，也就是64 byte的地址范围内共用一个状态），那就是Open和Exclusive。我们可以认为一个叫做exclusive monitor的模块来记录。根据CPU访问内存地址的属性（在页表里面定义），这个组件可能在处理器 L1 memory system, 处理器cluster level, 或者总线，DDR controller上。

![linux内存管理SMP一致性访问](pic_dir/linux内存管理smp一致性访问.png)

实例说明：
- CPU1发起了一个LDXR的读操作，记录当前的状态为Exclusive
- CPU2发起了一个LDXR的读操作，当前的状态为Exclusive，保持不变
- CPU2发起了一个STXR的写操作，状态从Exclusive变成Open，同时数据回写到DDR
- CPU1发起了一个STXR的写操作，因为当前的exclusive monitor状态为Open，写失败（假如程序这时用STR操作来写，写会成功，但是这个不是原子操作函数的本意，属于编程错误）

假如有多个CPU，同时对一个处于Exclusive的memory region来进行写，CPU有内部逻辑来保证串行化。Monitor的状态除了STXR会清掉，从Exclusive变成Open之外，还有其他因素也可以导致monitor的状态被清掉，所以软件在实现spinlock的时候，一般会用一个loop循环来实现，所谓“spin”。
Exclusive monitor实现所处的位置: 根据LDXR/STXR 访问的memory的属性，需要的monitor可以在CPU内部，总线，也可以DDR controller(例如ARM DMC-400 [2]在每个memory interface 支持8个 exclusive access monitors）。一般Memory属性配置为 normal cacheable， shareable，这种情形下，CPU发起的exclusive access会终结在CPU cluster内部，对外的表现，比如cacheline fill和line eviction和正常的读写操作产生的外部行为是一样的。具体实现上，需要结合local monitor的状态管理和cache coherency 的处理逻辑，比如MESI/MOESI的cacheline的状态管理来。

![linux内存管理soc内部一致性访问抽象图](pic_dir/linux内存管理soc内部一致性访问抽象图.png)

对于normal non-cacheable，或者Device类型的memory属性的memory地址，cpu会发出exclusive access的AXI 访问（AxLOCK signals ）到总线上去，总线需要有对应的External exclusive monitor支持，否则会返回错误。例如， 假如某个SOC不支持外部global exclusivemonitor，软件把MMU disabled的情况下，启动SMP Linux，系统是没法启动起来的，在spinlock处会挂掉。


* LDREX，本质上是一个LDR，CPU1做cache linefill，然后设置该line为E状态（Exclusive），额外的一个作用是设置exclusive monitor的状态为Exclusive；其他cpu做LDREX，该line也会分配到它的内部cache里面，状态都设置为Shared ，也会设置本CPU的monitor的状态。当一个CPU 做STREX时候，这个Write操作会把其它CPU里面的cacheline数据给invalidate掉。同时也把monitor的状态清掉，从Exclusive变成Open的状态，这个MESI协议导致cachline的状态在多CPU的变化，是执行Write操作一次性改变的。这样在保证数据一致性的同时，也保证了montitor的状态更新同步改变。




## 6 内存相关命令

**非常重要的小调试工具devmem**<br>
devmem的方式是提供给驱动开发人员，在应用层能够侦测内存地址中的数据变化，以此来检测驱动中对内存或者相关配置的正确性验证. 这个工具的原理也比较简单，就是应用程序通过mmap函数实现对/dev/mem驱动中mmap方法的使用，映射了设备的内存到用户空间，实现对这些物理地址的读写操作。基本Usage: devmem ADDRESS [WIDTH [VALUE]].如果/dev下没有mem这个node，会出现错误.这时可以在Host系统中手动创建一个（例如在NFS root filesystem模式）： host@host-laptop:~/embedded/tftpboot/nfsroot/dev$ sudo mknod mem -m666 c 1 1 注意
这里的权限是666，允许任何人任意读写，可以很好的配合程序debug。结合symbol map，可以方便的查看和改写内核中重要的全局变量。范例，在地址0x97000000写入32bit值0x7777ABCD :  devmem 0x97000000 32 0x7777ABCD

```cpp
//#include <stdio.h> #include <stdlib.h> #include <unistd.h> #include <string.h> #include <errno.h> #include <signal.h> #include <fcntl.h> #include <ctype.h> #include <termios.h> #include <sys/types.h> #include <sys/mman.h> #define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \ __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0) #define MAP_SIZE 4096UL #define MAP_MASK (MAP_SIZE - 1) int main(int argc, char **argv) { int fd; void *map_base, *virt_addr; unsigned long read_result, writeval; off_t target; int access_type = 'w'; if(argc < 2) {//若参数个数少于两个则打印此工具的使用方法 fprintf(stderr, "\nUsage:\t%s { address } [ type [ data ] ]\n" "\taddress : memory address to act upon\n" "\ttype    : access operation type : [b]yte, [h]alfword, [w]ord\n" "\tdata    : data to be written\n\n", argv[0]); exit(1); } target = strtoul(argv[1], 0, 0); if(argc > 2) access_type = tolower(argv[2][0]); if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL; printf("/dev/mem opened.\n"); fflush(stdout); /* Map one page */ //将内核空间映射到用户空间 map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK); if(map_base == (void *) -1) FATAL; printf("Memory mapped at address %p.\n", map_base); fflush(stdout); virt_addr = map_base + (target & MAP_MASK); //针对不同的参数获取不同类型内存数据 &nbsp; switch(access_type) { case 'b': read_result = *((unsigned char *) virt_addr); break; case 'h': read_result = *((unsigned short *) virt_addr); break; case 'w': read_result = *((unsigned long *) virt_addr); break; default: fprintf(stderr, "Illegal data type '%c'.\n", access_type); exit(2); } printf("Value at address 0x%X (%p): 0x%X\n", target, virt_addr, read_result); fflush(stdout); //若参数大于3个，则说明为写入操作，针对不同参数写入不同类型的数据 if(argc > 3) { writeval = strtoul(argv[3], 0, 0); switch(access_type) { case 'b': *((unsigned char *) virt_addr) = writeval; read_result = *((unsigned char *) virt_addr); break; case 'h': *((unsigned short *) virt_addr) = writeval; read_result = *((unsigned short *) virt_addr); break; case 'w': *((unsigned long *) virt_addr) = writeval; read_result = *((unsigned long *) virt_addr); break; } printf("Written 0x%X; readback 0x%X\n", writeval, read_result); fflush(stdout); } if(munmap(map_base, MAP_SIZE) == -1) FATAL; close(fd); return 0; }
```

### 6.1 空闲内存查看和回收
在Linux系统上查看内存使用状况最常用的命令是”free”，其中buffers和cache通常被认为是可以回收的；当内存紧张的时候，有一个常用的手段就是使用下面的命令来手工回收cache：
**手动释放内存：**
```cpp
cat /proc/sys/vm/drop_caches
sync //drop_caches只回收clean pages，不回收dirty pages;所以如果想回收更多的cache，应该在drop_caches之前先执行”sync”命令，把dirty pages变成clean pages。
echo 3 > /proc/sys/vm/drop_caches
free -m
```

注：drop_caches接受以下三种值：
- To free pagecache:<br>
    echo 1 > /proc/sys/vm/drop_caches
- To free reclaimable slab objects (includes dentries and inodes):<br>
    echo 2 > /proc/sys/vm/drop_caches
- To free slab objects and pagecache:<br>
    echo 3 > /proc/sys/vm/drop_caches

**tmpfs不能回收**<br>
其次，即使提前执行了sync命令，drop_cache操作也不可能把cache值降到0，甚至有时候cache值几乎没有下降，这是为什么呢？因为page cache中包含的tmpfs和共享内存是不能通过drop_caches回收的。Page cache用于缓存文件里的数据，不仅包括普通的磁盘文件，还包括了tmpfs文件，tmpfs文件系统是将一部分内存空间模拟成文件系统，由于背后并没有对应着磁盘，无法进行paging(换页)，只能进行swapping(交换)，在执行drop_cache操作的时候tmpfs对应的page cache并不会回收。Page cache用于缓存文件里的数据，不仅包括普通的磁盘文件，还包括了tmpfs文件，tmpfs文件系统是将一部分内存空间模拟成文件系统，由于背后并没有对应着磁盘，无法进行paging(换页)，只能进行swapping(交换)，在执行drop_cache操作的时候tmpfs对应的page cache并不会回收。


```cpp
//测试: 验证tmpfile的内存状态,mount之后并不占用内存，生成testfile之后，Shmem和Cached增加2M，删除testfile之后，这部分内存才可以释放
echo "before create tmp test file"
cat /proc/meminfo | grep -e "memfree" -e "Cached" -e "Shmem"
mount -t tmpfs -o size=3M none /mnt/
dd if=/dev/zero of=/mnt/testfile bs=1M count=2
echo "created test file, before drop caches"
cat /proc/meminfo | grep -e "memfree" -e "Cached" -e "Shmem"
echo "before remove testfile"
echo 3 > /proc/sys/vm/drop_caches;cat /proc/meminfo | grep -ie "memfree" -e "Cached" -e "Shmem"
rm /mnt/testfile
echo "after remove testfile"
echo 3 > /proc/sys/vm/drop_caches;cat /proc/meminfo | grep -e "memfree" -e "Cached" -e "Shmem"
```

**反向映射**<br>
 在进行页面回收的时候，Linux 2.6 在前边介绍的 shrink_page_list() 函数中调用 try_to_unmap() 函数去更新所有引用了回收页面的页表项. 函 数 try_to_unmap() 分别调用了两个函数 try_to_unmap_anon() 和 try_to_unmap_file()，其目的都是检查并确定都有哪些页表项引用了同一个物理页面，但是，由于匿名页面和文件映射页面分别采用了不同的 数据结构，所以二者采用了不同的方法。函数 try_to_unmap_anon() 用于匿名页面，该函数扫描相应的 anon_vma 表中包含的所有内存区域，并对这些内存区域分别调用 try_to_unmap_one() 函数。函数 try_to_unmap_file() 用于文件映射页面，该函数会在优先级搜索树中进行搜索，并为每一个搜索到的内存区域调用 try_to_unmap_one() 函数。两条代码路径最终汇合到 try_to_unmap_one() 函数中，更新引用特定物理页面的所有页表项的操作都是在这个函数中实现的。

### 6.2 内存详细查询： meminfo
	参考文档： http://linuxperf.com/?cat=7

```cpp
cat /proc/meminfo

MemTotal:          55872 kB
MemFree:           42848 kB
Buffers:             388 kB
Cached:             3572 kB
SwapCached:            0 kB
Active:             1648 kB
Inactive:           3260 kB
Active(anon):        948 kB
Inactive(anon):     2112 kB
Active(file):        700 kB
Inactive(file):     1148 kB
Unevictable:           0 kB
Mlocked:               0 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Dirty:                 0 kB
Writeback:             0 kB
AnonPages:           968 kB
Mapped:             1128 kB
Shmem:              2112 kB
Slab:               4748 kB
SReclaimable:        796 kB
SUnreclaim:         3952 kB
KernelStack:         320 kB
PageTables:          120 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:       27936 kB
Committed_AS:      21824 kB
VmallocTotal:     966656 kB
VmallocUsed:       14036 kB
VmallocChunk:     928764 kB
```

含义介绍：<br>
- MemTotal: 所有可用RAM大小（即物理内存减去一些预留位和内核的二进制代码大小）系统从加电开始到引导完成，firmware/BIOS要保留一些内存，kernel本身要占用一些内存，最后剩下可供kernel支配的内存就是MemTotal。这个值在系统运行期间一般是固定不变的
- MemFree: LowFree与HighFree的总和，被系统留着未使用的内存
- MemAvailable 有些应用程序会根据系统的可用内存大小自动调整内存申请的多少，所以需要一个记录当前可用内存数量的统计值，MemFree并不适用，因为MemFree不能代表全部可用的内存，系统中有些内存虽然已被使用但是可以回收的，比如cache/buffer、slab都有一部分可以回收，所以这部分可回收的内存加上MemFree才是系统可用的内存，即MemAvailable。/proc/meminfo中的MemAvailable是内核使用特定的算法估算出来的，要注意这是一个估计值，并不精确。
- Buffers: 块设备(block device)所占用的缓存页，包括：直接读写块设备、以及文件系统元数据(metadata)比如SuperBlock所使用的缓存页
- Cached: 被高速缓冲存储器（cache memory）用的内存的大小(等于 diskcache minus SwapCache),普通文件(通过文件系统访问的文件)占用的缓冲页
- SwapCached:被高速缓冲存储器（cache memory）用的交换空间的大小
             已经被交换出来的内存，但仍然被存放在swapfile中。用来在需要的时候很快的被替换而不需要再次打开I/O端口。
- Active: 在活跃使用中的缓冲或高速缓冲存储器页面文件的大小，除非非常必要否则不会被移作他用.
- Inactive: 在不经常使用中的缓冲或高速缓冲存储器页面文件的大小，可能被用于其他途径.
- Unevictable
- Mlocked
- SwapTotal: 交换空间的总大小，对于c20系统，无效项
	- SwapFree: 未被使用交换空间的大小
- Dirty: 等待被写回到磁盘的内存大小。
- Writeback: 正在被写回到磁盘的内存大小。
- AnonPages：未映射页的内存大小
- Mapped: 设备和文件等映射的大小。
- Slab:通过slab分配的内存
	- SReclaimable:slab中可回收的部分。调用kmem_getpages()时加上SLAB_RECLAIM_ACCOUNT标记，表明是可回收的
	- SUnreclaim：不可收回Slab的大小（SUnreclaim+SReclaimable＝Slab）
- PageTables：管理内存分页页面的索引表的大小。
- NFS_Unstable:不稳定页表的大小
- VmallocTotal: 可以vmalloc虚拟内存大小
	- VmallocUsed: 已经被使用的虚拟内存大小。
	- VmallocChunk: largest contigious block of vmalloc area which is free

### 6.4 misc命令

**统计所有进程占用内存：**<br>
```cpp
	grep Pss /proc/[1-9]*/smaps | awk '{total+=$2}; END {print total}'
	// /proc/<pid>/smaps 包含了进程的每一个内存映射的统计值，详见proc(5)的手册页。Pss(Proportional Set Size)把共享内存的Rss进行了平均分摊，比如某一块100MB的内存被10个进程共享，那么每个进程就摊到10MB。这样，累加Pss就不会导致共享内存被重复计算了。
	// 有人提出【MemTotal = MemFree + buff/cache + slab + 全部进程占用的内存】。这是不对的，原因之一是：进程占用的内存包含了一部分page cache，换句话说，就是进程占用的内存与page cache发生了重叠。比如进程的mmap文件映射同时也统计在page cache中。
```
