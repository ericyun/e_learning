时钟事件
clockevents.c

struct clock_event_device {
    void        (*event_handler)(struct clock_event_device *);//中断处理函数中调用此回调函数
    int         (*set_next_event)(unsigned long evt,struct clock_event_device *);//设置下次event触发时间,cycle单位
    int         (*set_next_ktime)(ktime_t expires,struct clock_event_device *);//设置下次event触发时间,ktime单位
    ktime_t         next_event;
    u64         max_delta_ns;  u64         min_delta_ns;//可设置的最大最小时间差
    u32         mult;  u32         shift; //时间计算参数
    enum clock_event_mode   mode;//单次触发和周期触发两种模式
    unsigned int        features;
    unsigned long       retries;

    void            (*broadcast)(const struct cpumask *mask);
    void            (*set_mode)(enum clock_event_mode mode,
                        struct clock_event_device *);
    unsigned long       min_delta_ticks;
    unsigned long       max_delta_ticks;

    const char      *name;
    int         rating;//时间精度
    int         irq;
    const struct cpumask    *cpumask;
    struct list_head    list;//通过此节点挂载在全局clockevent_devices链表上.
} ____cacheline_aligned;
全局的链表
static LIST_HEAD(clockevent_devices);
static LIST_HEAD(clockevents_released);

static struct clock_event_device cmn_clockevent = {
     .shift = 32,.features = CLOCK_EVT_FEAT_PERIODIC,.set_mode = q3f_cmn_set_mode,
     .rating = 250,     .cpumask = cpu_all_mask,
};

static struct irqaction cmn_timer_irq = {
.name = "cmn-timer0",     .flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
.handler = q3f_cmn_timer_interrupt,     .dev_id = &cmn_clockevent,
};

MACHINE_START(IMAPX15, "iMAPx15") //Q3F一些基本的定义, mach-q3f.c
  .nr = 0x8f9,
  .atag_offset = 0x100,
  .smp = smp_ops(q3f_smp_ops),
  .init_early = q3f_init_early,
  .init_irq = q3f_init_irq,
  .map_io = q3f_map_io,
  .init_machine = q3f_machine_init,
  .init_time = q3f_init_time,
  .init_late = q3f_init_late,
  .restart = q3f_restart,
  .reserve = q3f_reserve,
MACHINE_END
void __init start_kernel(void)
     timekeeping_init();
          read_persistent_clock(&now);//读取当前时间
          read_boot_clock(&boot);
          clock = clocksource_default_clock();
          tk_setup_internals(tk, clock);
          tk_set_xtime(tk, &now); //当前时间设置到xtime
          set_normalized_timespec(&tmp, -boot.tv_sec, -boot.tv_nsec);
          tk_set_wall_to_mono(tk, tmp);
          tk_set_sleep_time(tk, (0,0));
     time_init();
          if (machine_desc->init_time)   machine_desc->init_time(); //q3f_init_time
     sched_clock_postinit();
static void __init q3f_init_time(void)
     imapx_clock_init();
          imapx_osc_clk_init();imapx_pll_clk_init();imapx_cpu_clk_init();imapx_dev_clk_init();
          imapx_bus_clk_init();imapx_vitual_clk_init();imapx_init_infotm_conf();imapx_set_default_parent();
          register_syscore_ops(&clk_syscore_ops);
     module_power_on(SYSMGR_CMNTIMER_BASE);
     q3f_gtimer_init(IO_ADDRESS(SCU_GLOBAL_TIMER), "timer-ca5");//初始化并注册一个clocksource
          q3f_gtm = kzalloc(sizeof(struct clocksource_gtm), GFP_KERNEL);
          q3f_gtm->reg = reg;q3f_gtm->cs.name = name;q3f_gtm->cs.rating = 300;
          q3f_gtm->cs.read = q3f_gtimer_read;
          q3f_gtimer_start(&q3f_gtm->cs, 0);
          return clocksource_register_hz(&q3f_gtm->cs, rate);//通知内核工作频率/回调函数/时间精度/io地址等信息
               return __clocksource_register_scale(cs, 1, hz);
                    __clocksource_updatefreq_scale(cs, scale, freq);
                    clocksource_enqueue(cs);//按照rating挂在clocksource list上,rating越大越靠前
                    clocksource_enqueue_watchdog(cs);//每0.5s检查clocksource误差,判定其是否稳定可用
                    clocksource_select();//切换到最好的clocksource,通知timekeeper
                         if (curr_clocksource != best){curr_clocksource = best;timekeeping_notify(curr_clocksource);}
     q3f_cmn_timer_init(IO_ADDRESS(IMAP_TIMER_BASE),GIC_CMNT0_ID, "imap-cmn-timer");
          struct clock_event_device *evt = &cmn_clockevent;
          long rate = q3f_cmn_get_clock_rate(name);
          clkevt_base = base;  clkevt_reload = DIV_ROUND_CLOSEST(rate, HZ);
          evt->name = name; evt->irq = irq; evt->mult = div_sc(rate, NSEC_PER_SEC, evt->shift);
          setup_irq(irq, &cmn_timer_irq); //中断处理函数q3f_cmn_timer_interrupt(),传入参数 cmn_clockevent
          clockevents_register_device(evt);
               list_add(&dev->list, &clockevent_devices);
               clockevents_do_notify(CLOCK_EVT_NOTIFY_ADD, dev); -->notifier_call_chain();
                    foreach struct notifier_block *nb: ret = nb->notifier_call(nb, val, v);
               clockevents_notify_released();
               /*如果上层软件想要使用新的clock event device的话（clockevents_do_notify函数中有可能会进行此操作），它会调用clockevents_exchange_device函数（可以参考上面的描述）。这时候，旧的clock event会被从clockevent_devices链表中摘下，挂到clockevents_released队列中。在clockevents_notify_released函数中，会将old clock event device重新挂入clockevent_devices，并调用tick_check_new_device函数。*/
     q3f_twd_init();//cortex-a5 内部wdt?

void clockevents_config(struct clock_event_device *dev, u32 freq)

tick-common.c

DEFINE_PER_CPU(struct tick_device, tick_cpu_device);
static struct notifier_block tick_notifier = {
     .notifier_call = tick_notify,
};
void __init start_kernel(void)-->tick_init();
     clockevents_register_notifier(&tick_notifier);//注册tick_notify()回调函数
     tick_broadcast_init();

struct tick_device {//对clock_event_device非常简单的封装
     struct clock_event_device *evtdev;
     enum tick_device_mode mode; //TICKDEV_MODE_PERIODIC  TICKDEV_MODE_ONESHOT
};
static int tick_notify(struct notifier_block *nb, unsigned long reason,void *dev)
     case CLOCK_EVT_NOTIFY_ADD:     return tick_check_new_device(dev);
static int tick_check_new_device(struct clock_event_device *newdev)
     //一系列的判断,cpu原来没有或者新的更好(包括精度和触发模式比较),就用新的.
     clockevents_exchange_device(curdev, newdev);
     tick_setup_device(td, newdev, cpu, cpumask_of(cpu));//捆绑当前cpu的tick_device和newdev

void tick_setup_periodic(struct clock_event_device *dev, int broadcast)
     tick_set_periodic_handler(dev, broadcast);
          dev->event_handler = tick_handle_periodic;//clock_event_device中断处理函数中调用.
     clockevents_set_mode(dev, CLOCK_EVT_MODE_PERIODIC);
     or clockevents_set_mode(dev, CLOCK_EVT_MODE_ONESHOT);
void tick_handle_periodic(struct clock_event_device *dev)
     tick_periodic(cpu);
     if (dev->mode != CLOCK_EVT_MODE_ONESHOT) return; //后续为ONESHOT模式处理
     next = ktime_add(dev->next_event, tick_period);
     if (!clockevents_program_event(dev, next, false) return;
     if (timekeeping_valid_for_hres()) tick_periodic(cpu);
static void tick_periodic(int cpu)
     if (tick_do_timer_cpu == cpu)     do_timer(1);
     update_process_times(user_mode(get_irq_regs()));
          int cpu = smp_processor_id();
          account_process_tick(p, user_tick);//更新进程时间统计信息.
          run_local_timers();//触发软中断,处理传统的低分辨率timer
               hrtimer_run_queues();//高精度接口
               raise_softirq(TIMER_SOFTIRQ); //run_timer_softirq()
          scheduler_tick();//触发调度系统进行进程统计和调度

          run_posix_cpu_timers(p);
     profile_tick(CPU_PROFILING);

timer.c
struct timer_list {
     /*All fields that change during normal runtime grouped to the same cacheline */
     struct list_head entry; //接入定时器链表的节点
     unsigned long expires;     //到期时刻的jiffies
     struct tvec_base *base;
     void (*function)(unsigned long);//定时器挂载的操作
     unsigned long data;
     int slack;
};
void __init init_timers(void)
     err = timer_cpu_notify(&timers_nb, (unsigned long)CPU_UP_PREPARE,(void *)(long)smp_processor_id());
     init_timer_stats();
     register_cpu_notifier(&timers_nb);
     open_softirq(TIMER_SOFTIRQ, run_timer_softirq);
static void run_timer_softirq(struct softirq_action *h)
     hrtimer_run_pending();
     if (hrtimer_hres_active())     return;
     if (tick_check_oneshot_change(!hrtimer_is_hres_enabled()))
         hrtimer_switch_to_hres();
     if (time_after_eq(jiffies, base->timer_jiffies))  __run_timers(base);
int tick_check_oneshot_change(int allow_nohz)
     tick_nohz_switch_to_nohz();
          tick_switch_to_oneshot(tick_nohz_handler);
          ts->nohz_mode = NOHZ_MODE_LOWRES;
          hrtimer_init(&ts->sched_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
          next = tick_init_jiffy_update();
          hrtimer_set_expires(&ts->sched_timer, next);
          tick_program_event(next, 0);

