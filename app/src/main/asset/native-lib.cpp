#include <jni.h>
#include <string>
#include <android/log.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <libswresample/swresample.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include <libavutil/imgutils.h>
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_wyl_ffmpegtest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = avcodec_configuration();
    return env->NewStringUTF(hello.c_str());
}


//extern "C"
//JNIEXPORT void JNICALL
//Java_com_wyl_ffmpegtest_MainActivity_rgb2Yuv(JNIEnv *env, jobject thiz, jstring image_path,
//                                             jint src_w, jint src_h, jstring yuv_path) {
//    SwsContext *sws_ctx = NULL;
//    //输入的数据结构
//    AVFormatContext *fmt_cxt = NULL;
//    AVCodec *codec = NULL;
//    AVCodecContext *codec_cxt = NULL;
//    AVPacket packet;
//    AVFrame *frame = NULL;
//
//    //输出的数据结构
//    AVFrame *out_frame = NULL;
//    uint8_t *out_buff;
//    AVCodec *out_codec = NULL;
//    AVCodecID out_codec_id = AV_CODEC_ID_H264;
//    AVCodecContext *out_codec_cxt = NULL;
//    AVPacket out_packet;
//    AVFormatContext *out_fmt_cxt = NULL;
//    FILE *out_file = NULL;
//    char *err_buff = new char[1024]();
//
//
//    int ret = 0;
//    //png的存储方式是argb
//    AVPixelFormat dst_fmt = AV_PIX_FMT_YUV420P;
//    const char *src_image_path = env->GetStringUTFChars(image_path, 0);
//    /*
//    int ret = 0;
//    int src_img_w = src_w;
//    int src_img_h = src_h;
//    const char *src_image_path = env->GetStringUTFChars(image_path, 0);
//
//    //开始读入png像素数据
//    FILE *f_src = fopen(src_image_path, "r");
//    if (f_src == NULL) {
//        LOGE("文件%s打开失败", src_image_path);
//        return;
//    }
//    //计算出输入图片的字节大小
//    int src_img_size = src_img_h * src_img_h * 32 / 8;
//    LOGE("输入的图像的内存大小%i", src_img_size);
//    uint8_t *src_img_buf = static_cast<uint8_t *>(malloc(src_img_size));
//    //开始读取数据
//    ret = fread(src_img_buf, 1, src_img_size, f_src);
//    LOGE("读取到的字节数%i", ret);
//
//
//    sws_ctx = sws_getContext(src_img_w, src_img_h, src_fmt,
//    src_img_w, src_img_h, dst_fmt,
//                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
//    if (sws_ctx == NULL) {
//        LOGE("sws_getContext 构造失败");
//        return;
//    } else {
//        LOGE("sws_getContext 构造成功");
//    }
//     */
//    out_file = fopen("sdcard/编码后的视频.mp4", "wb");
//
//    if (avformat_open_input(&fmt_cxt, src_image_path, NULL, NULL) != 0) {
//        LOGE("avformat_open_input注册失败");
//        return;
//    }
//    if (avformat_find_stream_info(fmt_cxt, NULL) < 0) {
//        LOGE("avformat_find_stream_info 错误");
//        return;
//    }
//    int video_stream_index = -1;
//    AVCodecID codec_id = AV_CODEC_ID_NONE;
//    for (int i = 0; i < fmt_cxt->nb_streams; ++i) {
//        if (fmt_cxt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//            video_stream_index = i;
//            codec_id = fmt_cxt->streams[i]->codecpar->codec_id;
//            LOGE("找到视频的索引：%i， png解码器的值：%i", i, AV_CODEC_ID_PNG);
//        }
//    }
//    if (video_stream_index == -1) {
//        LOGE("未找到视频的索引");
//        goto __fail;
//    }
//
//    //找到解码器
//    LOGE("解码器id%i", codec_id);
//    codec = avcodec_find_decoder(codec_id);
//    if (codec == NULL) {
//        LOGE("未找到解码器");
//        return;
//    }
//    out_codec = avcodec_find_decoder(out_codec_id);
//    if (out_codec == NULL) {
//        LOGE("输出编码器未找到");
//        goto __fail;
//    }
//
//    codec_cxt = fmt_cxt->streams[video_stream_index]->codec;
//    if (codec_cxt == NULL) {
//        LOGE("avcodec_alloc_context3 创建解码器上下文失败");
//        goto __fail;
//    }
//    out_codec_cxt = avcodec_alloc_context3(out_codec);
//    if (out_codec_cxt == NULL) {
//        LOGE("输出的AVCodecContext创建失败");
//        goto __fail;
//    }
//    //打开解码器
//    if (avcodec_open2(codec_cxt, codec, NULL) < 0) {
//        LOGE("avcodec_open2 解码器打开失败");
//        goto __fail;
//    }
//    LOGE("打开解码器成功，输入的宽：%i，高：%i", codec_cxt->width, codec_cxt->height);
//
//    //设置输出视频的参数
////    out_codec_cxt->bit_rate = 400000;
//    /* resolution must be a multiple of two */
//    out_codec_cxt->width = codec_cxt->width;
//    out_codec_cxt->height = codec_cxt->height;
//    /* frames per second */
//    out_codec_cxt->time_base = (AVRational) {1, 25};
//    out_codec_cxt->framerate = (AVRational) {25, 1};
//
//    /* emit one intra frame every ten frames
//     * check frame pict_type before passing frame
//     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
//     * then gop_size is ignored and the output of encoder
//     * will always be I frame irrespective to gop_size
//     */
//    out_codec_cxt->gop_size = 10;
//    out_codec_cxt->max_b_frames = 1;
//    out_codec_cxt->pix_fmt = dst_fmt;
//
//
//
//    //打开编码器
//    if (avcodec_open2(out_codec_cxt, out_codec, NULL) < 0) {
//        LOGE("avcodec_open2 编码器打开失败");
//        goto __fail;
//    }
//
//    //准备读取数据
//    frame = av_frame_alloc();
//    if (frame == NULL) {
//        LOGE("AVFrame 内存申请失败");
//        goto __fail;
//
//    }
//    out_frame = av_frame_alloc();
//    if (out_frame == NULL) {
//        LOGE("申请out_frame失败");
//        goto __fail;
//    }
//
//    out_frame->format = dst_fmt;
//    out_frame->width = codec_cxt->width;
//    out_frame->height = codec_cxt->height;
//    LOGE("宽%i, 高%i, 像素格式:%i", codec_cxt->width, codec_cxt->height, dst_fmt);
//    ret = av_frame_get_buffer(out_frame, 0);
//    if (ret < 0) {
//        av_strerror(ret, err_buff, 1024);
//        LOGE("Could not allocate the out video frame data:%s", err_buff);
//        goto __fail;
//    }
//
////    out_buff = (uint8_t *) av_malloc(
////            av_image_get_buffer_size(dst_fmt, codec_cxt->width, codec_cxt->height, 1));
////    ret = av_image_fill_arrays(out_frame->data, out_frame->linesize, out_buff, dst_fmt,
////                               codec_cxt->width, codec_cxt->height, 1);
//    if (ret < 0) {
//        av_strerror(ret, err_buff, 1024);
//        LOGE("av_image_fill_arrays失败:%s", err_buff);
//        goto __fail;
//    }
//
//    sws_ctx = sws_getContext(codec_cxt->width, codec_cxt->height, codec_cxt->pix_fmt,
//                             codec_cxt->width, codec_cxt->height, dst_fmt,
//                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
//    if (sws_ctx == NULL) {
//        LOGE("sws_getContext 失败");
//        goto __fail;
//    }
//
//    //开始读取数据
//    LOGE("开始读取数据并处理");
//    av_init_packet(&packet);
//    while (av_read_frame(fmt_cxt, &packet) >= 0) {
//        if (packet.stream_index == video_stream_index) {
//            LOGE("读取到视频Frame");
//            ret = avcodec_send_packet(codec_cxt, &packet);
//            if (ret < 0) {
//                LOGE("Error sending a packet for decoding");
//                goto __fail;
//            }
//            while (ret >= 0) {
//                ret = avcodec_receive_frame(codec_cxt, frame);
//                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//                    LOGE("AVERROR(EAGAIN) or AVERROR_EOF");
//                    break;
//                } else if (ret < 0) {
//                    LOGE("Error during decoding");
//                    goto __fail;
//                }
//                LOGE("开始处理frame");
//                //将ARGB的数据转为YUV
//                sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_cxt->height,
//                          out_frame->data, out_frame->linesize);
//                out_frame->pts = 0;
//                //将变换后的Frame保存到文件
//                ret = avcodec_send_frame(out_codec_cxt, out_frame);
//                if (ret < 0) {
//                    LOGE("Error sending a frame for encoding:%s", av_err2str(ret));
//                    goto __fail;
//                }
//
//                while (ret >= 0) {
//                    ret = avcodec_receive_packet(out_codec_cxt, &out_packet);
//                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//                        LOGE("AVERROR(EAGAIN) or AVERROR_EOF");
//                        goto __fail;
//                    } else if (ret < 0) {
//                        LOGE("Error during encoding");
//                        goto __fail;
//                    }
//
//                    LOGE("Write packet ");
//                    fwrite(out_packet.data, 1, out_packet.size, out_file);
//                    av_packet_unref(&out_packet);
//                }
//
//
//            }
//
//        }
//        av_packet_unref(&packet);
//    }
//
//    __fail:
//    av_free(&out_buff);
//    sws_freeContext(sws_ctx);
//    av_frame_free(&frame);
//    avcodec_close(codec_cxt);
//    avcodec_close(out_codec_cxt);
//    avformat_close_input(&fmt_cxt);
////    sws_freeContext(sws_ctx);
//
//}
//
//extern "C"
//AVStream *add_vidio_stream(AVFormatContext *oc,
//                           enum AVCodecID codec_id) // AVFormatContext to initialize a structure for output
//{
//    AVStream *st;
//    AVCodec *codec;
//
//    st = avformat_new_stream(oc, NULL);
//    if (!st) {
//        printf("Could not alloc stream\n");
//        exit(1);
//    }
//    codec = avcodec_find_encoder(codec_id); // Find mjpeg decoder
//    if (!codec) {
//        printf("codec not found\n");
//        exit(1);
//    }
//    // Application AVStream-> codec (AVCodecContext object) space and set the default value (by the avcodec_get_context_defaults3 () provided
//    avcodec_get_context_defaults3(st->codec, codec);
//
//    st->codec->bit_rate = 400000; // set the sampling parameters, i.e., the bit rate
//    st->codec->width = 1332; // Set the video width and height, here is consistent with the width and height of the image can be saved
//    st->codec->height = 620;
//    st->codec->time_base.den = 25; // set the frame rate
//    st->codec->time_base.num = 1;
//
//    st->codec->pix_fmt = AV_PIX_FMT_YUV420P; // Pixel Format
//    st->codec->codec_tag = 0;
////    if (oc->oformat->flags &
////        AVFMT_GLOBALHEADER) // number of data header format requires a separate video stream
////        st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
//    return st;
//}
//
//
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_wyl_ffmpegtest_MainActivity_test22(JNIEnv *env, jobject thiz) {
//    AVFormatContext *ofmt_ctx = NULL; // stream comprising many parameters, a data structure is always through many function should be used as a parameter
//    const char *out_filename = "/sdcard/out.mp4"; // file output path, where mkv may be changed to another format supported ffmpeg, such as mp4, flv, avi like
//    int ret; // return flag
//
//    av_register_all(); // initialize a decoder and a multiplexer
//    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL,
//                                   out_filename);// initialize a structure for output, video frame rate, and aspect in which this function is provided
//    if (!ofmt_ctx) {
//        printf("Could not create output context\n");
//        return;
//    }
//
//    // AVStream * out_stream = add_vidio_stream (ofmt_ctx, AV_CODEC_ID_MJPEG); // create the output video stream
//    AVStream *out_stream = add_vidio_stream(ofmt_ctx,
//                                            AV_CODEC_ID_PNG); // create the output video stream (third format parameter points to the picture)
//    // This function will print out the information in the video stream, if not happy can not look at
//    av_dump_format(ofmt_ctx, 0, out_filename, 1);
//
//    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) // open the output video file
//    {
//        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            printf("Could not open output file '%s'", out_filename);
//            return;
//        }
//    }
//
//    if (avformat_write_header(ofmt_ctx, NULL) < 0) // write the file header (Write file header)
//    {
//        printf("Error occurred when opening output file\n");
//        return;
//    }
//
//    int frame_index = 0; // count into the video image
//    unsigned char *mydata = new unsigned char[DATASIZE];
//    AVPacket pkt;
//    av_init_packet(&pkt);
//    pkt.flags |= AV_PKT_FLAG_KEY;
//    pkt.stream_index = out_stream->index; // Get video information to prepare for the press-frame image
//
//    char buf[250] = {0};
//    while (frame_index < 200) // pressed into the video image
//    {
////        printf(buf, 255, "1_00%03d.png", frame_index);//1_00000
////
////        std::string dir = "/sdcard/";
//        std::string filePath = "/sdcard/dogntain.jpeg";
//        // open a jpeg image and read its data, where the image up to 1M, if more than 1M, you need to modify 1024 * 1024 here
//        FILE *file;
//        file = fopen(filePath.c_str(), "rb");
//        pkt.size = fread(mydata, 1, DATASIZE, file);
//        pkt.data = mydata;
//
//        fclose(file);
//        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) // writes the image to a video
//        {
//            printf("Error muxing packet\n");
//            break;
//        }
//        // print out the current number of frames pressed
//        LOGE("Write% 8d frames to output file \ n", frame_index);
//        frame_index++;
//    }
//
//
//    av_free_packet(&pkt); // freed frame data packet object
//    av_write_trailer(ofmt_ctx); // write end of the file (Write file trailer)
//    delete[] mydata; // release data object
//    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
//        avio_close(ofmt_ctx->pb); // close the video file
//    avformat_free_context(ofmt_ctx); // release output video data structure
//    return;
//}


//添加一个video stream和初始化编码器codec
extern "C"
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
extern "C"
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
extern "C"
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
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        LOGE("Could not allocate video frame");
        exit(1);
    }

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
extern "C"
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
extern "C"
AVFrame *get_video_frame(OutputStream *ost) {
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames，TODO:将时间转换进行分析 */
    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational) {1, 1}) > 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t *const *) ost->tmp_frame->data,
                  ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                  ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*打印包信息*/
extern "C"
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGE("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
         pkt->stream_index);
}

extern "C"
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
extern "C"
int write_video_frame(AVFormatContext *oc, OutputStream *ost) {
    return write_frame(oc, ost->enc, ost->st, get_video_frame(ost));
}

extern "C"
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
    const char *output_file_path = "/sdcard/test_media.mp4";
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