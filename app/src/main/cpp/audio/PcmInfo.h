//
// Created by wangyuelin on 2020/11/21.
//

#ifndef FFMPEGTEST_PCMINFO_H
#define FFMPEGTEST_PCMINFO_H

#include "log.h"

class PcmInfo {


public:
    PcmInfo( void *data, long size, int64_t pts);

//存Pcm数据地址
    void *data;

//数据的大小
    long size;
    //音频帧的pts
    int64_t pts;

    //音频时间基
    AVRational *tb;

};

#endif //FFMPEGTEST_PCMINFO_H
