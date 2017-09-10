LINUX日志
解决的问题：延时打印，信息过滤，找到　#define ring_buffer_alloc(size, flags)，　搜索发现ring buffer大小为CPU_BUFFER_SIZE_DEFAULT，增加到1M的buffer,取消isp线程实时设置，mmc_test进程为实时，不再丢失信息．








在做内核驱动开发的时候，可以使用/proc下的文件，获取相应的信息，以便调试。大多数/proc下的文件是只读的，但为了示例的完整性，都提供了写方法。方法一：使用create_proc_entry创建proc文件（简单，但写操作有缓冲区溢出的危险；//最新代码中无，放弃方法二：使用proc_create和seq_file创建proc文件（较方法三简洁；　//暂时不考虑方法三：使用proc_create_data和seq_file创建proc文件（较麻烦，但比较完整）；






kmsg dmesgFILE*       kmsg_file = popen("cat /proc/kmsg", "r"); int size = fread(buf, 1, maxsize, kmsg_file); 好像会阻塞等待maxsize长度的数据，受不了；改成使用 char* ptr = fgets(buf, maxsize, kmsg_file); 
open("/proc/kmsg", WR|RD); read(); 信息不全，麻烦。
硬盘有故障的时候，CPU4个被占用两个，全是io，想办法判断并杀掉线程，看是否有用。

syslog架构
#include <syslog.h>int main(void)
{
    int log_test;
    openlog("log_test", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "PID information, pid=%d", getpid());
    syslog(LOG_DEBUG, "debug message");
    closelog();
    return 0;
}
Unix/Linux系统中的大部分日志都是通过一种叫做syslog的机制产生和维护的。syslog是一种标准的协议，分为客户端和服务器端，客户端是产生日志消息的一方，而服务器端负责接收客户端发送来的日志消息，并做出保存到特定的日志文件中或者其他方式的处理。
在Linux中，常见的syslog服务器端程序是syslogd守护程序。这个程序可以从三个地方接收日志消息：（1）Unix域套接字 /dev/log；（2）UDP端口514；（3）特殊的设备/dev/klog（读取内核发出的消息）。相应地，产生日志消息的程序就需要通过上述三种方式写入消息，对于大多数程序而言就是向/dev/log这个套接字发送日志消息



syslog的协议介绍
完整的syslog日志中包含产生日志的程序模块（Facility）、严重性（Severity或 Level）、时间、主机名或IP、进程名、进程ID和正文。在Unix类操作系统上，能够按Facility和Severity的组合来决定什么样的日志消息是否需要记录，记录到什么地方，是否需要发送到一个接收syslog的服务器等。由于syslog简单而灵活的特性，syslog不再仅限于 Unix类主机的日志记录，任何需要记录和发送日志的场景，都可能会使用syslog。

长期以来，没有一个标准来规范syslog的格式，导致syslog的格式是非常随意的。最坏的情况下，根本就没有任何格式，导致程序不能对syslog 消息进行解析，只能将它看作是一个字符串。

在2001年定义的RFC3164中，描述了BSD syslog协议： http://www.ietf.org/rfc/rfc3164.txt
不过这个规范的很多内容都不是强制性的，常常是“建议”或者“约定”，也由于这个规范出的比较晚，很多设备并不遵守或不完全遵守这个规范。接下来就介绍一 下这个规范。

约定发送syslog的设备为Device，转发syslog的设备为Relay，接收syslog的设备为Collector。Relay本身也可以发送自身的syslog给Collector，这个时候它表现为一个Device。Relay也可以只转发部分接收到的syslog消息，这个时候它同时表现为Relay和Collector。

syslog消息发送到Collector的UDP 514端口，不需要接收方应答，RFC3164建议 Device 也使用514作为源端口。规定syslog消息的UDP报文不能超过1024字节，并且全部由可打印的字符组成。完整的syslog消息由3部分组成，分别是PRI、HEADER和MSG。大部分syslog都包含PRI和MSG部分，而HEADER可能没有。
syslog的格式

下面是一个syslog消息： <30>Oct 9 22:33:20 hlfedora auditd[1787]: The audit daemon is exiting.
其中“<30>”是PRI部分，“Oct 9 22:33:20 hlfedora”是HEADER部分，“auditd[1787]: The audit daemon is exiting.”是MSG部分。
PRI部分
PRI部分由尖括号包含的一个数字构成，这个数字包含了程序模块（Facility）、严重性（Severity），这个数字是由Facility(3个bit)乘以 8，然后加上Severity得来。不知道他们为什么发明了这么一种不直观的表示方式。也就是说这个数字如果换成2进制的话，低位的3个bit表示Severity，剩下的高位的部分右移3位，就是表示Facility的值。
十进制30 = 二进制0001 1110           0001 1... = Facility: DAEMON - system daemons (3)      .... .110 = Severity: INFO - informational (6)

Facility的定义如下，可以看出来syslog的Facility是早期为Unix操作系统定义的，不过它预留了User（1），Local0～7 （16～23）给其他程序使用：

      Numerical             Facility
         Code

          0             kernel messages          1             user-level messages          2             mail system          3             system daemons
          4             security/authorization messages (note 1)          5             messages generated internally by syslogd
          6             line printer subsystem          7             network news subsystem          8             UUCP subsystem
          9             clock daemon (note 2)         10             security/authorization messages (note 1)
         11             FTP daemon         12             NTP subsystem         13             log audit (note 1)         14             log alert (note 1)
         15             clock daemon (note 2)         16             local use 0  (local0)         17             local use 1  (local1)
         18             local use 2  (local2)         19             local use 3  (local3)         20             local use 4  (local4)
         21             local use 5  (local5)         22             local use 6  (local6)         23             local use 7  (local7)

       Note 1 - Various operating systems have been found to utilize  Facilities 4, 10, 13 and 14 for security/authorization, audit, and alert messages which seem to be similar.
       Note 2 - Various operating systems have been found to utilize both Facilities 9 and 15 for clock (cron/at) messages.

Severity的定义如下：
        Numerical         Severity
        Code

         0       Emergency: system is unusable         1       Alert: action must be taken immediately         2       Critical: critical conditions
         3       Error: error conditions         4       Warning: warning conditions         5       Notice: normal but significant condition
         6       Informational: informational messages         7       Debug: debug-level messages

也就是说，尖括号中有1～3个数字字符，只有当数字是0的时候，数字才以0开头，也就是说00和01这样在前面补0是不允许的。
HEADER部分
HEADER部分包括两个字段，时间和主机名（或IP）。
时间紧跟在PRI后面，中间没有空格，格式必须是“Mmm dd hh:mm:ss”，不包括年份。“日”的数字如果是1～9，前面会补一个空格（也就是月份后面有两个空格），而“小时”、“分”、“秒”则在前面补“0”。月份取值包括：Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec

时间后边跟一个空格，然后是主机名或者IP地址，主机名不得包括域名部分。

因为有些系统需要将日志长期归档，而时间字段又不包括年份，所以一些不标准的syslog格式中包含了年份，例如：
<165>Aug 24 05:34:00 CST 1987 mymachine myproc[10]: %% It's time to make the do-nuts. %% Ingredients: Mix=OK, Jelly=OK #
Devices: Mixer=OK, Jelly_Injector=OK, Frier=OK # Transport: Conveyer1=OK, Conveyer2=OK # %%
这样会导致解析程序将“CST”当作主机名，而“1987”开始的部分作为MSG部分。解析程序面对这种问题，可能要做很多容错处理，或者定制能解析多种syslog格式，而不仅仅是只能解析标准格式。

HEADER部分后面跟一个空格，然后是MSG部分。有些syslog中没有HEADER部分。这个时候MSG部分紧跟在PRI后面，中间没有空格。

MSG部分
MSG部分又分为两个部分，TAG和Content。其中TAG部分是可选的。
在前面的例子中（“<30>Oct 9 22:33:20 hlfedora auditd[1787]: The audit daemon is exiting.”），“auditd[1787]”是TAG部分，包含了进程名称和进程PID。PID可以没有，这个时候中括号也是没有的。
进程PID有时甚至不是一个数字，例如“root-1787”，解析程序要做好容错准备。

TAG后面用一个冒号隔开Content部分，这部分的内容是应用程序自定义的。
几种syslog的实现
由于syslog是一种RFC标准协议，所以其实现是平台无关的，目前在各种Unix/Linux系统、网络设备、终端设备中得到广泛支持，当然最重要的还是用于类Unix系统中。
syslogd
这是最传统的syslog协议服务器端的实现，运行于大多数的Linux系统中。由于其安全性差、语法太自由而导致了很多的问题，目前有了一些替代品出现。
rsyslogd
1. 后端存查日志支持的客户端多。
2. 在同一台机器上支持多直rsyslogd进程，可以监听在不同端口。
3. 直接兼容系统自带的syslog.conf 配置文件。
4. 可将消息过滤后再次转发。
5. 配置文件中可以写简单的逻辑判断
6. 有现成的前端web展示程序

现在RHEL以及CentOS都已经把rsyslog作为默认的syslog安装，不再使用syslogd了。
syslog-ng
也就是(next generation)下一代syslog，也是用来代替传统syslogd的，只是这个玩意竟然不开放源代码，所以也就不折腾这个了。
kiwisyslog
Kiwi Syslog Server 是一个免费的Windows平台上的syslog守护进程。它接收，记录，显示和转发系统日志，如路由器，防火墙，交换机，Unix主机和其他功能的设备主机的syslog消息。有许多可供自定义的选项。其特点包括PIX防火墙日志记录，Linksys的家庭防火墙日志，SNMP陷阱和TCP的支持，有能力进行筛选，分析和修改信息，并透过VBScript或JScript引擎执行动作。
syslog配置文件与应用
syslog配置文件决定了处理日志信息的规则。主要依据就是前面协议部分提到的facility和Severity。下面按照实际的例子说明。
基本语法格式
类型.级别 [；类型.级别] `TAB` 动作   --如--  *.info;mail.none;authpriv.none;cron.none                /var/log/messages

类型（Facility）
在配置文件中的名字如下：
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
|| click me || click me ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 facility参数
 || syslog.conf中对应的facility取值
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_KERN
 || kern
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_USER
 || user
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_MAIL
 || mail
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_DAEMON
 || daemon
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_AUTH
 || auth
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_SYSLOG
 || syslog
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_LPR
 || lpr
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_NEWS
 || news
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_UUCP
 || uucp
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_CRON
 || cron
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_AUTHPRIV
 || authpriv
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_FTP
 || ftp
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_LOCAL0～LOG_LOCAL7
 || local0～local7
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



级别（Severity）
在配置文件中的名字如下：

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
|| click me || click me ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 priority参数
 || syslog.conf中对应的level取值
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_EMERG
 || emerg
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_ALERT
 || alert
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_CRIT
 || crit
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_ERR
 || err
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_WARNING
 || warning
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_NOTICE
 || notice
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_INFO
 || info
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 LOG_DEBUG
 || debug
 ||
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



动作
动作”域指示信息发送的目的地。可以是：
    /filename   日志文件。由绝对路径指出的文件名，此文件必须事先建立； 如果在文件名之前加上减号(-)，则表示不将日志信息同步刷新到磁盘上(使用写入缓存)，这样可以提高日志写入性能，但是增加了系统崩溃后丢失日志的风险。
    @host       远程主机； @符号后面可以是ip,也可以是域名，默认在/etc/hosts文件下loghost这个别名已经指定给了本机。
    user1,user2 指定用户。如果指定用户已登录，那么他们将收到信息；
    *           所有用户。所有已登录的用户都将收到信息。





