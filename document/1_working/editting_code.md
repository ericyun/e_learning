
## 2.test

**时间统计**
```cpp
//#include "<linux/time.h>"
extern void do_gettimeofday(struct timeval *tv);
struct timeval {
　　time_t tv_sec;       /* seconds */
　　suseconds_t tv_usec; /* microseconds */
};

struct timeval time_val;
do_gettimeofday(&time_val);
```

**音频测试常用命令**
```
mount -t vfat /dev/mmcblk0p1 /mnt
abctrl play -w 32 -s 16000 -n 2 -d /mnt/pcm_16khz_ch2_32b.wav
abctrl play -w 32 -s 16000 -n 2 -v 256 -d /mnt/pcm_16khz_ch2_32b.wav
abctrl record -w 32 -s 16000 -t 12 -n 2 -o /mnt/voice.wav

mount  /dev/mmcblk0p1 /mnt
abctrl play -w 32 -s 16000 -n 2 -d /mnt/pcm_16khz_ch2_32b.wav

time dd if=/dev/zero of=/dev/mmcblk0p1 bs=64k count=6400
time dd if=/dev/mmcblk0p1 of=/dev/null bs=4k count=102400

sudo dd if=/home/yuan/work/h_lab_car/new_mbr of=/dev/sdb bs=512 count=1
sudo dd if=/dev/sdb if=/home/yuan/work/h_lab_car/new_mbr bs=512 block=1

abctrl record -w 32 -s 48000 -t 12 -n 2 -o /mnt/sd0/ten_years.wav
abctrl play -w 32 -s 48000 -n 2 -d /mnt/sd0/ten_years.wav
abctrl encode -t 0:7 -w 32:16 -s 48000:48000 -n 2:2 -d /mnt/sd0/ten_years.wav -o /mnt/sd0/ten_years.aac
abctrl decode -t 7:0 -w 16:32 -s 48000:16000 -n 2:2 -d /mnt/sd0/ten_years.aac -o /mnt/sd0/ten_years_2.wav
abctrl play -w 32 -s 16000 -n 2 -d /mnt/sd0/ten_years_2.wav
```
