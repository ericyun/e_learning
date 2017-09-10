自定义LOG机制
QT部分： QProcess说开来 QT错误重定向

一些关键字： klogctl
syslogd(最新为rsyslogd)，相关协议 RFC3164
syslogd守护进程根据/etc/syslog.conf(rsyslogd配置文件在/etc/rsyslogd.conf),将不同的服务产生的Log记录到不同的文件中.
    1. 所有系统信息是输出到ring buffer中去的，dmesg所显示的内容也是从ring buffer中读取的.
    2. LINUX系统中/etc/init.d/sysklogd会启动2个守护进程:Klogd, Syslogd
    3. klogd是负责读取内核信息的,有2种方式:
        a. syslog()系统调用(这个函数用法比较全,大家去MAN一下看看)；
        b. 直接的对/proc/kmsg进行读取(再这提一下,/proc/kmsg是专门输出内核信息的地方)来从系统缓冲区(ring buffer)
            中得到由内核printk()发出的信息
    4.Klogd的输出结果会传送给syslogd进行处理,而syslogd会根据/etc/syslog.conf的配置把log信息输出到/var/log/
        下的不同文件中

可以将printk与syslog接合使用, 用在内核开发方面很不错的应用：
修改/etc/syslog.conf （或者是/etc/rsyslogd.conf）    :   kern.* /tmp/my_kernel_debug.txt

Linux系统的日志主要分为两种类型：
1．进程所属日志
由用户进程或其他系统服务进程自行生成的日志，比如服务器上的access_log与error_log日志文件。
2．syslog消息 【syslogd, klogd协同作用，前面以及提及，新版本中都没了klogd了】
系统syslog记录的日志，任何希望记录日志的系统进程或者用户进程都可以给调用 syslog 来记录日志。

可以考虑，使用管道启动 "cat /proc/kmsg", 然后获取它的输出到内存中，这样可以方便的得到kernel级别的trace信息。

dmesg命令用于打印存储在ring buffer中的内核日志，开机信息亦保存在/var/log/dmesg的文件里，但HISI平台klogd没有启动没有看到这个文件。
在Unix类操作系统上，syslog广泛应用于系统日志。syslog日志消息既可以记录在本地文件中，也可以通过网络发送到接收syslog的服务器。接收syslog的服务器可以对多个设备的syslog消息进行统一的存储，或者解析其中的内容做相应的处理。常见的应用场景是网络管理工具、安全管理系统、日志审计系统。

改变console loglevel的方法有如下几种：
1.启动时Kernel boot option：loglevel=level
2.运行时Runtime: dmesg -n level
（注意：demsg -n level 改变的是console上的loglevel，dmesg命令仍然会打印出所有级别的系统信息。）
3.运行时Runtime: echo $level > /proc/sys/kernel/printk
4.运行时Runtime:写程序使用syslog系统调用（可以man syslog）

root权限执行cat /proc/kmsg，会阻塞打印内核消息(包含printk的信息)直到手动ctrl+C为止

dmesg命令通常与less/more/tail/grep 等命令配合使用。
demsg -n level 改变console上的loglevel，dmesg命令仍然会打印出所有级别的系统信息
dmesg > boot.messages 重定向到文件中，然后发送邮件 mail -s "Boot Log" public@web3q.net <boot.messages
dmesg | less 回车控制显示
dmesg | tail 查看dmesg尾部的信息
dmesg -c     打印并清除内核环形缓冲区

最好是能够把trace分类在多个窗口(文件)中预览，比如sata的信息一个窗口，不同模块的信息各自一个窗口。方便调试时候解析信息。
     每条信息如果能够增加(mode, prio)的头部，应该可以得到同样的效果。
     所以，有必要弄清楚syslogd的工作方式，然后自定义我们的trace

抓包工具(只要包头和全部两种)也集成进来。
     不同模块的trace输出到不同的文件或者是界面窗口，当然也可以选择混杂文件/窗口
     丰富显示颜色的控制。
     按照syslog的格式改造trace信息，karry部分按照无格式处理。
     NVR上可以从U盘启动此工具或者保存在usr目录下。

尽量在重要的基本函数中增加上层调用函数的trace，使用__FUNCTION__定义。还有就是临界区等各种系统资源，统一调度。

注意printf从右向左执行。

宏定义中，把函数作为callback函数传递进去，
#define     func_a()     fun_b(func_a, args...)     a和b是一一对应的。
或者，设计一个统一的注册函数，把func_a和参数对应进去

extern ssize_t writen_i(int fd, const void *vptr, size_t n, const char* format, ...);
#define writen(fd,vptr,n,args...) writen_i(fd,vptr,n,"%s",__FUNCTION__,##args)
static char    writen_buff[16*1024];
#include <stdarg.h>

ssize_t writen_i(int fd, const void *vptr, size_t n, const char* format, ...)
{
    size_t          nleft;
    ssize_t          nwritten;
    const char     *ptr;

    va_list args;
    va_start(args, format);
    int size = vsnprintf(writen_buff, sizeof(writen_buff), format, args);
    va_end(args);

    ptr = (char *)vptr;
    nleft = n;
    //if(n != 64*1024)
        //return -1;
    while (nleft > 0)
    {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0 && ((errno == EINTR)||(errno == EAGAIN)))
            {
                //return -1;
                    usleep(10000);
                nwritten = 0;          /* and call write() again */
            }
            else
                return -1;          /* error */
        }

        nleft -= nwritten;
        if(nleft > 0){
            printf("Function:%s total: %d left: %d\n", writen_buff, n, nleft);
            return -1;
        }
        ptr += nwritten;
    }
    return(n);
}

