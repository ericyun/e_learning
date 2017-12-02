# 基于BuildRoot集成开发环境

### 修订记录
| 修订说明 | 日期 | 作者 | 额外说明 |
| --- |
| 初版 | 2017/09/14 | 员清观 | 常用操作索引，尽量简短方便使用 |

----
## 99 处理中问题

buildroot:
Target options : 目标（也就是制作出来的工具给什么平台使用）
Build options ：配置（也就是buildroot一些配置项，比如下载的文件放在哪里，编译生成的文件放在哪里等）
Toolchain：编译器（可以配置生成交叉编译器或者引用已有的交叉编译），如果是配置生成交叉编译器则按照Target options生成对应的编译器。
System configuration：系统配置（其实就是是否配置制作根文件系统）
Kernel：存放Linux内核和Linux内核的配置
Target packages：目标包，简单来说就是里面可以配置生成根文件系统的busybox和其他的一些第三方库，比如：是否支持qt、mplayer等。


哪里定义的：  $(TOPDIR)   $(CONFIG_DIR)   $(BASE_DIR)
当前build路径配置选项
QSDK Options->App->Busybox lite -- ( ${TOPDIR}/output/product/configs/bblite_defconfig )
Bootloader->Local source -- ( ${TOPDIR}/bootloader/apollo3 )
Filesystem image->INITRD overlay directories -- ( ${TOPDIR}/output/product/root )
Target packages->Busybox version -- ( ${TOPDIR}/output/product/configs/busybox_defconfig )
Kernel->Local path of linux source -- ( ${TOPDIR}/kernel )
Kernel->Configuration file path -- ( ${TOPDIR}/output/product/configs/linux_defconfig )
System Configuration->Path to the permission tables -- ( ${TOPDIR}/output/product/device_table.txt )
System Configuration->Path to the permission tables -- ( ${TOPDIR}/buildroot/system/ramdisk_device_table.txt )
System Configuration->Root filesystem overlay directories -- ( $(TOPDIR)/output/product/system )

System Configuration->Custom scripts to run before creating filesystem images -- ( ${TOPDIR}/tools/prepare-script.sh )
System Configuration->Custom scripts to run after creating filesystem images -- ( ${TOPDIR}/tools/post-scripts.sh )
Toolchain->Toolchain path -- ( ${TOPDIR}/buildroot/prebuilts/uclibc-4.7.3 )
Toolchain->Toolchain prefix -- ( $(ARCH)-buildroot-linux-uclibcgnueabihf )

Build options->Location to save buildroot config -- ( $(CONFIG_DIR)/defconfig )
Build options->Download dir -- ( $(TOPDIR)/buildroot/download )
Build options->Host dir -- ( $(BASE_DIR)/host )
Build options->Compiler cache location -- ( $(TOPDIR)/.ccache )
Build options->location of a package override file -- ( $(TOPDIR)/local.mk )

busybox配置：
     debugging, profiling and benchmark --> strace
     shell and utilities --> file
     system tools --> fsck


----
## 3.常用分析工具

~~~
## hexdump -C ./fsck.fat


~~~

objdump -p ./fsck.fat


~~~
patch文件制作和使用场景：
cd buildroot-2009.11
patch -p1 < ../buildroot-2009.11.patch

redmine 的comment中，#4423 前后用空格区分开，可以显示bug的索引链接。

~~~

----
## 2.编译选项
-shared 
Produce a shared object which can then be linked with other objects to form
an executable. Not all systems support this option. 
(产生一个可以被其他obj链接生成可执行文件的共享obj。并非所有的系统都支持此选项)
For predictable results,
you must also specify the same set of options that were used to generate code
(‘-fpic’, ‘-fPIC’, or model suboptions) when you specify this option.1

gcc \
   -o libJava.so       \ #. 显示指定目标文件名. 否则就是a.out啦
   -shared -fPIC     \ #. 编译共享库的参数(组定搭档). 否则就没有“main”报错喽
   a.o b.o c.o          \ #. 由原代码a.c b.c c.c编译出来的一堆东东集中在一块
   ./libdemo.a          \ #. 依赖其它的一些lib库
   -ldl -lpthread      \ #. 依赖的系统共享库 对应libdl.so, libpthread.so (这里是举个例了，当然你的程序可能不需要哈)

上面几个文件中的函数才会被封装到库中，所以，hlibvideodebug链接找不到需要的audio函数是因为函数放在了错误的文件中，不要理解错误。
文档整理所有的编译相关的信息

----
## 1.代码组织

./tools/setproduct.sh 中　make qsdk_defconfig
output/product/root initrd

output目录的介绍：
./product, 当前product的拷贝
./build，下载的软件包，或者本地软件包的拷贝，可能经过工具的修改；无法在此手动修改，编译时自动更新。已经在此接触过的模块有，dosfstools/eventhub/uboot-lite/uboot1-1.0.0/libiconv-1.14。所有源码的编译路径
host目录：存放交叉编译器，如果指定是外部编译器，会把外部编译器拷贝到此处。buildroot编译生成的也是存放在此。
    ~/work/ipc_dev/output/host/usr为交叉工具，可以在host上查询控制目标板上软件
images目录：存放根文件系统的打包好的各个格式，比如：ext，yaffs等…..
target目录：编译出来的根文件系统存放的路径（也就是待会生成根文件系统的路径，用nfs挂载即可）

带videobox的测试：　　./tools/setproduct.sh　，　并且选择apoll3_evb, sensor1 x, sensor0 mipi, jason x

