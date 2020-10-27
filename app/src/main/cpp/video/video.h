//
// Created by wangyuelin on 2020/10/21.
//

#ifndef FFMPEGTEST_VIDEO_H
#define FFMPEGTEST_VIDEO_H

#include <jni.h>
#include <libavformat/avformat.h>

class Video {
private:
    AVFormatContext *c;
    //视频流的索引
    int videoIndex = -1;
    //文件路劲
    char* path;

public:


};
#endif //FFMPEGTEST_VIDEO_H
