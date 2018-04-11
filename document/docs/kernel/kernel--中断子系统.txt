# 中断子系统

----
## IRQ Domain介绍

在linux kernel中，我们使用下面两个ID来标识一个来自外设的中断：
1. IRQ number
CPU需要为每一个外设中断编号，我们称之IRQ Number。这个IRQ number是一个虚拟的interrupt ID，和硬件无关，仅仅是被CPU用来标识一个外设中断。
2. HW interrupt ID
对于interrupt controller而言，它收集了多个外设的interrupt request line并向上传递，因此，interrupt controller需要对外设中断进行编码。Interrupt controller用HW interrupt ID来标识外设的中断。在interrupt controller级联的情况下，仅仅用HW interrupt ID已经不能唯一标识一个外设中断，还需要知道该HW interrupt ID所属的interrupt controller
这样，CPU和interrupt controller在标识中断上就有了一些不同的概念，但是，对于驱动工程师而言，我们和CPU视角是一样的，我们只希望得到一个IRQ number，而不关系具体是那个interrupt controller上的那个HW interrupt ID。这样一个好处是在中断相关的硬件发生变化的时候，驱动软件不需要修改。因此，linux kernel中的中断子系统需要提供一个将HW interrupt ID映射到IRQ number上来的机制

一个系统好像可以同时支持多个irq domain, 如果理解没有问题的话. 公司的系统应该使用的是irq_domain_add_linear.

向系统注册irq domain
通用中断处理模块中有一个irq domain的子模块，该模块将这种映射关系分成了三类：
1. 线性映射。其实就是一个lookup table，HW interrupt ID作为index，通过查表可以获取对应的IRQ number。对于Linear map而言，interrupt controller对其HW interrupt ID进行编码的时候要满足一定的条件：hw ID不能过大，而且ID排列最好是紧密的。对于线性映射，其接口API如下：
  static inline struct irq_domain *irq_domain_add_linear(struct device_node *of_node,
       unsigned int size,－－－－－－－－－该interrupt domain支持多少IRQ
       const struct irq_domain_ops *ops,－－－callback函数
  void *host_data)－－－－－driver私有数据
  {
     return __irq_domain_add(of_node, size, size, 0, ops, host_data);
  }
2. Radix Tree map。建立一个Radix Tree来维护HW interrupt ID到IRQ number映射关系。HW interrupt ID作为lookup key，在Radix Tree检索到IRQ number。如果的确不能满足线性映射的条件，可以考虑Radix Tree map。实际上，内核中使用Radix Tree map的只有powerPC和MIPS的硬件平台。对于Radix Tree map，其接口API如下：
  static inline struct irq_domain *irq_domain_add_tree(struct device_node *of_node,
       const struct irq_domain_ops *ops,  void *host_data)
  {
     return __irq_domain_add(of_node, 0, ~0, 0, ops, host_data);
  }
3. no map。有些中断控制器很强，可以通过寄存器配置HW interrupt ID而不是由物理连接决定的。例如PowerPC 系统使用的MPIC (Multi-Processor Interrupt Controller)。在这种情况下，不需要进行映射，我们直接把IRQ number写入HW interrupt ID配置寄存器就OK了，这时候，生成的HW interrupt ID就是IRQ number，也就不需要进行mapping了。对于这种类型的映射，其接口API如下：
  static inline struct irq_domain *irq_domain_add_nomap(struct device_node *of_node,
       unsigned int max_irq, const struct irq_domain_ops *ops, void *host_data)
  {
     return __irq_domain_add(of_node, 0, max_irq, max_irq, ops, host_data);
  }
这类接口的逻辑很简单，根据自己的映射类型，初始化struct irq_domain中的各个成员，调用__irq_domain_add将该irq domain挂入irq_domain_list的全局列表。

为irq domain创建映射
上节的内容主要是向系统注册一个irq domain，具体HW interrupt ID和IRQ number的映射关系都是空的，因此，具体各个irq domain如何管理映射所需要的database还是需要建立的。例如：对于线性映射的irq domain，我们需要建立线性映射的lookup table，对于Radix Tree map，我们要把那个反应IRQ number和HW interrupt ID的Radix tree建立起来。创建映射有四个接口函数：
1. 调用irq_create_mapping函数建立HW interrupt ID和IRQ number的映射关系。该接口函数以irq domain和HW interrupt ID为参数，返回IRQ number（这个IRQ number是动态分配的）。该函数的原型定义如下：
  extern unsigned int irq_create_mapping(struct irq_domain *host, irq_hw_number_t hwirq);
驱动调用该函数的时候必须提供HW interrupt ID，也就是意味着driver知道自己使用的HW interrupt ID，而一般情况下，HW interrupt ID其实对具体的driver应该是不可见的，不过有些场景比较特殊，例如GPIO类型的中断，它的HW interrupt ID和GPIO有着特定的关系，driver知道自己使用那个GPIO，也就是知道使用哪一个HW interrupt ID了。
（2）irq_create_strict_mappings。这个接口函数用来为一组HW interrupt ID建立映射。具体函数的原型定义如下：
  int irq_create_strict_mappings(struct irq_domain *domain, unsigned int irq_base, irq_hw_number_t hwirq_base, int count);
（3）irq_create_of_mapping。看到函数名字中的of（open firmware），我想你也可以猜到了几分，这个接口当然是利用device tree进行映射关系的建立。具体函数的原型定义如下：
  unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data);
通常，一个普通设备的device tree node已经描述了足够的中断信息，在这种情况下，该设备的驱动在初始化的时候可以调用irq_of_parse_and_map这个接口函数进行该device node中和中断相关的内容（interrupts和interrupt-parent属性）进行分析，并建立映射关系，具体代码如下：

  unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
  {
  struct of_phandle_args oirq;

  if (of_irq_parse_one(dev, index, &oirq))－－－－分析device node中的interrupt相关属性
  return 0;

  return irq_create_of_mapping(&oirq);－－－－－创建映射，并返回对应的IRQ number
  }

对于一个使用Device tree的普通驱动程序（我们推荐这样做），基本上初始化需要调用irq_of_parse_and_map获取IRQ number，然后调用request_threaded_irq申请中断handler。

（4）irq_create_direct_mapping。这是给no map那种类型的interrupt controller使用的，这里不再赘述。

q3f使用的是GIC中断控制器, 但是在GIC的代码中没有调用标准的注册 irq domain 的接口函数, 而是irq_domain_add_legacy. 如果想充分发挥Device Tree的威力，3.14版本中的GIC 代码需要修改。

忽略 probe_irq_on 自动检测irq的相关机制, 应该没有在我们系统中用到吧.
忽略 resend一个中断

asmlinkage void __init start_kernel(void)
     init_IRQ();
          machine_desc->init_irq(); //q3f_init_irq()
     softirq_init();
          open_softirq(TASKLET_SOFTIRQ, tasklet_action);
          open_softirq(HI_SOFTIRQ, tasklet_hi_action);
void __init q3f_init_irq(void) //q3f中断子系统初始化
     gic_init(0, 29, IO_ADDRESS(IMAP_GIC_DIST_BASE),IO_ADDRESS(IMAP_GIC_CPU_BASE));
          gic_init_bases(nr, start, dist, cpu, 0, NULL);
               hwirq_base = 16; //index 0～15对应的IRQ无效, 之后线性映射.
               gic_irqs -= hwirq_base; /* calculate # of irqs to allocate */
               irq_base = irq_alloc_descs(irq_start, 16, gic_irqs, numa_node_id());
               gic->domain = irq_domain_add_legacy(node, gic_irqs, irq_base,hwirq_base, &gic_irq_domain_ops, gic);
                    domain = irq_domain_alloc(of_node, IRQ_DOMAIN_MAP_LEGACY, ops, host_data);
                    irq_domain_add(domain);          return domain;
               set_handle_irq(gic_handle_irq);
                    handle_arch_irq = handle_irq; //ARM的IRQ异常直接调用.
               gic_dist_init(gic); gic_cpu_init(gic);  gic_pm_init(gic);
static asmlinkage void __exception_irq_entry gic_handle_irq(struct pt_regs *regs)
     irqstat = readl_relaxed(cpu_base + GIC_CPU_INTACK);  irqnr = irqstat & ~0x1c00;
     if (likely(irqnr > 15 && irqnr < 1021)) {
          irqnr = irq_find_mapping(gic->domain, irqnr);
          handle_IRQ(irqnr, regs);
               struct pt_regs *old_regs = set_irq_regs(regs);  irq_enter(); //上下文
               generic_handle_irq(irq);
                    struct irq_desc *desc = irq_to_desc(irq);
                    generic_handle_irq_desc(irq, desc);
                         desc->handle_irq(irq, desc);
               irq_exit();                set_irq_regs(old_regs);
     }
     if (irqnr < 16) writel_relaxed(irqstat, cpu_base + GIC_CPU_EOI);
//下面两个函数设置irq的handle_irq,一般为handle_level_irq 或 handle_edge_irq
static inline void __irq_set_handler_locked(unsigned int irq, irq_flow_handler_t handler)
     struct irq_desc *desc = irq_to_desc(irq);
     desc->handle_irq = handler;
static inline void
__irq_set_chip_handler_name_locked(unsigned int irq, struct irq_chip *chip,irq_flow_handler_t handler, const char *name)
     struct irq_desc *desc = irq_to_desc(irq);
     irq_desc_get_irq_data(desc)->chip = chip;
     desc->handle_irq = handler;
     desc->name = name;
void handle_level_irq(unsigned int irq, struct irq_desc *desc)//电平触发中断的handler处理过程
     raw_spin_lock(&desc->lock);     //防止其他cpu访问中断
     mask_ack_irq(desc);  //ack中断,mask中断防止level反复触发中断
     desc->istate &= ~(IRQS_REPLAY | IRQS_WAITING);     kstat_incr_irqs_this_cpu(irq, desc);
     handle_irq_event(desc);
          desc->istate &= ~IRQS_PENDING;
          irqd_set(&desc->irq_data, IRQD_IRQ_INPROGRESS); raw_spin_unlock(&desc->lock);
          ret = handle_irq_event_percpu(desc, action);
               res = action->handler(irq, action->dev_id);
               if(IRQ_WAKE_THREAD == res) irq_wake_thread(desc, action); -->wake_up_process(action->thread);
          raw_spin_lock(&desc->lock);     irqd_clear(&desc->irq_data, IRQD_IRQ_INPROGRESS);
     cond_unmask_irq(desc);      //设备驱动应该已经clear外设中断,这里unmask中断,重新开始检测新的外设中断.
     raw_spin_unlock(&desc->lock);

Q3F GPIO中断控制, 范例:
GPIO这样就同时具备中断控制器的功能,所以需要单独注册一个irq_domain,建立相关irq映射等.

static struct platform_driver q3f_gpiochip_driver = {
 .probe = q3f_gpiochip_probe,.driver = {.name = "imap-gpiochip",.owner = THIS_MODULE,},
};
struct irq_chip q3f_irq_chip = {
 .name = "q3f-irqchip", .irq_ack = q3f_gpio_irq_ack, .irq_mask = q3f_gpio_irq_mask,
 .irq_unmask = q3f_gpio_irq_unmask, .irq_set_type = q3f_gpio_irq_set_type,
};
struct imapx_gpio_chip imapx_gpio_chip[] = {
 [0] = { .compatible = "apollo2", .chip = &q3f_gpio_chip, .irq_chip = &q3f_irq_chip, .bankcount = 8, .bank = q3f_gpio_bank,
}, };
static int __init q3f_gpiochip_init(void)
     return platform_driver_register(&q3f_gpiochip_driver);

static int q3f_gpiochip_probe(struct platform_device *pdev)
     igc = devm_kzalloc(dev, sizeof(struct imapx_gpio_chip), GFP_KERNEL);
     igc = imapx_gpiochip_get(pdata->compatible); // imapx_gpio_chip 结构
     chip = igc->chip; irq_chip = igc->irq_chip; bank = igc->bank;
     irq_domain = irq_domain_add_linear(pdev->dev.of_node,chip->ngpio,&irq_domain_simple_ops,NULL);
     clk_get_sys(); clk_prepare_enable(); platform_get_resource(); devm_request_mem_region(); devm_ioremap();
     platform_set_drvdata(pdev, igc);
     gpiochip_add(chip);
     foreach gpio:
          int irq = irq_create_mapping(irq_domain, gpio);
          irq_set_lockdep_class(irq, &gpio_lock_class);
          irq_set_chip_data(irq, chip);
          irq_set_chip_and_handler(irq, irq_chip, handle_simple_irq);
          set_irq_flags(irq, IRQF_VALID);
     foreach bank:
          irq_set_chained_handler(bank[i].irq, q3f_gpio_irq_handler);
          irq_set_handler_data(bank[i].irq, &bank[i]);

manage.c

int request_threaded_irq(unsigned int irq, irq_handler_t handler,irq_handler_t thread_fn, unsigned long irqflags,
                    const char *devname, void *dev_id)
     struct irq_desc* desc = irq_to_desc(irq);//获取中断描述资源
     struct irqaction* action = kzalloc(sizeof(struct irqaction), GFP_KERNEL);
     action->handler = handler; action->thread_fn = thread_fn; action->flags = irqflags;
     action->name = devname; action->dev_id = dev_id;
     retval = __setup_irq(irq, desc, action);
