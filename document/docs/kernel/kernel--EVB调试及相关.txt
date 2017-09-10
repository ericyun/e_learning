EVB调试及相关
两种方式添加软件包，autotools 和 cmake

庄老师检查过的几个文件
qlibsys/qlibsys.mk          /products/q3fevb_va/configs/qsdk_defconfig
qlibupgrade/qlibupgrade.mk  buildroot/package/pkg-autotools.mk  system/external.mk
qlibsys/Config.in
编译完成的文件，目标是../output/system/
make qlibsys-rebuild
make qlibsys-dirclean
vi testing/vbctrl/Makefile.am
make qlibsys-rebuild
ls system/
ls system/qlibsys/qlibsys.mk
vi system/qlibsys/qlibsys.mk
vi system/qlibsys/Makefile.am
ls -l hlibhid/
vi hlibhid/Makefile.am
vi hlibhid/hlibhid.mk
vi hlibhid/Makefile.am
 2362  vi hlibhid/hlibhid.mk
 2363  ls qlibsys/qlibsys.mk
 2364  vi qlibsys/qlibsys.mk

烧录设备的配置
当前 QSDK 上烧录设备和分区的配置都是通过配置 item 里面对应项来完成的，并且是唯一配置方式，现在烧录设备支持 spi norflash 和 emmc； 设备的配置是 board.disk 如果是 spi noflash 配置为 flash，emmc 则配置为 emmc。

分区的配置
分区的配置项是 part 后面加上数字（如 part0,part1），每个配置支持4个字段的配置信息，中间以 . 隔开（如part0 uboot.48.boot），各字段意思如下：
以part5 system.8000.fs.ext4为例
(1)分区的名字，上例表示该分区为system分区；
(2)分区的大小，单位为K字节，上例表示分区为8M，即 8000 * 1024 字节；
(3)分区类型，支持三种， boot 表示物理分区，即该分区的操作的是设备的真实物理地址, 并且所有 boot 类型分区在kernel里面会统一在一个节点里面(如/def/spiblock0)，主要用于存储 uboot kernel 等系统镜像； normal 也表示物理分区，不同的是每个 normal 分区会产生独立的节点，主要用于存储 uboot 需要访问且只读的文件系统， 如 uboot 需要烧录的 squashfs 文件系统； fs 表示逻辑分区，即有经过 FTL 映射过的分区， 有独立节点，主要用于存储可读写文件系统，如 ext4, fat等，或其它任意非 uboot 需要访问的分区；上例表示该分区为逻辑 fs 分区。
(4)文件系统类型， boot 分区该字段无效， noraml 分区支持各种只读文件系统，如squashfs, cramfs等， fs 支持各种只读和可读写文件系统，如 squashfs, ext2/ext4, fat等。

使用注意事项
(1) part 后面的数字，如果是需要 ius 烧录的系统镜像，必须和 ius 镜像模板里面 id 项一致， 如 part0 uboot.48.boot 和 ius r flash 0x0 uboot0.isi 粗体部分要保持一致，烧录的时候模板 id 所对应的镜像将会烧录到 part 后面对应数字的分区里面。如果是非镜像烧录分区，则可随意添加。
(2)如果不需要某个系统分区，如不需要 ramdisk 分区，只需要在 item 里面注释掉，#part2 ramdisk.2000.boot就行，不能把后面的 part3 改为 part2。至于用户自己添加的分区，可根据需要添加。
(3)uboot0以及item分区是固定的，目前不能修改。
(4)如果需要在uboot1进行烧录，除 uboot0, item 分区外其它分区的大小定义需 64K 对齐。

