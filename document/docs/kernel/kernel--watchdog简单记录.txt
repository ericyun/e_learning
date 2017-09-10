WATCHDOG简单记录
﻿struct imapx_wdt_chip *wdt;    设备对象

imapx_wdt_probe()
    struct imapx_wdt_chip *wdt 分配内存和初始化
    platform_set_drvdata(pdev, wdt);
    platform_get_resource [ IORESOURCE_MEM & IORESOURCE_IRQ ]
    devm_ioremap_resource
    imapx_wdt_stop(&wdt->wdd);
    imapx_wdt_set_heartbeat
    devm_request_irq
    watchdog_register_device(&wdt->wdd);
    imapx_wdt_stop(&wdt->wdd);

struct watchdog_device {
    int id;
    struct cdev cdev;
    struct device *dev;
    struct device *parent;
    const struct watchdog_info *info;
    const struct watchdog_ops *ops;
    unsigned int bootstatus;
    unsigned int timeout;
    unsigned int min_timeout;
    unsigned int max_timeout;
    void *driver_data;
    struct mutex lock;
    unsigned long status;
/* Bit numbers for status flags */
#define WDOG_ACTIVE        0    /* Is the watchdog running/active */
#define WDOG_DEV_OPEN        1    /* Opened via /dev/watchdog ? */
#define WDOG_ALLOW_RELEASE    2    /* Did we receive the magic char ? */
#define WDOG_NO_WAY_OUT        3    /* Is 'nowayout' feature set ? */
#define WDOG_UNREGISTERED    4    /* Has the device been unregistered */
};

struct watchdog_device *wdd     device对象
struct device *dev
mutex_init mutex_lock mutex_unlock
put_user container_of

watchdog_core.c 中，subsys_initcall(watchdog_init);
watchdog_init_timeout()
    wdd->timeout = timeout_parm;
watchdog_register_device(struct watchdog_device *wdd)
    wdd->id = ida_simple_get()
    watchdog_dev_register()
    wdd->dev = device_create(watchdog_class, wdd->parent, wdd->cdev.dev)
watchdog_unregister_device()
    watchdog_dev_unregister()
    device_destroy(watchdog_class, wdd->cdev.dev);
    ida_simple_remove(wdd->id)
watchdog_init()
    watchdog_class = class_create
    watchdog_dev_init()
watchdog_exit
    watchdog_dev_exit()
    class_destroy(watchdog_class);
    ida_destroy(&watchdog_ida);

wddev->ops 对应
    static struct watchdog_ops imapx_wdt_ops = {
    .owner = THIS_MODULE,
    .start = imapx_wdt_start,
    .stop = imapx_wdt_stop,
    .ping = imapx_wdt_keepalive,
    .set_timeout = imapx_wdt_set_heartbeat,
    .status = imapx_wdt_get_status,
    };
watchdog_dev.c
watchdog_ping()
    wddev->ops->ping or wddev->ops->start
watchdog_start
    wddev->ops->start
watchdog_stop
    wddev->ops->stop
watchdog_get_status
    wddev->ops->status
watchdog_set_timeout
    wddev->ops->set_timeout
watchdog_get_timeleft
    wddev->ops->get_timeleft
watchdog_ioctl_op
    wddev->ops->ioctl
watchdog_write
    watchdog_ping
static long watchdog_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
watchdog_open

watchdog_release

watchdog_dev_register
    misc_register     cdev_init  cdev_add
watchdog_dev_unregister
    cdev_del

