//
// Created by wangyuelin on 2020/11/30.
//

#ifndef FFMPEGTEST_THUMBNAILUTIL_H
#define FFMPEGTEST_THUMBNAILUTIL_H

#include "log.h"

/*
 * 获取视频缩略图的工具类
 */
class ThumbnailUtil {
public:
    /**
     * 获取指定时间(pts)的视频帧
     * @param codecCxt  解码上下文
     * @param pts       目标时间戳
     * @param streamIndex 流索引
     * @param pFrame    返回帧数据
     * @param c         格式上下文
     * @return
     */
    static int fetchFrame(AVFormatContext *c, AVCodecContext *codecCxt, int64_t pts, int streamIndex, AVFrame **pFrame);

};

#endif //FFMPEGTEST_THUMBNAILUTIL_H
