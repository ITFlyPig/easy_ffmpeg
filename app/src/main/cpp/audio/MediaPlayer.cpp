//
// Created by wangyuelin on 2020/11/23.
//

#include "MediaPlayer.h"
#include "../utils/FrameUtil.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 1;
static int RET_AGAIN = 2;

int MediaPlayer::open() {
    int ret = RET_ERROR;
    if (StringUtil::isEmpty(path)) {
        LOGE(TAG, "要打开的视频文件的路劲为空");
        return RET_ERROR;
    }
    ret = avformat_open_input(&c, path, NULL, NULL);
    if (ret) {
        LOGE(TAG, "视频文件：%s 打开失败:%s", path, av_err2str(ret))
        return RET_ERROR;
    }
    ret = avformat_find_stream_info(c, NULL);
    if (ret < 0) {
        LOGE(TAG, "avformat_find_stream_info 寻找流信息失败：%s", av_err2str(ret));
        return RET_ERROR;
    }
    //找到视频/音频流的索引
    for (int i = 0; i < c->nb_streams; i++) {
        AVMediaType mediaType = c->streams[i]->codecpar->codec_type;
        if (mediaType == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            videoCodecId = c->streams[i]->codecpar->codec_id;
        } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
            audioCodecId = c->streams[i]->codecpar->codec_id;
        }
    }
    if (videoIndex == -1 && audioIndex == -1) {
        LOGE(TAG, "未找到视频&音频流的索引");
        return RET_ERROR;
    }
    if (videoIndex >= 0) {
        ret = findAndOpenCodec(videoCodecId, &pVideoCodecCxt, &pVideoCodec,
                               c->streams[videoIndex]->codecpar);
        if (ret < 0) {
            LOGE(TAG, "打开视频的解码器失败");
        } else {
            isVideoOpen = true;
        }
    }
    if (audioIndex >= 0) {
        ret = findAndOpenCodec(audioCodecId, &pAudioCodecCxt, &pAudioCodec,
                               c->streams[audioIndex]->codecpar);
        if (ret < 0) {
            LOGE(TAG, "打开音频的解码器失败");
        } else {
            isAudioOpen = true;
        }
    }
    if (!isAudioOpen && !isVideoOpen) {
        LOGE(TAG, "打开音频和视频的解码器失败");
        return RET_ERROR;
    }
    //打印基本信息
    av_dump_format(c, videoIndex, path, 0);

    LOGD(TAG, "视频文件：%s 打开成功", path);

    /* //创建媒体数据提供者
     mediaProvider = new MediaProvider();
     //打开openglse的音频播放器
     if (isAudioOpen) {
         renderAudio = new RenderAudio(pAudioCodecCxt->channels, pAudioCodecCxt->sample_rate,
                                       mediaProvider);
         if (renderAudio->open() < 0) {
             LOGE(TAG, "创建音频播放器失败");
             return RET_ERROR;
         }
         renderAudio->setPlayVolume(2);
     }*/
    return RET_SUCCESS;
}

int MediaPlayer::close() {

    if (c != nullptr) {
        avformat_close_input(&c);
        avformat_free_context(c);
        c = nullptr;
    }
    if (pVideoCodecCxt != nullptr) {
        avcodec_free_context(&pVideoCodecCxt);
        pVideoCodecCxt = nullptr;
    }
    if (pAudioCodecCxt != nullptr) {
        avcodec_free_context(&pAudioCodecCxt);
        pAudioCodecCxt = nullptr;
    }
    LOGD(TAG, "释放资源成功");

    return RET_SUCCESS;
}

int MediaPlayer::findAndOpenCodec(AVCodecID codecId, AVCodecContext **pCodecCxt,
                                  AVCodec **pCodec, const AVCodecParameters *par) {
    int ret = RET_ERROR;
    *pCodec = avcodec_find_decoder(codecId);
    if (*pCodec == NULL) {
        LOGE(TAG, "avcodec_find_decoder未找到解码器");
        return RET_ERROR;
    }
    //创建解码器上下文
    *pCodecCxt = avcodec_alloc_context3(*pCodec);
    if (*pCodecCxt == NULL) {
        LOGE(TAG, "解码器上下文创建失败");
        return RET_ERROR;
    }

    //将解析得到的解码参数填充到解码上下文
    ret = avcodec_parameters_to_context(*pCodecCxt, par);
    if (ret < 0) {
        LOGE(TAG, "avcodec_parameters_to_context 填充参数失败：%s", av_err2str(ret));
        return RET_ERROR;
    }

    //打开解码器
    ret = avcodec_open2(*pCodecCxt, *pCodec, NULL);
    if (ret < 0) {
        LOGE(TAG, "解码器打开失败：%s", av_err2str(ret));
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

int MediaPlayer::decode() {
    AVFrame *pFrame = NULL;
    AVFrame *pAudioFrame = NULL;

    int ret = RET_ERROR;
    pFrame = av_frame_alloc();
    if (pFrame == NULL) {
        LOGE(TAG, "AVFrame申请失败");
        return RET_ERROR;
    }

    //Video 格式转换
    pVideoSwsCxt = sws_getContext(pVideoCodecCxt->width, pVideoCodecCxt->height,
                                  pVideoCodecCxt->pix_fmt,
                                  nDstWidth, nDstHeight, mDstFmt,
                                  SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (pVideoSwsCxt == nullptr) {
        LOGE(TAG, "sws_getContext 创建pVideoSwsCxt失败");
        return RET_ERROR;
    }

    pAudioFrame = av_frame_alloc();
    if (pAudioFrame == NULL) {
        LOGE(TAG, " Audio Frame申请失败");
        return RET_ERROR;
    }
    AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;
    //音频转换器
    pAudioSwsCxt = swr_alloc();
    // 配置输入/输出通道类型
    av_opt_set_int(pAudioSwsCxt, "in_channel_layout", pAudioCodecCxt->channel_layout, 0);
    // 这里 AUDIO_DEST_CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO，即 立体声
    av_opt_set_int(pAudioSwsCxt, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    // 配置输入/输出采样率
    av_opt_set_int(pAudioSwsCxt, "in_sample_rate", pAudioCodecCxt->sample_rate, 0);
    av_opt_set_int(pAudioSwsCxt, "out_sample_rate", AUDIO_DST_SAMPLE_RATE, 0);
    // 配置输入/输出数据格式
    av_opt_set_sample_fmt(pAudioSwsCxt, "in_sample_fmt", pAudioCodecCxt->sample_fmt, 0);
    av_opt_set_sample_fmt(pAudioSwsCxt, "out_sample_fmt", outSampleFmt, 0);
    //初始化
    swr_init(pAudioSwsCxt);

    //计算音频重采样后需要的缓冲区大小

    // 重采样后一个通道采样数
    int outNbSample = (int) av_rescale_rnd(ACC_NB_SAMPLES, AUDIO_DST_SAMPLE_RATE,
                                           pAudioCodecCxt->sample_rate, AV_ROUND_UP);
    //重采样后一帧音频的大小
    int outDataSize = (size_t) av_samples_get_buffer_size(
            NULL,
            2,//通道数
            outNbSample,//采样个数
            outSampleFmt,//采样格式
            1);

    AudioDecoder *audioDecoder = nullptr;
    //打开openglse的音频播放器
    if (isAudioOpen) {
        audioDecoder = new AudioDecoder(pAudioCodecCxt, nullptr);
        renderAudio = new RenderAudio(pAudioCodecCxt->channels, pAudioCodecCxt->sample_rate,
                                      audioDecoder);
        if (renderAudio->open() < 0) {
            LOGE(TAG, "创建音频播放器失败");
            return RET_ERROR;
        }
        renderAudio->setPlayVolume(2);
        //开启音频解码线程
        std::thread audioThread = std::thread(&AudioDecoder::start, audioDecoder);
        audioThread.detach();
    }

    if (isVideoOpen) {
        videoDecoder = new VideoDecoder(pVideoCodecCxt);
        //启动解码视频的线程
        std::thread videoThread = std::thread(&VideoDecoder::start, videoDecoder);
        videoThread.detach();
    }

    //开始解码
    int videoCount = 0;
    int audioCount = 0;
    AVPacket *pPacket = NULL;
    pPacket = av_packet_alloc();
    if (pPacket == NULL) {
        LOGE(TAG, "AVPacket申请失败");
        return RET_ERROR;
    }
    av_init_packet(pPacket);

    while (av_read_frame(c, pPacket) == 0) {
        //确保需要解码的是视频帧
        if (pPacket->stream_index == videoIndex) {
            PacketInfo *videoPkt = new PacketInfo();
            videoPkt->packet = pPacket;
            videoDecoder->put(videoPkt);

        } else if (pPacket->stream_index == audioIndex) {//音频数据
            PacketInfo *packetInfo = new PacketInfo();
            packetInfo->packet = pPacket;
            audioDecoder->put(packetInfo);

        }

        pPacket = av_packet_alloc();
        if (pPacket == NULL) {
            LOGE(TAG, "AVPacket申请失败");
            return RET_ERROR;
        }
        av_init_packet(pPacket);
    }


    LOGE(TAG, "解码结束");

    /* av_packet_unref(pPacket);
     av_packet_free(&pPacket);
     pPacket = nullptr;

     av_frame_free(&pFrame);
     pFrame = nullptr;

     sws_freeContext(pVideoSwsCxt);
     pVideoSwsCxt = nullptr;*/
    return RET_SUCCESS;
}


MediaPlayer::MediaPlayer(const char *path, int nDstWidth, int nDstHeight) : path(path),
                                                                            nDstWidth(nDstWidth),
                                                                            nDstHeight(
                                                                                    nDstHeight) {}

int MediaPlayer::decodeFrame(AVMediaType mediaType, AVCodecContext *codecCxt, AVPacket *pkt,
                             AVFrame *pFrame,
                             void (MediaPlayer::*onFrame)(AVMediaType mediaType, AVFrame *)) {
    int ret = RET_ERROR;
    ret = avcodec_send_packet(codecCxt, pkt);
    if (ret < 0) {
        LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
        return RET_ERROR;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(codecCxt, pFrame);
        if (ret == AVERROR(EAGAIN)) {
            break;
        }
        if (ret < 0) {
            LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
            return RET_ERROR;
        }
        //解码一帧成功
        MediaPlayer::onFrame(mediaType, pFrame);
    }
    return ret;
}

void MediaPlayer::onFrame(AVMediaType mediaType, AVFrame *pFrame) {
    if (mediaType == AVMEDIA_TYPE_AUDIO) {
        LOGE(TAG, "接收到音频帧");

    } else if (mediaType == AVMEDIA_TYPE_VIDEO) {
        LOGE(TAG, "接收到视频帧");
    }

}

void MediaPlayer::newThreadDecode() {
    std::thread readThread = std::thread(&MediaPlayer::decode, this);

    //不断的拿到视频渲染
    AVRational tb = c->streams[videoIndex]->time_base;
    while (true) {
        if (videoDecoder != nullptr) {
            FrameInfo *videoFrameInfo = videoDecoder->get();
            AVFrame *videoFrame = videoFrameInfo->videoFrame;
            if (videoFrame == nullptr) {
                return;
            }
            int tid = syscall(SYS_gettid);
            LOGE(TAG , "线程：%d, 开始渲染视频，时间：%f", tid, av_q2d(tb) * videoFrame->pts);
            opengl->draw(videoFrame->data[0], nDstWidth, nDstHeight, egl->eglDisplay,
                         egl->eglSurface);
            av_frame_free(&videoFrame);
            delete videoFrameInfo;
        }
    }
}




