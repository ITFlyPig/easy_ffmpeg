//
// Created by wangyuelin on 2020/11/11.
//


#include "player.h"


static const int ERROR = -1;
static const int SUCCESS = 0;

void *Player::openVideo(void *player) {
    Player *pPlayer = static_cast<Player *>(player);
    int ret = -1;
    //打开视频，初始化ffmpeg
    ret = pPlayer->open(pPlayer->path);
    if (ret < 0) {
        LOGE(TAG, "open视频打开失败");
        return (void *) ERROR;
    }
    //初始化Egl
    pPlayer->egl = new Egl();
    if (pPlayer->egl->open(pPlayer->aNativeWindow) < 0) {
        LOGE(TAG, "Egl初始化失败");
        return (void *) ERROR;
    }
    //初始化OpenGL
    pPlayer->opengl = new Opengl();
    if (!pPlayer->opengl->CreateProgram()) {
        LOGE(TAG, "OpenGL 初始化失败");
        return (void *) ERROR;
    } else {
        LOGE(TAG, "OpenGL 初始化成功");
    }
    //egl和OpenGL都准备好了，开始解码
    pPlayer->decode();
    //播放完毕，释放资源
    pPlayer->release();
    return reinterpret_cast<void *>(1);

}

void Player::newThread(char *path, ANativeWindow *aNativeWindow) {
    this->path = path;
    this->aNativeWindow = aNativeWindow;
    av_log_set_callback(FFLog::log_callback_android);
//    pthread_create(&pthread, NULL, openVideo, (void *) this);
}


int Player::open(char *path) {
    int ret = -1;

    if (StringUtil::isEmpty(path)) {
        LOGE(TAG, "要打开的视频文件的路劲为空");
        return RET_ERROR;
    }
    ret = avformat_open_input(&c, path, NULL, NULL);
    if (ret) {
        LOGE(TAG, "视频文件：%s 打开失败:%s", path, av_err2str(ret))
        return RET_ERROR;
    }
    ret = avformat_find_stream_info(c, NULL);
    if (ret < 0) {
        LOGE(TAG, "avformat_find_stream_info 寻找流信息失败：%s", av_err2str(ret));
        return RET_ERROR;
    }
    //找到视频流的索引
    for (int i = 0; i < c->nb_streams; i++) {
        if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            codecId = c->streams[i]->codecpar->codec_id;
            AVRational frameRate = c->streams[i]->r_frame_rate;
            if (frameRate.num > 0 && frameRate.den > 0) {
                delay = int(1000000 * ((float) frameRate.den / (float) frameRate.num));
            }
            break;
        }
    }
    if (videoIndex == -1) {
        LOGE(TAG, "未找到视频流的索引");
        return RET_ERROR;
    }
    codec = avcodec_find_decoder(codecId);
    if (codec == NULL) {
        LOGE(TAG, "avcodec_find_decoder未找到解码器");
        return RET_ERROR;
    }
    //创建解码器上下文
    codecCxt = avcodec_alloc_context3(codec);
    if (codecCxt == NULL) {
        LOGE(TAG, "解码器上下文创建失败");
        return RET_ERROR;
    }

    //将解析得到的解码参数填充到解码上下文
    ret = avcodec_parameters_to_context(codecCxt, c->streams[videoIndex]->codecpar);
    if (ret < 0) {
        LOGE(TAG, "avcodec_parameters_to_context 填充参数失败：%s", av_err2str(ret));
        return RET_ERROR;
    }

    //打开解码器
    ret = avcodec_open2(codecCxt, codec, NULL);
    if (ret < 0) {
        LOGE(TAG, "解码器打开失败：%s", av_err2str(ret));
        return RET_ERROR;
    }

    //打印基本信息
    av_dump_format(c, videoIndex, path, 0);

    LOGE(TAG, "视频文件：%s 打开成功", path);

    return SUCCESS;

}

int Player::decode() {
    AVFrame *pFrame = NULL;
    AVPacket *pPacket = NULL;
    SwsContext *pSwsCxt = NULL;
    int ret = RET_ERROR;

    pPacket = av_packet_alloc();
    if (pPacket == NULL) {
        LOGE(TAG, "AVPacket申请失败");
        return RET_ERROR;
    }
    av_init_packet(pPacket);
    pFrame = av_frame_alloc();
    if (pFrame == NULL) {
        LOGE(TAG, "AVFrame申请失败");
        return RET_ERROR;
    }

    //将YUV转为RGBA
    if (m_nDstHeight <= 0 || m_nDstWidth <= 0) {
        LOGE(TAG, "需要渲染的宽度或高度为0");
        return RET_ERROR;
    }
    pSwsCxt = sws_getContext(codecCxt->width, codecCxt->height, codecCxt->pix_fmt,
                             m_nDstWidth, m_nDstHeight, m_DstFmt,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);

    if (pSwsCxt == nullptr) {
        LOGE(TAG, "sws_getContext 失败");
        return RET_ERROR;
    }

    //开始解码
    int index = 0;
    while (av_read_frame(c, pPacket) == 0) {
        //确保需要解码的是视频帧
        if (pPacket->stream_index == videoIndex) {
            ret = avcodec_send_packet(codecCxt, pPacket);
            if (ret < 0) {
                LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
                return RET_ERROR;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codecCxt, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    LOGE(TAG, "直接开始下次的av_read_frame 和 avcodec_send_packet");
                    break;
                }
                if (ret < 0) {
                    LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                    return RET_ERROR;
                }

                AVFrame *pRGBAFrame = alloc_picture(m_DstFmt, m_nDstWidth, m_nDstHeight);
                if (pRGBAFrame == nullptr) {
                    LOGE(TAG, "申请帧空间RGBAFrame失败");
                    return RET_ERROR;
                }
//                LOGE(TAG, "申请帧空间RGBAFrame成功");
                //将YUV转为opengl能渲染的RGBA
                ret = sws_scale(pSwsCxt, pFrame->data, pFrame->linesize, 0, pFrame->height,
                                pRGBAFrame->data, pRGBAFrame->linesize);
//                LOGE(TAG, "%d 帧转换成功：slice 高度：%d", index, ret);
                //开始渲染
                opengl->draw(pRGBAFrame->data[0], m_nDstWidth, m_nDstHeight, egl->eglDisplay,
                             egl->eglSurface);
//
                //释放
//                av_frame_unref(pFrame);
                //释放存RGBA数据帧的空间
                index++;
                av_frame_free(&pRGBAFrame);

                //延迟部分时间，为了播放速度比较正常
                usleep(delay);
            }


        }
        av_packet_unref(pPacket);


    }

    av_packet_unref(pPacket);
    av_packet_free(&pPacket);
    pPacket = nullptr;

    av_frame_free(&pFrame);
    pFrame = nullptr;

    sws_freeContext(pSwsCxt);
    pSwsCxt = nullptr;

    return 1;

}

AVFrame *Player::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
    AVFrame *picture = NULL;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        LOGE(TAG, "Could not allocate frame data");
        return nullptr;
    }

    return picture;
}

int Player::release() {


    if (c != nullptr) {
        avformat_close_input(&c);
        avformat_free_context(c);
        c = nullptr;
    }
    if (codecCxt != nullptr) {
        avcodec_free_context(&codecCxt);
        codecCxt = nullptr;
    }


    return 0;
}

int Player::reversePlay() {

    int ret = -1;
    //初始化Egl
    egl = new Egl();
    if (egl->open(aNativeWindow) < 0) {
        LOGE(TAG, "Egl初始化失败");
        return ERROR;
    }
    //初始化OpenGL
    opengl = new Opengl();
    if (!opengl->CreateProgram()) {
        LOGE(TAG, "OpenGL 初始化失败");
        return ERROR;
    } else {
        LOGE(TAG, "OpenGL 初始化成功");
    }
    //开始生产
    th = std::thread(&Player::produceFrame, this);
    th.detach();
    //开始播放
    consumeFrame();

    //播放完毕，释放资源
    if (egl != nullptr) {
        egl->close();
        egl = nullptr;
    }
    return RET_SUCCESS;
}

std::vector<int64_t> Player::reverseDecode() {
    AVFrame *pFrame = NULL;
    AVPacket *pPacket = NULL;
    std::vector<int64_t> iFramePts;

    pPacket = av_packet_alloc();
    if (pPacket == NULL) {
        LOGE(TAG, "AVPacket申请失败");
        return iFramePts;
    }
    av_init_packet(pPacket);
    pPacket->pts;
    pFrame = av_frame_alloc();
    if (pFrame == NULL) {
        LOGE(TAG, "AVFrame申请失败");
        return iFramePts;
    }

    //将YUV转为RGBA
    if (m_nDstHeight <= 0 || m_nDstWidth <= 0) {
        LOGE(TAG, "需要渲染的宽度或高度为0");
        return iFramePts;
    }
    //拿到所有的I帧的pts
    int ret = RET_ERROR;
    while (av_read_frame(c, pPacket) == 0) {
        //确保需要解码的是视频帧
        if (pPacket->stream_index == videoIndex) {
            ret = avcodec_send_packet(codecCxt, pPacket);
            if (ret < 0) {
                LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
                return iFramePts;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codecCxt, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }
                if (ret < 0) {
                    LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                    return iFramePts;
                }
                LOGE(TAG, "获取到I帧，显示的pts为：%lld", pFrame->pts);
                if (pFrame->key_frame == 1) {
                    iFramePts.push_back(pFrame->pts);
                }
            }
        }
        av_packet_unref(pPacket);
    }


    av_packet_unref(pPacket);
    av_packet_free(&pPacket);
    pPacket = nullptr;

    av_frame_free(&pFrame);
    pFrame = nullptr;

    //检查是否获取到I帧序列了
    if (iFramePts.empty()) {
        LOGE(TAG, "未获取到I帧序列")
    }
    return iFramePts;

    //据I帧seek，然后倒叙播放
    /* int size = iFramePts.size();
     int64_t last = 0;
     for (int i = size - 1; i >= 0; i--) {
         LOGE(TAG, "开始播放地%d帧关键帧", i);
         ret = playByIFrame(iFramePts[i], last);
         if (ret < 0) {
             LOGE(TAG, "playByIFrame播放失败");
             return RET_ERROR;
         }
         last = iFramePts[i];
     }
     iFramePts.clear();
     return RET_SUCCESS;*/
}

int Player::playByIFrame(int64_t iFramePts, int64_t lastPts) {
    int ret = RET_ERROR;
    ret = av_seek_frame(c, videoIndex, iFramePts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE(TAG, "av_seek_frame 到指定I帧失败，timestamp：%lld", iFramePts);
        return RET_ERROR;
    }


    AVFrame *pFrame = NULL;
    AVPacket *pPacket = NULL;
    SwsContext *pSwsCxt = NULL;

    pPacket = av_packet_alloc();
    if (pPacket == NULL) {
        LOGE(TAG, "AVPacket申请失败");
        return RET_ERROR;
    }
    av_init_packet(pPacket);
    pFrame = av_frame_alloc();
    if (pFrame == NULL) {
        LOGE(TAG, "AVFrame申请失败");
        return RET_ERROR;
    }

    //将YUV转为RGBA
    if (m_nDstHeight <= 0 || m_nDstWidth <= 0) {
        LOGE(TAG, "需要渲染的宽度或高度为0");
        return RET_ERROR;
    }
    pSwsCxt = sws_getContext(codecCxt->width, codecCxt->height, codecCxt->pix_fmt,
                             m_nDstWidth, m_nDstHeight, m_DstFmt,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (pSwsCxt == nullptr) {
        LOGE(TAG, "sws_getContext 失败");
        return RET_ERROR;
    }


    //开始解码
    int index = 0;
    bool isStop = false;
    std::vector<AVFrame *> frames;
    while (av_read_frame(c, pPacket) == 0) {
        //确保需要解码的是视频帧
        if (isStop) {
            break;
        }
        if (pPacket->stream_index == videoIndex) {
            ret = avcodec_send_packet(codecCxt, pPacket);
            if (ret < 0) {
                LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
                return RET_ERROR;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codecCxt, pFrame);
                if (ret == AVERROR(EAGAIN)) {
//                    LOGE(TAG, "直接开始下次的av_read_frame 和 avcodec_send_packet");
                    break;
                }
                if (ret < 0) {
                    LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                    return RET_ERROR;
                }
                //检测是否解码到上次播放的帧
                if (pFrame->pts == lastPts) {
                    isStop = true;
                    break;
                }

                AVFrame *pRGBAFrame = alloc_picture(m_DstFmt, m_nDstWidth, m_nDstHeight);
                if (pRGBAFrame == nullptr) {
                    LOGE(TAG, "申请帧空间RGBAFrame失败");
                    return RET_ERROR;
                }
//                LOGE(TAG, "申请帧空间RGBAFrame成功");
                //将YUV转为opengl能渲染的RGBA
                ret = sws_scale(pSwsCxt, pFrame->data, pFrame->linesize, 0, pFrame->height,
                                pRGBAFrame->data, pRGBAFrame->linesize);
//                LOGE(TAG, "%d 帧转换成功：slice 高度：%d", index, ret);
                frames.push_back(pRGBAFrame);
            }
        }
        av_packet_unref(pPacket);
    }

    av_packet_unref(pPacket);
    av_packet_free(&pPacket);
    pPacket = nullptr;

    av_frame_free(&pFrame);
    pFrame = nullptr;

    sws_freeContext(pSwsCxt);
    pSwsCxt = nullptr;

    int size = frames.size();
    //将生产的Frame放入队列
    std::unique_lock<std::mutex> lck(mtx);
    for (int i = size - 1; i >= 0; --i) {
        while (rgbFrames.size() == MAX_CACHE_FRAMES) {
            //等待有空间
            produce.wait(lck);
        }
        rgbFrames.push(frames[i]);
//        LOGE(TAG, "生产Frame 目前队列中的帧数：%d", rgbFrames.size());
        //通知消费
        consume.notify_all();
    }
    //解锁
    lck.unlock();
    return RET_SUCCESS;
}

int Player::consumeFrame() {
    //开始消费渲染
    while (true) {
        std::unique_lock<std::mutex> lck(mtx);
        while (rgbFrames.empty() && !isEndDecode) {
            LOGE(TAG, "消耗线程休眠");
            consume.wait(lck);
        }
        if (!rgbFrames.empty()) {
//            LOGE(TAG, "消耗Frame 目前队列中的帧数：%d", rgbFrames.size());
            AVFrame *pRGBFrame = rgbFrames.front();
            last = pRGBFrame->pts;
            rgbFrames.pop();

            opengl->draw(pRGBFrame->data[0], m_nDstWidth, m_nDstHeight, egl->eglDisplay,
                         egl->eglSurface);

            av_frame_free(&pRGBFrame);
            produce.notify_all();
        }
        lck.unlock();
        if (rgbFrames.empty() && isEndDecode) {
            break;
        }
//        usleep(30000);
    }

    LOGE(TAG, "消费渲染结束");
    return 0;
}

int Player::produceFrame() {
    //打开视频，初始化ffmpeg
    int ret = RET_ERROR;
    ret = open(path);
    if (ret < 0) {
        LOGE(TAG, "open视频打开失败");
        return ERROR;
    }
    //获取I帧的pts
    std::vector<int64_t> iFramePts = reverseDecode();
    if (iFramePts.empty()) {
        return RET_ERROR;
    }

    //测试
//    std::vector<int64_t> iFramePts;
//    iFramePts.push_back(0);

    //开始循环生产Frame
    int size = iFramePts.size();

    for (int i = size - 1; i >= 0; i--) {
        LOGE(TAG, "开始播放地%d帧关键帧, pts%lld", i, iFramePts[i]);
        ret = playByIFrame(iFramePts[i], last);
        if (ret < 0) {
            LOGE(TAG, "playByIFrame播放失败");
            break;
        }
    }
    iFramePts.clear();

    //结束线程
    std::unique_lock<std::mutex> lck(mtx);
    isEndDecode = true;
    consume.notify_all();
    lck.unlock();

    //释放资源
    if (c != nullptr) {
        avformat_close_input(&c);
        avformat_free_context(c);
        c = nullptr;
    }
    if (codecCxt != nullptr) {
        avcodec_free_context(&codecCxt);
        codecCxt = nullptr;
    }
    LOGE(TAG, "生产线程结束");
    return NULL;
}

//void *Player::produceFrame() {
//    //打开视频，初始化ffmpeg
//   /* ret = open(path);
//    if (ret < 0) {
//        LOGE(TAG, "open视频打开失败");
//        return ERROR;
//    }*/
//    //获取I帧的pts
//  /*  vector<int64_t> iFramePts = reverseDecode();
//    if (iFramePts.empty()) {
//        return RET_ERROR;
//    }
//
//    //开始循环生产Frame
//    int size = iFramePts.size();
//    int64_t last = 0;
//    for (int i = size - 1; i >= 0; i--) {
//        LOGE(TAG, "开始播放地%d帧关键帧", i);
//        ret = playByIFrame(iFramePts[i], last);
//        if (ret < 0) {
//            LOGE(TAG, "playByIFrame播放失败");
//            break;
//        }
//        last = iFramePts[i];
//    }
//    iFramePts.clear();
//
//    //结束线程
//    unique_lock<mutex> lck(mtx);
//    isEndDecode = true;
//    consume.notify_all();
//    lck.unlock();
//    */
//
//    //释放资源
//    /*if (c != nullptr) {
//        avformat_close_input(&c);
//        avformat_free_context(c);
//        c = nullptr;
//    }
//    if (codecCxt != nullptr) {
//        avcodec_free_context(&codecCxt);
//        codecCxt = nullptr;
//    }*/
//    LOGE(TAG, "生产线程结束");
//    return NULL;
//}







