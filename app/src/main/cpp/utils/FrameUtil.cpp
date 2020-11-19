//
// Created by wangyuelin on 2020/11/19.
//

#include "FrameUtil.h"

void FrameUtil::fill_yuv_image(AVFrame *frame, int frame_index, int width, int height) {
    if (frame == nullptr || frame_index < 0 || width <= 0 || height <= 0) {
        LOGE(TAG, "fill_yuv_image 参数错误");
        return;
    }
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
            frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
        }
    }

}

AVFrame *FrameUtil::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
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
