RAID功能开发
rtsp://INFINOVA:INFINOVA@192.168.1.100:554/1/h265major

基本数据结构的新定义：
1. d_info[]管理通用的硬盘设备，包括sata和raid等，这是上层看到的结构。
2. sata_info[]管理物理sata硬盘，
3.

mdadm编译:
1. 下载并解压mdadm软件，进入目录
2. gcc配置,export命令修改使用的GCC
    3535平台编译mdadm的话， export CROSS_COMPILE="arm-hisiv200-linux-"
    3536平台编译mdadm的话， export CROSS_COMPILE="arm-hisiv400-linux-"
3. make
4. 把编译好的mdadm程序copy到开发板的/opt/bin目录下

RAID功能需要实现的几个点：
     1. 实现基本功能
     2. 硬盘故障通知与操作
     3. 如果系统恢复出厂设置，那么我们要删除所有的RAID信息。
     4. RAID盘多块sata的休眠与唤醒控制
     5.

硬盘分区
karry已经提供新的分区函数，增加一个参数，可以设定分区个数(最小为3)
          int ide_new_partition_number(char *disk_device, unsigned int part_num);
     9. RAID和盘组的功能，最好是互斥的，最多，只有备份盘组有效。
     使用分区功能，拿到了karry在3531平台上编译的 libparted.so libparted.so.1 ，拷贝到/usr/lib目录下
常用parted命令
     parted /dev/sdc print            parted /dev/sdc mklabel gpt               parted /dev/sdc rm sdc1
     i | parted /dev/sdc mkpart sdc1 ext2 0 100

下一步：RaidSleepFlag
     ide_wakeup则是唤醒所有对应的SATA，这个函数需要重新实现。
     然后是GET_INFO函数重新实现
     然后是RAID组配置，和删除RAID
     然后是spare配置。
     CopyDateToRecBuf函数中
          if ((1ull << d_info[rec_info[attr][ch].disk_no].mask) & ide_info64.ide_bad) 改成函数控制。
               mask大于 MAX_DISK_NUM 的话，直接返回正确。
     Ide_wakeup_bolck 函数
          ide_wakeup
     GetDiskInfo 函数中
          info[i].mask = d_info[i].mask; 好像可以保持不变。
     ThreadIdeStandby 函数
          ide_setstandby

     #ifdef IPC_PROTO_ARECONT          #ifdef IPC_PROTO_TURUIW

休眠: 写raid更新所有相关盘的访问时间last_access_time就可以，使用旧机制管理休眠
     sata_info                    硬件设备
     submd_info                ｍｄ设备
     raid_info                     RAID组设备信息

虚拟槽位，虚拟磁盘(最多支持256个)
          虚拟的磁盘，加上一个特殊的标记名称，比如"nfs"或者"md"之类的，也需要增加一个特殊的标号。
          GUI部分的硬盘管理
.mask相关的修正：MD被看做拓展的虚拟的SATA插口。其他的软件，是否可以同样处置呢？引入了虚拟槽位的概念

INDEX文件动态映射功能：
     映射mmap部分，排序完成之后，可以考虑，只安排若干个动态的文件映射，当前不访问的磁盘，不做映射。
     但是，录像的搜索功能必须要有这部分的信息。所以，有必要限制一下录像搜索的范围，一次不能过多
     现场可以试试看，有这个BUG的最小时间点在哪里。


     Ide_wakeup_bolck等唤醒函数，需要同时唤醒多块硬盘。      ThreadIdeWakeup

./mdadm --create --verbose /dev/md1 --level=5 --raid-devices=3 /dev/sda /dev/sdb /dev/sdc --spare-devices=1 /dev/sdd

海康设备RAID功能
     增加阵列     删除阵列     阵列状态，管理     阵列自动和手动重建。     热备盘重建和物理磁盘重建
     阵列迁移，包括增加硬盘到阵列，或者RAID5到RIAD10
     阵列上虚拟磁盘的创建和删除和修复，我们需要自动进行，如果继续使用FAT32的话。

     通过下面命令检测RAID功能是否就绪：
          dmesg |grep -i raid           dmesg |grep -i md     cat /proc/devices | grep md
     cat /etc/mdadm.conf
     mdadm --detail /dev/md0
     cat /proc/mdstat
     df -hT               //显示mount信息
     启动过程脚本完成，或者启动另外一个程序，在ruby之前运行。
     State : clean // clean, degraded  // clean, degraded, recovering
     mdadm /dev/md0 --fail /dev/sdb
     mdadm /dev/md0 --remove /dev/sdb
     mdadm /dev/md0 --add /dev/sdb
自动启动：
     mdadm --detail --scan > /etc/mdadm.conf
     mdadm -Ds >>/etc/mdadm.conf
     mdadm -E /dev/sda1      检查/dev/sda1上是否构建有RAID
     mdadm -As /dev/md0           已经有/etc/mdadm.conf配置文件的情况下，调用这个命令从配置文件读取信息执行。
     mdadm -S /dev/md0
     mdadm -A -s /dev/md0   停止之后重新启动
     mdadm -A /dev/md0 /dev/sda1 /dev/sdb1 /dev/sdc1
RAID设备销毁过程： 先删除RAID中的所有设备，然后停止该RAID即可
    ./mdadm /dev/md1 --fail /dev/sda --remove /dev/sda
    ./mdadm /dev/md1 --fail /dev/sdb --remove /dev/sdb
    ./mdadm /dev/md1 --fail /dev/sdc --remove /dev/sdc

    ./mdadm --stop /dev/md1
    ./mdadm --remove /dev/md1

    ./mdadm --misc --zero-superblock /dev/sda
    ./mdadm --misc --zero-superblock /dev/sdb
    ./mdadm --misc --zero-superblock /dev/sdc

    为了防止系统启动时候启动raid
    rm -f /etc/mdadm.conf
    rm -f /etc/raidtab
    检查系统启动文件中是否还有其他mdad启动方式
    vi /etc/rc.sysinit +/raid\c

