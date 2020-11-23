//
// Created by wangyuelin on 2020/11/14.
//


#include "RmAudio.h"
#include "../utils/FrameUtil.h"


static const int RET_ERROR = -1;
static const int RET_SUCCESS = 0;


RmAudio::RmAudio(const char *srcPath, const char *outPath) : srcPath(srcPath), outPath(outPath) {}

RmAudio::~RmAudio() {
    srcPath = nullptr;
    outPath = nullptr;
}

int RmAudio::startRmAudio() {
    int ret = RET_ERROR;
    ret = openInput();
    if (ret < 0) {
        LOGE(TAG, "输入文件打开失败");
        return RET_ERROR;
    }
    ret = openOutput();
    if (ret < 0) {
        LOGE(TAG, "输出文件打开失败");
        return RET_ERROR;
    }
//    ret = decode();
    if (ret < 0) {
        LOGE(TAG, "解码和编码过程有错误");
    }
    closeInput();
    closeOutput();

    return RET_SUCCESS;
}

int RmAudio::openInput() {
    int ret = -1;

    if (StringUtil::isEmpty(srcPath)) {
        LOGE(TAG, "要打开的视频文件的路劲为空");
        return RET_ERROR;
    }
    ret = avformat_open_input(&c, srcPath, NULL, NULL);
    if (ret) {
        LOGE(TAG, "视频文件：%s 打开失败:%s", srcPath, av_err2str(ret))
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
    av_dump_format(c, videoIndex, srcPath, 0);

    LOGE(TAG, "视频文件：%s 打开成功", srcPath);

    return RET_SUCCESS;
}

int RmAudio::openOutput() {
    int ret = RET_ERROR;
    const char *fileName = StringUtil::parseFileName(outPath);
    if (fileName == nullptr) {
        LOGE(TAG, "解析输出文件的名称错误");
        return RET_ERROR;
    }
    LOGE(TAG, "输出文件的名称为：%s", fileName);
    avformat_alloc_output_context2(&oc, nullptr, nullptr, fileName);
    if (!oc) {
        LOGE(TAG, "据输出文件名称生成 AVFormatContext 失败，尝试直接指定输出格式为：mpeg");
        avformat_alloc_output_context2(&oc, nullptr, "mpeg", fileName);
    }
    if (!oc) {
        LOGE(TAG, "直接指定文件的输出编码格式为：mpeg， 还是失败");
        return RET_ERROR;
    }
    //记录输出格式
    oFmt = oc->oformat;

    //是否有编码器
    if (oFmt->video_codec == AV_CODEC_ID_NONE) {
        LOGE(TAG, "没有视频编码器，直接退出")
        return RET_ERROR;
    }
    /* Some formats want stream headers to be separate. */
    if (oFmt->flags & AVFMT_GLOBALHEADER)
        oc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //找到编码器
    encoder = avcodec_find_encoder(oFmt->video_codec);
    if (encoder == nullptr) {
        LOGE(TAG, "avcodec_find_encoder 未找到编码器");
        return RET_ERROR;
    }
    //创建编码器上下文
    encoderCxt = avcodec_alloc_context3(encoder);
    if (encoderCxt == nullptr) {
        LOGE(TAG, "申请编码器上下文失败");
        return RET_ERROR;
    }
    //设置编码视频的相关属性
    //设置编码器id
    encoderCxt->codec_id = oFmt->video_codec;
    if (codecCxt == nullptr) {
        LOGE(TAG, "需要用到的解码器上下文为空");
        return RET_ERROR;
    }
    //设置输出的比特率

//    encoderCxt->bit_rate = 400000;
    //设置输出的宽和高
    encoderCxt->width = codecCxt->width;
    encoderCxt->height = codecCxt->height;
    //设置时间基
//    encoderCxt->time_base = codecCxt->time_base;
    encoderCxt->framerate = (AVRational) {25, 1};
    encoderCxt->time_base = (AVRational) {1, 25};

//    encoderCxt->gop_size = codecCxt->gop_size;
    encoderCxt->gop_size = 12;
    //设置像素格式
    encoderCxt->pix_fmt = codecCxt->pix_fmt;
    //以下两个设置暂时未知
    if (encoderCxt->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
//        encoderCxt->max_b_frames = codecCxt->max_b_frames;
        /* just for testing, we also add B-frames */
        encoderCxt->max_b_frames = 2;
    }
    if (encoderCxt->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
//        encoderCxt->mb_decision = codecCxt->mb_decision;
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        encoderCxt->mb_decision = 2;
    }



    //创建输出流
    ost = avformat_new_stream(oc, nullptr);
    if (ost == nullptr) {
        LOGE(TAG, "avformat_new_stream 输出流创建失败");
        return RET_ERROR;
    }
    ost->id = oc->nb_streams - 1;
    ost->time_base = (AVRational) {1, 25};

    //打开编码器
    ret = avcodec_open2(encoderCxt, encoder, NULL);
    if (ret < 0) {
        LOGE(TAG, "avcodec_open2 编码器打开失败：%s", av_err2str(ret));
        return RET_ERROR;
    }

    //将参数拷贝到复用器
    ret = avcodec_parameters_from_context(ost->codecpar, encoderCxt);
    if (ret < 0) {
        LOGE(TAG, "avcodec_parameters_from_context 拷贝参数到复用器失败");
        return RET_ERROR;
    }
    LOGE(TAG, "输出视频相关参数:");
    av_dump_format(oc, 0, outPath, 1);

    /* open the output file, if needed */
    if (!(oFmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, outPath, AVIO_FLAG_READ_WRITE);
        if (ret < 0) {
            LOGE(TAG, "Could not open '%s': %s\n", outPath, av_err2str(ret))
            return RET_ERROR;
        }
    }
    //写文件头
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        LOGE(TAG, "avformat_write_header 写文件头失败");
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

int RmAudio::decode(Encoder *encoder) {
    encoderCxt = encoder->enc;
    if (encoderCxt == nullptr) {
        LOGE(TAG, "编码器环境为空");
        return RET_ERROR;
    }
    //获取编码器的参数
    m_nDstHeight = encoderCxt->height;
    m_nDstWidth = encoderCxt->width;
    m_DstFmt = encoderCxt->pix_fmt;

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
    int nextPts = 0;
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


//                AVFrame *pRGBAFrame = alloc_picture(m_DstFmt, m_nDstWidth, m_nDstHeight);
//                if (pRGBAFrame == nullptr) {
//                    LOGE(TAG, "申请帧空间失败");
//                    return RET_ERROR;
//                }
////            //将YUV转为opengl能渲染的RGBA
//                ret = sws_scale(pSwsCxt, pFrame->data, pFrame->linesize, 0, pFrame->height,
//                                pRGBAFrame->data, pRGBAFrame->linesize);
//                LOGE(TAG, "%d 帧转换成功：slice 高度：%d", index, ret);

                //传递到编码器，编码
                pFrame->pts = nextPts;
                nextPts++;
                if (encoder != nullptr) {
                    ret = encoder->encode(pFrame);
                    if (ret < 0) {
                        LOGE(TAG, "编码失败");
                        return RET_ERROR;
                    }
                }


                av_frame_unref(pFrame);
                //释放存RGBA数据帧的空间
//                av_frame_free(&pRGBAFrame);
            }

            av_packet_unref(pPacket);
            index++;
        }

    }

    return RET_SUCCESS;

}

int RmAudio::encode(AVFrame *decodedFrame) {
    if (decodedFrame == nullptr || encoderCxt == nullptr) {
        LOGE(TAG, "传入的帧为空，或者编码器环境为空");
        return RET_ERROR;
    }
    int ret = RET_ERROR;
    // send the frame to the encoder
    ret = avcodec_send_frame(encoderCxt, decodedFrame);
    if (ret < 0) {
        LOGE(TAG, "Error sending a frame to the encoder: %s\n", av_err2str(ret));
        return RET_ERROR;
    }

    while (ret >= 0) {
        AVPacket pkt = {0};

        ret = avcodec_receive_packet(encoderCxt, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            LOGE(TAG, "Error encoding a frame: %s\n", av_err2str(ret));
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&pkt, codecCxt->time_base, ost->time_base);
        pkt.stream_index = ost->index;

        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(oc, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            LOGE(TAG, "Error while writing output packet: %s\n", av_err2str(ret))
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

void RmAudio::closeInput() {
    if (c != nullptr) {
        avformat_close_input(&c);
        avformat_free_context(c);
        c = nullptr;
    }
    if (codecCxt != nullptr) {
        avcodec_free_context(&codecCxt);
        codecCxt = nullptr;
    }
}

void RmAudio::closeOutput() {
    //写入文件尾
    av_write_trailer(oc);
    if (encoderCxt != nullptr) {
        avcodec_free_context(&encoderCxt);
        encoderCxt = nullptr;
    }
    if (!(oFmt->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_closep(&oc->pb);
    }
    /* free the stream */
    avformat_free_context(oc);
    oc = nullptr;
}

void RmAudio::testEncode() {
    int ret = RET_ERROR;
    ret = openInput();
    if (ret < 0) {
        LOGE(TAG, "输入文件打开失败");
        return;
    }

    Encoder *encoder = new Encoder(outPath, codecCxt->width, codecCxt->height);
    ret = encoder->open();
    if (ret < 0) {
        LOGE(TAG, "encoder打开失败");
        return;
    }
    /*for (int i = 0; i < 20; ++i) {
        AVFrame *pFrame = FrameUtil::alloc_picture(AV_PIX_FMT_YUV420P, codecCxt->width, codecCxt->height);
        FrameUtil::fill_yuv_image(pFrame, i, codecCxt->width, codecCxt->height);

        encoder->encode(pFrame);

    }*/
    decode(encoder);
    encoder->close();
    if (ret < 0) {
        LOGE(TAG, "解码和编码过程有错误");
    }
    closeInput();
}

void RmAudio::reverse() {
    int ret = RET_ERROR;
    ret = openInput();
    if (ret < 0) {
        LOGE(TAG, "输入文件打开失败");
        return;
    }
    AVPacket *pPacket = nullptr;
    AVFrame * pFrame = nullptr;
    pPacket = av_packet_alloc();
    pFrame = av_frame_alloc();
    int gopSize = codecCxt->gop_size;


    int64_t lastPts = c->duration;
    ret = av_seek_frame(c, videoIndex, lastPts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE(TAG, "av_seek_frame 到：%d 失败", lastPts);
        return ;
    }

    while (av_read_frame(c, pPacket) == 0) {
        //确保需要解码的是视频帧
        if (pPacket->stream_index == videoIndex) {
            ret = avcodec_send_packet(codecCxt, pPacket);
            if (ret < 0) {
                LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
                return;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codecCxt, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    LOGE(TAG, "直接开始下次的av_read_frame 和 avcodec_send_packet");
                    break;
                }

                if (ret < 0) {
                    LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                    return ;
                }
                av_frame_unref(pFrame);
            }
            av_packet_unref(pPacket);
        }

    }

}
