时间子系统
相关的硬件, RTC,时钟源,时钟事件

static struct notifier_block __cpuinitdata hrtimers_nb = {
     .notifier_call = hrtimer_cpu_notify,
};//时间子系统的notify类似于驱动部分的probe函数,注册了新的clocksource或者clockevent的时候调用.所以初始化的时候总是要定义notify函数.

void __init start_kernel(void) //内核初始化中timer相关部分
     tick_init();
     init_timers();
     hrtimers_init();
     softirq_init();
     timekeeping_init();
     time_init();

static __init int init_posix_cpu_timers(void)
     posix_timers_register_clock(CLOCK_PROCESS_CPUTIME_ID, &process);
     posix_timers_register_clock(CLOCK_THREAD_CPUTIME_ID, &thread);
     cputime_to_timespec(cputime_one_jiffy, &ts);
     onecputick = ts.tv_nsec;
static __init int init_posix_timers(void)  //posix_timers.c
     struct k_clock clock_realtime = {
         .clock_getres = hrtimer_get_res,
         .clock_get = posix_clock_realtime_get,
         .clock_set = posix_clock_realtime_set,
         .clock_adj = posix_clock_realtime_adj,
         .nsleep = common_nsleep,
         .nsleep_restart = hrtimer_nanosleep_restart,
         .timer_create = common_timer_create,
         .timer_set = common_timer_set,
         .timer_get = common_timer_get,
         .timer_del = common_timer_del,
     };
     posix_timers_register_clock(CLOCK_REALTIME, &clock_realtime);
void __init timekeeping_init(void)
     read_persistent_clock(&now); persistent_clock_exist = true;//假定read_persistent_clock()返回有效值
     read_boot_clock(&boot);
     clock = clocksource_default_clock();
     if (clock->enable)     clock->enable(clock);
     tk_setup_internals(tk, clock);
     tk_set_xtime(tk, &now);//timekeeping realtime
     tk->raw_time.tv_sec = 0; tk->raw_time.tv_nsec = 0;//raw time 从0开始
     if (boot.tv_sec == 0 && boot.tv_nsec == 0)
         boot = tk_xtime(tk); －－－如果没有获取到有效的booting time，那么就选择当前的real time clock
     set_normalized_timespec(&tmp, -boot.tv_sec, -boot.tv_nsec);
     //启动时将monotonic clock设定为负的real time clock，timekeeper并没有直接保存monotonic clock，而是保存了一个wall_to_monotonic的值，这个值类似offset，real time clock加上这个offset就可以得到monotonic clock。因此，初始化的时间点上，monotonic clock实际上等于0（如果没有获取到有效的booting time）。当系统运行之后，real time clock+ wall_to_monotonic是系统的uptime，而real time clock+ wall_to_monotonic + sleep time也就是系统的boot time。

struct clocksource {
     cycle_t (*read)(struct clocksource *cs);
cycle_t cycle_last;
cycle_t mask;
     u32 mult;//t = (cycle * mult) >> shift; clocksource_cyc2ns()函数中计算
     u32 shift;
u64 max_idle_ns;
u32 maxadj;
#ifdef CONFIG_ARCH_CLOCKSOURCE_DATA
struct arch_clocksource_data archdata;
#endif

const char *name;
struct list_head list;
int rating;
int (*enable)(struct clocksource *cs);
void (*disable)(struct clocksource *cs);
unsigned long flags;
void (*suspend)(struct clocksource *cs);
void (*resume)(struct clocksource *cs);

/* private: */
#ifdef CONFIG_CLOCKSOURCE_WATCHDOG
/* Watchdog related data, used by the framework */
struct list_head wd_list;
cycle_t cs_last;
cycle_t wd_last;
#endif
} ____cacheline_aligned;

static cycle_t jiffies_read(struct clocksource *cs)
     return (cycle_t) jiffies;
//初始化kernel时使用此clocksource作为jiffies
static struct clocksource clocksource_jiffies = { //jiffies.c
     .name = "jiffies",.rating = 1, /* lowest valid rating*/ .read = jiffies_read,.mask = 0xffffffff, /*32bits*/
     .mult = NSEC_PER_JIFFY << JIFFIES_SHIFT, /* details above */.shift = JIFFIES_SHIFT,
};

static int __init init_jiffies_clocksource(void)
     return clocksource_register(&clocksource_jiffies);

core_initcall(init_jiffies_clocksource);

时间的计算以到1970年1月1日0时(Linux Epoch)的差值.
传统的Linux使用秒计时,:   typedef long        time_t; 无法满足需要, 然后定义了微秒级别的
struct timeval {
    time_t        tv_sec;        /* seconds */
    suseconds_t    tv_usec;    /* microseconds */
};

之后进一步定义了纳秒级别的POSIX标准时间点表示:
struct timespec {
  time_t/__kernel_time_t/long      tv_sec;
  long tv_nsec;
};

面向用户的:
struct tm {
    /*the number of seconds after the minute, normally in the range 0 to 59, but can be up to 60 to allow for leap seconds*/
    int tm_sec;
    int tm_min; /* the number of minutes after the hour, in the range 0 to 59*/
    int tm_hour; /* the number of hours past midnight, in the range 0 to 23 */
    int tm_mday; /* the day of the month, in the range 1 to 31 */
    int tm_mon; /* the number of months since January, in the range 0 to 11 */
    long tm_year; /* the number of years since 1900 */ －－－－－以NTP epoch为基准点
    int tm_wday; /* the number of days since Sunday, in the range 0 to 6 */
    int tm_yday; /* the number of days since January 1, in the range 0 to 365 */
};

启动时将monotonic clock设定为负的real time clock，timekeeper并没有直接保存monotonic clock，而是保存了一个wall_to_monotonic的值，这个值类似offset，real time clock加上这个offset就可以得到monotonic clock。因此，初始化的时间点上，monotonic clock实际上等于0（如果没有获取到有效的booting time）。当系统运行之后，real time clock+ wall_to_monotonic是系统的uptime，而real time clock+ wall_to_monotonic + sleep time也就是系统的boot time。

1. jiffies
内核用jiffies变量记录系统启动以来经过的时钟滴答数，它的声明如下：
  #define __jiffy_data __attribute__((section(".data")))
  extern u64 __jiffy_data jiffies_64;
  extern unsigned long volatile __jiffy_data jiffies;

可见，在32位的系统上，jiffies是一个32位的无符号数，系统每过1/HZ秒，jiffies的值就会加1，最终该变量可能会溢出，所以内核同时又定义了一个64位的变量jiffies_64，链接的脚本保证jiffies变量和jiffies_64变量的内存地址是相同的，通常，我们可以直接访问jiffies变量，但是要获得jiffies_64变量，必须通过辅助函数get_jiffies_64来实现。jiffies是内核的低精度定时器的计时单位，所以内核配置的HZ数决定了低精度定时器的精度，如果HZ数被设定为1000，那么，低精度定时器（timer_list）的精度就是1ms=1/1000秒。因为jiffies变量可能存在溢出的问题，所以在用基于jiffies进行比较时，应该使用以下辅助宏来实现：
  time_after(a,b) time_before(a,b) time_after_eq(a,b) time_before_eq(a,b) time_in_range(a,b,c)
  unsigned int jiffies_to_msecs(const unsigned long j);
  unsigned int jiffies_to_usecs(const unsigned long j);
  unsigned long msecs_to_jiffies(const unsigned int m);
  unsigned long usecs_to_jiffies(const unsigned int u);

2. struct timeval
timeval由秒和微秒组成，它的定义如下：
  struct timeval {
  __kernel_time_t tv_sec; /* seconds */
  __kernel_suseconds_t tv_usec; /* microseconds */
  };
__kernel_time_t 和__kernel_suseconds_t 实际上都是long型的整数。gettimeofday和settimeofday使用timeval作为时间单位。
3. struct timespec
timespec由秒和纳秒组成，它的定义如下：
  struct timespec {
  __kernel_time_t tv_sec; /* seconds */
  long tv_nsec; /* nanoseconds */
  };
同样地，内核也提供了一些辅助函数用于jiffies、timeval、timespec之间的转换：
  static inline int timespec_equal(const struct timespec *a, const struct timespec *b);
  static inline int timespec_compare(const struct timespec *lhs, const struct timespec *rhs);
  static inline int timeval_compare(const struct timeval *lhs, const struct timeval *rhs);
  extern unsigned long mktime(const unsigned int year, const unsigned int mon,
       const unsigned int day, const unsigned int hour,
       const unsigned int min, const unsigned int sec);
  extern void set_normalized_timespec(struct timespec *ts, time_t sec, s64 nsec);
  static inline struct timespec timespec_add(struct timespec lhs, struct timespec rhs);
  static inline struct timespec timespec_sub(struct timespec lhs, struct timespec rhs);

  static inline s64 timespec_to_ns(const struct timespec *ts);
  static inline s64 timeval_to_ns(const struct timeval *tv);
  extern struct timespec ns_to_timespec(const s64 nsec);
  extern struct timeval ns_to_timeval(const s64 nsec);
  static __always_inline void timespec_add_ns(struct timespec *a, u64 ns);

  unsigned long timespec_to_jiffies(const struct timespec *value);
  void jiffies_to_timespec(const unsigned long jiffies, struct timespec *value);
  unsigned long timeval_to_jiffies(const struct timeval *value);
  void jiffies_to_timeval(const unsigned long jiffies, struct timeval *value);

timekeeper中的xtime字段用timespec作为时间单位。
4. struct ktime
linux的通用时间架构用ktime来表示时间，为了兼容32位和64位以及big-little endian系统，ktime结构被定义如下：
  union ktime {
  s64 tv64;
  #if BITS_PER_LONG != 64 && !defined(CONFIG_KTIME_SCALAR)
  struct {
  # ifdef __BIG_ENDIAN
  s32 sec, nsec;
  # else
  s32 nsec, sec;
  # endif
  } tv;
  #endif
  };

64位的系统可以直接访问tv64字段，单位是纳秒，32位的系统则被拆分为两个字段：sec和nsec，并且照顾了大小端的不同。高精度定时器通常用ktime作为计时单位。下面是一些辅助函数用于计算和转换：
  ktime_t ktime_set(const long secs, const unsigned long nsecs);
  ktime_t ktime_sub(const ktime_t lhs, const ktime_t rhs);
  ktime_t ktime_add(const ktime_t add1, const ktime_t add2);
  ktime_t ktime_add_ns(const ktime_t kt, u64 nsec);
  ktime_t ktime_sub_ns(const ktime_t kt, u64 nsec);
  ktime_t timespec_to_ktime(const struct timespec ts);
  ktime_t timeval_to_ktime(const struct timeval tv);
  struct timespec ktime_to_timespec(const ktime_t kt);
  struct timeval ktime_to_timeval(const ktime_t kt);
  s64 ktime_to_ns(const ktime_t kt);
  int ktime_equal(const ktime_t cmp1, const ktime_t cmp2);
  s64 ktime_to_us(const ktime_t kt);
  s64 ktime_to_ms(const ktime_t kt);
  ktime_t ns_to_ktime(u64 ns);

timekeeper提供了一系列的接口用于获取各种时间信息,应该是内核调用的接口:
  void getboottime(struct timespec *ts); 获取系统启动时刻的实时时间
  void get_monotonic_boottime(struct timespec *ts); 获取系统启动以来所经过的时间，包含休眠时间
  ktime_t ktime_get_boottime(void); 获取系统启动以来所经过的c时间，包含休眠时间，返回ktime类型
  ktime_t ktime_get(void); 获取系统启动以来所经过的c时间，不包含休眠时间，返回ktime类型
  void ktime_get_ts(struct timespec *ts) ; 获取系统启动以来所经过的c时间，不包含休眠时间，返回timespec结构
  unsigned long get_seconds(void); 返回xtime中的秒计数值
  struct timespec current_kernel_time(void); 返回内核最后一次更新的xtime时间，不累计最后一次更新至今clocksource的计数值
  void getnstimeofday(struct timespec *ts); 获取当前时间，返回timespec结构
  void do_gettimeofday(struct timeval *tv); 获取当前时间，返回timeval结构

timekeeper时间的更新
xtime一旦初始化完成后，timekeeper就开始独立于RTC，利用自身关联的clocksource进行时间的更新操作，根据内核的配置项的不同，更新时间的操作发生的频度也不尽相同，如果没有配置NO_HZ选项，通常每个tick的定时中断周期，do_timer会被调用一次，相反，如果配置了NO_HZ选项，可能会在好几个tick后，do_timer才会被调用一次，当然传入的参数是本次更新离上一次更新时相隔了多少个tick周期，系统会保证在clocksource的max_idle_ns时间内调用do_timer，以防止clocksource的溢出：
  void do_timer(unsigned long ticks)
  {
  jiffies_64 += ticks;
  update_wall_time();
  calc_global_load(ticks);
  }

从应用程序的角度看，内核需要提供的和时间相关的服务有三种：
1、和系统时间相关的服务。例如，在向数据库写入一条记录的时候，需要记录操作时间（何年何月何日何时）。
2、让进程睡眠一段时间
3、和timer相关的服务。在一段指定的时间过去后，kernel要alert用户进程

二、和系统时间相关的服务

1、秒级别的时间函数：time和stime

time和stime函数的定义如下：
  #include <time.h> time_t time(time_t *t); int stime(time_t *t);
time函数返回了当前时间点到linux epoch的秒数（内核中timekeeper模块保存了这个值，timekeeper->xtime_sec）。stime是设定当前时间点到linux epoch的秒数。对于linux kernel，设定时间的进程必须拥有CAP_SYS_TIME的权利，否则会失败。
linux kernel用系统调用sys_time和sys_stime来支持这两个函数。实际上，在引入更高精度的时间相关的系统调用之后（例如：sys_gettimeofday），上面这两个系统调用可以用新的系统调在用户空间实现time和stime函数。在kernel中，只有定义了__ARCH_WANT_SYS_TIME这个宏，系统才会提供上面这两个系统调用。当然，提供这样的系统调用多半是为了兼容旧的应用软件。
配合上面的接口函数还有一系列将当前时间点到linux epoch的秒数转换成适合人类阅读的接口函数，例如, mktime, asctime_r,
char* asctime (const struct tm * timeptr);
char *asctime_r(const struct tm *tm, char *buf);//
struct tm *localtime(const time_t *clock); //转化为本地时间,包含了时区转换
struct tm *localtime_r(const time_t *timep, struct tm *result); //线程安全的.
struct tm *gmtime(const time_t *timeptr); //转化为格林威治GMT时间
struct tm *gmtime_r(const time_t *timep, struct tm *result);
char *ctime(const time_t *time);
char *ctime_r(const time_t *timep, char *buf);
time_t mktime(struct tm * timeptr); //反向过程.
带_r的函数,应该传递有效buffer进去,结果保存在buffer中.

void main ()
{
  time_t rawtime; struct tm * timeinfo; struct tm * gm_timeinfo;
  time ( &rawtime ); timeinfo = localtime ( &rawtime ); gm_timeinfo = localtime ( &rawtime );
  printf ( "The current date/time is: %s", asctime (timeinfo) );
  printf ( "The current GMT is: %s", asctime (gm_timeinfo) );
  printf("Today'sdateandtime:%s\n",ctime(&rawtime));
}

2、微秒级别的时间函数：gettimeofday和settimeofday

  #include <sys/time.h>
  int gettimeofday(struct timeval *tv, struct timezone *tz);
  int settimeofday(const struct timeval *tv, const struct timezone *tz);
内核中不能用上述函数, void do_gettimeofday(struct timeval *tv); int do_settimeofday(const struct timespec *tv)

这两个函数和上一小节秒数的函数类似，只不过时间精度可以达到微秒级别。gettimeofday函数可以获取从linux epoch到当前时间点的秒数以及微秒数（在内核态，这个时间值仍然是通过timekeeper模块获得的，具体接口是getnstimeofday64，该接口的时间精度是纳秒级别的，不过没有关系，除以1000就获得微秒级别的精度了），settimeofday则是设定从linux epoch到当前时间点的秒数以及微秒数。同样的，设定时间的进程必须拥有CAP_SYS_TIME的权利，否则会失败。tz参数是由于历史原因而存在，实际上内核并没有对timezone进行支持。

显然，sys_gettimeofday和sys_settimeofday这两个系统调用是用来支持上面两个函数功能的，值得一提的是：这些系统调用在新的POSIX标准中 gettimeofday和settimeofday接口函数被标注为obsolescent，取而代之的是clock_gettime和clock_settime接口函数

3、纳秒级别的时间函数：clock_gettime和clock_settime

  #include <time.h>
  int clock_getres(clockid_t clk_id, struct timespec *res);
  int clock_gettime(clockid_t clk_id, struct timespec *tp);
  int clock_settime(clockid_t clk_id, const struct timespec *tp);

如果不是clk_id这个参数，clock_gettime和clock_settime基本上是不用解释的，其概念和gettimeofday和settimeofday接口函数是完全类似的，除了精度是纳秒。clock就是时钟的意思，它记录了时间的流逝。clock ID当然就是识别system clock（系统时钟）的ID了，定义如下：
  CLOCK_REALTIME
  CLOCK_MONOTONIC
  CLOCK_MONOTONIC_RAW
  CLOCK_PROCESS_CPUTIME_ID
  CLOCK_THREAD_CPUTIME_ID

根据应用的需求，内核维护了几个不同系统时钟。大家最熟悉的当然就是CLOCK_REALTIME这个系统时钟，因为它表示了真实世界的墙上时钟（前面两节的接口函数没有指定CLOCK ID，实际上获取的就是CLOCK_REALTIME的时间值）。CLOCK_REALTIME这个系统时钟允许用户对其进行设定（当然要有CAP_SYS_TIME权限），这也就表示在用户空间可以对该系统时钟进行修改，产生不连续的时间间断点。除此之外，也可以通过NTP对该时钟进行调整（不会有间断点，NTP调整的是local oscillator和上游服务器频率误差而已）。

仅仅从名字上就可以看出CLOCK_MONOTONIC的系统时钟应该是单调递增的，此外，该时钟也是真实世界的墙上时钟，只不过其基准点不一定是linux epoch（当然也可以是），一般会把系统启动的时间点设定为其基准点。随后该时钟会不断的递增。除了可以通过NTP对该时钟进行调整之外，其他任何程序不允许设定该时钟，这样也就保证了该时钟的单调性。

CLOCK_MONOTONIC_RAW具备CLOCK_MONOTONIC的特性，除了NTP调整。也就是说，clock id是CLOCK_MONOTONIC_RAW的系统时钟是一个完全基于本地晶振的时钟。不能设定，也不能对对晶振频率进行调整。

在调用clock_gettime和clock_settime接口函数时，如果传递clock id参数是CLOCK_REALTIME的话，那么这两个函数的行为和前两个小节描述的一致，除了是ns精度。读到这里，我详细广大人民群众不免要问：为何要有其他类型的系统时钟呢？MONOTONIC类型的时钟相对比较简单，如果你设定事件A之后5秒进行动作B，那么用MONOTONIC类型的时钟是一个比较好的选择，如果使用REALTIME的时钟，当用户在事件A和动作B之间插入时间设定的操作，那么你设定事件A之后5秒进行动作B将不能触发。此外，用户需要了解系统启动时间，这个需求需要使用MONOTONIC类型的时钟的时钟。需要指出的是MONOTONIC类型的时钟不是绝对时间的概念，多半是计算两个采样点之间的时间，并且保证采样点之间时间的单调性。MONOTONIC_RAW是一个底层工具，一般而言程序员不会操作它，使用MONOTONIC类型的时钟就够用了，当然，一些高级的应用场合，例如你想使用另外的方法（不是NTP）来调整时间，那么就可以使用MONOTONIC_RAW了。

有些应用场景使用real time的时钟（墙上时钟）是不合适的，例如当我们进行系统中各个应用程序的性能分析和统计的时候。正因为如此，kernel提供了基于进程或者线程的系统时钟，也就是CLOCK_PROCESS_CPUTIME_ID和CLOCK_THREAD_CPUTIME_ID了。当我们打算使用基于进程或者线程的系统时钟的时候，需要首先获取clock id：
  #include <time.h>
  int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
如果是线程的话，需要调用pthread_getcpuclockid接口函数：
  #include <pthread.h> #include <time.h>
  int pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id);
虽然这组函数接口的精度可以达到ns级别，但是实际的系统可以达到什么样的精度是实现相关的，因此，clock_getres用来获取系统时钟的精度。

4、系统时钟的调整
设定系统时间是一个比较粗暴的做法，一旦修改了系统时间，系统中的很多依赖绝对时间的进程会有各种奇奇怪怪的行为。正因为如此，系统提供了时间同步的接口函数，可以让外部的精准的计时服务器来不断的修正系统时钟。

（1）adjtime接口函数
  int adjtime(const struct timeval *delta, struct timeval *olddelta);
该函数可以根据delta参数缓慢的修正系统时钟（CLOCK_REALTIME那个）。olddelta返回上一次调整中尚未完整的delta。

（2）adjtimex
  #include <sys/timex.h> int adjtimex(struct timex *buf);
RFC 1305定义了更复杂，更强大的时间调整算法，因此linux kernel通过sys_adjtimex支持这个算法，其用户空间的接口函数就是adjtimex。由于这个算法过去强大，这里就不再赘述，等有时间、有兴趣之后再填补这里的空白吧。
Linux内核提供了sys_adjtimex系统调用来支持上面两个接口函数。此外，还提供了sys_clock_adjtime的系统调用来支持POSIX clock tunning。

三、进程睡眠
1、秒级别的sleep函数：sleep
  #include <unistd.h> unsigned int sleep(unsigned int seconds);
调用该函数会导致当前进程sleep，seconds之后（基于CLOCK_REALTIME）会返回继续执行程序。该函数的返回值说明了进程没有进入睡眠的时间。例如如果我们想要睡眠8秒，但是由于siganl中断了睡眠，只是sleep了5秒，那么返回值就是3，表示有3秒还没有睡。

2、微秒级别的sleep函数：usleep
  #include <unistd.h> int usleep(useconds_t usec);
概念上和sleep一样，不过返回值的定义不同。usleep返回0表示执行成功，返回-1说明执行失败，错误码在errno中获取。

3、纳秒级别的sleep函数：nanosleep
  #include <time.h> int nanosleep(const struct timespec *req, struct timespec *rem);
usleep函数已经是过去式，不建议使用，取而代之的是nanosleep函数。req中设定你要sleep的秒以及纳秒值，然后调用该函数让当前进程sleep。返回0表示执行成功，返回-1说明执行失败，错误码在errno中获取。EINTR表示该函数被signal打断。rem参数是remaining time的意思，也就是说还有多少时间没有睡完。

linux kernel并没有提供sleep和usleep对应的系统调用，sleep和usleep的实现位于c lib。在有些系统中，这些实现是依赖信号的，也有的系统使用timer来实现的，对于GNU系统，sleep和usleep和nanosleep函数一样，都是通过kernel的sys_nanosleep的系统调用实现的（底层是基于hrtimer）。

4、更高级的sleep函数：clock_nanosleep
  #include <time.h>
  int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);
clock_nanosleep接口函数需要传递更多的参数，当然也就是意味着它功能更强大。clock_id说明该接口函数不仅能基于real time clock睡眠，还可以基于其他的系统时钟睡眠。flag等于0或者1，分别指明request参数设定的时间值是相对时间还是绝对时间。

四、和timer相关的服务
1、alarm函数

  #include <unistd.h>
  static void tutk_alarm_signal_handler(int sig)
  {
  if(sig == SIGALRM){}
  }
  unsigned int alarm(unsigned int seconds);
  signal(SIGALRM, tutk_alarm_signal_handler);
alarm函数是使用timer最简单的接口。在指定秒数（基于CLOCK_REALTIME）的时间过去后，向该进程发送SIGALRM信号。当然，调用该接口的程序需要设定signal handler。

2、Interval timer函数, 新的POSIX标准obsolescent
虽然interval timer函数也是POSIX标准的一部分，不过在新的POSIX标准中，interval timer接口函数被标注为obsolescent，取而代之的是POSIX timer接口函数。

  #include <sys/time.h>

  int getitimer(int which, struct itimerval *curr_value);
  int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
Interval timer函数的行为和alarm函数类似，不过功能更强大。
每个进程支持3种timer，不同的timer定义了如何计时以及发送什么样的信号给进程，which参数指明使用哪个timer：
（1）ITIMER_REAL。基于CLOCK_REALTIME计时，超时后发送SIGALRM信号，和alarm函数一样。
  配合 signal(SIGALRM, tutk_alarm_signal_handler);
（2）ITIMER_VIRTUAL。只有当该进程的用户空间代码执行的时候才计时，超时后发送SIGVTALRM信号。
（3）ITIMER_PROF。只有该进程执行的时候才计时，不论是执行用户空间代码还是陷入内核执行（例如系统调用），超时后发送SIGPROF信号。
  struct itimerval {
  struct timeval it_interval; /* next value */
  struct timeval it_value; /* current value */
  };
  tick.it_value.tv_sec = 0; tick.it_value.tv_usec = 1; //1us之后第一次超时
  tick.it_interval.tv_sec = 0; tick.it_interval.tv_usec = TUTK_BASIC_TIMER_INTERVAL * 1000;//周期定时
  setitimer(ITIMER_REAL, &tick, NULL);
两个成员分别指明了本次和下次（超期后如何设定）的时间值。通过这样的定义，interval timer可以实现one shot类型的timer和periodic的timer。例如current value设定为5秒，next value设定为3秒，设定这样的timer后，it_value值会不断递减，直到5秒后触发，而随后it_value的值会被重新加载（使用it_interval的值），也就是等于3秒，之后会按照3为周期不断的触发。

old_value返回上次setitimer函数的设定值。getitimer函数获取当前的Interval timer的状态，其中的it_value成员可以得到当前时刻到下一次触发点的世时间信息，it_interval成员保持不变，除非你重新调用setitimer重新设定。

3、更高级，更灵活的timer函数
上一节介绍的Interval timer函数还是有功能不足之处：例如一个进程只能有ITIMER_REAL、ITIMER_VIRTUAL和ITIMER_PROF三个timer，如果连续设定其中一种timer（例如ITIMER_REAL），这会导致第一个设定被第二次设定覆盖。此外，超时处理永远是用信号的方式，而且发送的signal不能修改。当mask信号处理的时候，虽然timer多次超期，但是signal handler只会调用一次，无法获取更详细的信息。最后一点，Interval timer函数精度是微秒级别，精度有进一步提升的空间。正因为传统的Interval timer函数的不足之处，POSIX标准定义了更高级，更灵活的timer函数，我们称之POSIX （interval）Timer。

（1）创建timer
  #include <signal.h> #include <time.h>
  int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
在这个接口函数中，clock id相信大家都很熟悉了，timerid一看就是返回的timer ID的句柄，就像open函数返回的文件描述符一样。因此，要理解这个接口函数重点是了解struct sigevent这个数据结构：

  typedef union sigval { /* Data passed with notification */
  int sival_int; /* Integer value */
  void *sival_ptr; /* Pointer value */
  }sigval_t;

  typedef struct sigevent {
  sigval_t sigev_value;
  int sigev_signo;
  int sigev_notify;
  union {
  int _pad[SIGEV_PAD_SIZE]; int _tid;
  struct { void (*_function)(sigval_t); void *_attribute;/*really pthread_attr_t*/ } _sigev_thread;
  } _sigev_un;
  } sigevent_t;
#define sigev_notify_function _sigev_un._sigev_thread._function
#define sigev_notify_attributes _sigev_un._sigev_thread._attribute
#define sigev_notify_thread_id _sigev_un._tid

sigev_notify定义了当timer超期后如何通知该进程，可以设定：
（a）SIGEV_NONE。不需要异步通知，程序自己调用timer_gettime来轮询timer的当前状态
（b）SIGEV_SIGNAL。使用sinal这样的异步通知方式。发送的信号由sigev_signo定义。如果发送的是realtime signal，该信号的附加数据由sigev_value定义。
（c）SIGEV_THREAD。创建一个线程执行timer超期callback函数，_attribute定义了该线程的属性。
（d）SIGEV_THREAD_ID。行为和SIGEV_SIGNAL类似，不过发送的信号被送达进程内的一个指定的thread，这个thread由_tid标识。

（2）设定timer
  #include <time.h>
  int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec * old_value);
  int timer_gettime(timer_t timerid, struct itimerspec *curr_value);

timerid就是上一节中通过timer_create创建的timer。new_value和old_value这两个参数类似setitimer函数，这里就不再细述了。flag等于0或者1，分别指明new_value参数设定的时间值是相对时间还是绝对时间。如果new_value.it_value是一个非0值，那么调用timer_settime可以启动该timer。如果new_value.it_value是一个0值，那么调用timer_settime可以stop该timer。

timer_gettime函数和getitimer类似，可以参考上面的描述。

（3）删除timer
  #include <time.h> int timer_delete(timer_t timerid);
有创建就有删除，timer_delete用来删除指定的timer，释放资源。

范例1: 采用新线程派驻的通知方式

#include <stdio.h> #include <signal.h> #include <time.h>
#include <string.h> #include <stdlib.h> #include <unistd.h>
#define CLOCKID CLOCK_REALTIME
void timer_thread(union sigval v){
  printf("timer_thread function! %d\n", v.sival_int);
}

int main(){
  timer_t timerid; struct sigevent evp; memset(&evp, 0, sizeof(struct sigevent)); //清零初始化
  evp.sigev_value.sival_int = 111; //也是标识定时器的，这和timerid有什么区别？回调函数可以获得
  evp.sigev_notify = SIGEV_THREAD; //线程通知的方式，派驻新线程
  evp.sigev_notify_function = timer_thread; //线程函数地址
  if (timer_create(CLOCKID, &evp, &timerid) == -1){ perror("fail to timer_create"); exit(-1); }
  //第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值
  struct itimerspec it;
  it.it_interval.tv_sec = 1; it.it_interval.tv_nsec = 0; it.it_value.tv_sec = 1; it.it_value.tv_nsec = 0;
  if (timer_settime(timerid, 0, &it, NULL) == -1){ perror("fail to timer_settime"); exit(-1); }
  pause();
  return 0;
}

范例2: 通知方式为信号的处理方式

  #include <stdio.h> #include <time.h> #include <stdlib.h>
  #include <signal.h> #include <string.h> #include <unistd.h>
  #define CLOCKID CLOCK_REALTIME
  void sig_handler(int signo){
  printf("timer_signal function! %d\n", signo);
  }

  int main()
  { // struct sigaction{
  // void (*sa_handler)(int); //信号响应函数地址
  // void (*sa_sigaction)(int, siginfo_t *, void *); //但sa_flags为SA——SIGINFO时才使用
  // sigset_t sa_mask; //说明一个信号集在调用捕捉函数之前，会加入进程的屏蔽中，当捕捉函数返回时，还原
  // int sa_flags; void (*sa_restorer)(void); //未用
  // };
  timer_t timerid; struct sigevent evp; struct sigaction act;
  memset(&act, 0, sizeof(act)); act.sa_handler = sig_handler; act.sa_flags = 0;

  // XXX int sigaddset(sigset_t *set, int signum); //将signum指定的信号加入set信号集
  sigemptyset(&act.sa_mask); //初始化信号集
  if (sigaction(SIGUSR1, &act, NULL) == -1){ perror("fail to sigaction"); exit(-1); }

  memset(&evp, 0, sizeof(struct sigevent)); evp.sigev_signo = SIGUSR1; evp.sigev_notify = SIGEV_SIGNAL;
  if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1){ perror("fail to timer_create"); exit(-1); }

  struct itimerspec it;
  it.it_interval.tv_sec = 2; it.it_interval.tv_nsec = 0; it.it_value.tv_sec = 1; it.it_value.tv_nsec = 0;
  if (timer_settime(timerid, 0, &it, 0) == -1) { perror("fail to timer_settime"); exit(-1); }
  pause();
  return 0;
  }

Linux内核延时: 提供3个函数分别进行纳秒，微妙和毫秒延时：
#include <linux/delay.h>
void ndelay(unsigned long nsecs);
void udelay(unsigned long usecs);
void mdelay(unsigned long msecs);
这3个函数的延时原理是忙等待，也就是说在延时的过程中并没有放弃cpu，根据cpu的频率进行一定次数的循环。
在内核中对于毫秒级以上的延时，最好不要直接使用mdelay函数，这将无谓的浪费cpu的资源，对于毫秒级以上的延时，内核提供了下列函数：
#include <linux/delay.h>
void msleep(unsigned int millisecs);
unsigned long msleep_interruptible(unsigned int milosecs);
void ssleep(unsigned int seconds);
注：受系统HZ以及进程调度的影响，msleep类似函数的精度是有限的。

长延时
在内核中，一个直观的延时的方法是将所要延迟的时间设置的当前的jiffies加上要延迟的时间，这样就可以简单的通过比较当前的jiffies和设置的时间来判断延时的时间时候到来。针对此方法，内核中提供了简单的宏用于判断延时是否完成。
time_after(jiffies,delay); /*此刻如果还没有到达延时的时间，则返回真，否则返回0*/
time_before(jiffies,delay);/*如果延时还没有完成，则返回真，否则返回0*/
unsigned long timeout = jiffies + msecs_to_jiffies(500);
do {...} while (time_before(jiffies, timeout));

下面两个函数可以将当前进程添加到等待队列中，从而在等待队列上睡眠，当超时发生时，进程将被唤醒：
sleep_on_timeout(wait_queue_head_t *q, unsigned long timeout);
interrupt_sleep_on_timeout(wait_queue_head_t *q, unsigned long timeout);

