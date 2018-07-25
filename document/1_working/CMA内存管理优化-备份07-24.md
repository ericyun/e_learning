## CMA内存管理优化

###  修订记录
| 修订说明 | 日期 | 作者 |
| --- | --- | --- |
| 初版 | 2018/06/28 | 员清观 |

---
## 1 优化需求
videobox和客户应用共享使用CMA内存,客户应用占用大量内存的情况下,会使CMA出现大量碎片,从而导致videobox分配大块连续内存失败.已知场景至少包括:
- 客户高优先级程序和videobox进程同时后台启动,竞争CMA内存,可以参考`RM#6667 videobox 启动出现几率性失败`
- videobox改变分辨率

本文将主要涵盖两个方面的改进:
- 为fr驱动模块提供独立内存和分配算法(带诊断机制),以避免应用影响videobox和audiobox内存分配
- 解决现有CMA机制下由于碎片问题导致分配连续内存失败的问题

---
## 2 内存分配算法优化
### 2.1 CMA内存分配算法
新算法的实现将参考现有的CMA机制,比如它的对齐要求,物理地址和虚拟地址转换等,这里先简单介绍一下现有CMA机制

分配CMA内存综合使用了下面两种算法:
- bitmap算法
  CMA中每一个page都对应bitmap数组中一个bit,此bit为1表示该page已经被分配,为0代表该page空闲.分配连续内存的时候,在bitmap中寻找第一个连续为0的bit区域,并设置为1;释放内存的时候将此bit区域清0
- 伙伴系统
  这是linux内存管理基本算法,不在此介绍;videobox和客户应用共享使用CMA内存的时候,CMA内存初始化时会被释放到伙伴系统中

CMA内存分配算法主要功能接口:
- 初始化 将预留的CMA内存块释放到伙伴系统中
- 申请内存 先在bitmap中找到满足要求的连续空闲内存块,并标记为占用;同时在伙伴系统中申请此内存块,其中已经被用户程序占用的页要强制迁移到其他空闲页
- 释放内存 在bitmap中把对应内存块标记为空闲;同时在伙伴系统中释放内存块

**CMA内存分配和释放主流程**<br>
```cpp
dma_alloc_coherent(dev, ...)
  |--> dma_alloc_attrs(dev, ...)
    |--> arm_dma_alloc(dev, ...)
      //if (dma_alloc_from_coherent(dev, size, handle, &memory))  return memory; //dev非空的情况
      |--> return __dma_alloc(dev, size, handle, gfp, prot, false,__builtin_return_address(0)); //dev空的情况
        |--> return __alloc_from_contiguous(dev, size, prot, &page, caller);
          page = dma_alloc_from_contiguous(dev, count, order);
          ...
dma_free_coherent(dev, ...)
  |--> dma_free_attrs(dev, ...)
    |--> __arm_dma_free(dev, ...)
      if (dma_release_from_coherent(dev, get_order(size), cpu_addr))  return;//dev非空的情况
      |--> __free_from_contiguous(dev, page, cpu_addr, size);
        dma_release_from_contiguous(dev, page, size >> PAGE_SHIFT);
        ...
```

`dma_alloc_coherent()`函数第一个参数`dev`确定了它的两种主要使用场景:<br>
- `dev`为`NULL`
  同时使用了bitmap和伙伴系统两种算法管理内存<br>
  从系统公有的CMA中分配内存,当前fr和驱动模块都是此种方式<br>
- `dev`为具体设备指针
  仅使用bitmap算法管理内存<br>
  从设备独享的私有CMA中分配内存,当前系统中未使用此种方式<br>

### 2.2 算法定义
没有必要完全设计全新的算法,参考了linux初始化阶段的boootmem和memblock内存管理算法之后,决定采用CMA正在使用的bitmap算法,排除伙伴系统部分,管理fr内存.bitmap算法简单高效,比之memblock算法更便于诊断调试

实现方式: 保留原CMA机制,在CMA初始化完成之后,fr驱动调用`dma_alloc_coherent()`直接分配大块空间(比如32M)用于fr内存分配,驱动或者客户应用可以继续共享访问CMA剩余空间.新的bitmap算法将基于fr驱动预留的这部分内存实现,为videobox和audiobox使用内存提供新的alloc和free函数

**fr内存分配函数接口**<br>
```cpp
void *fr_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp)
  pageno = bitmap_find_next_zero_area(mempool->bitmap, mempool->count, start, count, mask);//在bitmap中找到第一个满足要求的连续块
  bitmap_set(mempool->bitmap, pageno, count); //设置bitmap中对应内存位域
  pfn = mempool->base_pfn + pageno;
  *handle = pfn_to_dma(dev, pfn); //返回dma线性地址
  addr = mempool->vaddr + PAGE_SIZE * pageno;//返回虚拟地址
  return addr;

void fr_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle)
  bitmap_clear(mempool->bitmap, pfn - mempool->base_pfn, count);

int __init fr_mempool_init(void)
  void *vaddr = dma_alloc_coherent(NULL, fr_pool.size, &fr_handle, GFP_KERNEL|__GFP_NOWARN);//为videobox预留足够大的内存
  mempool->bitmap = kzalloc(bitmap_size, GFP_KERNEL);  //申请并零初始化bitmap算法位图数组
  mempool->base_pfn = PFN_DOWN(fr_handle);
  mempool->vaddr = vaddr;
core_initcall_sync(fr_mempool_init)//将在所有驱动模块初始化之前调用
```

`fr_mempool_init()`函数申请mempool并初始化bitmap,可以在内核初始化过程中调用,但如果需要动态确定mempool的大小,也可通过用户空间调用

## 2.3 bitmap算法诊断机制
为了方便内存管理机制开发和调试,增加bitmap算法诊断信息打印.每次内存分配,无论成功失败,都会打印相关的bitmap信息.但如果每次内存分配都打印完整的bitmmap位图信息,一方面显示内容过多,另一方面不方便从中找出有用的信息,因而显示位图信息时做了过滤,在下面两个小节中分别描述

### 2.3.1 显示bitmap完整mapping
当前在下面场景下,会显示完整的mapping信息:
- 本次内存分配和上次分配来自不用的pid时,一般来说,对应于切换到新进程,比如videobox或audiobox启动后第一次分配内存时
- 本次内存分配失败,打印完整mapping帮助分析
- 待实现,提供主动查询bitmap的功能

打印完整位图信息时,每个映射字段对应一个32bit的mapping信息,含义:
- `00000000,` 对应的32个页全部空闲
- `________,` 相当于`ffffffff,` 对应的32个页全部被占用,对比分析时比后者有更为直观的视觉效果
- 非全0非全f,如`00f7ffff`,对应9个页空闲,23个页被占用

`free page: `域显示当前bitmap行空闲page的个数

**videobox启动时首次alloc显示完整的bitmap free mapping**
```
~~~~~~~~~~~~~~~~~~~~~~~~Switch to new thread, print all(and only) free mapping information~~~~~~~~~~~~~~~~~~~~~~~~
0000:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,0007ffff,________,________,000000f7,________,________, free page: 358
0064:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0128:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0192:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0256:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0320:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0384:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0448:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0512:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0576:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0640:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0704:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0768:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0832:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0896:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
0960:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
1024:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
1088:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000, free page: 512
~~~~~~~~~~~~~~~~~~~~~~~~Total 9062 free pages, 35M and 408k bytes~~~~~~~~~~~~~~~~~~~~~~~~
```
映射第一行非0字段,对应与videobox之前driver部分分配的约600k内存

**audiobox启动时首次alloc显示完整的bitmap free mapping**
```
~~~~~~~~~~~~~~~~~~~~~~~~Switch to new thread, print all(and only) free mapping information~~~~~~~~~~~~~~~~~~~~~~~~
0000:bitmap->________,________,________,________,________,________,________,________,01ff077f,________,________,________,________,________,________,________, free page: 13
0064:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 0
0128:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 0
0192:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 0
0256:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 0
0320:bitmap->________,________,________,________,________,________,________,________,0001ffff,________,________,________,0fff0000,01ffffff,________,________, free page: 42
0384:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 0
0448:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,0fff01ff,________,________,________, free page: 11
0512:bitmap->________,________,________,________,________,________,________,________,00000003,________,7fff0003,________,________,________,________,________, free page: 45
0576:bitmap->1fffffff,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 3
0640:bitmap->________,________,________,________,________,________,________,________,00000000,________,________,________,00000000,00000000,0003ffff,________, free page: 110
0704:bitmap->1fffffff,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 3
0768:bitmap->________,________,________,________,________,________,________,________,________,________,________,________,________,________,________,________, free page: 0
0832:bitmap->________,________,________,________,________,________,________,________,1fffffff,________,________,________,________,________,________,________, free page: 3
0896:bitmap->________,________,________,________,________,________,________,________,3fffffff,________,________,________,________,________,________,________, free page: 2
0960:bitmap->00000000,000007ff,________,________,00000000,000007ff,________,________,3fffffff,________,________,________,________,________,________,________, free page: 108
1024:bitmap->7fffffff,________,________,________,________,________,________,________,7fffffff,________,________,________,________,________,________,________, free page: 2
1088:bitmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,0000003f,________,0000003f,________, free page: 436
~~~~~~~~~~~~~~~~~~~~~~~~Total 778 free pages, 3M and 40k bytes~~~~~~~~~~~~~~~~~~~~~~~~
```
此时videobox已经占用了大多数内存,trace中可以看到剩余空间大小还有约3M,最大一个连续块为1.5M

### 2.3.2 单次内存分配trace
打印单次分配相关位图信息时,如果某行对应的页不相关,行首会显示`xxxx:bitmap->`并且后面映射信息不打印;否则,会同时打印分配前后的映射信息,分配前映射以`xxxx:oldmap->`开始,分配后映射以`xxxx:newmap->`开始

打印单次内存分配相关位图信息时,每个映射字段对应一个32bit的mapping信息,含义和上面打印完整映射稍有不同:
- `00000000,` 对应的32个页全部空闲
- `ffffffff,` 对应的32个页全部占用
- 非全0非全f,如`00f7ffff`,对应9个页空闲,23个页被占用
- `********,` 只会出现在`xxxx:newmap->`行中,表示对应的映射没有变化

**videobox某次分配小块内存**
```
__alloc_from_contiguous succcess: size=16384, count=4, order=2 page=c03c0680
bitmap(pid:526, command:videoboxd)->
0000:oldmap->ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,00000000,00000000,0007ffff,ffffffff,ffffffff,37fffff7,ffffffff,ffffffff
0000:newmap->********,********,********,********,********,********,********,********,********,********,00f7ffff,********,********,********,********,
0064:bitmap->
0128:bitmap->
0192:bitmap->
0256:bitmap->
0320:bitmap->
0384:bitmap->
0448:bitmap->
0512:bitmap->
0576:bitmap->
0640:bitmap->
0704:bitmap->
0768:bitmap->
0832:bitmap->
0896:bitmap->
0960:bitmap->
1024:bitmap->
1088:bitmap->
~~~~~~~~~~~~~~~~~~~~~~~~Total 5146 free pages, 20M and 104k bytes~~~~~~~~~~~~~~~~~~~~~~~~
```
可以看到跳过了不满足连续要求的`37fffff7`段,从`0007ffff`段中分配,分配完成之后映射变为`00f7ffff`

**videobox对齐分配大块内存**
```
__alloc_from_contiguous succcess: size=3112960, count=760, order=10 page=c03c1000
bitmap(pid:526, command:videoboxd)->
0000:oldmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,0007ffff,ffffffff,ffffffff,07fffff7,ffffffff,ffffffff
0000:newmap->ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,********,********,********,********,********,********,********,
0064:oldmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000
0064:newmap->00ffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff
0128:bitmap->
0192:bitmap->
0256:bitmap->
0320:bitmap->
0384:bitmap->
0448:bitmap->
0512:bitmap->
0576:bitmap->
0640:bitmap->
0704:bitmap->
0768:bitmap->
0832:bitmap->
0896:bitmap->
0960:bitmap->
1024:bitmap->
1088:bitmap->
~~~~~~~~~~~~~~~~~~~~~~~~Total 8283 free pages, 32M and 364k bytes~~~~~~~~~~~~~~~~~~~~~~~~
```
有对齐要求,所以跳过了`00000000,00000000,0007ffff`空闲段

**videobox无对齐分配大块内存**
```
__alloc_from_contiguous_noalign succcess: size=3112960, count=760, page=c03c6f00
bitmap(pid:526, command:videoboxd)->
0000:bitmap->
0064:oldmap->00ffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff
0064:newmap->ffffffff,********,********,********,********,********,********,********,********,********,********,********,********,********,********,
0128:oldmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000
0128:newmap->ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff
0192:oldmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000
0192:newmap->********,********,********,********,********,********,********,********,0000ffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff
0256:bitmap->
0320:bitmap->
0384:bitmap->
0448:bitmap->
0512:bitmap->
0576:bitmap->
0640:bitmap->
0704:bitmap->
0768:bitmap->
0832:bitmap->
0896:bitmap->
0960:bitmap->
1024:bitmap->
1088:bitmap->
~~~~~~~~~~~~~~~~~~~~~~~~Total 7523 free pages, 29M and 396k bytes~~~~~~~~~~~~~~~~~~~~~~~~
```
无对齐要求,从第一个包含空闲页的`00ffffff`段开始分配

**videobox释放内存**
```
fr-core: (isp): clean-fp(c07a99c0) encrecord-stream (c0830000)
bitmap(pid:533, command:isp)->
0000:bitmap->
0064:bitmap->
0128:bitmap->
0192:bitmap->
0256:bitmap->
0320:bitmap->
0384:bitmap->
0448:oldmap->ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,00000000,00000000,00000000,00000000
0448:newmap->00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,********,********,********,********,********,********,********,
0512:oldmap->ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,00000003,ffffffff,00000003,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff
0512:newmap->********,********,********,********,********,********,********,********,********,********,********,********,00000000,00000000,00000000,00000000
0576:bitmap->
0640:bitmap->
0704:bitmap->
0768:bitmap->
0832:bitmap->
0896:bitmap->
0960:bitmap->
1024:bitmap->
1088:bitmap->
~~~~~~~~~~~~~~~~~~~~~~~~Total 6486 free pages, 25M and 344k bytes~~~~~~~~~~~~~~~~~~~~~~~~
```

---
## 3 修正CMA机制Bug
`RM#6667 videobox 启动出现几率性失败`问题中,videobox分配内存失败后`cat /proc/pagetypeinfo`的显示结果如下:
```
Free pages count per migrate type at order       0      1      2      3      4      5      6      7      8      9     10
Node    0, zone   Normal, type    Unmovable      0      1      2      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type  Reclaimable     70     92     52      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type      Movable      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type      Reserve      0      0      0      1      0      0      1      1      1      1      0
Node    0, zone   Normal, type          CMA   2161   2160    356      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type      Isolate      0      0      0      0      0      0      0      0      0      0      0

Number of blocks type     Unmovable  Reclaimable      Movable      Reserve          CMA      Isolate
Node 0, zone   Normal            1            1            3            1           10            0
```

上述场景下,用户的应用程序大量抢占CMA的内存后产生过多内存碎片,导致videobox无法申请到足够长的连续内存. 基于此,有两个疑点:
- 碎片太过琐碎,基本上都是1或2或4个page,完全不存在8个page或更多page的块,严重怀疑当前使用的CMA内存算法是否合理
- 伙伴系统有碎片整理的能力,但没有生效,怀疑碎片回收的算法有缺陷

比较系统的跟踪内核相关内存管理部分逻辑之后,发现上述两个问题确实存在,建议下一步修正这两个问题

### 3.1 CMA迁移算法bug
跟踪应用层`malloc()`函数调用流程到`__alloc_pages_nodemask()`:
```cpp
__alloc_pages_nodemask()
  |--> get_page_from_freelist()
    |--> buffered_rmqueue()
      |--> __rmqueue()
        page = __rmqueue_smallest(zone, order, migratetype);
        if (unlikely(!page) && migratetype != MIGRATE_RESERVE)
          |--> page = __rmqueue_fallback(zone, order, migratetype);
            for (current_order = MAX_ORDER-1; current_order >= order; --current_order) {
  		        for (i = 0;; i++) {
                migratetype = fallbacks[start_migratetype][i];
                ...
              }
            }
```

应用层调用`__rmqueue_smallest()`申请不到非CMA内存时,就会调用`__rmqueue_fallback()`从CMA申请内存. 阅读代码至此发现逻辑上存在严重问题,检查分配的循环每次从`current_order = MAX_ORDER-1`开始,也就是说每次优先分配最大的内存块,这种逻辑简直就是个内存粉碎器. 比如我们有10个4M大小的CMA块,10次调用分配之后,哪怕每次只分配4k小内存,我们也只剩下10个2M大小的块和大量小碎片;继续反复大量调用后,最后只会剩下4k碎片

简单尝试修改逻辑如下进行测试,优先分配小内存块,不会再出现大量碎片:
```cpp
  ...
    for (current_order = order; current_order <= MAX_ORDER-1; current_order++)
    		for (i = 0;; i++)
          ...
```

查看最新linux4.18 kernel,这部分代码也已经被修正,系统优先分配小的CMA块

### 3.2 碎片整理问题
因为涉及到更复杂的流程,尚未发现问题根源,需要更多时间跟踪.修正内存分配算法中逻辑错误之后,没有再次发现,这应该是超量碎片场景下才会出现的

当前实现了一个简单的测试程序,每次启动都能重现此问题,如果需要解决此问题,就进一步跟踪

---
## 4 待讨论确定项
**待讨论确定内容:**<br>
- 增加CONFIG项,使能禁止fr独立内存分配算法<br>
  禁止时,完全和之前机制相同<br>
  使用单独的文件,通过CONFIG控制其编译<br>
  或者,应该在item文件中,通过frsize的定义来确定,如果frsize没有定义,就继续使用之前的设定,或者,应该定义sharesize,cma-sharesize就是fr的大小
- fr增加ioctrl command,控制bitmap信息的打印
- 参考最新版本kernel修正CMA迁移算法bug
- 追踪碎片整理失败原因
- 考虑采用寻找最合适大小的算法
- 考虑内存泄露情况下,确定泄露了哪些内存,对应pid等信息
- 之前的alloc函数中,包含强制0初始化,是否需要移植过来

---
## 5 验证测试
验证测试内容:<br>
- 压力测试:不断切换分辨率抓图<br>
- 循环压力测试<br>
  每一个循环中,启动`videoboxd`,工作几秒之后`cat /proc/fr_swap`确认工作状态,然后`killall videoboxd`
- 其他,建伟之前文档中有些测试场景<br>
  http://platsoft.in.infotm.com/#!qsdk/sharing/cma_case/cma_case.md
- 拍照和录像不断切换的场景
- anni项目场景
- 小心per_cpu的释放链表

验证内容:
- anni项目场景
- 基本流程没问题
- 压力测试:切换分辨率抓图<br>
- videbox部分仍然会分配普通内存,所以,及时独立分配fr,仍然可能失败.

实现方式:
- CONFIG中定义CMA和fr大小,缺省启用;fr大小和CMA之间还必须满足一定的关系,实际觉得最好是自动.
- item中定义fr大小,如果未定义就缺省为关闭,因为之前的item文件中没哟定义
- 我喜欢自动配置的方式,初始化时锁定cma,分配完毕之后保留
- audiobox部分fr的使用,多个通道需要共享的时候,现在使用物理地址,也许之后可以改成其他共享方式,比如共享内存的方式.;dev和aec和codec部分实际并不需要连续内存
- 从客户角度考虑,缺省应该和以前保持兼容,新feature应该作为扩展,倒是参考project的设置中,缺省增加config.frsize项;缺省设置倒是可以增加这样的设定:fr启动时申请全部的cma内存,videobox初始化完毕之后调用特定函数,解除锁定.

## 6 代码修正记录
```cpp
  mempool->base_pfn + mempool->count -->  mempool->end_pfn
```

**相关bug:**<br>
  http://180.168.66.238:8810/redmine/issues/3177
  http://180.168.66.238:8810/redmine/issues/4635

### 6.1 item用法

```cpp
  char tmp[MAX_STR_LEN] = {0};
  char tag[MAX_STR_LEN] = {0};
  char itm[MAX_STR_LEN] = {0};
  sprintf(tmp, "%s%d.%s.%d", "sensor", index, "bootup.seq", i);
  pr_db("tmp = %s\n", tmp);
  if(item_exist(tmp)) {
    item_string(tag, tmp, 0);
    item_string(itm, tmp, 1);
    val = item_integer(tmp, 2);
    dly = item_integer(tmp, 3);

ITEMS_EINT

int item_integer(const char *key, int section)
int item_equal(const char *key, const char *value, int section)
int item_string(char *buf, const char *key, int section)
int item_exist(const char *key)


if (item_exist("config.frsize")) {
    frsize = item_integer("config.frsize", 2);
}



```
