//
// Created by wangyuelin on 2020/12/4.
//

#ifndef FFMPEGTEST_OTHERBGPLAYER_H
#define FFMPEGTEST_OTHERBGPLAYER_H

#include "../utils/StringUtil.h"
#include "log.h"
#include "FormatCxt.h"
#include "OnFrameCallBack.h"
#include "VideoDecoder.h"
#include <thread>
#include "../player/Egl.h"
#include "../player/Ogl.h"
#include "VideoSws.h"
#include "AudioDecoder.h"
#include "RenderAudio.h"
#include "AudioSwr.h"

class OtherBgPlayer : public OnFrameCallBack {
private:
    //===========视频需要start===============
    //视频文件的上下文
    FormatCxt *videoCxt = nullptr;
    //视频解码
    VideoDecoder *videoDecoder = nullptr;
    //视频变换
    VideoSws *videoSws = nullptr;

    Egl *egl = nullptr;
    Opengl *opengl = nullptr;

    //渲染窗口
    ANativeWindow *nativeWindow = nullptr;
    //目标宽度
    int dstWidth = 0;
    //目标高度
    int dstHeight = 0;
    //目标格式
    AVPixelFormat dstFmt = AV_PIX_FMT_RGBA;
    //===========视频需要end===============

    //===========音频需要start===============
    //背景音乐文件的上下文
    FormatCxt *bgCxt = nullptr;
    //解码音频
    AudioDecoder *audioDecoder = nullptr;
    //渲染音频需要
    RenderAudio * renderAudio = nullptr;
    //音频重采样需要
    AudioSwr *audioSwr = nullptr;
    int outSampleRate;//音频输出采样率
    int outChannels;//音频输出声道数
    AVSampleFormat outSampleFmt;//音频输出格式
    //===========音频需要end===============



    //表示是否停止播放
    bool isClose = false;
    //是否暂听播放
    bool isPause = false;


    int openVideo(const char *videoPath);
    int openOtherAudio(const char * audioPath);
    /**
    * 不断的读取数据包
    */
    void readStream();

    /**
     * 读取视频流
     */
    void readVideoStream();

    /**
     * 读取音频流
     */
    void readAudioStream();


public:
    /**
     * 开始播放视频和音频
     * @return
     */
    int startPlay(const char *videoPath,const char * audioPath);
    int close();

    OtherBgPlayer(ANativeWindow *nativeWindow, int dstWidth, int dstHeight);
};

#endif //FFMPEGTEST_OTHERBGPLAYER_H
