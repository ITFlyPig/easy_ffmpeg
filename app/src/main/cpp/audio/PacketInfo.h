//
// Created by wangyuelin on 2020/11/27.
//

#ifndef FFMPEGTEST_PACKETINFO_H
#define FFMPEGTEST_PACKETINFO_H

#include "log.h"

class PacketInfo{
public:
    AVPacket *packet;
    int pos;

    virtual ~PacketInfo();

};

#endif //FFMPEGTEST_PACKETINFO_H
