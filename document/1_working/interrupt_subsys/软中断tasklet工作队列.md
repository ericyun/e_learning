# 软中断tasklet工作队列

## 0.修订记录
| 修订说明 | 日期 | 作者 | 额外说明 |
| --- |
| 初版 | 2018/04/10 | 员清观 |  |

## 1.中断子系统一些基本概念:
HI_SOFTIRQ用于高优先级的tasklet，TASKLET_SOFTIRQ用于普通的tasklet. 这样的话,是不是timer-cmn1可以使用HI_SOFTIRQ的方式来实现呢? 方法就是使用 tasklet_hi_schedule() 代替 tasklet_schedule().

一个合格的linux驱动工程师需要对kernel中的中断子系统有深刻的理解，只有这样，在写具体driver的时候才能：
1、正确的使用linux kernel提供的的API，例如最著名的request_threaded_irq（request_irq）接口
2、正确使用同步机制保护驱动代码中的临界区
3、正确的使用kernel提供的softirq、tasklet、workqueue等机制来完成具体的中断处理

linux kernel的中断子系统分成4个部分：
1. 硬件无关的代码，我们称之Linux kernel通用中断处理模块。无论是哪种CPU，哪种controller，其中断处理的过程都有一些相同的内容，这些相同的内容被抽象出来，和HW无关。此外，各个外设的驱动代码中，也希望能用一个统一的接口实现irq相关的管理（不和具体的中断硬件系统以及CPU体系结构相关）这些“通用”的代码组成了linux kernel interrupt subsystem的核心部分。
2. CPU architecture相关的中断处理。 和系统使用的具体的CPU architecture相关。
3. Interrupt controller驱动代码 。和系统使用的Interrupt controller相关。
4. 普通外设的驱动。这些驱动将使用Linux kernel通用中断处理模块的API来实现自己的驱动逻辑。

当外设触发一次中断后，一个大概的处理过程是：
1、具体CPU architecture相关的模块会进行现场保护，然后调用machine driver对应的中断处理handler.--> ARM的IRQ异常,保护现场,调用中断handler
2、machine driver对应的中断处理handler中会根据硬件的信息获取HW interrupt ID，并且通过irq domain模块翻译成IRQ number.
3、调用该IRQ number对应的high level irq event handler，在这个high level的handler中，会通过和interupt controller交互，进行中断处理的flow control（处理中断的嵌套、抢占等），当然最终会遍历该中断描述符的IRQ action list，调用外设的specific handler来处理该中断
          //中断控制器相关的处理.
4、具体CPU architecture相关的模块会进行现场恢复。

对于中断处理而言，linux将其分成了两个部分，一个叫做中断handler（top half），属于不那么紧急需要处理的事情被推迟执行，我们称之deferable task，或者叫做bottom half，。具体如何推迟执行分成下面几种情况：
1、推迟到top half执行完毕, 包括softirq机制和tasklet机制
2、推迟到某个指定的时间片（例如40ms）之后执行, softirq机制的一种应用场景（timer类型的softirq）
3、推迟到某个内核线程被调度的时候执行,包括threaded irq handler以及通用的workqueue机制和驱动专属kernel thread（不推荐使用）


## 2 tasklet
### 2.1 比softirq优点
tasklet对于softirq而言有哪些好处：
1. tasklet可以动态分配，也可以静态分配，数量不限。
2. 同一种tasklet在多个cpu上也不会并行执行，这使得程序员在撰写tasklet function的时候比较方便，减少了对并发的考虑（当然损失了性能）。

对于第一种好处，其实也就是为乱用tasklet打开了方便之门，很多撰写驱动的软件工程师不会仔细考量其driver是否有性能需求就直接使用了tasklet机制。对于第二种好处，本身考虑并发就是软件工程师的职责。因此，看起来tasklet并没有引入特别的好处，而且和softirq一样，都不能sleep，限制了handler撰写的方便性，看起来其实并没有存在的必要。在4.0 kernel的代码中，grep一下tasklet的使用，实际上是一个很长的列表，只要对这些使用进行简单的归类就可以删除对tasklet的使用。对于那些有性能需求的，可以考虑并入softirq，其他的可以考虑使用workqueue来取代。Steven Rostedt试图进行这方面的尝试（http://lwn.net/Articles/239484/），不过这个patch始终未能进入main line。

### 2.2 tasklet应用

```
struct tasklet_struct {
  struct tasklet_struct *next;
  unsigned long state;
  atomic_t count;
  void (*func)(unsigned long);
  unsigned long data;
};
enum
{ HI_SOFTIRQ=0, TIMER_SOFTIRQ, NET_TX_SOFTIRQ,
  NET_RX_SOFTIRQ, BLOCK_SOFTIRQ, BLOCK_IOPOLL_SOFTIRQ,
  TASKLET_SOFTIRQ, SCHED_SOFTIRQ,   HRTIMER_SOFTIRQ, RCU_SOFTIRQ,
  /* Preferable RCU should always be the last softirq */
  NR_SOFTIRQS
};
```

linux kernel中，和tasklet相关的softirq有两项，`HI_SOFTIRQ`用于高优先级的tasklet，`TASKLET_SOFTIRQ`用于普通的tasklet

```
void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data);
//#define DECLARE_TASKLET(name, func, data) \ struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(0), func, data }
//#define DECLARE_TASKLET_DISABLED(name, func, data) \ struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(1), func, data }
```

```
1) void tasklet_disable(struct tasklet_struct *t);
  函数暂时禁止tasklet被tasklet_schedule调度，直到这个 tasklet 再次被enable；若这个 tasklet 当前在运行, 这个函数忙等待直到这个tasklet退出。
2) void tasklet_disable_nosync(struct tasklet_struct *t);
  和tasklet_disable类似，但是tasklet可能仍然运行在另一个 CPU。
3) void tasklet_enable(struct tasklet_struct *t);
  使能一个之前被disable的 tasklet。若这个 tasklet 已经被调度, 它会很快运行。 tasklet_enable和tasklet_disable必须匹配调用, 因为内核跟踪每个 tasklet 的"禁止次数"。
4) void tasklet_schedule(struct tasklet_struct *t);
  调度 tasklet 执行，如果tasklet在运行中被调度, 它在完成后会再次运行; 这保证了在其他事件被处理当中发生的事件受到应有的注意. 这个做法也允许一个 tasklet 重新调度它自己。
5) void tasklet_hi_schedule(struct tasklet_struct *t);
  和tasklet_schedule类似，只是在更高优先级执行。当软中断处理运行时, 它处理高优先级 tasklet。在其他软中断之前，只有具有低响应周期要求的驱动才应使用这个函数, 可避免其他软件中断处理引入的附加周期。
6) void tasklet_kill(struct tasklet_struct *t);
  确 保了 tasklet 不会被再次调度来运行，通常当一个设备正被关闭或者模块卸载时被调用。如果 tasklet正在运行, 这个函数等待直到它执行完毕。若 tasklet 重新调度它自己，则必须阻止在调用 tasklet_kill前它重新调度它自己，如同使用 del_timer_sync。
```

state成员表示该tasklet的状态，`TASKLET_STATE_SCHED`表示该tasklet以及被调度到某个CPU上执行，`TASKLET_STATE_RUN`表示该tasklet正在某个cpu上执行。count成员是和enable或者disable该tasklet的状态相关，如果count等于0那么该tasklet是处于enable的，如果大于0，表示该tasklet是disable的。在softirq文档中，我们知道local_bh_disable/enable函数就是用来disable/enable bottom half的，这里就包括softirq和tasklet。但是，有的时候内核同步的场景不需disable所有的softirq和tasklet，而仅仅是disable该tasklet，这时候，tasklet_disable和tasklet_enable就派上用场了。

范例:
```
struct tasklet_struct my_tasklet;
void my_tasklet_func(unsigned long);
//第一种：
  DECLARE_TASKLET(my_tasklet,my_tasklet_func,data)
  代码DECLARE_TASKLET实现了定义名称为my_tasklet的tasklet并将其与my_tasklet_func这个函数绑定，而传入这个函数的参数为data。
//第二种：
  tasklet_init(&my_tasklet, my_tasklet_func, data);
  //需要调度tasklet的时候引用一个tasklet_schedule()函数就能使系统在适当的时候进行调度，如下所示
  tasklet_schedule(&my_tasklet);
  tasklet_hi_schedule(&my_tasklet);
```

---
## 3 workqueue工作队列
### 3.1

### 3.2

### 3.3

### 3.4

## 4 struct kthread_worker

```
init_kthread_worker(&master->kworker);
master->kworker_task = kthread_run(kthread_worker_fn,
					   &master->kworker,
					   dev_name(&master->dev));
init_kthread_work(&master->pump_messages, spi_pump_messages);
```
