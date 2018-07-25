UART驱动乱记
疑问： amba 和 platform 区别

1个uart涉及两个driver: ａｍｂａ driver + tty driver, ａｍｂａ driver完成ｐｒｏｂｅ/remove

分层设计
ａｍｂａ_pl011.c 设备驱动层
serial_core.c ｃｏｒｅ核心层, 地位类似于watchdog_dev.c, 封装了基于tty_driver的串口设备，数据结构对应了tty的ops:
    static const struct tty_operations uart_ops
    static const struct tty_port_operations uart_port_ops

uart模块

struct uart_amba_port {//包含ｕａｒｔ dma pinctrl clk等相关内容
    struct uart_port    port;
    struct clk        *clk;
    /* Two optional pin states - default & sleep */
    struct pinctrl        *pinctrl;
    struct pinctrl_state    *pins_default;
    struct pinctrl_state    *pins_sleep;
    const struct vendor_data *vendor;
    unsigned int        dmacr;        /* dma control reg */
    unsigned int        im;        /* interrupt mask */
    unsigned int        old_status;
    unsigned int        fifosize;    /* vendor-specific */
    unsigned int        lcrh_tx;    /* vendor-specific */
    unsigned int        lcrh_rx;    /* vendor-specific */
    unsigned int        old_cr;        /* state during shutdown */
    bool            autorts;
    char            type[12];
#ifdef CONFIG_DMA_ENGINE
    /* DMA stuff */
    bool            using_tx_dma;
    bool            using_rx_dma;
    struct pl011_dmarx_data dmarx;
    struct pl011_dmatx_data    dmatx;
#endif
};
struct amba_device imap_uart0_device = {     .dev = {        .init_name = "imap-uart.0",        .platform_data = NULL,        },
    .res = {   .start = IMAP_UART0_BASE,   .end = IMAP_UART0_BASE + IMAP_UART0_SIZE - 1,  .flags = IORESOURCE_MEM,  },
    .irq = {GIC_UART0_ID},    .periphid = 0x00041011,};
void __init q3f_init_devices(void)
     amba_device_register（&imap_uart0_device, xxx）;　//触发ｐｒｏｂｅ()

static struct amba_driver pl011_driver = {
    .drv = { .name    = "uart-pl011", },   .id_table    = pl011_ids,    .probe        = pl011_probe,    .remove        = pl011_remove,
     #ifdef CONFIG_PM    .suspend    = pl011_suspend,    .resume        = pl011_resume,     #endif };
static int __init pl011_init(void)
     uart_register_driver(&amba_reg);
     amba_driver_register(&pl011_driver);    //register callback_ops, and  struct amba_id *id which will be passed to pl011_probe()
static void __exit pl011_exit(void)
     amba_driver_unregister(&pl011_driver);
     uart_unregister_driver(&amba_reg);
static int pl011_probe(struct amba_device *dev, const struct amba_id *id)
     找到空闲的amba_ports[ｉ]；分配内存给struct uart_amba_port *uap；
     uap->pinctrl相关的控制，好像是ｐｉｎ脚复用
     clk = clk_get_sys("imap-uart-core",NULL); ret = clk_prepare_enable(clk);
     uap->clk = clk_get_sys("imap-uart",NULL);ret = clk_prepare_enable(uap->clk);
     imapx800_uart_module_init(port_num);    //ｐａｄ相关控制
     uap初始化，包括多个ｍｉｓｃ参数和uap->port的ｍｅｍ，ｉｒｑ，ｏｐｓ回调等
              uap->port.line = i;         amba_ports[i] = uap;
     pl011_dma_probe(&dev->dev, uap);
     amba_set_drvdata(dev, uap);
     uart_add_one_port(&amba_reg, &uap->port);

static int pl011_hwinit(struct uart_port *port)
     clk_prepare_enable(uap->clk);
     uap->port.uartclk = clk_get_rate(uap->clk);　... ...
static int pl011_startup(struct uart_port *port) irq资源申请和寄存器初始化在ｏｐｓ中
     pl011_hwinit(port);  -- enable clock
     request_irq(uap->port.irq, pl011_int, 0, "uart-pl011", uap);
     ... ...
     pl011_dma_startup(uap);

static struct uart_amba_port *amba_ports[UART_NR];

serial_core模块
tty driver对于我们是透明的完全不需要了解的，serial_core的中间层作用正在于此，对于我们只需要了解下面４个函数就可以。需要构建其他中间层的时候可以好好学习一下。watchdog_core也是相同的作用。功能分解成最细小的逻辑功能由物理硬件实现。

struct uart_state {
    struct tty_port        port;              //ｔｔｙ设备对应的数据结构
    enum uart_pm_state    pm_state;
    struct circ_buf        xmit;
    struct uart_port    *uart_port;     //物理设备对应的数据结构, uart_add_one_port()函数注册};
struct uart_driver {
    struct module        *owner;                //THIS_MODULE
    const char        *driver_name;           //"ttyAMA"
    const char        *dev_name;               //"ttyAMA"
    int             major;                                  //SERIAL_AMBA_MAJOR : 204
    int             minor;                                  //SERIAL_AMBA_MINOR : 64
    int             nr;                                         //UART_NR : 4
    struct console        *cons;
    struct uart_state    *state;
    struct tty_driver    *tty_driver;
} amba_reg;
int uart_register_driver(struct uart_driver *drv)
     drv->state = kzalloc(sizeof(struct uart_state) * drv->nr, GFP_KERNEL);　//为每个ｐｏｒｔ分配一个struct uart_state
     drv->tty_driver = alloc_tty_driver(drv->nr);
     init drv->tty_driver;
     tty_set_operations(drv->tty_driver, &uart_ops);
         //uart_ops中函数在serial_core模块中实现，实现抽象的uart功能，定义如下static const struct tty_operations uart_ops
     foreach port {     tty_port_init(&struct uart_state.port);         set uart_port_ops for port    }
         //init tty_driver 的 struct tty_port
     tty_register_driver（drv->tty_driver）；
     put_tty_driver（drv->tty_driver）；
void uart_unregister_driver(struct uart_driver *drv)
     struct tty_driver *p = drv->tty_driver;
     tty_unregister_driver(p);     put_tty_driver(p);
     foreach port {tty_port_destroy(&drv->state[i].port);}
     kfree(drv->state);     drv->state = NULL;    drv->tty_driver = NULL;
int uart_add_one_port(struct uart_driver *drv, struct uart_port *uport)
     关联uport和drv->state ： struct uart_state *state
     init uport
     uart_configure_port(drv, state, uport);等其他初始化动作
         port->ops->config_port(port, flags); -- pl011_config_port（）
              request_mem_region(port->mapbase, SZ_4K, "uart-pl011")
int uart_remove_one_port(struct uart_driver *drv, struct uart_port *uport)
     取消关联uport和drv->state
     tty_unregister_device(drv->tty_driver, uport->line);

几个关键数据结构
static const struct tty_operations 　           uart_ops；
static const struct tty_port_operations     uart_port_ops;
static struct uart_ops                                     amba_pl011_pops;

static struct uart_driver                                 amba_reg;
static struct amba_driver                               pl011_driver;

struct amba_device                                         imap_uart0_device;
struct amba_device                                         imap_uart３_device;
     包含uart line ｉｄ(0/3), MEM开始结束地址，中断号和periphid等resource info
     amba_device_register（）函数在q3f_init_devices（）中添加ａｍｂａ ｄｅｖｉｃｅ

uart类型tty的操作级别的行为

static const struct tty_operations uart_ops = {
    .open        = uart_open,    .close        = uart_close,    .write        = uart_write,    .put_char    = uart_put_char,
    .flush_chars    = uart_flush_chars,    .write_room    = uart_write_room,    .chars_in_buffer= uart_chars_in_buffer,
    .flush_buffer    = uart_flush_buffer,    .ioctl        = uart_ioctl,    .throttle    = uart_throttle,    .unthrottle    = uart_unthrottle,
    .send_xchar    = uart_send_xchar,    .set_termios    = uart_set_termios,    .set_ldisc    = uart_set_ldisc,
    .stop        = uart_stop,    .start        = uart_start,    .hangup        = uart_hangup,    .break_ctl    = uart_break_ctl,
    .wait_until_sent= uart_wait_until_sent,
#ifdef CONFIG_PROC_FS
    .proc_fops    = &uart_proc_fops,
#endif
    .tiocmget    = uart_tiocmget,    .tiocmset    = uart_tiocmset,    .get_icount    = uart_get_icount,
#ifdef CONFIG_CONSOLE_POLL
    .poll_init    = uart_poll_init,    .poll_get_char    = uart_poll_get_char,    .poll_put_char    = uart_poll_put_char,
#endif
};

uart类型tty的设备级别的行为
static const struct tty_port_operations uart_port_ops = {
    .activate    = uart_port_activate,
    .shutdown    = uart_port_shutdown,
    .carrier_raised = uart_carrier_raised,
    .dtr_rts    = uart_dtr_rts,
};

static struct uart_driver amba_reg = {
    .owner            = THIS_MODULE,
    .driver_name        = "ttyAMA",
    .dev_name        = "ttyAMA",
    .major            = SERIAL_AMBA_MAJOR,
    .minor            = SERIAL_AMBA_MINOR,
    .nr            = UART_NR,
    .cons            = AMBA_CONSOLE,
};
uart_register_driver(&amba_reg);

static struct vendor_data vendor_arm = {
    .ifls            = UART011_IFLS_RX4_8|UART011_IFLS_TX4_8,
    .lcrh_tx        = UART011_LCRH,
    .lcrh_rx        = UART011_LCRH,
    .oversampling        = false,
    .dma_threshold        = false,
    .cts_event_workaround    = false,
    .get_fifosize        = get_fifosize_arm,
};
static struct amba_id pl011_ids[] = {
    {
        .id    = 0x00041011,
        .mask    = 0x000fffff,
        .data    = &vendor_arm,
    },
    {
        .id    = 0x00380802,
        .mask    = 0x00ffffff,
        .data    = &vendor_st,
    },
    { 0, 0 },
};
static struct amba_driver pl011_driver = {
    .drv = {
        .name    = "uart-pl011",
    },
    .id_table    = pl011_ids,
    .probe        = pl011_probe,
    .remove        = pl011_remove,
#ifdef CONFIG_PM
    .suspend    = pl011_suspend,
    .resume        = pl011_resume,
#endif
};

了解这几个函数的基本流程就可以了。
static struct uart_ops amba_pl011_pops = {
    .tx_empty    = pl011_tx_empty,     //串口的Tx FIFO缓存是否为空
    .set_mctrl    = pl011_set_mctrl,     //设置串口modem控制, 需要把控制ｂｉｔ映射到寄存器ｂｉｔ
    .get_mctrl    = pl011_get_mctrl,     //获取串口modem控制
    .stop_tx    = pl011_stop_tx,     //禁止串口发送数据
    .start_tx    = pl011_start_tx,     //使能串口发送数据
    .stop_rx    = pl011_stop_rx,     //禁止串口接收数据
    .enable_ms    = pl011_enable_ms,     //使能modem的状态信号
    .break_ctl    = pl011_break_ctl,     //设置break信号
    .startup    = pl011_startup,     //启动串口,应用程序打开串口设备文件时,该函数会被调用
    .shutdown    = pl011_shutdown,     //关闭串口,应用程序关闭串口设备文件时,该函数会被调用
    .flush_buffer    = pl011_dma_flush_buffer,     //
    .set_termios    = pl011_set_termios,     //设置串口参数
    .type        = pl0     11_type,     //返回一描述串口类型的字符串
    .release_port    = pl011_release_port,     //释放串口已申请的IO端口/IO内存资源,必要时还需iounmap
    .request_port    = pl011_request_port,     //申请必要的IO端口/IO内存资源,必要时还可以重新映射串口端口
    .config_port    = pl011_config_port,     //执行串口所需的自动配置
    .verify_port    = pl011_verify_port,     //核实新串口的信息
#ifdef CONFIG_CONSOLE_POLL
    .poll_init     = pl011_hwinit,
    .poll_get_char = pl011_get_poll_char,
    .poll_put_char = pl011_put_poll_char,
#endif
};

struct uart_port {
    spinlock_t        lock;            /* port lock */
    unsigned long        iobase;            /* in/out[bwl] */
    unsigned char __iomem    *membase;        /* read/write[bwl] */
    unsigned int        (*serial_in)(struct uart_port *, int);
    void            (*serial_out)(struct uart_port *, int, int);
    void            (*set_termios)(struct uart_port *,
                               struct ktermios *new,
                               struct ktermios *old);
    int            (*handle_irq)(struct uart_port *);
    void            (*pm)(struct uart_port *, unsigned int state,
                      unsigned int old);
    void            (*handle_break)(struct uart_port *);
    unsigned int        irq;            /* irq number */
    unsigned long        irqflags;        /* irq flags  */
    unsigned int        uartclk;        /* base uart clock */
    unsigned int        fifosize;        /* tx fifo size */
    unsigned char        x_char;            /* xon/xoff char */
    unsigned char        regshift;        /* reg offset shift */
    unsigned char        iotype;            /* io access style */
    unsigned char        unused1;

#define UPIO_PORT        (0)
... ...
#define UPIO_TSI        (5)            /* Tsi108/109 type IO */

    unsigned int        read_status_mask;    /* driver specific */
    unsigned int        ignore_status_mask;    /* driver specific */
    struct uart_state    *state;            /* pointer to parent state */
    struct uart_icount    icount;            /* statistics */

    struct console        *cons;            /* struct console, if any */
#if defined(CONFIG_SERIAL_CORE_CONSOLE) || defined(SUPPORT_SYSRQ)
    unsigned long        sysrq;            /* sysrq timeout */
#endif

    upf_t            flags;

#define UPF_FOURPORT        ((__force upf_t) (1 << 1))
... ...
#define UPF_IOREMAP        ((__force upf_t) (1 << 31))

#define UPF_CHANGE_MASK        ((__force upf_t) (0x17fff))
#define UPF_USR_MASK        ((__force upf_t) (UPF_SPD_MASK|UPF_LOW_LATENCY))

    unsigned int        mctrl;            /* current modem ctrl settings */
    unsigned int        timeout;        /* character-based timeout */
    unsigned int        type;            /* port type */
    const struct uart_ops    *ops;
    unsigned int        custom_divisor;
    unsigned int        line;            /* port index */
    resource_size_t        mapbase;        /* for ioremap */
    struct device        *dev;            /* parent device */
    unsigned char        hub6;            /* this should be in the 8250 driver */
    unsigned char        suspended;
    unsigned char        irq_wake;
    unsigned char        unused[2];
    void            *private_data;        /* generic platform data pointer */
};

