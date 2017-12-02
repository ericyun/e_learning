# QSDK功能说明 `v2.3.0`


Note: 功能点共计`94`项，`v2.3.0`新增`6`项

|新增功能编号|类别|功能说明|
|---|
|`089`|图像处理|双目全景拼接 - 适用于`Apollo-ECO`|
|`090`|图像处理|实时调试信息输出|
|`091`|视频编码|实时调试信息输出|
|`092`|视频编码|SEI userdata设置|
|`093`|音频编码|实时调试信息输出|
|`094`|IQ|IQ调试工具|

----
## Bootloader

QSDK的Bootloader基于U-boot定制。

* `[001]` `uboot0` - 小于48KB，提供启动Kernel的功能，可以单独使用
* `[002]` `uboot1` - 200KB左右，需要与`uboot0`配合使用。提供命令行交互、网络及MTD分区烧写功能。主要用于安防等需要U-boot标准功能的产品

启动设备支持：

* `[003]` SPI Norflash
* `[004]` eMMC


## Kernel
----

`[005]` QSDK的Kernel基于Linux 3.10定制。

## RootFS
----

QSDK的RootFS基于Buildroot系统定制，围绕Apollo系列芯片提供以下功能。

### 图像处理
* Camera接入
- `[069]` Container

  - mkv
  - mov
  - mp4
  - 