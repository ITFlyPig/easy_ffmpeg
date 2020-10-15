package com.wyl.ffmpegtest.permission;

import androidx.fragment.app.FragmentActivity;

/**
 * 创建人   ：yuelinwang
 * 创建时间 ：2020/5/20
 * 描述     ：权限申请帮助类
 */
public class PermissionRequestUtil {

    /**
     * 请求权限
     *
     * @param activity
     * @param listener
     * @param permissions
     */
    public static void request(FragmentActivity activity, PermissionListener listener, String... permissions) {
        if (activity == null || permissions == null) {
            return;
        }
        RequestWrapper requestWrapper = new RequestWrapper(activity, listener, permissions);
        requestWrapper.request();
    }

}
