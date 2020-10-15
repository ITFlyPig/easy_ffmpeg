package com.wyl.ffmpegtest.permission;

/**
 * 创建人   ：yuelinwang
 * 创建时间 ：2020/5/20
 * 描述     ：权限监听接口
 */
public interface PermissionListener {
    void onResult(String[] permission, boolean[] grantedResults);
}
