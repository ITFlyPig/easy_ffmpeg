//
// Created by wangyuelin on 2020/10/21.
//


#include "video.h"

char *Video::TAG = "Video";

int Video::Open() {
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

    return 0;
}

int Video::Play(GLRender* glRender, EglCore *eglCore) {
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
                    LOGE(TAG, "申请帧空间失败");
                    return RET_ERROR;
                }
//            //将YUV转为opengl能渲染的RGBA
                ret = sws_scale(pSwsCxt, pFrame->data, pFrame->linesize, 0, pFrame->height,
                                pRGBAFrame->data, pRGBAFrame->linesize);
                LOGE(TAG, "%d 帧转换成功：slice 高度：%d", index, ret);

                glRender->BindBitmap(pRGBAFrame->data, pRGBAFrame->width, pRGBAFrame->height);
                glRender->Draw();
                eglCore->SwapBuffers();


                av_frame_unref(pFrame);
                //释放存RGBA数据帧的空间
                av_frame_free(&pRGBAFrame);
            }

            av_packet_unref(pPacket);
            index++;
        }

    }

    return 0;
}

Video::Video(char *path, int width, int height, AVPixelFormat dstFmt) : path(path),
                                                                        m_nDstWidth(width),
                                                                        m_nDstHeight(height),
                                                                        m_DstFmt(dstFmt) {}

Video::~Video() {
    if (codecCxt != nullptr) {
        avcodec_free_context(&codecCxt);
    }
    if (c != nullptr) {
        avformat_free_context(c);
    }

}

int Video::Close() {
    return 0;
}

AVFrame *Video::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
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
