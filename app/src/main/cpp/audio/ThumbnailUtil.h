//
// Created by wangyuelin on 2020/11/30.
//

#ifndef FFMPEGTEST_THUMBNAILUTIL_H
#define FFMPEGTEST_THUMBNAILUTIL_H

#include "log.h"
#include <mutex>


/*
 * 获取视频缩略图的工具类
 *
 * //一开始初始化200个Frame，然后就固定使用这20个Frame来做解码缓存
 * 1s内不用缓存全部，缓存10帧就可以了
 * 可以缓存当前事件前后分别100帧，也就是10s的时间
 */
class ThumbnailUtil {
private:
    static const int MAX_CACHED_NUM = 21;
    int ptsStep = 1;//隔1个pts缓存一帧
    AVFrame *cachedFrames[MAX_CACHED_NUM];
    //当前需要的帧的pts


    std::mutex mtx;
    std::condition_variable adjustCond;
    std::condition_variable getCond;



public:
    int64_t curPts = MAX_CACHED_NUM / 2;//默认初始值
    AVFormatContext *c;
    AVCodecContext *codecCxt;
    int streamIndex;



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

     int getFrame(AVFormatContext *c, AVCodecContext *codecCxt, int64_t pts, int streamIndex, AVFrame **pFrame);
    //调整缓存
    void adjustCache();

    /**
     * 尝试直接获取
     * @param pts
     * @return
     */
    AVFrame *tryGetFrame(int64_t pts);
    AVFrame *getFrame(int64_t pts);


    /**
     * 等待调整
     * @return
     */
    int waitAdjust();


};

#endif //FFMPEGTEST_THUMBNAILUTIL_H
