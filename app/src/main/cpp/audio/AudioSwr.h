//
// Created by wangyuelin on 2020/12/4.
//

#ifndef FFMPEGTEST_AUDIOSWR_H
#define FFMPEGTEST_AUDIOSWR_H

#include "log.h"

/**
 * 音频重采样封装
 */
class AudioSwr {
public:
    SwrContext *pSwrContext = nullptr;
    //输入声道数
    int inChannels;
    //输出声道数
    int outChannels;
    //输入采样率
    int inSampleRate;
    //输出采样率
    int outSampleRate;
    //输入音频格式
    AVSampleFormat inSampleFmt;
    //输出音频格式
    AVSampleFormat outSampleFmt;

public:
    AudioSwr(int inChannels, int outChannels, int inSampleRate, int outSampleRate,
             AVSampleFormat inSampleFmt, AVSampleFormat outSampleFmt);


    /**
     * 准备变换
     * @return
     */
    int prepare();

    /**
     * 重采样
     * @param out        输出缓冲区
     * @param out_count  输出的采样个数
     * @param in         输入缓冲区
     * @param in_count   输入采样个数
     * @return
     */
    int convert(uint8_t **out, int out_count, const uint8_t **in , int in_count);

    /**
     * 结束，释放资源
     * @return
     */
    int end();

};


#endif //FFMPEGTEST_AUDIOSWR_H
