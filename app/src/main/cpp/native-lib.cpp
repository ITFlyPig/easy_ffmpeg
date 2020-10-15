#include <jni.h>
#include <string>
#include <android/log.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>


extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}

#define STREAM_DURATION   10.0
#define SCALE_FLAGS SWS_BICUBIC

// log标签
#define  TAG    "ffmpegtest"
// 定义info信息
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
// 定义debug信息
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
// 定义error信息
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)


AVFrame *yuv_frame = NULL;
int w = 0;
int h = 0;


void log_callback_test2(void *ptr, int level, const char *fmt, va_list vl) {
    va_list vl2;
    char *line = static_cast<char *>(malloc(128 * sizeof(char)));
    static int print_prefix = 1;
    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, 128, &print_prefix);
    va_end(vl2);
    line[127] = '\0';
    LOGE("%s", line);
    free(line);
}


// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;


#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define DATASIZE 2048*2048


//添加一个video stream和初始化编码器codec
void add_stream(OutputStream *ost, AVFormatContext *oc,
                AVCodec **codec,
                enum AVCodecID codec_id) {
    AVCodecContext *c;
    int i;

    //据id找到编码器
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        LOGE("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        exit(1);
    }

    //新加一个流
    ost->st = avformat_new_stream(oc, NULL);

    if (!ost->st) {
        LOGE("Could not allocate stream");
        exit(1);
    }
    //设置流的id
    ost->st->id = oc->nb_streams - 1;
    //创建编码器上下文
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOGE("Could not alloc an encoding context");
        exit(1);
    }
    //将编码器上下文记录下来
    ost->enc = c;

    //编码器上下文设置
    c->codec_id = codec_id;
    c->bit_rate = 400000;
    /* Resolution must be a multiple of two. */
    c->width = 352;
    c->height = 288;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    ost->st->time_base = (AVRational) {1, STREAM_FRAME_RATE};
    c->time_base = ost->st->time_base;

    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

//申请帧
AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
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
        LOGE("Could not allocate frame data");
        exit(1);
    }

    return picture;
}


//打开编码器，申请编码需要的缓存
void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    //打开编码器
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        LOGE("Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* allocate and init a re-usable frame */
    //申请和初始化可重用的frame
//    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
//    if (!ost->frame) {
//        LOGE("Could not allocate video frame");
//        exit(1);
//    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            LOGE("Could not allocate temporary picture");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        LOGE("Could not copy the stream parameters");
        exit(1);
    }
}

/* Prepare a dummy image. 这里可以看出YUV420P数据是怎么存储的 */
void fill_yuv_image(AVFrame *pict, int frame_index,
                    int width, int height) {
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

//得到一帧数据
AVFrame *get_video_frame(OutputStream *ost) {
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames，TODO:将时间转换进行分析 */
    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational) {1, 1}) > 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
//
//    if (av_frame_make_writable(ost->frame) < 0)
//        exit(1);
//
//    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
//        /* as we only generate a YUV420P picture, we must convert it
//         * to the codec pixel format if needed */
//        if (!ost->sws_ctx) {
//            ost->sws_ctx = sws_getContext(c->width, c->height,
//                                          AV_PIX_FMT_YUV420P,
//                                          c->width, c->height,
//                                          c->pix_fmt,
//                                          SCALE_FLAGS, NULL, NULL, NULL);
//            if (!ost->sws_ctx) {
//                fprintf(stderr,
//                        "Could not initialize the conversion context\n");
//                exit(1);
//            }
//        }
//        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
//        sws_scale(ost->sws_ctx, (const uint8_t *const *) ost->tmp_frame->data,
//                  ost->tmp_frame->linesize,
//                  0, c->height, ost->frame->data, ost->frame->linesize);
//    } else {
//        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
//    }
    ost->frame = yuv_frame;

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*打印包信息*/
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGE("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
         pkt->stream_index);
}

int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                AVStream *st, AVFrame *frame) {
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        LOGE("Error sending a frame to the encoder: %s\n", av_err2str(ret));
        exit(1);
    }

    while (ret >= 0) {
        AVPacket pkt = {0};

        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            LOGE("Error encoding a frame: %s\n", av_err2str(ret));
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
        pkt.stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, &pkt);
        ret = av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}


/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int write_video_frame(AVFormatContext *oc, OutputStream *ost) {
    return write_frame(oc, ost->enc, ost->st, get_video_frame(ost));
}

void close_stream(AVFormatContext *oc, OutputStream *ost) {
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
}

//主要功能就是YUV数据 -> 编码 -> 压缩 -> MP4文件
extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_makeMediaFile(JNIEnv *env, jobject thiz) {

    av_log_set_callback(log_callback_test2);

    //文件输出路劲
    const char *output_file_path = "/data/user/0/com.wyl.ffmpegtest/files/test_media.mp4";
    const char *filename = "test_media.mp4";

    //格式上下文
    AVFormatContext *oc = NULL;
    //输出格式
    AVOutputFormat *fmt = NULL;
    AVCodec *video_codec = NULL;
    AVDictionary *opt = NULL;
    int have_video = 0;
    int encode_video = 0;
    int ret;

    OutputStream video_st = {0};

    avformat_alloc_output_context2(&oc, fmt, NULL, filename);
    if (!oc) {
        LOGE("avformat_alloc_output_context2 失败，使用MPEG格式");
        avformat_alloc_output_context2(&oc, fmt, "mpeg", filename);
    }
    if (!oc) {
        LOGE("avformat_alloc_output_context2 失败，使用MPEG格式也失败");
        return;
    }

    //使用格式上下文初始化输出格式
    fmt = oc->oformat;

    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        LOGE("输出有编码器，创建stream和初始化编码器codec");
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (have_video) {
        open_video(oc, video_codec, &video_st, opt);
    }

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, output_file_path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s\n", filename,
                 av_err2str(ret));
            exit(1);
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        LOGE("Error occurred when opening output file: %s\n",
             av_err2str(ret));
        exit(1);
    }

    //开始写入内容
    while (encode_video) {
        encode_video = !write_video_frame(oc, &video_st);
    }
    /* Write the trailer, if any. The trailer must be written before you
        * close the CodecContexts open when you wrote the header; otherwise
        * av_write_trailer() may try to use memory that was freed on
        * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video)
        close_stream(oc, &video_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);

}


void save_one_frame(AVFrame *frame) {
    const char *out_file = "/data/user/0/com.wyl.ffmpegtest/files/test.rgb";
    FILE *fd = fopen(out_file, "wb+");
    if (fd == NULL) {
        LOGE("文件打开失败：%s", strerror(errno));
        return;
    }
    fwrite(frame->data[0], 1, frame->linesize[0] * frame->height, fd);
    fclose(fd);
}



extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_decodeImages(JNIEnv *env, jobject thiz) {
    av_log_set_callback(log_callback_test2);

//    av_parser_parse2();直接将数据解析为frame

    const char *in_img_path = NULL;
    AVFormatContext *c = NULL;
    int ret = -1;
    int video_index = -1;
    AVCodecID dec_codec_id = AV_CODEC_ID_NONE;
    AVCodec *dec_codec = NULL;
    AVCodecContext *dec_codec_ctx = NULL;
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    SwsContext *sws_cxt = NULL;


    in_img_path = "sdcard/img_video_1.png";
    //打开文件，创建一个流，读取文件头
    ret = avformat_open_input(&c, in_img_path, NULL, NULL);
    if (ret < 0) {
        LOGE("文件打开失败：%s", av_err2str(ret));
        return;
    }
    //通过读取packets获得流相关的信息，这个方法对于没有header的媒体文件特别有用，读取到的packets会被缓存后续使用。
    ret = avformat_find_stream_info(c, NULL);
    if (ret < 0) {
        return;
    }

    //通过流获取解码器相关
    for (int i = 0; i < c->nb_streams; ++i) {
        if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //找到视频流的索引
            video_index = i;
            //记录解码器的id
            dec_codec_id = c->streams[i]->codecpar->codec_id;
        }
    }
    if (video_index == -1) {
        LOGE("未找到视频流");
        return;
    }
    //据id找找到解码器
    LOGE("解码器ID：%i  %i", dec_codec_id, AV_CODEC_ID_PNG);
    dec_codec = avcodec_find_decoder(dec_codec_id);


    if (dec_codec == NULL) {
        LOGE("未找到解码器");
        return;
    }
    //创建解码上下文，初始化解码器默认值
    dec_codec_ctx = avcodec_alloc_context3(dec_codec);
    if (dec_codec_ctx == NULL) {
        LOGE("解码器上下文创建失败");
        return;
    }

    //使用支持的解码器参数填充 解码器上下文
    ret = avcodec_parameters_to_context(dec_codec_ctx, c->streams[video_index]->codecpar);
    if (ret < 0) {
        LOGE("Failed to copy decoder parameters to input decoder context：%s", av_err2str(ret));
        goto __end;
    }

    LOGE("解码器的名字：%s, 像素格式：%i", dec_codec->name, dec_codec_ctx->pix_fmt);

    //打开解码器:Initialize the AVCodecContext to use the given AVCodec.
    ret = avcodec_open2(dec_codec_ctx, dec_codec, NULL);
    if (ret < 0) {
        LOGE("Failed to open decoder for stream");
        goto __end;
    }
    w = dec_codec_ctx->width;
    h = dec_codec_ctx->height;

    //打印信息
    av_dump_format(c, video_index, in_img_path, 0);

    //申请Packet容器
    packet = av_packet_alloc();
    if (packet == NULL) {
        LOGE("申请packet失败");
        goto __end;
    }

    frame = av_frame_alloc();
    if (frame == NULL) {
        LOGE("申请packet失败");
        goto __end1;
    }
    av_init_packet(packet);

    sws_cxt = sws_getContext(dec_codec_ctx->width, dec_codec_ctx->height, dec_codec_ctx->pix_fmt,
                             dec_codec_ctx->width, dec_codec_ctx->height, AV_PIX_FMT_YUV420P,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (sws_cxt == NULL) {
        LOGE("获取SwsContext失败");
        goto __end2;
    }

    while (av_read_frame(c, packet) == 0) {
        LOGE("packet size:%5d", packet->size);
        int got_frame = -1;
        int size = avcodec_decode_video2(dec_codec_ctx, frame, &got_frame, packet);
        LOGE("解码的字节数：%d, linesize[0]:%d", size, frame->linesize[0]);
        if (size < 0) {
            LOGE("解码Packet错误：%s", av_err2str(size));
            goto __end2;
        }

        yuv_frame = alloc_picture(AV_PIX_FMT_YUV420P, frame->width, frame->height);
        if (yuv_frame == NULL) {
            LOGE("申请转换后的yuv存储Frame失败");
            goto __end2;
        }
        //将rgb转为yuv420p
        ret = sws_scale(sws_cxt, frame->data, frame->linesize, 0, frame->height, yuv_frame->data, yuv_frame->linesize);
        LOGE("转换后的slice height为：%i", ret);

        //生成视频
        Java_com_wyl_ffmpegtest_MainActivity_makeMediaFile(NULL, NULL);


//        save_one_frame(frame);

//        ret = avcodec_send_packet(dec_codec_ctx, packet);
//        if (ret < 0) {
//            LOGE("Error sending a packet for decoding");
//            goto __end2;
//        }
//
//        while (ret >= 0) {
//            ret = avcodec_receive_frame(dec_codec_ctx, frame);
//            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//                LOGE("结束读取packet：%s", av_err2str(ret));
//
//            }else if (ret < 0) {
//                LOGE("Error during decoding");
//                goto __end2;
//            }
//
//            //打印读取到的数据信息
//            LOGE("frame width%i, height:%i ", frame->width, frame->height);
//            LOGE("linesize0：%i", frame->linesize[0]);
//            LOGE("linesize1：%i", frame->linesize[1]);
//            LOGE("linesize2：%i", frame->linesize[2]);
//            LOGE("linesize3：%i", frame->linesize[3]);
//
//        }
    }
    __end3:
    sws_freeContext(sws_cxt);
    __end2:
    av_frame_free(&frame);
    __end1:
    av_packet_free(&packet);
    __end:
    avcodec_free_context(&dec_codec_ctx);

}
