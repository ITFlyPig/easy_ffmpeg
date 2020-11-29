//
// Created by wangyuelin on 2020/11/27.
//

#ifndef FFMPEGTEST_FRAMEINFO_H
#define FFMPEGTEST_FRAMEINFO_H

#include <cstdlib>

class FrameInfo{
public:
    //存Pcm数据地址
    void *data;
    //数据的大小
    long size;
    //音频帧的pts
    int64_t pts;

public:
    FrameInfo(void *data, long size, int64_t pts);


};

#endif //FFMPEGTEST_FRAMEINFO_H
