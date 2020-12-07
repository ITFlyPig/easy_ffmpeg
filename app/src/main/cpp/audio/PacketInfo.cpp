//
// Created by wangyuelin on 2020/11/27.
//

#include "PacketInfo.h"

PacketInfo::~PacketInfo() {
    if (packet != nullptr) {
        av_packet_free(&packet);
        packet = nullptr;

    }

}
