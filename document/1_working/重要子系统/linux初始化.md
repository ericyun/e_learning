## linux初始化过程

###  修订记录
| 修订说明 | 日期 | 作者 | 额外说明 |
| --- |
| 初版 | 2018/04/10 | 员清观 |  |

## 1 启动过程加速优化

**linux开机启动优化补丁**<br>
`patch -p1 < patches/time_analyze.patch`

跟踪 module_init() 调用方式
```cpp
kernel/include/init.h中
    typedef int (*initcall_t)(void);
    typedef void (*exitcall_t)(void);

    #define __define_initcall(fn, id) \
    static initcall_t __initcall_##fn##id __used \
    __attribute__((__section__(".initcall" #id ".init"))) = fn

//“##”符号可以是连接的意思,例如 __initcall_##fn##id 为__initcall_fnid, 那么，fn = test_init，id = 6时，__initcall_##fn##id 为 __initcall_test_init6
//“#”符号可以是字符串化的意思,例如 #id 为 “id”，id=6 时，#id 为“6”
//__used
//__attribute__
//__section__

那么module_init(test_init) 可以解析为：
    static initcall_t __initcall_test6 __used __attribute__((__section__(".initcall""6" ".init"))) =test_init
通过__attribute__（__section__）设置函数属性，也就是将test_init放在.initcall6.init段中

arch/arm/kernel/vmlinux.lds这个链接脚本中

    /*
     * Early initcalls run before initializing SMP.
     * Only for built-in code, not modules.
     */
    #define early_initcall(fn)		__define_initcall(fn, early)

    #define pure_initcall(fn)		__define_initcall(fn, 0)
    #define core_initcall(fn)		__define_initcall(fn, 1)
    #define core_initcall_sync(fn)		__define_initcall(fn, 1s)
    #define postcore_initcall(fn)		__define_initcall(fn, 2)
    #define postcore_initcall_sync(fn)	__define_initcall(fn, 2s)
    #define arch_initcall(fn)		__define_initcall(fn, 3)
    #define arch_initcall_sync(fn)		__define_initcall(fn, 3s)
    #define subsys_initcall(fn)		__define_initcall(fn, 4)
    #define subsys_initcall_sync(fn)	__define_initcall(fn, 4s)
    #define fs_initcall(fn)			__define_initcall(fn, 5)
    #define fs_initcall_sync(fn)		__define_initcall(fn, 5s)
    #define rootfs_initcall(fn)		__define_initcall(fn, rootfs)
    #define device_initcall(fn)		__define_initcall(fn, 6)
    #define device_initcall_sync(fn)	__define_initcall(fn, 6s)
    #define late_initcall(fn)		__define_initcall(fn, 7)
    #define late_initcall_sync(fn)		__define_initcall(fn, 7s)

    #define device_initcall(fn)		__define_initcall(fn, 6)
    #define __initcall(fn) device_initcall(fn)
    #define module_init(x)	__initcall(x);

    module_init(fn)---> __initcall(fn) ---> device_initcall(fn) ---> __define_initcall(fn, 6)
kernel/init/main.c中
    static void __init do_initcall_level(int level)
    {
        extern const struct kernel_param __start___param[], __stop___param[];
        initcall_t *fn;
        int t, _t;

        strcpy(static_command_line, saved_command_line);
        parse_args(initcall_level_names[level],
               static_command_line, __start___param,
               __stop___param - __start___param,
               level, level,
               &repair_env_string);

        f0r (fn = initcall_levels[level]; fn < initcall_levels[level+1]; fn++) {
            _t = __getms();
            do_one_initcall(*fn);
            if((t = __getms() - _t) > 0)
                printk(KERN_ALERT "%pf: %dms\n", *fn, t);
        }
    }
    static void __init do_initcalls(void)
    {
        int level;
        f0r (level = 0; level < ARRAY_SIZE(initcall_levels) - 1; level++)
            do_initcall_level(level);
    }
```
**kernel初始化过程解析**
```cpp
asmlinkage void __init start_kernel(void)
    从 lockdep_init(); 到 ftrace_init(); // 基本的初始化，没有打印出来时间。
    rest_init();
        rcu_scheduler_starting();
        kernel_thread(kernel_init, NULL, CLONE_FS | CLONE_SIGHAND);
            kernel_init_freeable();
                从wait_for_completion(&kthreadd_done);到sched_init_smp();
                do_basic_setup();
                    从cpuset_init_smp();到usermodehelper_enable();
                    do_initcalls();
                从sys_open((const char __user *) "/dev/console", O_RDWR, 0)到load_default_modules();
            从...到run_init_process()，启动脚本，比如/etc/init中的脚本，未打印时间。
        cpu_startup_entry(CPUHP_ONLINE);
```

## 2

## 3

## 4

## 5
### 5.1

## 6
