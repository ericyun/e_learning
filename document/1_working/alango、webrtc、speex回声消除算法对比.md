# alango/webrtc/speex三种回声消除算法对比

###  修订记录
| 修订说明 | 日期 | 作者 |
| --- |
| 初版 | 2018/11/05 | 员清观 |

## 1 概述
本文通过回声消除效果量化评估，完成了下面三种AEC算法效果的对比测试：
- alango aec算法(基于Q3F DSP实现)
- speex aec算法(基于ARM实现)
- webrtc aec算法(基于ARM实现)

基于Q3F音频测试环境，在回声消除效果方面，speex算法和alango算法差别不大，都优于webrtc算法；在占用cpu资源方面，speex算法占用arm资源仅为webrtc算法一半，alango算法不占用arm资源。

我们使用了7个不同的参考音源作为回声消除参考音来完成对比测试，其中包括4个人声文件，3个音乐文件。为了脚本中处理方便，按照`test?.wav`命名(?通配0~6的数字)。双向通话场景下，另外使用`pc_voice.wav`作为前景录音：
- test0.wav ～ 英文歌曲，女声，背景音乐很强
- test1.wav ～ 中文歌曲，女声，背景音乐弱
- test2.wav ～ 中文歌曲，男声，背景音乐适中
- test3.wav ～ 英文对话，男声
- test4.wav ～ 故事朗诵，男声
- test5.wav ～ 不断重复的女声: "录像开始"
- test6.wav ～ 故事朗诵，女声
- pc_voice.wav ～ 故事朗诵，女声

测试结果中产生的中间文件的定义如下(?通配0~6的数字)：
- test?_cap, aec算法输入的双声道原始音频文件，由audiobox录音线程取alango算法输入音频产生，其左声道为回声参考音，右声道为实际录音，采样率8000Hz，32bits
- test?_dsp.wav，alango aec算法处理结果, 单声道，采样率8000Hz，16bits
- test?_cap_speex_256_512.wav，speex aec算法处理结果, 单声道，采样率8000Hz，16bits
- test?_cap_webrtc_80_3ms.wav，webrtc aec算法处理结果, 单声道，采样率8000Hz，16bits

回声消除效果量化评估可以分成两个场景，不同场景使用不同方式来分析量化：１．单向通话aec效果使用RMS振幅来评估;２．双向通话aec效果使用PESQ算法来评估。我们用如下方式模拟单向和双向通话场景：
- 单向通话场景 ～ EVB板播放test?.wav的同时开启录音
- 双向通话场景 ～ EVB板播放test?.wav的同时开启录音，pc同步播放pc_voice.wav

----
## 2 单向通话场景aec效果测试
单向通话的场景下，回声消除后的信号越接近静音代表回声消除的越干净．这种情况使用RMS振幅来评估．RMS就是均方根，是一组统计数据的平方和的平均值的平方根，即：将N个项的平方和除以N后开平方的结果，即均方根的结果。RMS振幅是将语音信号振幅平方的平均值再开平方，由于振幅值在平均前平方了，因此，它对特别大的振幅非常敏感，适合用来描述信号偏差程度，比如此处单边语音时回声消除之后的残留回声．

使用Adobe Audition->窗口->振幅统计(A)功能来统计总体RMS振幅．总体RMS振幅越小，表示回声消除的越干净．

模拟单向通话场景下测试结果如下：
```
~~~~~~~~RMS level for capture file:  test0_cap~~~~~~~~
test0_dsp.wav                    :  -62.86 dB
test0_cap_speex_256_512.wav      :  -77.36 dB
test0_cap_webrtc_80_3ms.wav      :  -50.76 dB

~~~~~~~~RMS level for capture file:  test1_cap~~~~~~~~
test1_dsp.wav                    :  -59.45 dB
test1_cap_speex_256_512.wav      :  -74.78 dB
test1_cap_webrtc_80_3ms.wav      :  -51.97 dB

~~~~~~~~RMS level for capture file:  test2_cap~~~~~~~~
test2_dsp.wav                    :  -59.04 dB
test2_cap_speex_256_512.wav      :  -66.11 dB
test2_cap_webrtc_80_3ms.wav      :  -46.92 dB

~~~~~~~~RMS level for capture file:  test3_cap~~~~~~~~
test3_dsp.wav                    :  -72.90 dB
test3_cap_speex_256_512.wav      :  -77.64 dB
test3_cap_webrtc_80_3ms.wav      :  -47.03 dB

~~~~~~~~RMS level for capture file:  test4_cap~~~~~~~~
test4_dsp.wav                    :  -71.34 dB
test4_cap_speex_256_512.wav      :  -74.81 dB
test4_cap_webrtc_80_3ms.wav      :  -50.13 dB

~~~~~~~~RMS level for capture file:  test5_cap~~~~~~~~
test5_dsp.wav                    :  -71.01 dB
test5_cap_speex_256_512.wav      :  -77.69 dB
test5_cap_webrtc_80_3ms.wav      :  -45.90 dB

~~~~~~~~RMS level for capture file:  test6_cap~~~~~~~~
test6_dsp.wav                    :  -72.38 dB
test6_cap_speex_256_512.wav      :  -72.91 dB
test6_cap_webrtc_80_3ms.wav      :  -48.95 dB
```

**结论：**
speex和alango算法效果差别不大，都优于webrtc算法： speex和alango算法回声消除处理结束后都听不到回声，噪声也很小，对应RMS振幅也偏小；webrtc算法回声消除处理之后，经常出现回声消除不干净，而且噪声也较大，整体RMS振幅偏大。

----
## 3 双向通话场景aec效果测试
双向通话场景下，通过pesq算法评估音频处理效果。PESQ 即：主观语音质量评估，这是ITU-T P.862建议书提供的客观MOS值评价方法．PESQ程序可以比较原始音源文件和回声消除后失真的语音文件，计算得到量化的主观评分，最高4.5分，语音失真越大分数越低，低于1.0分听觉一般已经无法正常识别．

测试结果如下：
```
~~~~~~~~caculate pesq score for capture file:  test0_cap~~~~~~~~
test0_dsp.wav                    : PESQ_MOS = 3.241
test0_cap_speex_256_512.wav      : PESQ_MOS = 3.419
test0_cap_webrtc_80_3ms.wav      : PESQ_MOS = 2.726

~~~~~~~~caculate pesq score for capture file:  test1_cap~~~~~~~~
test1_dsp.wav                    : PESQ_MOS = 3.474
test1_cap_speex_256_512.wav      : PESQ_MOS = 3.528
test1_cap_webrtc_80_3ms.wav      : PESQ_MOS = 2.913

~~~~~~~~caculate pesq score for capture file:  test2_cap~~~~~~~~
test2_dsp.wav                    : PESQ_MOS = 3.201
test2_cap_speex_256_512.wav      : PESQ_MOS = 3.373
test2_cap_webrtc_80_3ms.wav      : PESQ_MOS = 2.683

~~~~~~~~caculate pesq score for capture file:  test3_cap~~~~~~~~
test3_dsp.wav                    : PESQ_MOS = 3.417
test3_cap_speex_256_512.wav      : PESQ_MOS = 3.526
test3_cap_webrtc_80_3ms.wav      : PESQ_MOS = 2.690

~~~~~~~~caculate pesq score for capture file:  test4_cap~~~~~~~~
test4_dsp.wav                    : PESQ_MOS = 3.489
test4_cap_speex_256_512.wav      : PESQ_MOS = 3.582
test4_cap_webrtc_80_3ms.wav      : PESQ_MOS = 3.007

~~~~~~~~caculate pesq score for capture file:  test5_cap~~~~~~~~
test5_dsp.wav                    : PESQ_MOS = 3.421
test5_cap_speex_256_512.wav      : PESQ_MOS = 3.459
test5_cap_webrtc_80_3ms.wav      : PESQ_MOS = 2.555

~~~~~~~~caculate pesq score for capture file:  test6_cap~~~~~~~~
test6_dsp.wav                    : PESQ_MOS = 3.504
test6_cap_speex_256_512.wav      : PESQ_MOS = 3.562
test6_cap_webrtc_80_3ms.wav      : PESQ_MOS = 2.808
```

**结论：**
speex和alango算法效果差别不大，都优于webrtc算法： webrtc算法处理结果能明显感觉到声音畸变，而另外两个算法不会。主观评测和pesq评估的结果是一致的。

----
## 4 cpu资源开销
将speex和webrtc的aec算法移植到q3f平台，使用双向通话场景的原始音频文件，统计转换时间。

q3f平台测试结果如下：
```
$ ./speex_aec test0_cap 256 512
aec proc file_size: 0xd8e00 finished in 19272 ms...
$ ./speex_aec test1_cap 256 512
aec proc file_size: 0xd8e00 finished in 18600 ms...
$ ./speex_aec test2_cap 256 512
aec proc file_size: 0xd8e00 finished in 19572 ms...
$ ./speex_aec test3_cap 256 512
aec proc file_size: 0xd8e00 finished in 18647 ms...
$ ./speex_aec test4_cap 256 512
aec proc file_size: 0xd8e00 finished in 19605 ms...
$ ./speex_aec test5_cap 256 512
aec proc file_size: 0xd8e00 finished in 18652 ms...
$ ./speex_aec test6_cap 256 512
aec proc file_size: 0xd8e00 finished in 19519 ms...

$ ./webrtc_aec test0_cap 80 3
aec proc file_size: 0xd8f40 finished in 40757 ms...
$ ./webrtc_aec test1_cap 80 3
aec proc file_size: 0xd8f40 finished in 40926 ms...
$ ./webrtc_aec test2_cap 80 3
aec proc file_size: 0xd8f40 finished in 40794 ms...
$ ./webrtc_aec test3_cap 80 3
aec proc file_size: 0xd8f40 finished in 40815 ms...
$ ./webrtc_aec test4_cap 80 3
aec proc file_size: 0xd8f40 finished in 40569 ms...
$ ./webrtc_aec test5_cap 80 3
aec proc file_size: 0xd8f40 finished in 40600 ms...
$ ./webrtc_aec test6_cap 80 3
aec proc file_size: 0xd8f40 finished in 40603 ms...
```

原始音频文件总时间长度为55520ms，根据aec算法执行转换的时间，大致估算speex cpu占用约34%，webrtc cpu占用约73%，而alango由于基于dsp实现，cpu占用基本为0%。

**结论：**
webrtc比speex算法开销大一倍。

----
## 5 结论
基于Q3F音频环境获取的原始音频数据，speex算法回声消除算法在消除效果和占用cpu资源方面，优于webrtc回声消除算法。和alango算法相比，speex算法有时候仍然会有一个很小幅度的回音，仍然保留了一些录音的细节，这是因为相对比于alango算法没有包含消噪处理的缘故。

NOTE: 提交一个新的包到owncloud，包含下面内容：
- 文件夹speexdsp-1.2rc3 调整过的speex程序，speexdsp-1.2rc3/libspeexdsp/testecho为回声消除处理程序
- 文件夹webrtc_aec 调整过的webrtc程序，webrtc_aec/audioproc为回声消除处理程序
- 文件夹pesqv2 新调整的pesq程序，比原来增加两个参数(可参考脚本中应用)：
  - 开始偏移位置 避开文件开头可能包含无效差异的时间段
  - 比较长度 避开文件结尾可能包含无效差异的时间段
- 文件夹single_talk_test 单向通话场景实际案例测试结果
- 文件夹double_talk_test 双向通话场景实际案例测试结果
- aec_filter.sh  需要一个参数指定test?_cap文件所在文件夹，自动执行speex和webrtc回声消除处理并产生结果音频文件
- pesq_calc.sh 对于双向通话场景，需要一个参数指定test?_cap文件所在文件夹，然后自动执行pesq过程，打印pesq评估结果
- pc_voice.wav 前景录音文件
- test?.wav 7个参考音文件
- alango、webrtc、speex回声消除算法对比.md.html 即本文档
