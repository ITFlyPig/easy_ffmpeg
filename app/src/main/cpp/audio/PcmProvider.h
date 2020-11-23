//
// Created by wangyuelin on 2020/11/21.
//

#ifndef FFMPEGTEST_PCMPROVIDER_H
#define FFMPEGTEST_PCMPROVIDER_H

#include "PcmInfo.h"

class PcmProvider {
    //提供一帧Pcm数据
public:
    virtual PcmInfo* provide();
};

#endif //FFMPEGTEST_PCMPROVIDER_H
