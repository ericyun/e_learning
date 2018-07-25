TMP. 环境. 碎片信息
公司相关一些信息:
BU1： 车载事业部 BU2： 安防事业部

mail.infotm.com   Infotm123

主要中心: 代码必须是可调试的, 方便定位的. 实验室, 现场. 提高对客户问题反应速度.

查阅内部电话：钉钉中，联系人，上海盈方微电子，组织架构，研发管理中心，品控部，陈孟

陌生内容
tasklet workqueue
buildroot教程，autotools  cmake make makefile
stat wdt_test.c

驱动模块的clock管理
     host->biu_clk = clk_get_sys("sdmmc.0", "sdmmc0");
     clk_prepare_enable(host->biu_clk);
     host->bus_hz = clk_get_rate(host->ciu_clk);
     ret = drv_data->setup_clock(host);
resource资源管理机制
     wdt_mem/wdt_irq = platform_get_resource(pdev, IORESOURCE_MEM/IORESOURCE_IRQ, 0);
     wdt->wdt_base = devm_ioremap_resource(wdt->wdt_dev, wdt_mem);
     ret = devm_request_irq(wdt->wdt_dev, wdt_irq->start, imapx_wdt_irq, 0, pdev->name, wdt);

阅读代码中，陌生函数，定义等
#define __jiffy_data  __attribute__((section(".data")))
MACHINE_START     MACHINE_END

内核态kmalloc kfree　常用flag为GFP_ATOMIC，GFP_KERNEL(无资源允许休眠， 非中断，非等待队列)
vmalloc vfree     GFP_USER      GFP_HIGHUSER      GFP_NOIO GFP_NOFS
phys_to_virt           virt_to_phys
setup_irq() 和 request_irq()
container_of
simple_strtoul

用户区和内核区数据copy ： int __user *p = argp;  get_user put_user copy_to_user
    clear_bit set_bit test_and_clear_bit subsys_initcall spin_lock
devm_request_irq  devm_kzalloc  devm_表示带资源管理的，自动回收，释放同样要带devm_
devm_request_irq devm_free_irq devm_ioremap
devm_pinctrl_get 　http://www.wowotech.net/gpio_subsystem/pin-control-subsystem.html    pinctrl子系统

用户空间和内核空间数据交换
长的数据ｃｏｐｙ，大于８效率比较高：
     unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);
     unsigned long copy_from_user(void * to, const void __user * from, unsigned long n)
char int之类的简单类型数据ｃｏｐｙ
     put_user ( x, ptr); --  Write a simple value into user space, len = sizeof(*ptr)
     get_user ( x, ptr); --  Get a simple variable from user space, len = sizeof(*ptr)
     例如：　unsigned int val; int __user *p = argp; put_user(val, p); get_user(val, p);
驱动中实现延时的函数：bool mci_wait(){ unsigned long timeout = jiffies + msecs_to_jiffies(500);do {...} while (time_before(jiffies, timeout)); }

字符设备驱动
比较简单，申请和释放字符设备号，分配初始化和注册注销ｃｄｅｖ设备数据结构，填充和实现ｏｐｓ。ｃｄｅｖ是内核对字符设备的一个抽象，特定的字符设备往往还需要额外的信息，通常新定义一个设备结构体实现ｃｄｅｖ＋特有信息,
     struct ｘｘｘ_dev  {struct cdev cdev; ... ...; };
     ｄｅｖ_t xx_devno;         (typedef u_long dev_t;)
     static int __init xxx_init(void){
         alloc_chrdev_region(&xx_devno, ...); cdev_init(&xxx_cdev, ＆ｘｘｘ_fops); cdev_add(&xxx_dev.cdev, xxx_devno, 1);
     }
     static void __exit xxx_exit(void) {
         ｃｄｅｖ_del(&xxx_cdev); unregister_chrdev_region(xxx_devno, 1);
     }
cat /proc/devices 查看设备号分配情况
MKDEV(major, minor)　MAJOR(devt)　　MINOR(devt)     ３２位系统中，１２位主设备号＋２０位次设备号

#include<linux/cdev.h>

void cdev_init(struct cdev *dev,struct file_operations *fops);//初始化cdev结构
int cdev_add(struct cdev *dev,dev_t num,unsigned int count);
void cdev_del(struct cdev *dev);//移除一个字符设备

struct cdev {
     struct kobject kobj;          // 每个cdev 都是一个 kobject
     struct module *owner;       // 指向实现驱动的模块的指针,用于引用计数
     const struct file_operations *ops;   // 操纵这个字符设备文件的方法
     struct list_head list;       // 与cdev 对应的字符设备文件的 inode->i_devices 的链表头，将所有ｃｄｅｖ设备组成一个链表；
     dev_t dev;                  // 起始设备编号
     unsigned int count;       // 设备范围号大小，使用该字符设备驱动的设备数量
};

ｉｎｏｄｅ结构体表示/dev目录下的设备文件，打开一个设备文件系统产生一个ｉｎｏｄｅ节点，通过其i_cdev字段找到ｃｄｅｖ字符设备结构体，通过ｃｄｅｖ的ｆｏｐｓ调用设备操作函数。一个ｃｄｅｖ的驱动，和所有自己支持的设备文件组成一个链表。

内核模块一些定义：
头文件＋模块参数＋模块加载函数＋模块卸载函数＋模块许可声明，首先还要保证有内核源码，而且内核成功编译过且版本一致

基本命令： insmod lsmode rmmod
内核模块必然包含的头文件：
     #include <linux/module.h>
     #include <linux/init.h>
在用户态下编程可以通过main(intargc,char*argv[])的参数来传递命令行参数，而编写一个内核模块则通过module_param()来传递参数
module_param(name,type,perm定义权限值);   例如：  int a = 0;  ｍｏdule_param(a, int, S_IRUSR);　运行：   inmod xxx.ko a=1
     #define S_IRUSR 00400文件所有者可读　#defineS_IWUSR00200文件所有者可写　#defineS_IXUSR 00100文件所有者可执行
     那么需要额外包含　#include <linux/moduleparam.h>

EXPORT_SYMBOL(FUNCTION) FUNCTION在模块加载的时候记录到内核符号表中，别的模块可以调用，用它来对应别的模块中未实现的定义。

MODULE_PARM_DESC(a, "....");
MODULE_AUTHOR("Infotm");
MODULE_DESCRIPTION("imapx Watchdog Device Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:imap-wdt");
MODULE_VERSION("V1.0.0");

wine和source insight的安装使用
sudo apt-get install wine
wget http://kegel.com/wine/winetricks
chmod +x winetricks
sudo mv winetricks /usr/local/bin
winetricks dotnet20   (并不是必须的)
wine sourceinsight35.exe   安装完毕
直接点击搜索source(和alt+f2不同?),找到source insight;
用户主目录/home/yuan下生成一个隐藏的.wine目录,下面的drive_c 即相当于Windows系统中的C盘。我们还可以使用winecfg命令来配置Wine.
搜索configure wine,配置添加D:, path为: /home/yuan/work，然后就可以为工程添加文件了。之后可以考虑多添加几个disk做不同的开发

sshpass无需密码ssh
安装sshpass
     https://sourceforge.net/projects/sshpass/ 下载 sshpass-1.05.tar.gz
     tar -zxvf sshpass-1.05.tar.gz
     cd sshpass-1.05
     ./configure
     make && make install
脚本自动执行如下：
     sshpass -p karryalex1 ssh karry@192.168.1.106 -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no
     cd app_release
     ./ExUpgradePack_hisi3536.sh v3000 V3.0.0.0 1
     exit
    注： sshpass -p karryalex1 避免输入密码；-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no 避免输入yes

/work/dev_qsdk/products/q3fevb_va_ipc/system, 产品相关的定义，脚本等，比如gdb就可以放在这里
cat /proc/devices
/sys/kernel/uevent_helper

开发环境相关
Secure shell/putty      cJSON
FTL是Flash translation layer的英文缩写，FTL是一种软件中间层，最初是由intel提出的，用于将闪存模拟成为虚拟块设备，从而能够在闪存上实现FAT等等块设备类文件系统。

Jimmy邮件建议流程：
    1)  gerrit登录地址 ;
    直接登录  http://gerrit.in.infotm.com     账户：eric.yun     密码：637004
    2)  登录后请先在gerrit上注册你的邮件地址(gerrit邮件注册链接应该已经通过邮件发送到你的邮箱，请登录邮箱查收)，然后在本地生成public key ;
    #ssh-keygen     #cat ~/.ssh/id_rsa.pub     将id_rsa.pub的内容复制到gerrit中的 setting>>>ssh public keys>>>add
    3)验证public key是否生效 ;
       #ssh -p 29418 eric.yun@gerrit.in.infotm.com  检查欢迎信息，确认public key是否生效
    4)  在本地请下载repo工具并安装 ;
    #git clone http://gerrit.in.infotm.com/repos/repo_tools
    #cd repo_tools     #./install.sh
    5)  配置账户信息
    # git config --global user.email "eric.yun@infotm.com"
    # git config --global user.name "eric.yun"
    6)  可以通过repo下载git分支了, 工程直接下载到当前目录下
    repo init -u ssh://eric.yun@gerrit.in.infotm.com:29418/manifest/buildroot/ -b dev_qsdk
    repo sync -c
    repo start dev_qsdk --all

    repo init -u ssh://user.name@gerrit.in.infotm.com:29418/manifest/buildroot -b dev_Q3_carDv_mc
    repo sync -c
    repo start --all dev_Q3_carDv_mc

    repo init -u ssh://eric.yun@gerrit.in.infotm.com:29418/manifest/buildroot -b dev_qsdk_dv
    repo sync -c &
    repo start --all dev_qsdk_dv

    在编译前，需要先确保以下包已经安装：
        sudo apt-get install make automake autoconf gcc g++ python curl lzop perl build-essential libncurses5 libssl-dev

     编译服务器:  123456
          ssh eric.yun@192.168.0.14

     //服务器上exchange应该是home的映射, 用于访问自己的软件信息
          sudo mount -t nfs 192.168.0.14:/exchange /home/yuan/work/mynfs
          sudo chown -R yuan.yuan /home/yuan/work

Redmine 帐号信息: * 登录名: eric.yun * 密码: mPRCAAFVMH   登录: http://27.115.104.234:8810/redmine/login

增加库依赖:
testing目录下, configure.ac 中 增加 PKG_CHECK_MODULES(LIBSYSTEM, qlibsys >= 1.0.0)
修改Makefile.am文件,
AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS = mmc_test
mmc_test_SOURCES = mmc_test.c
mmc_test_CFLAGS= -I./ -I$(top_srcdir)/ $(LIBSYSTEM_CFLAGS) -g -pthread -Wall -Werror
mmc_test_LDFLAGS= -version-info 1:0:0 $(LIBSYSTEM_LIBS) -g -pthread

ｍｉｓｃ信息
拿到的Ｑ３Ｆ EVM板标签上ID是A5　     USB转串口，查询驱动是否就绪 modinfo ftdi_sio , 线的顺序为 黑色 白色 绿色。
BSP岗位职责：      RTL及FPGA黑盒测试
owncloud/shared/InfoTM/Q3有电路图和手册

GitHub : eckelyun eckelyun@hotmai.com 哀家    https://github.com/        http://git-scm.com/
     如果要使用GitHub的服务，到GitHub上创建GitHub帐号

建立SVN
1 设置SVN_EDITOR环境变量:               export SVN_EDITOR=vim
2 创建并且import项目,需要root:           su
     cd /home/svn
     mkdir app2016
     cd /home/svn/app2016
     svnadmin create /home/svn/app2016
     svn  import  -m "Import at 2016/03/02" /home/eric/app2016  file:///home/svn/app2016
3. 启动svn服务
     svnserve -d -r /home/svn/     //-d表示后台运行       -r/svn/指定根目录是/svn/
4. 访问SVN:
     svn://192.168.1.111/application/gui
     svn://192.168.1.111/application/gui_WinVersion
     svn://192.168.1.111/application/gui_TY_Base
     svn://192.168.1.111/app2016/nvrsrc/trunk
     svn://192.168.1.111/app2016/nvrtool
     svn://192.168.1.111/app2016/nvrsrc/
     svn://192.168.1.111/app2016/nvrdoc
5. linux环境导出
     svn checkout svn:///home/svn/app2016/nvrsrc/trunk /home/eric/hisi
     svn checkout svn:///192.168.1.111/app2016/nvrsrc/trunk /home/eric/new
     svn checkout svn://192.168.1.111/appllication/gui //创建并导出到gui目录
     svn update -r 1230 hisi //更新指定目录到1230版本
     svn update hisi     //更新指定目录
6  备份与恢复
     svnadmin dump /home/svn/application/gui  > guiBranchDumpFile
     可以指定版本-r 0:50，方便定期备份
          svnadmin dump newRepo -r 0:50 > dumpfile1
          svnadmin dump newRepo -r 51:100 --incremental > dumpfile2
          svnadmin dump newRepo -r 101:161 --incremental > dumpfile3
     恢复或者转移
          svnadmin load newRepo2 < dumpfile1
          svnadmin load newRepo2 < dumpfile2
          svnadmin load newRepo2 < dumpfile3

重启Apache 2 web 服务器
sudo /etc/init.d/apache2 restart

关闭启动svn
killall svnserve

启动
svnserve -d -r /home/svn/

#更改版本库所属用户、组           chown -R root:subversion myproject

密码文件dav_svn.passwd的创建
sudo htpasswd -c /etc/subversion/dav_svn.passwd user_name
它会提示你输入密码，当您输入了密码，该用户就建立了。“-c”选项表示创建新的/etc/subversion/dav_svn.passwd 文件，
所以user_name所指的用户将是文件中唯一的用户。如果要添加其他用户，则去掉“-c”选项即可：
sudo htpasswd /etc/subversion/dav_svn.passwd other_user_name

配置SVN
获取唯一 root权限，svn目录禁止其他用户查看
1 修改/conf/svnserve.conf文件，禁止匿名用户读写
anon-access = none
auth-access = write
password-db = passwd
authz-db = authz

2 修改/conf/passwd 文件
[users]
chenlong = chenloug
eric = visking
fanliang = 123456
xiushu = xiushu

3 修改/conf/authz 文件
[groups]
g_manager = eric,chenlong
g_all = eric,chenlong,xiushu,fanliang
g_gui = fanliang,eric
g_onvif = xiushu,chenlong

[app2016:/]
@g_manager = rw
# @g_all = r

[app2016:/nvrsrc]
@g_all = rw

[app2016:/nvrdoc]
@g_all = rw

[app2016:/nvrtool]
@g_all = rw

[app2016:/CustomerIPCdoc]
@g_all = rw

相关文章描述如何实现精细目录管理: http://www.cnblogs.com/wuhenke/archive/2011/09/21/2184127.html

