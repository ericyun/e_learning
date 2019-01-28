H265 MISC信息
需要学习的相关协议：
     (RTSP) [RFC2326]      HEVC coded data over RTP [RFC3550]     Session Description Protocol (SDP) [RFC4566]

流媒体开发论坛    ORTP     下载vlc源代码： http://www.videolan.org/vlc/download-sources.html
MPlayer和VLC都是开放源代码的；自己编译，使用ffmpeg+live555编写一个自己的媒体播放器，可以在手机端直接播放电脑上视频文件。
live555.cpp好像是和live555之间的调用接口。

英飞拓摄像机  rtsp://192.168.1.100/1/h265major     rtsp://192.168.1.100/1/h265minor

Network Abstraction Layer(NAL)     sequence-level parameter sets(SPS)   picture-level parameter sets(PPS)
     supplemental enhancement information (SEI)     Video parameter set(VPS)

NAL基本格式( (type&0x7E)>>1 )：
+---------------+---------------+
|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+-+-+-+-+-+
|F| Type         | LayerId     | TID  |
+-------------+-----------------+

NAL数据例子：
VPS: 40 1 c 1 ff ff 1 60 0 0 3 0 b0 0 0 3 0 0 3 0 5d aa 2 40
SPS: 42 1 1 1 60 0 0 3 0 b0 0 0 3 0 0 3 0 5d a0 2 80 80 2d 16 36 aa 49 32 f9
PPS: 44 1 c0 f2 f0 3c 90
在H265文档中搜索 NAL unit type codes and NAL unit type classes 得到NAL unit类型列表
     I帧： 19      20      VPS:32  SPS:33  PPS:34   P帧： 0-9或者小于19？   其他：暂时作为无效还是P帧呢？

一个H265的RTSP描述范例：
v=0
o=- 1444035698757914 1 IN IP4 192.168.1.100
s=streamed by the LIVE555 Media Server
i=1/h265major$transportmode#unicast+Qos#Normal+wrap#none+
t=0 0
a=tool:LIVE555 Streaming Media v2015.04.16
a=type:broadcast
a=control:*
a=range:npt=0-
a=x-qt-text-nam:streamed by the LIVE555 Media Server
a=x-qt-text-inf:1/h265major$transportmode#unicast+Qos#Normal+wrap#none+
m=audio 0 RTP/AVP 0
c=IN IP4 0.0.0.0
b=AS:64
a=control:track1
m=video 0 RTP/AVP 97
c=IN IP4 0.0.0.0
b=AS:500
a=rtpmap:97 H265/90000
a=fmtp:97 profile-space=0;profile-id=1;tier-flag=0;level-id=93;interop-constraints=B00000000000;sprop-vps=QAEMAf//AWAAAAMAsAAAAwAAAwBdqgJA;sprop-sps==
a=control:track2

