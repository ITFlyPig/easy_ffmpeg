//
// Created by wangyuelin on 2020/11/23.
//

#ifndef FFMPEGTEST_MEDIAPLAYER_H
#define FFMPEGTEST_MEDIAPLAYER_H

#include "../utils/StringUtil.h"
#include "../log/log.h"
#include "MediaProvider.h"
#include "RenderAudio.h"
#include "../player/Egl.h"
#include "../player/Ogl.h"
#include "../utils/FrameUtil.h"


class MediaPlayer {
private:
    AVFormatContext *c = nullptr;
    //===========解码视频===========
    int videoIndex = -1;//视频流的索引
    AVCodecID videoCodecId;//解码器id
    AVCodec *pVideoCodec = NULL;
    AVCodecContext *pVideoCodecCxt = NULL;
    bool isVideoOpen = false;
    AVPixelFormat mDstFmt = AV_PIX_FMT_RGBA;//目标格式

    //===========解码音频===========
    int audioIndex = -1;//视频流的索引
    AVCodecID audioCodecId;//解码器id
    AVCodec *pAudioCodec = NULL;
    AVCodecContext *pAudioCodecCxt = NULL;
    bool isAudioOpen = false;
    //===========================
    //文件路劲
    const char *path;
    // ACC音频一帧采样数
    static const int ACC_NB_SAMPLES = 1024;
    //渲染目标的宽和高
    int nDstWidth, nDstHeight;

    MediaProvider *mediaProvider;
    RenderAudio * renderAudio;


    //找到并打开解码器
    int findAndOpenCodec(AVCodecID codecId, AVCodecContext **pVideoCodecCxt,AVCodec **pVideoCodec, const AVCodecParameters *par);

public:
    int open();
    int decode();

    int close();
    //播放时音频的采样率
    static const int AUDIO_DST_SAMPLE_RATE = 44100;

    Egl *egl;
    Opengl *opengl;

    MediaPlayer(const char *path, int nDstWidth, int nDstHeight);


private:
    //解码得到一帧一帧的数据
    int decodeFrame(AVMediaType mediaType, AVCodecContext *codecCxt, AVPacket *pkt, AVFrame *pFrame, void (MediaPlayer::*onFrame)(AVMediaType mediaType, AVFrame *));
    void onFrame(AVMediaType mediaType, AVFrame *pFrame);
};

#endif //FFMPEGTEST_MEDIAPLAYER_H
