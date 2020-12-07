//
// Created by wangyuelin on 2020/11/21.
//

#ifndef FFMPEGTEST_RENDERAUDIO_H
#define FFMPEGTEST_RENDERAUDIO_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "log.h"
#include "PcmProvider.h"
#include "MediaProvider.h"
#include "AudioDecoder.h"

class RenderAudio {
private:
    //据声道数，返回opengles识别的声道：立体声 | 单声道
    int GetChannelMask(int channels);
    //据传入的采样率，转为opengles识别的采样率
    int openSLSampleRate(SLuint32 sampleRate);


private:
    SLObjectItf engineObj = NULL;//用SLObjectItf声明引擎接口对象
    SLEngineItf iEngine = NULL;//声明具体的引擎对象实例
    //混音器
    SLObjectItf mixObj = NULL;//用SLObjectItf创建混音器接口对象
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;//创建具体的混音器对象实例

    //播放器
    SLObjectItf playerObj = NULL;
    //player接口
    SLPlayItf iPlayer = NULL;

    //音量接口
    SLVolumeItf iVolume = NULL;
    //缓冲区接口
    SLAndroidSimpleBufferQueueItf iPcmBufferQueue = NULL;

    int channels;//声道数
    long sampleRate;//采样率
    AudioDecoder *pcmProvider;//pcm数据提供者


public:
    RenderAudio(int channels, long sampleRate, AudioDecoder *pcmProvider);

public:
    int open();

    int close();
    //设置播放的音量
    void setPlayVolume(int percent);
    //将pcm数据送入到opengles的播放队列
    int enqueuePcm();

};

#endif //FFMPEGTEST_RENDERAUDIO_H
