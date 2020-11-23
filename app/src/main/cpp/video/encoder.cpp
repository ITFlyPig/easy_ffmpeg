//
// Created by wangyuelin on 2020/11/19.
//
#include "encoder.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 0;

int Encoder::open() {
    int ret = RET_ERROR;
    const char *fileName = StringUtil::parseFileName(outPath);
    avformat_alloc_output_context2(&oc, fmt, NULL, fileName);
    if (!oc) {
        LOGE(TAG, "avformat_alloc_output_context2 失败，使用MPEG格式");
        avformat_alloc_output_context2(&oc, fmt, "mpeg", fileName);
        return RET_ERROR;
    }
    if (!oc) {
        LOGE(TAG, "avformat_alloc_output_context2 失败，使用MPEG格式也失败");
        return RET_ERROR;
    }

    //使用格式上下文初始化输出格式
    fmt = oc->oformat;
    if (fmt->video_codec == AV_CODEC_ID_NONE) {
        LOGE(TAG, "avformat_alloc_output_context2 未找到合适的编码器ID");
        return RET_ERROR;
    }


    LOGE(TAG, "输出有编码器，创建stream和初始化编码器codec");
//        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
    //据id找到编码器
    video_codec = avcodec_find_encoder(fmt->video_codec);
    if (!(video_codec)) {
        LOGE(TAG, "Could not find encoder for '%s'\n", avcodec_get_name(fmt->video_codec));
        return RET_ERROR;
    }

    //新加一个流
    st = avformat_new_stream(oc, NULL);

    if (!st) {
        LOGE(TAG, "Could not allocate stream");
        return RET_ERROR;
    }
    //设置流的id
    st->id = oc->nb_streams - 1;
    //创建编码器上下文
    enc = avcodec_alloc_context3(video_codec);
    if (!enc) {
        LOGE(TAG, "Could not alloc an encoding context");
        return RET_ERROR;
    }

    //编码器上下文设置
    enc->codec_id = fmt->video_codec;
    /* Resolution must be a multiple of two. */
    enc->width = this->width;
    enc->height = this->height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    st->time_base = (AVRational) {1, STREAM_FRAME_RATE};//设置帧率
    enc->time_base = (AVRational) {1, STREAM_FRAME_RATE};

    enc->gop_size = 12; /* emit one intra frame every twelve frames at most */
    enc->pix_fmt = STREAM_PIX_FMT;
    if (enc->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        enc->max_b_frames = 2;
    }
    if (enc->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        enc->mb_decision = 2;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //打开编码器
    ret = avcodec_open2(enc, video_codec, nullptr);
    if (ret < 0) {
        LOGE(TAG, "Could not open video pVideoCodec: %s\n", av_err2str(ret));
        return RET_ERROR;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(st->codecpar, enc);
    if (ret < 0) {
        LOGE(TAG, "Could not copy the stream parameters");
        return RET_ERROR;
    }

    av_dump_format(oc, 0, fileName, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, outPath, AVIO_FLAG_READ_WRITE);
        if (ret < 0) {
            LOGE(TAG, "Could not open '%s': %s\n", fileName,
                 av_err2str(ret));
            return RET_ERROR;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        LOGE(TAG, "Error occurred when opening output file: %s\n",
             av_err2str(ret));
        return RET_ERROR;
    }
    return RET_SUCCESS;

}

int Encoder::encode(AVFrame *frame) {
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(enc, frame);
    if (ret < 0) {
        LOGE(TAG, "Error sending a frame to the encoder: %s\n", av_err2str(ret));
        return RET_ERROR;
    }

    while (ret >= 0) {
        AVPacket pkt = {0};

        ret = avcodec_receive_packet(enc, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            LOGE(TAG, "Error encoding a frame: %s\n", av_err2str(ret));
        }

        /* rescale output packet timestamp values from pVideoCodec to stream timebase */
        av_packet_rescale_ts(&pkt, enc->time_base, st->time_base);
        pkt.stream_index = st->index;

        ret = av_interleaved_write_frame(oc, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
            return RET_ERROR;
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

void Encoder::close() {
    av_write_trailer(oc);
    /* Close each pVideoCodec. */
    avcodec_free_context(&enc);


    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);
    /* free the stream */
    avformat_free_context(oc);

}

Encoder::Encoder(const char *outPath, int width, int height) : outPath(outPath), width(width),
                                                               height(height) {}

