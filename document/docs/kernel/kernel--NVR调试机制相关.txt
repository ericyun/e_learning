NVR调试机制相关
查查看那，硬盘的休眠和唤醒有没有U盘trace。
硬盘写不完整的扇区，是否先读出来，做好运算，然后再次写回去？ 是否一次只能读写一个扇区呢？

浏览器与服务端建立连接后，若刷新或关闭浏览器窗口（未监听相应事件并处理），服务端无法得知连接断开，按理说，这种情况属于client异常终止，跟拔网线的情况类似。这种情况下，服务端不知情，仍保留此连接，仍按照既定逻辑向client写数据，写了两次后，服务端程序终止（多次测试，均是第2次后终止），不是崩溃，是异常终止，非常不解。
问了一位很有经验的同事，得知是SIGPIPE信号导致程序终止。
查了相关资料，大致明白：连接建立，若某一端关闭连接，而另一端仍然向它写数据，第一次写数据后会收到RST响应，此后再写数据，内核将向进程发出SIGPIPE信号，通知进程此连接已经断开。而SIGPIPE信号的默认处理是终止程序，导致上述问题的发生。
为避免这种情况，可以选择忽略SIGPIPE信号，不执行任何动作。
#include <signal.h>
//SIGPIPE ignore
struct sigaction act;
act.sa_handler = SIG_IGN;
if (sigaction(SIGPIPE, &act, NULL) == 0) {
    LOG("SIGPIPE ignore");
}

U盘相关开发
Linux的文件系统是异步的，也就是说写一个文件不是立刻保存到介质（硬盘，U盘等）中，而是存到缓冲区内，等积累到一定程度再一起保存到介质中。如果没有umount就非法拔出U盘，程序是不知道的，fopen，fwrite等函数都依然返回正确，知道操作系统要把写介质的时候，才会提示I/O错误。可是很多数据都会因为这个不及时的错误报告而丢失。

U盘检测的方法：
1. U盘加载之后，可以增加一个新的小文件，访问这个文件就知道U盘状态。
2. U盘驱动程序会在插入或拔出时在 /proc/scsi/创建usb-storage文件夹，生成一个1-n的设备文件，全部拔出删除usb-storage目录
          int dir;     struct dirent* dirent;
          if((dir = opendir("/proc/scsi/usb-storage" )) != NULL){
               while (dir) {
                    if ( (dirent = readdir(dir)) != NULL) {

                            APP_STDERR_LOG(DBMOD_0, DBLEV_1,"file:%s detected\n", dirent->d_name);
                            break;
                    }
                    closedir(dir);
               }
          }
          //用来判断存储设备是否插上，记得之后需要调用closedir(dir);
          使用access()
         usb-storage目录的状态与是否挂载（mount）无关。

U盘管理状态机：
使用状态机来管理U盘：
0. 初始化，调用SystemUpdateMountUDisk()函数mount存储盘，成功且U盘trace转到状态2，非U盘trace转到状态1，失败转到状态3
     不处理任何事件，强行加载U盘。
1. 正常访问，sem_wait 10s等待新的事件；U盘trace使能的话，进入状态2

2. 写入U盘trace信息，U盘拔出，或者停止U盘trace，或者写入U盘出错的话，退出到状态3.
3. 当前无有效U盘，检查/proc/scsi/usb-storage，确认这个判断，执行umount过程，然后sem_wait等待有效事件触发，或者10s超时
    U盘有效事件触发，切换到状态0.

U盘刷新控制，trace信息调用fsync().

U盘事件：
1. 开启U盘trace，2. 备份请求，3. 刷新U盘内容，4. 检测到U盘插入的log
U盘操作：
U盘内容及时同步的问题：fsync(fd) 或者启动O_SYNC打开文件

1M的缓冲区，保存的应该是最新的trace，或者按照帧的方式来组织，或者，BufferWrite写入空间不够的话直接清理下一个'0'之前的trace。计算缺多少空间，然后，直接向后推移。或者，使用链表的方式来管理，链表的头部信息在buffer中而已。
链表和ringbuffer的复合管理数据结构。

需要简化硬盘trace的结构，不然太多了，平时根本无法使用；NVR上最好能够flush并且预览U盘上文件;NVR退出的速度太慢了，看是否有线程异常。
     时间相关的函数，放在一个文件中。

NVR可以考虑通过飞秋协议直接发送trace信息，飞秋应该是比较开放的协议吧，而且这个工具总是会被开启，方便。
     发送一个"trace open"消息过去，就会不断发送trace信息过来；发送一个"trace close"消息，就停止传递。
     所有录像都共享过来，可以直接备份；所有日志，所有参数。库函数可以直接替换而不需要其他方式。
     NVR开机启动飞秋协议。

支持多个生产者，多个消费者的ringbuffer机制，同时支持帧为单位的访问。
TRACE信息可以分多个文件保存，也可以混杂保存到一个文件中，或者两个文件，一个保存需要的信息，一个保存其他的信息。
多种储存： 硬盘， FLAG， U盘，UART，网络上传。 还要支持远程从硬盘导出的功能。

能否得到某个线程当前的stack信息呢？每个函数的开头和结尾，增加一个宏，记录函数调用过程。
所有的线程，尽量留下控制可以禁止线程的执行，或者留下其他的控制后门。

新的日志机制，应该支持syslog协议，这样方便以后与其他工具对接。
硬盘上是否应该专门有8G的空间预留，做日志和图片等用处。

应该增加功能在NVR上可以直接预览日志文本文件的信息。

tmpfs是一种基于内存的文件系统，它和虚拟磁盘ramdisk比较类似像，但不完全相同，和ramdisk一样，tmpfs可以使用RAM，但它也可以使用swap分区来存储。而且传统的ramdisk是个块设备，要用mkfs来格式化它，才能真正地使用它；而tmpfs是一个文件系统，并不是块设备，只是安装它，就可以使用了。tmpfs是最好的基于RAM的文件系统

U盘Trace机制，看能否弄好。每10分钟刷新一次。
*** 应该有一个系统trace重定向的东西，可以使用自己的函数输出trace，这样，系统级别的trace也可以输出到U盘或者网络之类的接口，
     当然，也只能输出到单独的接口了。
使用系统库函数printf()(),这就需要重载输入,输出函数int fputc(int ch, FILE *f);int fgetc(FILE *f).
     linux stdout重定向

能给每个函数退出的时候都增加一个HOOK函数调用么，这样，方便很多调试
一个NVR开发工具，可以完成自动测试，调试的目标。还有客户端类型功能开发等。
需要一个小工具，可以直接查询当前的一些全局变量的情况。
     程序不知道在什么地方停滞，使用step类变量。
     程序内存泄露，监控所有的malloc和free等；程序锁死，监控所有的lock；
     所有软件运行状态，应该统一登记注册，出问题才会方便搞定；
     所有的trace信息，需要梳理的有规则，有秩序，命名和确立是否有必要必须要有规律有条理。
     trace，不仅仅是调试当前问题，更重要的解决未来的问题。
暂时可以这样，开启一个线程，scanf字符，根据结果打印当前的一些全局变量等。
线程管理：
     每个线程创建的时候，都有一个自己的名称，注册，并从全局数组中申请一个step定点记录当前线程执行位置，程序中多点设置，用来
     注册信息包括，线程名称描述用处，分配的stack大小和优先级等线程信息，step的ID，
     每个线程，当前执行
     会有一些特殊的step值，比如，调用可疑函数前后应该是特殊的值，这样，查询的时候可以很方便的找到可能有问题的点。
同步资源如互斥锁等
     所有的资源，统一注册
     注册信息包含名称描述用处和关联信息
     使用统一的函数调用；调用时通过宏定义，至少把文件和行号信息包含到资源信息中，
所有的函数
     能否把函数调用层次信息保存到一个堆栈形式的数组中呢？每个线程一个。以前做过一个，在操作系统级别实现的。

内存管理
     每次调用都使用新函数，分配一个ID，释放的时候注销，可以方便查看内存泄漏问题；
     内存越界，是否有？
     堆栈中数组越界？
     堆栈越界，如何处理？

自动化测试的问题。
对于 SYSLOG_STORAGE JPEG_STORAGE 等扩展类型录像，
     录像策略初始化为手动 (REC_STRA_HAND == manager->recstra)，
     并且Ip_GetStreamStatus函数对这两个通道永远返回1.
     GetRecType 函数返回对应的新定义存储类型。
     GuiSetRecStatus 函数做无效处理

对于rtsp onvif gui这几个库，我们最好是传递一个可变参数的callback函数指针，代码更和谐。
domake.sh 脚本，修改一下几个库的release函数
之前trace需要输出到U盘，才会使用System_Log函数，现在可以取消掉这个功能，直接完全使用我们自己的函数来实现。
UpdateCurLocalTime  函数中所有全局变量，应该改用函数来访问。
用QT重写网络trace工具，拓展功能
APP_LVLED_LOG 等日志宏，需要改成函数，这样，修改之后不需要重新编译库。

是否所有的临界区都统一管理呢？用一个数组，记录上次进入时间，出问题时候，可以直接打印所有的临界区状态，
     每次lock的时候参数传递一个ID作为数组索引，我们记录进入时间，还有lock了多少次，什么时候退出等。
     如果有GDB，可以直接查看lock状态，现在，我们只能自己想把法了。
     需要使用的时候，申请一次ID，之后通过宏和ID来访问，所有的互斥锁资源统一管理。
     同样的，这个概念可以扩展到所有的线程，所有的malloc和free等。
     系统资源的使用，最好是可统计管理的。
     现场调试未必方便，所以，尽量收集运行时信息。

自定义ring buffer

typedef struct SHARED_BUFFER
{
    char*           pPool;
    int             maxSize;
    int             writeindex;
    int             readindex;
    int             discarded;
    pthread_mutex_t buflock;
} BufPool, *BufPoolPtr;

BufPool     trace_pool;

bool    BufferInit(BufPoolPtr B, int maxsize)
{
    if(maxsize < 1024)
    {
        maxsize = 1024;
    }

    B->pPool = (char*)malloc(maxsize);
    if(B->pPool == NULL)
    {
        return FALSE;
    }
    B->maxSize = maxsize;
    B->readindex = 0;
    B->writeindex = 0;
    pthread_mutex_init(&B->buflock, NULL);
    return TRUE;
}

int    BufferWrite(BufPoolPtr B, char* buf, int len)
{
    int ret_len = 0;
    int part1len, part2len;

    if(B != NULL)
    {
        pthread_mutex_lock(&B->buflock);
        if(len <= (B->maxSize - B->writeindex + B->readindex))
        {
            if((B->writeindex+len) <= B->maxSize)
            {
                memcpy(B->pPool+B->writeindex, buf, len);
                B->writeindex += len;
            }
            else
            {
                part1len = B->maxSize - B->writeindex;
                part2len = len - (B->maxSize - B->writeindex);
                memcpy(B->pPool+B->writeindex, buf, part1len);
                memcpy(B->pPool, buf+part1len, part2len);
                B->writeindex = part2len;
            }
            ret_len = len;
        }
        pthread_mutex_unlock(&B->buflock);
    }

    return ret_len;
}

int    BufferRead(BufPoolPtr B, char* buf, int len)
{
    bool ret_len = 0;
    int part1len, part2len;
    if(B == NULL)   return ret_len;

    pthread_mutex_lock(&B->buflock);
    if(B->readindex == B->writeindex)
    {
        ret_len = 0;
    }
    else if(B->readindex < B->writeindex)
    {
        len = MIN(len, (B->writeindex - B->readindex));
        memcpy(buf, B->pPool+B->readindex, len);
        ret_len = len;
        B->readindex += ret_len;
    }
    else
    {
        if((B->readindex+len) <= B->maxSize)
        {
            memcpy(buf, B->pPool+B->readindex, len);
            ret_len = len;
            B->readindex += ret_len;
        }
        else
        {
            part1len = (B->maxSize - B->readindex);
            memcpy(buf, B->pPool+B->readindex, part1len);
            part2len = len - (B->maxSize - B->readindex);
            part2len = MIN(part2len, B->writeindex);
            memcpy(buf+part1len, B->pPool, part2len);
            ret_len = part1len + part2len;
            B->readindex += ret_len;
        }
    }
    pthread_mutex_unlock(&B->buflock);

    return ret_len;
}

dmesg命令管理内核trace

可以考虑周期性的调用dmesg -c命令，缓冲区设定的大一些，可以把kernel级别的消息打印出来，而不需要开启klogd和syslogd
如果系统运行了klogd和syslogd，则无论console_loglevel为何值，内核消息都将追加到/var/log/messages中。如果klogd没有运行，消息不会传递到用户空间，只能查看/proc/kmsg。

可以开启一个线程，专门读取并且分析/proc/kmsg文件的数据。比如，检测到"USB disconnect"我们主动停止写入U盘；检测到"USB Mass Storage device detected"，我们就可以加载U盘。"Attached SCSI disk" 新接入硬盘就绪。

syslog()
用户程序可以自己管理trace信息，比如我们的APP_LVLED_LOG机制，也可以调用syslog()函数把trace交由syslog进程管理。

输入输出重定向
Linux的shell环境中支持，用符号<和>来表示。暂时不做深入学习
0、1和2分别表示标准输入stdin、标准输出stdout和标准错误stderr信息输出，可以用来指定需要重定向的标准输入或输出，比如 2>lee.dat 表示将错误信息输出到文件lee.dat中； 错误信息重定向到标准输出，可以用 2>&1来实现；这三个文件描述符是FILE*的流，所以记得用fwrite/fprintf/fread来访问。

/dev/null 黑洞，不输出任何信息

int no = fileno(stdout); 获取

重定向stdout到文件

把stdout重定向到文件的两种方法：
第一种方法  没有恢复，通过freopen把stdout重新打开到文件

#include <stdio.h>
FILE *stream;
void main( void )
{
   stream = freopen( "freopen.out", "w", stdout ); // 重定向

  if( stream == NULL )
     fprintf( stdout, "error on freopen\n" );
  else
  {
     //system( "type freopen.out" );
     system( "ls -l" );
     fprintf( stream, "This will go to the file 'freopen.out'\n" );
     fprintf( stdout, "successfully reassigned\n" );
     fclose( stream );
  }
  fprintf( stdout, "this is not print out\n" );//这里没有输出
}

输出结果

----------------------
第二种方法 使用dup复制备份 dup2设定和恢复
先把 1 复制出来， 然后建立个文件，用fileno取到文件描述符 覆盖到1 所有对1的操作都输出到文件了
用完之后，再把开始复制出来的 用dup2还给 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main( )
{
   int old;
   FILE *new;
   old = dup( 1 );   // 取标准输出句柄
   if( old == -1 )
   {
      perror( "dup( 1 ) failure" );
      exit( 1 );
   }
   write( old, "This goes to stdout first\r\n", 27 );
   if( ( new = fopen( "data", "w" ) ) == NULL )
   {
      puts( "Can't open file 'data'\n" );
      exit( 1 );
   }
   if( -1 == dup2( fileno( new ), 1 ) )//把文件的描述符给到1,1就不代表stdout了
   {
      perror( "Can't dup2 stdout" );
      exit( 1 );
   }
   system( "ls -l" );
   puts( "This goes to file 'data'\r\n" );
   fflush( stdout );
   fclose( new );
   dup2( old, 1 ); // 恢复
   puts( "This goes to stdout\n" );
   puts( "The file 'data' contains:" );
   //system( "type data" );
   system( "file data" );
}

printk()

# cat /proc/sys/kernel/printk     缺省为 6       4       1      7
该文件有四个数字值，它们根据日志记录消息的重要性，定义将其发送到何处。关于不同日志级别的更多信息，请查阅syslog(2)联机帮助。上面显示的4个数据分别对应：
     控制台日志级别：优先级[s1] 高于该值的消息将被打印至控制台,[s1]数值越小，优先级越高
     默认的消息日志级别：将用该优先级来打印没有优先级的消息
     最低的控制台日志级别：控制台日志级别可被设置的最小值(最高优先级)
     默认的控制台日志级别：控制台日志级别的缺省值

setconsole setlevel

内核通过printk() 输出的信息具有日志级别，日志级别是通过在printk() 输出的字符串前加一个带尖括号的整数来控制的，如printk("<6>Hello, world!\n");。内核中共提供了八种不同的日志级别，在 linux/kernel.h 中有相应的宏对应。
#define KERN_EMERG  "<0>"  #define KERN_ALERT  "<1>"   #define KERN_CRIT    "<2>"   #define KERN_ERR     "<3>"
#define KERN_WARNING "<4>"   #define KERN_NOTICE  "<5>"   #define KERN_INFO    "<6>"   #define KERN_DEBUG   "<7>"
所以printk() 可以这样用：printk(KERN_INFO"Hello, world!\n");。
未指定日志级别的printk() 采用的默认级别是DEFAULT_MESSAGE_LOGLEVEL，这个宏在kernel/printk.c 中被定义为整数4，即对应KERN_WARNING。
#define DEFAULT_MESSAGE_LOGLEVEL 4

printk("<6>Hello, world!\n") 尖括号表示trace的级别，我们好像也可以这样，使用多个尖括号，并且保证向前兼容以及功能扩展方便。
     现在的APP_LVLED_LOG(DBMOD_1, DBLEV_3,"")的方式不方便进一步的扩展。

了解了上面的这些知识后，我们就应该知道如何手动控制printk打印了。例如，我想屏蔽掉所有的内核printk打印，那么我只需要把第一个数值调到最小值1或者0。
# echo 1       4       1      7 > /proc/sys/kernel/printk 或者 # echo 0       4       0      7 > /proc/sys/kernel/printk
另外，/proc/sys/kernel/printk_ratelimit和/proc/sys/kernel/printk_ratelimit_burst也可以用来控制打印，具体有待研究。
也能够使用系统调用sys_syslog或klogd -c来修改console_loglevel值。
也能够指定显示在其他控制台，通过调用ioctl(TIOCLINUX)或shell命令setconsole来配置。
假如运行了klogd和syslogd，则printk打印到var/log/messages。

嵌入式系统日志功能

需要知道硬件使用额软件版本历史。应该把硬件情况，产品型号和软件版本等都输出来

这个功能可以一直开启部分重要的trace。通过网络上传crash和普通的trace
     先简单使用普通的录像段机制来写入log信息，浪费点空间没有什么关系，以后再处理好了。何况平时不打开这个功能，只有需要调试问题的时候再开启；
     10分钟一个段改掉就可以了。文件写满，或者硬盘切换的时候。
     最重要的，如何创造条件充分测试呢？
     对于硬盘日志，应该忽略原来的一些trace的通道限制，所有通道全部输出。
     每条日志的头部应该增加一个时间标签，日期秒级别精度。
     回放窗口下，测试TRACE好像不应该能够被客户看到，应该输入密码才能进入。
     trace文件刷新周期好像比较长，有丢trace的可能，尽量避免
     _WriteSysLog EndSysLogPack StartSysLogPack enable_show_log  syslog_active 可以通过检查syslog_mutex的lock状态来代替。GUI部分定义的控件ID为ID_SYSTEMMAINTAIN_SHOW_LOG，enable_show_log 全局变量控制是否实际开启硬盘trace，现在没有参数保存，每次启动需要手动开启或者关闭。当前缺省为关闭。_WriteSysLog 会在所有线程中调用。trace先写到一个buffer中，满64K或者满1s时间，作为I帧写入到mempool里面去，然后作为一个普通的录像通道来写入硬盘。这样，还不需要保证写入过程中不打印trace。
     使用普通录像文件的管理机制
          SearchRecFile，当前文件写满并且在当前盘，那么继续使用当前文件，并且segment头部信息不要改动。文件的开始结束时间，正常修改
          录像文件分成100个段，每段10M左右。所有的trace信息按照I帧写入，这样可以正常的搜索回放等
     搜索trace信息的时候，不需要时间信息了。切换硬盘的时候，关闭当前trace文件，切换到下一个。并且记录时间。
JPEG文件，有点浪费了，每个硬盘上通道数个文件。
     切换硬盘的时候，强制关闭当前log文件和预录。

_WriteSysLog 函数中，可以像autosnap线程中那样，直接写文件，不使用段和文件的管理，这样，trace信息不会再有死锁。
     那么如何管理呢？可以考虑直接更新SEGMENT_INFO的方式。是否可以用data_start_offset 表示新的；data_end_offset 表示，log已经写到了什么位置。
     每个硬盘上专门分配一个log文件，环形方式写入，每次只要有最后一段的结束位置，注意保证重启之前sync就可以了。非正常崩溃的话，
          下次启动之前预留足够的空间比如8k就好了。每8K的trace强制同步。
           用类似于autosnap模块的方式，直接使用read/write函数写入。

联网平台抓图功能
尽量写入到MEMPOOL中呢？然后，可以自动递增的方式
SearchRecFile ： 一个文件写满之后，继续使用当前文件就好了。不要分段，不要ENDpack，当成一个文件来写就是了。
如果已经切换到一个新的文件，并且至少已经写入了最长时间比如60s

