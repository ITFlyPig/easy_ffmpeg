//
// Created by wangyuelin on 2020/11/27.
//
#include "AudioDecoder.h"
#include "MediaPlayer.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 1;

void AudioDecoder::put(PacketInfo *packetInfo) {
    std::unique_lock<std::mutex> lck(packeTmtx);
    //放入队列
    if (packetInfo != nullptr) {
        LOGE(TAG, "将packet：%d放入队列", packetInfo->pos);
        packetQueue.push_back(packetInfo);
        //通知获线程获取解码后的帧去处理
        getPacketCondition.notify_one();
        //判断是否超过队列的限制
        while (packetQueue.size() > MIN_PACKET_NUM) {
            LOGE(TAG, "将packet：%d放入队列后，队列满了，线程休眠等待packet被消耗", packetInfo->pos);
            putPacketCondition.wait(lck);
        }
    }
    lck.unlock();
}

FrameInfo *AudioDecoder::get() {
    FrameInfo *frame = nullptr;
    std::unique_lock<std::mutex> frameLock(frameTmtx);
    while (frameQueue.empty()) {
        LOGE(TAG, "Frame队列为空，线层休眠等待新的Frame");
        getFrameCondition.wait(frameLock);
    }
    frame = frameQueue.front();
    frameQueue.pop_front();
    LOGE(TAG, "从Frame队列中获取到Frame，剩余的Frame数量%d", frameQueue.size());
    putFrameCondition.notify_one();
    frameLock.unlock();
    return frame;
}

void AudioDecoder::start() {
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
        AVCodecContext *pAudioCodecCxt = audioCodecCxt;
        ret = avcodec_send_packet(pAudioCodecCxt, packetInfo->packet);
        if (ret < 0) {
            LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
            return;
        }
        while (ret >= 0) {
            AVFrame *pAudioFrame = av_frame_alloc();
            ret = avcodec_receive_frame(pAudioCodecCxt, pAudioFrame);
            if (ret == AVERROR(EAGAIN)) {
                break;
            }
            if (ret < 0) {
                LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                return;
            }
            //过滤掉没有数据的帧
//            if (*(pAudioFrame->data[0]) == 0) {
//                continue;
//            }
            if (audioSwr == nullptr) {
                LOGE(TAG, "audioSwr为空，直接退出audio解码");
                return;
            }

            //解码完为了播放，进行重采样
            FrameInfo *audioFrame = new FrameInfo();

            //音频所占用字节数 = 通道数 * 采用频率(Hz) * 采用位数(byte)
            int resampleSize = av_samples_get_buffer_size(
                    NULL,
                    audioSwr->outChannels,//通道数
                    audioSwr->outSampleRate,//单个通道的采样个数
                    audioSwr->outSampleFmt,//采样格式
                    1);//计算1s的数据大小，使缓冲区足够大
            uint8_t *audioOutBuffer = (uint8_t *) malloc(resampleSize);
            //获取新采样率下的采样个数
            int dstNbSamples = av_rescale_rnd(pAudioFrame->nb_samples,
                                              audioSwr->outSampleRate,
                                              audioSwr->inSampleRate,
                                              AV_ROUND_ZERO);
            //重采样，frame 为解码帧
            /**
             * out：输出缓冲区
             * out_count：？
             * in：输如缓冲区
             * in_count：一个声道上有效的输入采样数
             * 返回值：每个通道的采样数
             */
           int len = swr_convert(audioSwr->pSwrContext, &audioOutBuffer, dstNbSamples,
                              (const uint8_t **) pAudioFrame->data, pAudioFrame->nb_samples);
            if (len < 0) {
                continue;
            }
            //计算重采样后的数据大小
            int resampledDataSize = len * audioSwr->outChannels * av_get_bytes_per_sample(audioSwr->outSampleFmt);
            audioFrame->data = audioOutBuffer;
            audioFrame->size = resampledDataSize;

//            uint8_t *resampleFrame = nullptr;
//            int size = audioSwr->convert(&resampleFrame, pAudioFrame->nb_samples,
//                                         (const uint8_t**)pAudioFrame->data,
//                                         pAudioFrame->nb_samples);
//            if (size > 0) {
//                audioFrame->data = resampleFrame;
//                audioFrame->size = size;
//            } else {
//                delete audioFrame;
//                continue;
//            }


            /*//得到音频的时间基
            AVRational tb = (AVRational) {1, pAudioCodecCxt->sample_rate};
//                LOGD(TAG, "是否是平面音频数据:%d", av_sample_fmt_is_planar(pAudioCodecCxt->sample_fmt))
            uint8_t *audioOutBuffer = (uint8_t *) malloc(resampleFrameSize);
            //重采样，frame 为解码帧
            ret = swr_convert(mediaPlayer->pAudioSwsCxt, &audioOutBuffer, resampleFrameSize / 2,
                              (const uint8_t **) pAudioFrame->data, pAudioFrame->nb_samples);
            FrameInfo *frameInfo = nullptr;
            frameInfo = new FrameInfo(audioOutBuffer, resampleFrameSize, pAudioFrame->pts);
            frameInfo->tb = tb;*/


            LOGE(TAG, "将解码得到的Frame放入队列");
            frameQueue.push_back(audioFrame);
            //将采样后的数据放入帧队列
            std::unique_lock<std::mutex> frameLock(frameTmtx);
            while (frameQueue.size() >= MAX_FRAME_NUM) {
                LOGE(TAG, "将解码后得到的Frame放入到帧队列，队列满了，线程休眠等待Frame被消耗，此时Frame队列中的数量%d",
                     frameQueue.size());
                putFrameCondition.wait(frameLock);
            }
            getFrameCondition.notify_one();
            frameLock.unlock();
        }

    } while (ret >= 0 || ret == AVERROR(EAGAIN));
    LOGE(TAG, "AudioDecoder::start() 方法结束");


}

AudioDecoder::AudioDecoder(AVCodecContext *audioCodecCxt, AudioSwr *audioSwr) : audioCodecCxt(
        audioCodecCxt), audioSwr(audioSwr) {}


