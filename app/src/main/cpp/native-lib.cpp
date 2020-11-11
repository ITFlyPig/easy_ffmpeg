#include <jni.h>
#include <string>
#include <android/log.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <locale>
//#include <thread>
#include <unistd.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/bitmap.h>
#include <thread>

#include "log/log.h"
#include "image.h"

#include "GLRender.h"
#include "egl/egl_core.h"
//#include "video/video.h"
#include "player/player.h"

#define STREAM_DURATION   10.0
#define SCALE_FLAGS SWS_BICUBIC
#define ERROR -1;


AVFrame *yuv_frame = NULL;
int w = 0;
int h = 0;


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

typedef struct OpenInfo {
    AVFormatContext *c = nullptr;
    int video_index;
    AVCodec *codec = nullptr;
    AVCodecContext *codec_ctx = nullptr;

} OpenInfo;


#define STREAM_FRAME_RATE 1 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define DATASIZE 2048*2048


AVFrame *parse_image(char *img_path);

//添加一个video stream和初始化编码器codec
void add_stream(OutputStream *ost, AVFormatContext *oc,
                AVCodec **codec,
                enum AVCodecID codec_id) {
    AVCodecContext *c;
    int i;

    //据id找到编码器
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        LOGE(TAG, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        exit(1);
    }

    //新加一个流
    ost->st = avformat_new_stream(oc, NULL);

    if (!ost->st) {
        LOGE(TAG, "Could not allocate stream");
        exit(1);
    }
    //设置流的id
    ost->st->id = oc->nb_streams - 1;
    //创建编码器上下文
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOGE(TAG, "Could not alloc an encoding context");
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
    ost->st->time_base = (AVRational) {1, STREAM_FRAME_RATE};//设置帧率
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
        LOGE(TAG, "Could not allocate frame data");
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
        LOGE(TAG, "Could not open video codec: %s\n", av_err2str(ret));
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
            LOGE(TAG, "Could not allocate temporary picture");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        LOGE(TAG, "Could not copy the stream parameters");
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


int index = 1;
const char *img_dir = "/sdcard/video_img/rank_title_bg%i.png";

/*获取输入的图片的地址*/
void get_img_path(char *img_path) {
    if (img_path == nullptr) {
        return;
    }
    if (index < 1) {
        index = 5;
    } else if (index > 5) {
        index = 1;
    }
    sprintf(img_path, img_dir, index);
    index++;
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


    //申请内存，存放图片的地址
    char *img_path = static_cast<char *>(malloc(1024));
    get_img_path(img_path);
    LOGE(TAG, "将要解析的图片的地址：%s", img_path);
    ost->frame = parse_image(img_path);
    //释放内存
    free(img_path);
    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}


/*打印包信息*/
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGE(TAG,
         "pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
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
        LOGE(TAG, "Error sending a frame to the encoder: %s\n", av_err2str(ret));
        exit(1);
    }

    while (ret >= 0) {
        AVPacket pkt = {0};

        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            LOGE(TAG, "Error encoding a frame: %s\n", av_err2str(ret));
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

    av_log_set_callback(FFLog::log_callback_android);

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
        LOGE(TAG, "avformat_alloc_output_context2 失败，使用MPEG格式");
        avformat_alloc_output_context2(&oc, fmt, "mpeg", filename);
    }
    if (!oc) {
        LOGE(TAG, "avformat_alloc_output_context2 失败，使用MPEG格式也失败");
        return;
    }

    //使用格式上下文初始化输出格式
    fmt = oc->oformat;

    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        LOGE(TAG, "输出有编码器，创建stream和初始化编码器codec");
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
        ret = avio_open(&oc->pb, output_file_path, AVIO_FLAG_READ_WRITE);
        if (ret < 0) {
            LOGE(TAG, "Could not open '%s': %s\n", filename,
                 av_err2str(ret));
            exit(1);
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        LOGE(TAG, "Error occurred when opening output file: %s\n",
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
        LOGE(TAG, "文件打开失败：%s", strerror(errno));
        return;
    }
    fwrite(frame->data[0], 1, frame->linesize[0] * frame->height, fd);
    fclose(fd);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_decodeImages(JNIEnv *env, jobject thiz) {
    av_log_set_callback(FFLog::log_callback_android);

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
        LOGE(TAG, "文件打开失败：%s", av_err2str(ret));
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
        LOGE(TAG, "未找到视频流");
        return;
    }
    //据id找找到解码器
    LOGE(TAG, "解码器ID：%i  %i", dec_codec_id, AV_CODEC_ID_PNG);
    dec_codec = avcodec_find_decoder(dec_codec_id);


    if (dec_codec == NULL) {
        LOGE(TAG, "未找到解码器");
        return;
    }
    //创建解码上下文，初始化解码器默认值
    dec_codec_ctx = avcodec_alloc_context3(dec_codec);
    if (dec_codec_ctx == NULL) {
        LOGE(TAG, "解码器上下文创建失败");
        return;
    }

    //使用支持的解码器参数填充 解码器上下文
    ret = avcodec_parameters_to_context(dec_codec_ctx, c->streams[video_index]->codecpar);
    if (ret < 0) {
        LOGE(TAG, "Failed to copy decoder parameters to input decoder context：%s", av_err2str(ret));
        goto __end;
    }

    LOGE(TAG, "解码器的名字：%s, 像素格式：%i", dec_codec->name, dec_codec_ctx->pix_fmt);

    //打开解码器:Initialize the AVCodecContext to use the given AVCodec.
    ret = avcodec_open2(dec_codec_ctx, dec_codec, NULL);
    if (ret < 0) {
        LOGE(TAG, "Failed to open decoder for stream");
        goto __end;
    }
    w = dec_codec_ctx->width;
    h = dec_codec_ctx->height;

    //打印信息
    av_dump_format(c, video_index, in_img_path, 0);

    //申请Packet容器
    packet = av_packet_alloc();
    if (packet == NULL) {
        LOGE(TAG, "申请packet失败");
        goto __end;
    }

    frame = av_frame_alloc();
    if (frame == NULL) {
        LOGE(TAG, "申请packet失败");
        goto __end1;
    }
    av_init_packet(packet);

    sws_cxt = sws_getContext(dec_codec_ctx->width, dec_codec_ctx->height, dec_codec_ctx->pix_fmt,
                             dec_codec_ctx->width, dec_codec_ctx->height, AV_PIX_FMT_YUV420P,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (sws_cxt == NULL) {
        LOGE(TAG, "获取SwsContext失败");
        goto __end2;
    }

    while (av_read_frame(c, packet) == 0) {
        LOGE(TAG, "packet size:%5d", packet->size);
        int got_frame = -1;
        int size = avcodec_decode_video2(dec_codec_ctx, frame, &got_frame, packet);
        LOGE(TAG, "解码的字节数：%d, linesize[0]:%d", size, frame->linesize[0]);
        if (size < 0) {
            LOGE(TAG, "解码Packet错误：%s", av_err2str(size));
            goto __end2;
        }

        yuv_frame = alloc_picture(AV_PIX_FMT_YUV420P, frame->width, frame->height);
        if (yuv_frame == NULL) {
            LOGE(TAG, "申请转换后的yuv存储Frame失败");
            goto __end2;
        }
        //将rgb转为yuv420p
        ret = sws_scale(sws_cxt, frame->data, frame->linesize, 0, frame->height, yuv_frame->data,
                        yuv_frame->linesize);
        LOGE(TAG, "转换后的slice height为：%i", ret);

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

bool is_empty(const char *s) {
    return s == nullptr || *s == '\0';
}

/*打开文件，创建流，读取文件信息*/
int open_stream(char *path, OpenInfo *info) {
    int ret = -1;
    if (StringUtil::isEmpty(path)) {
        LOGE(TAG, "传入的文件路劲为空");
        return ret;
    }

    AVFormatContext *c = nullptr;

    /*创建流，读取文件头，流必须使用avformat_close_input关闭*/
    ret = avformat_open_input(&c, path, NULL, NULL);
    if (ret < 0) {
        LOGE(TAG, "文件打开失败：%s", av_err2str(ret));
        return ret;
    }
    //通过读取packets获得流相关的信息，这个方法对于没有header的媒体文件特别有用，读取到的packets会被缓存后续使用。
    ret = avformat_find_stream_info(c, NULL);
    if (ret < 0) {
        LOGE(TAG, " avformat_find_stream_info 获取流信息失败");
        return ret;
    }

    //通过流获取解码器相关
    int video_index = -1;
    for (int i = 0; i < c->nb_streams; ++i) {
        if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //找到视频流的索引
            video_index = i;
        }
    }
    if (video_index == -1) {
        LOGE(TAG, "未找到视频流");
    }
    info->video_index = video_index;
    info->c = c;

    return ret;
}

/*找到和打开解码器*/
int find_and_open_decoder(OpenInfo *info) {
    AVCodec *codec = nullptr;
    AVCodecContext *codec_cxt = nullptr;
    int ret = -1;

    //据id找找到解码器
    AVCodecParameters *codecpar = info->c->streams[info->video_index]->codecpar;
    AVCodecID codec_id = codecpar->codec_id;
    LOGE(TAG, "找到解码器ID：%i， AV_CODEC_ID_PNG解码器的id： %i", codec_id, AV_CODEC_ID_PNG);
    codec = avcodec_find_decoder(codec_id);
    if (codec == nullptr) {
        LOGE(TAG, "未找到解码器");
        return ret;
    }
    //创建解码上下文，初始化解码器默认值，必须使用avcodec_free_context释放
    codec_cxt = avcodec_alloc_context3(codec);
    if (codec_cxt == NULL) {
        LOGE(TAG, "解码器上下文创建失败");
        return ret;
    }
    //使用支持的解码器参数 填充 解码器上下文
    ret = avcodec_parameters_to_context(codec_cxt, codecpar);
    if (ret < 0) {
        LOGE(TAG, "Failed to copy decoder parameters to input decoder context：%s", av_err2str(ret));
        return ret;
    }

    LOGE(TAG, "解码器的名字：%s, 媒体文件像素格式：%i", codec->name, codec_cxt->pix_fmt);
    //打开解码器:Initialize the AVCodecContext to use the given AVCodec.
    ret = avcodec_open2(codec_cxt, codec, nullptr);
    if (ret < 0) {
        LOGE(TAG, "Failed to open decoder for stream");
    }
    info->codec = codec;
    info->codec_ctx = codec_cxt;
    return ret;
}

/*解码，得到yuv格式的数据*/
AVFrame *decode_image(OpenInfo *info) {

    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *yuv_frame = nullptr;
    SwsContext *sws_cxt = nullptr;
    int ret = -1;

    AVPixelFormat out_pix_fmt = AV_PIX_FMT_YUV420P;
    AVCodecContext *codec_ctx = info->codec_ctx;

    //申请Packet容器
    packet = av_packet_alloc();
    if (packet == nullptr) {
        LOGE(TAG, "申请packet失败");
        return nullptr;
    }
    av_init_packet(packet);

    frame = av_frame_alloc();
    if (frame == nullptr) {
        LOGE(TAG, "申请packet失败");
        goto end;
    }

    sws_cxt = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                             codec_ctx->width, codec_ctx->height, out_pix_fmt,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (sws_cxt == nullptr) {
        LOGE(TAG, "获取SwsContext失败");
        goto end;
    }

    while (av_read_frame(info->c, packet) == 0) {
        LOGE(TAG, "读取到的packet size:%5d", packet->size);
        int got_frame = -1;
        int size = avcodec_decode_video2(codec_ctx, frame, &got_frame, packet);
        LOGE(TAG, "解码得到的的字节数：%d, linesize[0]:%d", size, frame->linesize[0]);
        if (size < 0) {
            LOGE(TAG, "解码Packet错误：%s", av_err2str(size));
            goto end;
        }

        yuv_frame = alloc_picture(out_pix_fmt, frame->width, frame->height);
        if (yuv_frame == NULL) {
            LOGE(TAG, "申请转换后的yuv存储Frame失败");
            goto end;
        }
        //将rgb转为yuv420p
        ret = sws_scale(sws_cxt, frame->data, frame->linesize, 0, frame->height, yuv_frame->data,
                        yuv_frame->linesize);
        LOGE(TAG, "转换的slice height为：%i", ret);
    }

    end:
    if (packet != nullptr) {
        av_packet_unref(packet);
    }
    if (frame != nullptr) {
        av_frame_free(&frame);
    }
    if (sws_cxt != nullptr) {
        sws_freeContext(sws_cxt);
    }
    return yuv_frame;

}

/*解析输入的图片，得到一帧Frame*/
AVFrame *parse_image(char *img_path) {
    AVFrame *yuv_frame = nullptr;
    if (StringUtil::isEmpty(img_path)) {
        return nullptr;
    }
    OpenInfo open_info;

    if (open_stream(img_path, &open_info) < 0) {
        goto end;
    }

    if (find_and_open_decoder(&open_info) < 0) {
        goto end;
    }
    //打印正在解析的多媒体的信息
    av_dump_format(open_info.c, open_info.video_index, img_path, 0);

    yuv_frame = decode_image(&open_info);
    if (yuv_frame == nullptr) {
        goto end;
    }

    end:
//    if (yuv_frame != nullptr) {
//        av_frame_free(&yuv_frame);
//    }
    if (open_info.c != nullptr) {
        avformat_close_input(&open_info.c);
    }
    if (open_info.codec_ctx != nullptr) {
        avcodec_free_context(&open_info.codec_ctx);
    }

    return yuv_frame;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_testParse(JNIEnv *env, jobject thiz) {
    AVFrame *frame;
    frame = parse_image("sdcard/img_video_1.png");
    if (frame == nullptr) {
        LOGE(TAG, "parse_image 返回的Frame为空");
    } else {
        LOGE(TAG, "parse_image 返回的Frame为width:%i, height:%i", frame->width, frame->height);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_parseImageInfo(JNIEnv *env, jobject thiz, jstring jpath) {
    char *path = const_cast<char *>(env->GetStringUTFChars(jpath, JNI_FALSE));

    Image image;
    int ret = image.prase(path);
    if (ret < 0) {
        LOGE(TAG, "解析图片失败");
    } else {
        LOGE(TAG, "解析到的图片的宽：%i, 高：%i", image.w, image.h);
    }

    env->ReleaseStringUTFChars(jpath, path);
}


JavaVM *javaVM;


void renderBitmap(std::shared_ptr<EglCore> egl, jobject surface, void *pixel, int w, int h) {
    /* while (true) {

         usleep(2000);
     }*/
    int ret = RET_ERROR;
    JNIEnv *env = nullptr;
    if (javaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE(TAG, "AttachCurrentThread 错误");
        return;
    }
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        LOGE(TAG, "ANativeWindow创建失败");
        return;
    }

    ret = egl->OpenEGL(nativeWindow);
    if (!ret) {
        LOGE(TAG, "OpenEGL失败");
        return;
    }
    /*开始使用opengl*/
    GLRender *cglRender = new GLRender();
    /*创建opengl程序*/
    cglRender->CreateProgram();
    //创建和绑定纹理单元
    cglRender->CreateAndBindTexture();
    //绑定bitmap
    cglRender->BindBitmap(pixel, w, h);
    //绘制
    cglRender->Draw();
    //显示
    egl->SwapBuffers();

    delete cglRender;

}

void startBitmapRenderThread(jobject surface) {
    //新建一个线程，在线程里完成Egl和OpenGL的初始化和绘制
//    std::shared_ptr<EglCore> that(new EglCore());
//    std::thread t(renderBitmap, that, surface);
//    t.join();
}

//打开解码器
void openCodec() {


}

//打开EGL
void openEgl() {

}

//打开opengl
void openGL() {

}


///////视频播放三部曲/////////
void open() {


}

void draw() {
    //解码
    //渲染
}

void cloase() {
    //关闭资源
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_renderBitmap(JNIEnv *env, jobject thiz, jobject sf,
                                                  jobject bitmap) {
//    startBitmapRenderThread(sf);
    env->GetJavaVM(&javaVM);
    std::shared_ptr<EglCore> that(new EglCore());
    AndroidBitmapInfo bitmapInfo;
    void *pixel = nullptr;
    int ret = 0;
    ret = AndroidBitmap_getInfo(env, bitmap, &bitmapInfo);
    if (ret < 0) {
        LOGE(TAG, "获取Bitmap信息失败");
        return;
    }
    if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE(TAG, "无效的Bitmap格式");
        return;
    }
    ret = AndroidBitmap_lockPixels(env, bitmap, &pixel);
    if (ret < 0) {
        LOGE(TAG, "获取Bitmap像素数据失败");
        return;
    }

    renderBitmap(that, sf, pixel, bitmapInfo.width, bitmapInfo.height);

    AndroidBitmap_unlockPixels(env, bitmap);
}
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_wyl_ffmpegtest_MainActivity_renderVideo(JNIEnv *env, jobject thiz, jstring path,  jint width, jint height) {
//    av_log_set_callback(FFLog::log_callback_android);
//    char *videoPath = const_cast<char *>(env->GetStringUTFChars(path, JNI_FALSE));
//    int ret = RET_ERROR;
//    Video *video = new Video(videoPath);
//    ret = video->Open();
//    if (ret < 0) {
//        LOGE(TAG, "视频:%s  打开失败", videoPath);
//        return;
//    }
//    GLRender glRender;
//    video->Play(glRender);
////    delete video;
//
//}


char *name = "wangyuelin";
extern "C"
JNIEXPORT jlong JNICALL
Java_com_wyl_ffmpegtest_MainActivity_getAddr(JNIEnv *env, jobject thiz) {
    LOGE(TAG, "长度：%d", strlen(name));
    return (long) name;
}


extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_wyl_ffmpegtest_MainActivity_getStringByteArr(JNIEnv *env, jobject thiz, jlong addr,
                                                      jint len) {
    unsigned char *data = reinterpret_cast<unsigned char *>(addr);
    jbyteArray jarr = env->NewByteArray(len);
    env->SetByteArrayRegion(jarr, 0, len, reinterpret_cast<const jbyte *>(data));
    return jarr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_renderVideo(JNIEnv *env, jobject thiz, jstring path,
                                                 jint width, jint height, jobject surface) {
    av_log_set_callback(FFLog::log_callback_android);

//    JavaVM *javaVM;
//    env->GetJavaVM(&javaVM);
//    JNIEnv *newEnv = nullptr;
//    if (javaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
//        LOGE(TAG, "AttachCurrentThread 错误");
//        return;
//    }
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        LOGE(TAG, "ANativeWindow创建失败");
        return;
    }

    char *videoPath = const_cast<char *>(env->GetStringUTFChars(path, JNI_FALSE));
    int ret = RET_ERROR;
//    Video *video = new Video(videoPath, width, height, AV_PIX_FMT_RGBA);
//    ret = video->Open();
    if (ret < 0) {
        LOGE(TAG, "视频:%s  打开失败", videoPath);
        return;
    }
    EglCore *eglCore = new EglCore();
    ret = eglCore->OpenEGL(nativeWindow);
    if (!ret) {
        LOGE(TAG, "OpenEGL 失败");
        return;
    }

    /*开始使用opengl*/
    GLRender *cglRender = new GLRender();
    /*创建opengl程序*/
    cglRender->CreateProgram();
    //创建和绑定纹理单元
    cglRender->CreateAndBindTexture();

//    video->Play(cglRender, eglCore);
//    delete video;

}


extern "C"
JNIEXPORT void JNICALL
Java_com_wyl_ffmpegtest_MainActivity_openVideo(JNIEnv *env, jobject thiz, jstring path) {
    jboolean isCopy;
     char *videoPath = const_cast<char *>(env->GetStringUTFChars(path, &isCopy));
    Player* player = new Player();
    player->newThread(videoPath);

}