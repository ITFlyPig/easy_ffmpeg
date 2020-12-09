//
// Created by wangyuelin on 2020/12/4.
//

#include "OtherBgPlayer.h"

int OtherBgPlayer::openVideo(const char *videoPath) {
    if (StringUtil::isEmpty(videoPath)) {
        LOGE(TAG, "视频文件路劲为空")
        return ERROR_CODE;
    }
    videoCxt = new FormatCxt(videoPath);
    if (videoCxt->open() < 0) {
        LOGE(TAG, "媒体文件打开失败，路劲为：%s", videoPath);
        return ERROR_CODE;
    }

    //初始化视频变换
    AVCodecContext *codecCxt = videoCxt->videoCodecCxt;
    videoSws = new VideoSws(
            codecCxt->width,//源宽
            codecCxt->height,//源高
            codecCxt->pix_fmt,//源格式
            dstWidth,
            dstHeight,
            dstFmt);
    int ret = videoSws->prepare();
    if (ret < 0) {
        LOGE(TAG, "OtherBgPlayer::openVideo 准备VideoSws失败");
        return ERROR_CODE;
    }

    //初始化Egl
    egl = new Egl();
    if (egl->open(nativeWindow) < 0) {
        LOGE(TAG, "Egl初始化失败");
        return ERROR_CODE;
    } else {
        LOGE(TAG, "Egl初始化成功");
    }
    //初始化OpenGL
    opengl = new Opengl();
    if (!opengl->CreateProgram()) {
        LOGE(TAG, "OpenGL 初始化失败");
        return ERROR_CODE;
    } else {
        LOGE(TAG, "OpenGL 初始化成功");
    }
    return SUCCESS_CODE;
}

int OtherBgPlayer::openOtherAudio(const char *audioPath) {
    if (StringUtil::isEmpty(audioPath)) {
        LOGE(TAG, "背景音乐文件路劲为空")
        return ERROR_CODE;
    }
    bgCxt = new FormatCxt(audioPath);
    if (bgCxt->open() < 0) {
        LOGE(TAG, "媒体文件打开失败，路劲为：%s", audioPath);
        return ERROR_CODE;
    }
    //初始化音频重采样
    AVCodecContext *codecCxt = bgCxt->audioCodecCxt;
    outChannels = codecCxt->channels;
    outSampleRate = codecCxt->sample_rate;
    outSampleFmt = AV_SAMPLE_FMT_S16;
    audioSwr = new AudioSwr(codecCxt->channels, outChannels, codecCxt->sample_rate, outSampleRate,
                            codecCxt->sample_fmt, outSampleFmt);
    if (audioSwr->prepare() < 0) {
        LOGE(TAG, "AudioSwr准备失败");
    }
    return SUCCESS_CODE;
}

int OtherBgPlayer::startPlay(const char *videoPath, const char *audioPath) {
    if (openVideo(videoPath) < 0) {
        LOGE(TAG, "OtherBgPlayer::startPlay 视频文件打开失败")
        return ERROR_CODE;
    }
    if (openOtherAudio(audioPath) < 0) {
        LOGE(TAG, "OtherBgPlayer::startPlay 背景音乐文件打开失败")
        return ERROR_CODE;
    }
    //1. 开启视频解码线程
    videoDecoder = new VideoDecoder(videoCxt->videoCodecCxt);
    std::thread videoThread = std::thread(&VideoDecoder::start, videoDecoder);
    //在音频打开成功的情况下才处理音频相关的数据
    if (bgCxt->getAudioOpen()) {
        //2. 开启音频解码线程
        audioDecoder = new AudioDecoder(bgCxt->audioCodecCxt, audioSwr);
        std::thread audioThread = std::thread(&AudioDecoder::start, audioDecoder);
        audioThread.detach();
        //开始渲染音频数据
        AVCodecContext *audioCodecCxt = bgCxt->audioCodecCxt;
        renderAudio = new RenderAudio(audioCodecCxt->channels, audioCodecCxt->sample_rate,
                                      audioDecoder);
        if (renderAudio->open() < 0) {
            LOGE(TAG, "RenderAudio open打开音频渲染器失败");
        } else {
            renderAudio->setPlayVolume(5);
        }

    }

    //3. 在单独的线程读取音频
    std::thread readAudioThread = std::thread(&OtherBgPlayer::readAudioStream, this);
    readAudioThread.detach();
    //4. 在单独的线程读取视频
    std::thread readVideoThread = std::thread(&OtherBgPlayer::readVideoStream, this);
    readVideoThread.detach();

    int ret = ERROR_CODE;
    //不断的渲染视频，在当前线程渲染视频
    while (!isClose) {
        FrameInfo *frameInfo = videoDecoder->get();
        if (frameInfo == nullptr) {
            continue;
        }
        AVFrame *frame = frameInfo->videoFrame;
        if (frame == nullptr) {
            continue;
        }
        //据音频的pts同步当前视频的播放进度
        double curAudioTime = audioDecoder->curPts * av_q2d(bgCxt->c->streams[bgCxt->audioStreamIndex]->time_base) * 1000;
        double curVideoTime = frame->pts * av_q2d(videoCxt->videoCodecCxt->time_base) * 1000;
        double delay = av_q2d(videoCxt->videoCodecCxt->time_base) * 1000;
        double diff = curVideoTime - curAudioTime;
        if (diff < 20 || diff > -20 ) {

        } else if (diff >= 20) {
            delay *= 2;
        } else {
            delay = 0;
        }

        av_usleep(delay * 1000);

        AVFrame *rgbFrame = nullptr;
        ret = videoSws->scale(frame, &rgbFrame);
        av_frame_free(&frame);
        if (ret < 0) {
            LOGE(TAG, "OtherBgPlayer::startPlay scale 转换失败");
            continue;
        }

        //开始渲染
        opengl->draw(rgbFrame->data[0], dstWidth, dstHeight, egl->eglDisplay, egl->eglSurface);
        av_frame_free(&rgbFrame);
        LOGE(TAG, "当前播放的音频时间：%f, 当前播放的视频时间：%f", curAudioTime, curVideoTime);
    }
    return SUCCESS_CODE;
}

void OtherBgPlayer::readStream() {
    while (!isClose) {
        int ret = ERROR_CODE;
        //读取视频包
        /*AVFormatContext *c = videoCxt->c;
        AVPacket *packet = av_packet_alloc();
        if (packet == nullptr) {
            LOGE(TAG, "OtherBgPlayer::readStream Packet 申请失败");
            return;
        }
        av_init_packet(packet);
        while (av_read_frame(c, packet) == 0) {
            if (packet->stream_index == videoCxt->videoStreamIndex) {//视频包
                PacketInfo *packetInfo = new PacketInfo();
                packetInfo->packet = packet;
                videoDecoder->put(packetInfo);

                packet = av_packet_alloc();
                if (packet == nullptr) {
                    LOGE(TAG, "OtherBgPlayer::readStream Packet 申请失败");
                    return;
                }
                av_init_packet(packet);
            }
        }*/

        //读取音频包
        AVFormatContext *audioFmtCxt = bgCxt->c;

        AVPacket *packet = av_packet_alloc();
        if (packet == nullptr) {
            LOGE(TAG, "OtherBgPlayer::readStream Packet 申请失败");
            return;
        }
        av_init_packet(packet);

        while (av_read_frame(audioFmtCxt, packet) == 0) {
            if (packet->stream_index == bgCxt->audioStreamIndex) {
                PacketInfo *packetInfo = new PacketInfo();
                packetInfo->packet = packet;
                audioDecoder->put(packetInfo);

                packet = av_packet_alloc();
                if (packet == nullptr) {
                    LOGE(TAG, "OtherBgPlayer::readStream Packet 申请失败");
                    return;
                }
                av_init_packet(packet);
            }
        }
    }


}

OtherBgPlayer::OtherBgPlayer(ANativeWindow *nativeWindow, int dstWidth, int dstHeight)
        : nativeWindow(nativeWindow), dstWidth(dstWidth), dstHeight(dstHeight) {}

void OtherBgPlayer::readAudioStream() {
    while (!isClose) {
        int ret = ERROR_CODE;
        //读取音频包
        AVFormatContext *audioFmtCxt = bgCxt->c;

        AVPacket *packet = av_packet_alloc();
        if (packet == nullptr) {
            LOGE(TAG, "OtherBgPlayer::readStream Packet 申请失败");
            return;
        }
        av_init_packet(packet);

        while (av_read_frame(audioFmtCxt, packet) == 0) {
            if (packet->stream_index == bgCxt->audioStreamIndex) {
                PacketInfo *packetInfo = new PacketInfo();
                packetInfo->packet = packet;
                audioDecoder->put(packetInfo);

                packet = av_packet_alloc();
                if (packet == nullptr) {
                    LOGE(TAG, "OtherBgPlayer::readStream Packet 申请失败");
                    return;
                }
                av_init_packet(packet);
            }
        }
    }

}

void OtherBgPlayer::readVideoStream() {
    while (!isClose) {
        int ret = ERROR_CODE;
        //读取视频包
        AVFormatContext *c = videoCxt->c;
        AVPacket *packet = av_packet_alloc();
        if (packet == nullptr) {
            LOGE(TAG, "OtherBgPlayer::readVideoStream Packet 申请失败");
            return;
        }
        av_init_packet(packet);
        while (av_read_frame(c, packet) == 0) {
            if (packet->stream_index == videoCxt->videoStreamIndex) {//视频包
                PacketInfo *packetInfo = new PacketInfo();
                packetInfo->packet = packet;
                videoDecoder->put(packetInfo);

                packet = av_packet_alloc();
                if (packet == nullptr) {
                    LOGE(TAG, "OtherBgPlayer::readVideoStream Packet 申请失败");
                    return;
                }
                av_init_packet(packet);
            }
        }
    }

}


void OnFrameCallBack::onFrame(AVFrame *frame, AVMediaType mediaType) {

}

