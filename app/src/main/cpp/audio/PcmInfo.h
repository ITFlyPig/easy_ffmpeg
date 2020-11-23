//
// Created by wangyuelin on 2020/11/21.
//

#ifndef FFMPEGTEST_PCMINFO_H
#define FFMPEGTEST_PCMINFO_H

class PcmInfo {


public:
    PcmInfo( void *data, long size);

//存Pcm数据地址
    void *data;

//数据的大小
    long size;
};

#endif //FFMPEGTEST_PCMINFO_H
