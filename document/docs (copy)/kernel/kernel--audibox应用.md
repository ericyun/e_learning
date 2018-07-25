AUDIBOX应用
audiobox_2　server

void *audio_capture_preproc_server(void* arg)
     module_buf = (char *)malloc(module_size); //output buf: allocate a temporary buffer for module processing
     softvol_buf = (char *)malloc(softvol_size); //output buf: allocate a temporary buffer for softvol
     encode_buf = (char *)malloc(encode_size); //output buf: allocate a temporary buffer for output

rpcname 和　channel的关系
     sprintf(rpcname, "%s-%s", AUDIOBOX_RPC_BASE, channel);
//物理通道名称
#define BT_CODEC_PLAY "btcodec"
#define BT_CODEC_RECORD "btcodecmic"
#define NORMAL_CODEC_PLAY "default"
#define NORMAL_CODEC_RECORD "default_mic"

#define AUDIOBOX_RPC_BASE "audiobox-listen"
/*注册到eventhub的4个event名称*/
#define AUDIOBOX_RPC_DEFAULT "audiobox-listen-default"
#define AUDIOBOX_RPC_DEFAULTMIC "audiobox-listen-default_mic"
#define AUDIOBOX_RPC_BTCODEC "audiobox-listen-btcodec"
#define AUDIOBOX_RPC_BTCODECMIC "audiobox-listen-btcodecmic"
#define AUDIOBOX_RPC_END "audiobox-listen-end"

struct logic_chn {//逻辑通道的属性
     char devname[32]; //物理通道名称
     int handle;　//逻辑通道的id
     int direct;
     int frsize;
     struct list_head node;
     struct fr_buf_info fr;
};
struct audio_dev {
     char devname[32];
     int flag;
     int timeout;
     int priority;
     int direct;
     int volume;
     void *handle; // for soft volume
     int id;
     int req_stop;
     pthread_mutex_t mutex;
     /* fr circle buf */
     struct fr_info fr;
     struct fr_buf_info ref; //when playback, save ref
     struct list_head node;
};
struct channel_node {
     char nodename[32];
     int priority; //record highest priority in the list
     snd_pcm_t *handle;
     snd_ctl_t *ctl;
     audio_fmt_t fmt;
     pthread_mutex_t mutex;
     int volume;
     /* server for playback or capture */
     int direct;
     pthread_t serv_id;
     int stop_server;
     /* phy chn list */
     struct list_head node;
     struct list_head head;
     /* for capture fr buf */
     struct fr_info fr;
};

先了解录音，然后再了解播放。

VplayTimeStamp::VplayTimeStamp(){
AudioStreamInfo::AudioStreamInfo(){
Statistics::Statistics()
VideoStreamInfo::VideoStreamInfo()

mediamuxer.cpp

ThreadingState MediaMuxer::WorkingThreadTask(void )
     如果是新文件，等待获取第一个I帧；
     首先，检查视频流的PFrFifoVideo队列，封装保存所有的视频帧
     然后，检查视频流的PFrFifoAudio队列，封装保存最新视频帧之前到来的音频帧。
     本函数调用之间间隔5ms

bool MediaMuxer::UpdateMediaFile(void)
     this->GetFinalFileName(newFileName, VPLAY_PATH_MAX, false);
     rename(this->CurrentMediatFileName,newFileName);
     chmod(newFileName, 0666);
     if(this->ValidMediaFile == true)
          this->ValidMediaFile = false;
          vplay_report_event (VPLAY_EVENT_NEWFILE_FINISH ,newFileName)
               vplay_event_info_t eventInfo ;     memcpy( eventInfo.buf , dat ,len+1);
               eventInfo.index = index++;         eventInfo.type = event ;
               return event_send(EVENT_VPLAY ,(char *)&eventInfo,sizeof(vplay_event_info_t) );
                    vplay_event_handler()--> SpvAddFile(pathname);-->SpvSendMessage(g_fm_handler, FM_ADD_FILE, (int)filename, 0);
bool MediaMuxer::AddVideoFrFrame(struct fr_buf_info *frame, int stream_index)
     AVPacket pkt;     av_init_packet(&pkt);
     if (frame->priv == VIDEO_FRAME_I )     pkt.flags |= AV_PKT_FLAG_KEY;　//I frame 标记
     对每个I/P帧都save_tmp_file() 的调用，感觉影响性能，因为每秒３０次。
     pkt.pts = pkt.dts = timestamp;pkt.duration = 0;pkt.pos = -1;
     this->stat.UpdatePacketInfo(PACKET_TYPE_VIDEO, pkt.size, pkt.pts, stream_index);
     ret = av_write_frame(this->MediaFileFormatContext, &pkt);
     this->ValidMediaFile = true;
bool MediaMuxer::AddAudioFrFrame(struct fr_buf_info *frame ){
     AVPacket pkt;     av_init_packet(&pkt);
     pkt.stream_index = AudioIndex;pkt.data = (uint8_t *)frame->virt_addr;pkt.size = frame->size;
     pkt.pts = pkt.dts = timestamp;     pkt.duration = 0;
     this->stat.UpdatePacketInfo(PACKET_TYPE_AUDIO, pkt.size, audioPTS, 0);
     ret = av_write_frame(this->MediaFileFormatContext, &pkt);
bool MediaMuxer::StartNewMediaFile(bool firstFlags)
     this->InitStreamChannel();


camera_create()
     camera_spv_g->start_video = start_video;
     camera_spv_g->stop_video = stop_video;
     camera_spv_g->start_photo = start_photo;
     camera_spv_g->take_picture = take_picture;
     camera_spv_g->stop_photo = stop_photo;
     memset(&vplayState, 0, sizeof(vplay_event_info_t));
     event_register_handler(EVENT_VPLAY, 0, vplay_event_handler);//"play" event
int start_video()     //camera_spv.c,  camera_spv_g->start_video = start_video; //开始录像
     char *chn=VCHN; or chn=VTCHN; //#define VCHN "encavc0-stream"  #define VTCHN "encavc2-stream"
     video_enable_channel(chn);          video_set_fps(chn, 30);
     video_enable_channel(QVGACHN);     //#define QVGACHN "encavc3-stream"，看起来是子码流
     struct v_frc_info frc_info = {0};     frc_info.framebase = 1;          frc_info.framerate = video_attr_g.frameRate;
     video_set_frc(chn, &frc_info);
     video_set_bitrate(chn, &LOCAL_MEDIA_BITRATE_INFO);
     media_init(chn);
          memset(&recorderInfo ,0,sizeof(VRecorderInfo));
          set_video_recorderInfo(&recorderInfo,chn);
          g_recorder = vplay_new_recorder(&recorderInfo);
               vplay_show_recorder_info(info);
               VRecorder *recorder =(VRecorder *) malloc(sizeof(VRecorder));
               QRecorder *recInst = new QRecorder(info);     recInst->Prepare(); //创建recorder并初始化其video/audio/extra
               recorder->inst = (void *) recInst ;     return recorder ;
     vplay_mute_recorder(g_recorder, VPLAY_MEDIA_TYPE_AUDIO, !strcasecmp(config_camera_g->video_recordaudio, "Off"));
          QRecorder *recInst = (QRecorder *)rec->inst ;     recInst->SetMute(type,xxxx);
     vplayState.type = VPLAY_EVENT_NONE;
     vplay_start_recorder(g_recorder);
           QRecorder *recInst = (QRecorder *)rec->inst ;      recInst->SetWorkingState(ThreadingState::Start);
int stop_video()
     vplay_stop_recorder(g_recorder);
          QRecorder *recInst = (QRecorder *)rec->inst ;     recInst->SetWorkingState(ThreadingState::Pause);
     vplay_delete_recorder(g_recorder);
          QRecorder *recInst = (QRecorder *)rec->inst ;     recInst->SetWorkingState(ThreadingState::Stop);
          delete recInst ;     free(rec);
     video_disable_channel(chn);
     video_disable_channel(QVGACHN);

录像： qrecorder.cpp

int vplay_control_recorder(vplay_recorder_inst_t *rec, vplay_ctrl_action_t action, void *para)
     QRecorder *recInst = (QRecorder *)rec->inst ;     ThreadingState curState = recInst->GetWorkingState();
     case VPLAY_RECORDER_SLOW_MOTION:          ret = recInst->Muxer->SetSlowMotion(*(int *)para);
     case VPLAY_RECORDER_FAST_MOTION:          ret = recInst->Muxer->SetFastMotion(*(int *)para);
     case VPLAY_RECORDER_START:               ret = recInst->SetWorkingState(ThreadingState::Start);
     case VPLAY_RECORDER_STOP:               ret = recInst->SetWorkingState(ThreadingState::Pause);
     case VPLAY_RECORDER_MUTE:               recInst->SetMute(VPLAY_MEDIA_TYPE_AUDIO, true);
     case VPLAY_RECORDER_UNMUTE:               recInst->SetMute(VPLAY_MEDIA_TYPE_AUDIO, false);
     case VPLAY_RECORDER_SET_UDTA: ret = recInst->SetMediaUdta(((vplay_udta_info_t *)para)->buf, (para)->size);
     case VPLAY_RECORDER_FAST_MOTION_EX:     ret = recInst->Muxer->SetFastMotionEx(*(int *)para);
QRecorder::QRecorder(VRecorderInfo *info){
     this->RecInfo = *info ;    this->CallerVideo = NULL;          this->CallerAudio = NULL;
     this->FifoLoopVideo = NULL;     this->FifoLoopAudio = NULL;          this->FifoLoopExtra = NULL;
     this->VideoInfo = NULL;     this->VideoIndex = 0;     this->VideoCount = 0;
     this->Muxer = NULL;
     this->CallerExtra = NULL;     this->FifoLoopExtra = NULL;
bool QRecorder::SetWorkingState(ThreadingState state){
     所有的state变化，应用到所有的video和audio对象。
bool QRecorder::CreateVideoResource(char * video_channel,VideoStreamInfo *video_info)//创建新的ApiCallerVideo
     this->CallerVideo[VideoIndex] = new ApiCallerVideo(video_channel, video_info);

bool QRecorder::Prepare(void)
     if(strlen(this->RecInfo.audio_channel) != 0){//audio初始化
          this->CallerAudio = new ApiCallerAudio(this->RecInfo.audio_channel,&this->RecInfo.audio_format);
          this->CallerAudio->Prepare();
          this->FifoLoopAudio = new QMediaFifo((char *)"AudioFifo");
          this->FifoLoopAudio->SetMaxCacheTime( (RECORDER_CACHE_SECOND)*1000);
          this->CallerAudio->PFrFifo = this->FifoLoopAudio ;
          this->CallerAudio->SetFifoAutoFlush(true);
          this->CallerAudio->SetWorkingState(ThreadingState::Standby);
     this->VideoCount++;     //video初始化
          CallerVideo = new ApiCallerVideo *[VideoCount];
          VideoInfo = new VideoStreamInfo *[VideoCount];
          FifoLoopVideo = new QMediaFifo *[VideoCount];
          for(int i=0;i<VideoCount;i++)     VideoInfo[i] = new VideoStreamInfo();
     if(this->RecInfo.enable_gps )     //gps handler初始化
          this->CallerExtra = new ApiCallerExtra();
          this->CallerExtra->Prepare();
          this->FifoLoopExtra = new QMediaFifo((char *)"ExtraFifo");
          this->FifoLoopExtra->SetMaxCacheTime( (RECORDER_CACHE_SECOND)*1000);
          this->CallerExtra->PFrFifo = this->FifoLoopExtra ;
     this->Muxer = new MediaMuxer(&this->RecInfo, this->VideoInfo, VideoCount);//muxer初始化
       this->Muxer->SetErrorCallBack(error_process_callback, this);
       this->Muxer->SetFrFifo(this->FifoLoopVideo ,this->FifoLoopAudio ,this->FifoLoopExtra);
          //设置３个fifo
       this->Muxer->Prepare();
     if(this->RecInfo.enable_gps ) this->CallerExtra->SetWorkingState(ThreadingState::Standby);//状态初始化
     if(this->RecInfo.time_backward > 0 )
          for(int i=0;i<VideoCount;i++)     this->CallerVideo[i]->SetWorkingState(ThreadingState::Start);
          this->CallerAudio->SetWorkingState(ThreadingState::Start);
          if(this->RecInfo.enable_gps )     this->CallerExtra->SetWorkingState(ThreadingState::Start);

sound.c

static int __init alsa_sound_init(void)

ffmpeg在我们的应用中好像只是用于编解码。

DSP_ENABLE 和　DSP_AAC_SUPPORT 来控制开启dsp编码

void *codec_open(codec_info_t *fmt)
     codec_t  dev = (codec_t)malloc(sizeof(*dev));     memcpy(&dev->fmt, fmt, sizeof(codec_info_t));
     dev->codec_mode = codec_check_fmt(fmt); //it return CODEC_RESAMPLE or CODEC_ENCODE or CODEC_DECODE
     if ((dev->codec_mode & CODEC_ENCODE) &&(fmt->out.codec_type == AUDIO_CODEC_AAC)) {
          ret = ceva_tl421_open(NULL);  ret = ceva_tl421_set_format(NULL, fmt); dev->codec_dsp = 1;    }
     av_register_all();     ret = ffmpeg_init(dev);     dev->dframe = (uint8_t *)malloc(CODEC_DF_BUF);
     dev->dframe_len = CODEC_DF_BUF;     dev->dfrestore = 0;     dev->framelen = 0;
int codec_close(void *codec)
     if (dev->dframe)     free(dev->dframe);
     if (dev->codec_dsp)     ceva_tl421_close(NULL);
     ffmpeg_exit(dev);     free(dev);
int codec_encode_frame(void *codec, codec_addr_t *desc)
     if (dev->codec_mode & CODEC_RESAMPLE) ffmpeg_resample(dev, desc->out, desc->len_out, desc->in, desc->len_in);
     if (dev->codec_mode & CODEC_ENCODE){
          if (dev->codec_dsp)　　　return ceva_tl421_encode(NULL, desc->out, (need ? dev->ref : desc->in),
          else        ffmpeg_encode(dev, desc->out, desc->len_out, (need ? dev->ref : desc->in),
     }
int codec_encode(void *codec, codec_addr_t *desc)

int codec_decode(void *codec, codec_addr_t *desc)
codec_flush_t codec_flush(void *codec, codec_addr_t *desc, int *len)
int codec_resample(void *codec, char *dst, int dstlen, char *src, int srclen)
     return ffmpeg_resample(dev, dst, dstlen, src, srclen);

media_play *media_play_g = NULL; static VPlayer *player = NULL; static VPlayerFileInfo vplayFileInfo;

media_play *media_play_create()
     VPlayerInfo playerInfo ;  init_player_info(&playerInfo);  show_player_info(&playerInfo);
     player = vplay_new_player(&playerInfo);//创建Qplayer对象
          QPlayer *qPlayer = new QPlayer(info);qPlayer->Prepare();
          VPlayer *player = (VPlayer *) malloc(sizeof(VPlayer));
          player->inst = (void *)qPlayer ;
          return player;
     media_play_g->set_file_path = set_file_path;//开始初始化Qplayer对象行为集
     media_play_g->begin_video = begin_video;
          vplay_control_player(player, VPLAY_PLAYER_QUEUE_FILE, media_play_g->media_filename)
               ret = qPlayer->AddMediaFile((char *)para); //QPlayer::AddMediaFile()
                    vplay_file_info_t fileInfo ;memset(&fileInfo, 0, sizeof(vplay_file_info_t));
                    memcpy(fileInfo.file, fileName, nameLen); this->PFileList->push_back(fileInfo);//添加文件
          if(player_mode==0) vplay_control_player(player,VPLAY_PLAYER_SET_VIDEO_FILTER, &index);
               ret = qPlayer->SetVideoFilter(*(int32_t *)para); //QPlayer::SetVideoFilter(int index)
                    this->VideoIndex = index;
          vplay_control_player(player,VPLAY_PLAYER_PLAY , &value)
               ret = qPlayer->Play(*(int *)para);　//QPlayer::Play()
                    this->OpenNextMediaDemuxer(true);//QPlayer::OpenNextMediaDemuxer()
                         vplay_file_info_t file = this->PFileList->front();this->PFileList->pop_front();//弹出文件
                         this->DemuxInstance = open_media_demuxer(&this->CurFile,&this->CurMediaInfo);
                         this->CurMediaInfo.video_index ,this->CurMediaInfo.audio_index, this->CurMediaInfo.extra_index
                         demux_set_stream_filter(this->DemuxInstance,...);
                    this->VPlayTs.SetPlaySpeed(speed);
                    this->PlaySpeed.store(speed);
                    this->SetWorkingState(ThreadingState::Start);
                    if(this->PDemuxVideoInfo != NULL){this->PSpeedCtrl->SetPlaySpeed(speed);
                         this->PSpeedCtrl->SetWorkingState(ThreadingState::Start);}
          vplayState->type = VPLAY_EVENT_NONE;
          pthread_create(&callback_thread_id, &attr, callback_thread, NULL);
               while(1){
                    if(!state_changed)　switch (vplayState->type)
                         case VPLAY_EVENT_PLAY_FINISH:  media_play_g->video_callback(MSG_PLAY_STATE_END);
                         case VPLAY_EVENT_PLAY_ERROR:   media_play_g->video_callback(MSG_PLAY_STATE_ERROR);
                    pthread_testcancel(); usleep(100*1000);
               }
     media_play_g->end_video = end_video;
          vplay_control_player(player,VPLAY_PLAYER_STOP ,1);
               qPlayer->Stop();     //QPlayer::Stop
          pthread_cancel(callback_thread_id);
     media_play_g->resume_video = resume_video;
     media_play_g->pause_video = pause_video;
     media_play_g->prepare_video = prepare_video;
     media_play_g->get_video_duration = get_video_duration;
     media_play_g->get_video_position = get_video_position;
     media_play_g->video_speedup = video_speedup;
     media_play_g->display_photo = display_photo;
     media_play_g->video_callback = NULL;

pInfo = (PPlaybackInfo)lParam;
pInfo->player = media_play_create();//逆向调用
     static int PlaybackDialogBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)//回放的核心控制函数
          void ShowPlaybackDialog(HWND hWnd, FileList *pFileList)
开始播放录像过程：
static int PlaybackDialogBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
     switch(message) {
          case MSG_INITDIALOG:　//进入回放界面
               SpvSetPreviewMode(PREVIEW_PLAYBACK_VIDEO);
               pInfo->player = media_play_create(PLAYER_MODE_VIDEO);
               register_video_callback(pInfo->player, OnVideoCallback);
               PostMessage(hDlg, MSG_SHOW_PREVIEW, 0, 0);
          case MSG_SHOW_PREVIEW://回放界面视频预览
               ShowPreviewInfo(hDlg, pInfo, TRUE);
                    StopVideo(hDlg, pInfo);
                    pInfo->player->set_file_path(filePath);
                    pInfo->player->prepare_video();
                    pInfo->player->show_video_thumbnail();
          case MSG_KEYUP:      //按下播放按钮
               OnKeyUp(hDlg, pInfo, wParam);
                    switch(keyCode) {
                         case SCANCODE_SPV_OK:  OnButtonConfirmed(hDlg, pInfo);
                              PlayVideo(hDlg, pInfo, 1.0f)
                                   pInfo->player->begin_video()
                         ．．．．　．．．．．

AAC编解码在当前软硬件播放器中已经越来越成为成为主流。

至少有六套AAC库：
- FAAC, FAAD/FAAD2 ---编码只有AAC LC
- neroAACenc ---非商业可以使用（提供编码工具）
- FFmpeg''s native AAC encoder(part of libavcodec), experimental by the developers as of December 2010 ---只有AAC LC，且质量不好
- libvo_aacenc, the Android VisualOn AAC encoder ---只有AAC LC， opencore-amr-vo-aacenc，只有编码
- libfdk-aac, the Fraunhofer FDK AAC libray ---包含到HE-AACv2，且性能较好 opencore-amr-fdk-aac
- libaacplus, 3GPP released reference implementations 3GPP High Efficiency Advanced Audio Codec (HE-AAC) Codec (3GPP TS 26.410 V 8.0.0). --只有编码，“Enhanced aacPlus general audio codec; Floating-point ANSI-C code”

aac-enc.c

ADTS_HEADER_SIZE 的长度定义是否有问题，　一个7一个8
     FF F1(前缀) 4C 80 C 61 0 21 10 5 20 A4 1B C5 0 0

bool QPlayer::Play(int speed)　应该是处理的入口吧

监控一下，
     LoopThreadFunction　函数的调用情况，
     QAudioPlayer::WorkingThreadTask　是否被调用

fr.c
abctrl.c
enum AB_CMD_TYPE {
     AB_GET_CHANNEL,     AB_PUT_CHANNEL,     AB_GET_FORMAT,     AB_SET_FORMAT,
     AB_GET_MUTE,     AB_SET_MUTE,     AB_GET_VOLUME,     AB_SET_VOLUME,
     AB_START_SERVICE,     AB_STOP_SERVICE,     AB_GET_MASTER_VOLUME,     AB_SET_MASTER_VOLUME,     AB_ENABLE_AEC,
};
int main(int argc, char **argv)
     if (!strcmp(argv[1], "start"))          audio_start_service();
          dir = opendir("/proc");
          while ((ptr = readdir(dir))) {
               sprintf(path, "/proc/%s/cmdline", ptr->d_name);fp = fopen(path, "r");len = fread(buf, 1, 49, fp);
               if (strstr(buf, "audiobox")) return; //audiobox已经启动，如果没启动
               fclose(fp);
          }
          system("audiobox &");closedir(dir);
     else if (!strcmp(argv[1], "stop"))      audio_stop_service();
          AB_EVENT(sc, &cmd, NULL, NULL, &result);
          ret = audiobox_rpc_call_scatter(AUDIOBOX_RPC_BASE, sc,AB_EVENT_SIZE(sc));
          return *(int *)AB_GET_RESULT(sc);
     else if (!strcmp(argv[1], "list"))      audio_list_pcminfo();
          对每个card,snd_pcm_open / snd_pcm_hw_params_malloc / snd_pcm_hw_params_any / snd_pcm_hw_params_dump / snd_pcm_hw_params_free / snd_pcm_close
     else if (!strcmp(argv[1], "play"))      playback(--argc, ++argv);

     else if (!strcmp(argv[1], "record"))    capture(--argc, ++argv);
     else if (!strcmp(argv[1], "mute"))      mute(--argc, ++argv);
     else if (!strcmp(argv[1], "unmute"))    unmute(--argc, ++argv);
     else if (!strcmp(argv[1], "set"))     　set(--argc, ++argv);
     else if (!strcmp(argv[1], "encode"))    encode(--argc, ++argv);
     else if (!strcmp(argv[1], "decode"))    decode(--argc, ++argv);
     else if (!strcmp(argv[1], "read"))      readi(--argc, ++argv);

audio_get_format / audio_set_format / audio_get_mute / audio_set_mute / audio_get_volume / audio_set_volume
audio_get_master_volume / audio_set_master_volume / audio_enable_aec 这几个函数同样的处理流程：
     audio_get_rpcname_by_channel(rpcname, chnname);
     AB_EVENT_RESV(sc, &cmd, chnname, &volume, &result, &dir);
     ret = audiobox_rpc_call_scatter(rpcname, sc,AB_EVENT_SIZE(sc));
     return *(int *)AB_GET_RESULT(sc);

int audiobox_rpc_call_scatter(char *name, struct event_scatter sc[], int count)
     char* buf = (char *)malloc(AUDIO_RPC_MAX_BUF); 合并sc[]到buffer中
     ret = event_rpc_call(name, buf, len);
          struct msg_data_buffer provider_data_msg;　填充消息体
          Send_Data_Event_To_Eventd(&provider_data_msg);
          int msg_id = msg_queue_create(provider_data_msg.event.pid);      provider_data_msg.msg_type = gettid();
          Read_Return_From_Handler(msg_id, &provider_data_msg);   memcpy(buf, provider_data_msg.event.buf, size);
     ret = audiobox_rpc_parse(sc, count, buf);//解析返回数据．
     free(buf);

audiobox_listener.c
audiobox_server.c

struct audio_dev {
     char devname[32];
     int flag;
     int timeout;
     int priority;
     int direct;
     int volume;
     void *handle; // for soft volume
     int id;
     int req_stop;
     pthread_mutex_t mutex;
     /* fr circle buf */
     struct fr_info fr;
     struct fr_buf_info ref; //when playback, save ref
     struct list_head node; //用于链接到channel_node
};
struct channel_node {
     char nodename[32];
     int priority; //record highest priority in the list
     snd_pcm_t *handle;
     snd_ctl_t *ctl;
     audio_fmt_t fmt;
     pthread_mutex_t mutex;
     int volume;
     /* server for playback or capture */
     int direct;
     pthread_t serv_id;
     int stop_server;
     /* phy chn list */
     struct list_head node;     //用于链接到channel_list
     struct list_head head;     //audio_dev链表的头
     /* for capture fr buf */
     struct fr_info fr;
};
typedef struct audio_dev * audio_t;
typedef struct channel_node * channel_t;

channel_t audiobox_get_chn_by_id(int id)
     list_for_each(pos, &audiobox_chnlist) {
          chn = list_entry(pos, struct channel_node, node);
          list_for_each(pos1, &chn->head) {
               dev = list_entry(pos1, struct audio_dev, node);
               if (dev->id == id)  return chn;
          }
     }
audio_t audiobox_get_handle_by_id(int id)
     list_for_each(pos, &audiobox_chnlist) {
          chn = list_entry(pos, struct channel_node, node);
          list_for_each(pos1, &chn->head) {
               dev = list_entry(pos1, struct audio_dev, node);
               if (dev->id == id)  return dev;
          }
     }
channel_t audiobox_get_phychn(char *devname, audio_fmt_t *fmt)
     list_for_each(pos, &audiobox_chnlist) { //先查找确认指定dev是否已经存在
          node = list_entry(pos, struct channel_node, node);
          if (!strcmp(node->nodename, devname)) {
               if (fmt && memcmp(fmt, &node->fmt, 12)) return NULL; else  return node;
          }
     }
     node = (channel_t)malloc(sizeof(*node));     memset(node, 0, sizeof(*node));
     node->volume_p = VOLUME_DEFAULT;      node->volume_c = VOLUME_DEFAULT;
     node->priority = CHANNEL_BACKGROUND;node->serv_id = -1;node->direct = audiobox_get_direct(node->nodename);
     audio_hal_open(node);
          err = snd_pcm_open(&(handle->handle), handle->nodename,
          err = sethardwareparams(handle, NULL);
               err = snd_pcm_hw_params_malloc(&hardwareParams);
               err = snd_pcm_hw_params_any(handle->handle, hardwareParams);
               err = snd_pcm_hw_params_set_access(handle->handle, hardwareParams,SND_PCM_ACCESS_RW_INTERLEAVED);
               err = snd_pcm_hw_params_set_format(handle->handle, hardwareParams,format_to_alsa(format->bitwidth));
               err = snd_pcm_hw_params_set_channels(handle->handle, hardwareParams,format->channels);
               err = snd_pcm_hw_params_set_rate_near(handle->handle, hardwareParams,&format->samplingrate, 0);
               err = snd_pcm_hw_params_set_period_size_near(handle->handle,hardwareParams, &format->sample_size, 0);
               buffersize = 4 * periodsize; //PCM定义了4*1024个sample的缓冲区
               err = snd_pcm_hw_params_set_buffer_size_near(handle->handle,hardwareParams, &buffersize);
               err = snd_pcm_hw_params(handle->handle, hardwareParams);
          err = setsoftwareparams(handle, NULL);
               snd_pcm_sw_params_malloc(&softwareParams);
               err = snd_pcm_sw_params_current(handle->handle, softwareParams);
               snd_pcm_get_params(handle->handle, &buffersize, &periodsize);
               if (handle->direct == DEVICE_OUT_ALL) {
                    startThreshold = buffersize / get_threshold_val(format) - 1;
                    stopThreshold = buffersize;
               }else{
                    startThreshold = 1;　　　stopThreshold = buffersize; }
               err = snd_pcm_sw_params_set_start_threshold(handle->handle, softwareParams,startThreshold);
               err = snd_pcm_sw_params_set_stop_threshold(handle->handle, softwareParams,stopThreshold);
               err = snd_pcm_sw_params_set_avail_min(handle->handle, softwareParams,periodsize);//
               err = snd_pcm_sw_params(handle->handle, softwareParams);
               snd_pcm_sw_params_free(softwareParams);
     audio_open_ctl(node);
          ret = snd_ctl_open(&dev->ctl, dev->nodename, 0);
          if (ret < 0)     return snd_ctl_open(&dev->ctl, "default", 0);
     if (node->direct == DEVICE_IN_ALL)
          ret = fr_alloc(&node->fr, node->nodename, audiobox_get_buffer_size(&node->fmt),FR_FLAG_RING(5));
     pthread_mutex_init(&node->mutex, NULL);  pthread_mutex_init(&node->aec.mutex, NULL);
     pthread_rwlock_init(&node->aec.rwlock, NULL);     pthread_cond_init(&node->aec.cond, NULL);
     INIT_LIST_HEAD(&node->head);
     audio_create_chnserv(node);//为channel创建录音或者播放线程
          dev->stop_server = 0;
          static const audio_server server[] = {
         audio_playback_server,  audio_capture_server, };
    ret = pthread_create(&dev->serv_id, NULL, server[dev->direct], dev);
          pthread_detach(dev->serv_id);
     pthread_mutex_init(&node->mutex, NULL);
     list_add_tail(&node->node, &audiobox_chnlist);//添加channel到audiobox_chnlist中
int audiobox_listener_init(void)　　//audiobox进程调用，event_register_handler虽然在eventhub中定义，但相关数据结构是audiobox的
     for (i = 0; i < AUDIO_RPC_MAX; i++) {//为５个AUDIOBOX_RPC创建线程，每个线程一个消息队列。
          err = event_register_handler(eventrpc[i], EVENT_RPC, audiobox_listener);
     while(1){sleep(1);  if (audiobox_stop) {audio_stop_service();audiobox_exit(); }       }
void *audio_playback_server(void *arg)
     buf = malloc(SOFT_BUF);     bufsize = SOFT_BUF;
     audio_t dev = audio_get_logchn(chn);
     ret = fr_test_new_by_name(dev->devname, &dev->ref);
     ret = fr_get_ref_by_name(dev->devname, &dev->ref);
     if (dev->volume != PRESET_RESOLUTION || chn->volume != VOLUME_DEFAULT)
          softvol_set_volume(dev->handle, dev->volume * chn->volume / VOLUME_DEFAULT);
          len = softvol_convert(dev->handle, buf, bufsize, dev->ref.virt_addr, dev->ref.size);
     ret = audio_hal_write(chn, (need ? buf : dev->ref.virt_addr),(need ? len : dev->ref.size));
          err = snd_pcm_writei(handle->handle, buf, buflen_to_frames(handle, len));
     fr_put_ref(&dev->ref);
          ret = ioctl(fd, FRING_PUT_REF, ref);
          munmap(ref->virt_addr, ref->map_size);
void *audio_capture_server(void *arg)
     fr_INITBUF(&ref, NULL);
     audio_update_timestamp(&ref);
          fr_STAMPBUF(fr);
               clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
               buf->timestamp = ts.tv_sec * 1000ull + ts.tv_nsec / 1000000;
     while(1){
          list_for_each(pos, &chn->head) {
               dev = list_entry(pos, struct audio_dev, node);
               if (dev && dev->req_stop)     audiobox_inner_put_channel(dev);//释放停止的逻辑通道
          }
          if (chn->stop_server)     break;
          ret = fr_get_buf_by_name(chn->nodename, &ref);
          ret = audio_hal_read(chn, ref.virt_addr, ref.size);
          ref.timestamp = 0;
          for (i = 0; i < 4; i++) {
              buf_time = *(uint16_t*)(ref.virt_addr + i*4);
              *(uint16_t*)(ref.virt_addr + i*4) = *(uint16_t*)((ref.virt_addr + i*4 + 2));
              ref.timestamp |= (buf_time << i*16);
          }
          fr_put_buf(&ref);
               ret = ioctl(fd, FRING_PUT_BUF, buf);
               munmap(buf->virt_addr, buf->map_size);
     }

void audiobox_listener(char *name, void *arg)
     audiobox_rpc_parse(audiobox, AB_COUNT, (char *)arg);//从arg中解析消息到audiobox
     case AB_START_SERVICE:     *result = 0;
     case AB_STOP_SERVICE:      *result = audiobox_stop_service(audiobox);
          list_for_each(pos, &audiobox_chnlist){
               chn = list_entry(pos, struct channel_node, node);
               list_for_each(pos1, &chn->head) {
                    dev = list_entry(pos1, struct audio_dev, node);
                    list_del(&dev->node);
                    audiobox_release_phychn(dev->devname);
                    fr_free(&dev->fr);audiobox_put_id(dev->id);free(dev);
               }
          }
          audiobox_signal_handler(0);
               audiobox_stop = 1;
     case AB_GET_CHANNEL:       *result = audiobox_get_channel(audiobox);
          devname = AB_GET_CHN(event); flag = *(int *)AB_GET_PRIV(event);//获取dev名称，format,flag信息
          fmt = (audio_fmt_t *)AB_GET_RESV(event);  dev = malloc(sizeof(*dev));memset(dev, 0, sizeof(*dev));
          sprintf(dev->devname, "%s", devname);dev->flag = flag;dev->timeout = (flag & NOTIMEOUT_MASK)? 0 : 360; //360sec
          dev->priority = (flag & PRIORITY_MASK) >> PRIORITY_BIT;
          dev->id = audiobox_get_id();     pthread_mutex_init(&dev->mutex, NULL);//初始化dev信息，获取id
          fr_INITBUF(&dev->ref, NULL);//dev获取ref
          node = audiobox_get_phychn(dev->devname, fmt);
          if (node->direct == DEVICE_OUT_ALL) {//播放需要申请fr
               sprintf(dev->devname, "%s%d", devname, dev->id);
               ret = fr_alloc(&dev->fr, dev->devname, audiobox_get_buffer_size(&node->fmt),FR_FLAG_RING(5) | FR_FLAG_NODROP);}
          dev->direct = node->direct;
          list_add_tail(&dev->node, &node->head);//dev添加到node channel.
          return dev->id;
     case AB_PUT_CHANNEL:       *result = audiobox_put_channel(audiobox);
     case AB_GET_FORMAT:        *result = audiobox_get_format(audiobox);
     case AB_SET_FORMAT:        *result = audiobox_set_format(audiobox);
     case AB_GET_MUTE:          *result = audiobox_get_mute(audiobox);
     case AB_SET_MUTE:          *result = audiobox_set_mute(audiobox);
     case AB_GET_VOLUME:        *result = audiobox_get_volume(audiobox);
     case AB_SET_VOLUME:        *result = audiobox_set_volume(audiobox);
     case AB_GET_MASTER_VOLUME: *result = audiobox_get_master_volume(audiobox);
     case AB_SET_MASTER_VOLUME: *result = audiobox_set_master_volume(audiobox);

audiobox_hal.c

基于channel_t的函数接口，snd_pcm的接口．

int audio_hal_set_format(channel_t handle, audio_fmt_t *fmt)
     err = sethardwareparams(handle, fmt);
     err = setsoftwareparams(handle, fmt);
int audio_hal_close(channel_t handle)
     return snd_pcm_close(handle->handle);
uint64_t audio_frame_times(channel_t handle, int length) //返回长度对应的ms时间
     len = format_to_byte(format->bitwidth) * format->channels * format->samplingrate;
     return (1000 * length) / len;

int audio_hal_read(channel_t handle, void *buf, int len)
     err = snd_pcm_readi(handle->handle, buf, buflen_to_frames(handle, len));

static int play_audio(char *filename)
     handle = get_audio_handle();
     ret = audio_get_format(pcm_name, &real);
     bufsize = get_buffer_size(&real);
     fd = open(filename, O_RDONLY);
     do{
          len = safe_read(fd, audiobuf, bufsize);
          ret = audio_write_frame(handle, audiobuf, len);
               ret = audio_get_frname(handle, frname);
               fr_INITBUF(&frbuf, NULL);
               ret = fr_get_buf_by_name(frname, &frbuf);
                    ret = ioctl(fd, FRING_GET_FR, &fr);
                    fr_INITBUF(buf, &fr);
                    return fr_get_buf(buf);
                         ret = ioctl(fd, FRING_GET_BUF, buf);
                         buf->virt_addr = mmap(0, buf->map_size, PROT_READ | PROT_WRITE,
MAP_SHARED, fd, buf->phys_addr);
               frsize = audio_get_frsize(handle);
               memcpy(frbuf.virt_addr, buf + writed, len);
               fr_put_buf(&frbuf);
                    ret = ioctl(fd, FRING_PUT_BUF, buf);
                    munmap(buf->virt_addr, buf->map_size);

     }while (len == bufsize);
     close(fd);
     audio_put_channel(handle);

/# abctrl list
**** List of Hardware Devices Pcminfo ****
Card 0:
~~~~ playback: ~~~~
ACCESS: MMAP_INTERLEAVED RW_INTERLEAVED
FORMAT: S16_LE S24_LE S32_LE
SUBFORMAT: STD
SAMPLE_BITS: [16 32]
FRAME_BITS: [32 64]
CHANNELS: 2
RATE: [8000 192000]
PERIOD_TIME: (2666 256000]
PERIOD_SIZE: [512 2048]
PERIOD_BYTES: [4096 8192]
PERIODS: [2 4]
BUFFER_TIME: (5333 1024000]
BUFFER_SIZE: [1024 8192]
BUFFER_BYTES: [4096 65536]
TICK_TIME: ALL
~~~~ capture: ~~~~
ACCESS: MMAP_INTERLEAVED RW_INTERLEAVED
FORMAT: S16_LE S24_LE S32_LE
SUBFORMAT: STD
SAMPLE_BITS: [16 32]
FRAME_BITS: [32 64]
CHANNELS: 2
RATE: [8000 192000]
PERIOD_TIME: (2666 256000]
PERIOD_SIZE: [512 2048]
PERIOD_BYTES: [4096 8192]
PERIODS: [2 4]
BUFFER_TIME: (5333 1024000]
BUFFER_SIZE: [1024 8192]
BUFFER_BYTES: [4096 65536]
TICK_TIME: ALL

