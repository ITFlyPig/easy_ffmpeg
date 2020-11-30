//
// Created by wangyuelin on 2020/11/29.
//
#include "VideoDecoder.h"
#include "MediaPlayer.h"


static int RET_ERROR = -1;
static int RET_SUCCESS = 1;
void VideoDecoder::put(PacketInfo *packetInfo) {
    std::unique_lock<std::mutex> lck(packeTmtx);
    //放入队列
    if (packetInfo != nullptr) {
        LOGE(TAG, "将video packet：%d放入队列", packetInfo->pos);
        packetQueue.push_back(packetInfo);
        //通知获线程获取解码后的帧去处理
        getPacketCondition.notify_one();
        //判断是否超过队列的限制
        while (packetQueue.size() > MIN_PACKET_NUM) {
            LOGE(TAG, "将video packet：%d放入队列后，队列满了，线程休眠等待video packet被消耗", packetInfo->pos);
            putPacketCondition.wait(lck);
        }
    }
    lck.unlock();
}

FrameInfo *VideoDecoder::get() {
    FrameInfo *frame = nullptr;
    std::unique_lock<std::mutex> frameLock(frameTmtx);
    while (frameQueue.empty()) {
        LOGE(TAG, "video Frame队列为空，线层休眠等待新的video Frame");
        getFrameCondition.wait(frameLock);
    }
    frame = frameQueue.front();
    frameQueue.pop_front();
    LOGE(TAG, "从video Frame队列中获取到video Frame，剩余的video Frame数量%d", frameQueue.size());
    putFrameCondition.notify_one();
    frameLock.unlock();
    return frame;
}

void VideoDecoder::start() {
    //1.获取packet
    //2.解码packet得到frame
    //3.frame放入队列
    int ret = RET_ERROR;
    do {
        PacketInfo *packetInfo = nullptr;
        std::unique_lock<std::mutex> lck(packeTmtx);
        while (packetQueue.empty()) {
            LOGE(TAG, "Packet队列为空，线层休眠等待新的Packet");
            getPacketCondition.wait(lck);
        }
        packetInfo = packetQueue.front();
        packetQueue.pop_front();
        LOGE(TAG, "从Packet队列中获取到Packet，剩余的Packet数量%d", packetQueue.size());
        putPacketCondition.notify_one();
        lck.unlock();

        //开始解码
        LOGE(TAG, "开始解码获取到的Packet，pos为：%lld", packetInfo->packet->pos);
        AVCodecContext *pVideoCodecCxt = mediaPlayer->pVideoCodecCxt;

        ret = avcodec_send_packet(pVideoCodecCxt, packetInfo->packet);
        if (ret < 0) {
            LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
            break;
        }
        while (ret >= 0) {
            AVFrame *pFrame = NULL;
            pFrame = av_frame_alloc();
            ret = avcodec_receive_frame(pVideoCodecCxt, pFrame);
            if (ret == AVERROR(EAGAIN)) {
                break;
            }
            if (ret < 0) {
                LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                return;
            }
            //开始格式转换
            AVFrame *pRGBFame = FrameUtil::alloc_picture(mediaPlayer->mDstFmt, mediaPlayer->nDstWidth, mediaPlayer->nDstHeight);
            if (pRGBFame == nullptr) {
                LOGE(TAG, "alloc_picture 申请Frame失败");
                continue;
            }
            ret = sws_scale(mediaPlayer->pVideoSwsCxt, pFrame->data, pFrame->linesize, 0, pFrame->height,
                            pRGBFame->data, pRGBFame->linesize);
            if (ret < 0) {
                LOGE(TAG, "sws_scale 转换失败:%s", av_err2str(ret));
                return;
            }

//            LOGD(TAG, "开始渲染的视频的时间：%f", pFrame->pts * av_q2d(pVideoCodecCxt->time_base));
            //开始渲染
            FrameInfo *frameInfo = nullptr;
            frameInfo = new FrameInfo(pRGBFame);
            LOGE(TAG, "将解码得到的video Frame放入队列");
            //将采样后的数据放入帧队列
            std::unique_lock<std::mutex> frameLock(frameTmtx);
            frameQueue.push_back(frameInfo);
            while (frameQueue.size() >= MAX_FRAME_NUM) {
                LOGE(TAG, "将解码后得到的Video Frame放入到帧队列，队列满了，线程休眠等待Frame被消耗，此时Frame队列中的数量%d", frameQueue.size());
                putFrameCondition.wait(frameLock);
            }
            getFrameCondition.notify_one();
            frameLock.unlock();
        }

    } while (ret >= 0 || ret == AVERROR(EAGAIN));
    LOGE(TAG, "AudioDecoder::start() 方法结束");


}

VideoDecoder::VideoDecoder(MediaPlayer *mediaPlayer) {
    this->mediaPlayer = mediaPlayer;

}

