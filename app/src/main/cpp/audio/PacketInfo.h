//
// Created by wangyuelin on 2020/11/27.
//

#ifndef FFMPEGTEST_PACKETINFO_H
#define FFMPEGTEST_PACKETINFO_H

#include <libavcodec/packet.h>

class PacketInfo{
public:
    AVPacket *packet;
    int pos;

};

#endif //FFMPEGTEST_PACKETINFO_H
