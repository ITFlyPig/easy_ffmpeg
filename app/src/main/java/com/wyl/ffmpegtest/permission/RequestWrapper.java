package com.wyl.ffmpegtest.permission;

import android.content.pm.PackageManager;
import android.os.Build;

import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;

import java.lang.ref.WeakReference;

/**
 * 创建人   ：yuelinwang
 * 创建时间 ：2020/5/21
 * 描述     ：实际权限请求的封装
 */
public class RequestWrapper {
    private WeakReference<FragmentActivity> mFragmentActivityRef;
    private PermissionListener mListener;
    private String[] permissions;
    private static final int REQUEST_PERMISSION_CODE = 100;
    private static final String FRAGMENT_TAG = "request_fragment";

    public RequestWrapper(FragmentActivity activity, PermissionListener mListener, String[] permissions) {
        this.mFragmentActivityRef = new WeakReference<>(activity);
        this.mListener = mListener;
        this.permissions = permissions;
    }

    private boolean[] getResults(boolean isGranted) {
        int len = permissions.length;
        boolean[] results = new boolean[len];
        for (int i = 0; i < len; i++) {
            results[i] = isGranted;
        }
        return results;
    }

    /**
     * 请求权限
     */
    public void request() {
        //6.0以下，直接返回授权
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M && mListener != null) {
            mListener.onResult(permissions, getResults(true));
            return;
        }
        //6.0及以上动态申请
        final RequestFragment fragment = addFragment();
        if (fragment == null) {
            return;
        }
        if (permissions == null) {
            return;
        }
        fragment.setListener((requestCode, permissions, grantResults) -> {
            if (requestCode == REQUEST_PERMISSION_CODE) {
                //将数字转为布尔
                boolean[] results = new boolean[permissions.length];
                int len = permissions.length;
                int resultLen = grantResults.length;
                for (int i = 0; i < len; i++) {
                    //多校验一下
                    if (i < resultLen) {
                        results[i] = PackageManager.PERMISSION_GRANTED == grantResults[i];
                    }
                }
                mListener.onResult(permissions, results);
            }
            //移除Fragment
            fragment.setListener(null);
            delFragment();
        });
        fragment.requestPermissions(permissions, REQUEST_PERMISSION_CODE);
    }

    /**
     * 获取activity
     *
     * @return
     */
    private FragmentActivity getActivity() {
        FragmentActivity activity = mFragmentActivityRef.get();
        //安全校验
        if (activity == null || activity.isFinishing() || activity.isDestroyed()) {
            if (mListener != null) {
                mListener.onResult(permissions, getResults(false));
            }
            return null;
        }
        return activity;
    }

    /**
     * 添加Fragment
     *
     * @return
     */
    private RequestFragment addFragment() {
        FragmentActivity activity = getActivity();
        if (activity == null) {
            return null;
        }
        //添加Fragment
        FragmentManager fm = activity.getSupportFragmentManager();
        RequestFragment requestFragment = (RequestFragment) fm.findFragmentByTag(FRAGMENT_TAG);
        if (requestFragment == null) {
            requestFragment = new RequestFragment();
            fm.beginTransaction().add(requestFragment, FRAGMENT_TAG).commitNowAllowingStateLoss();
        }
        return requestFragment;
    }

    /**
     * 删除Fragment
     */
    private void delFragment() {
        FragmentActivity activity = getActivity();
        if (activity == null) {
            return;
        }
        FragmentManager fm = activity.getSupportFragmentManager();
        RequestFragment requestFragment = (RequestFragment) fm.findFragmentByTag(FRAGMENT_TAG);
        if (requestFragment != null) {
            fm.beginTransaction().remove(requestFragment).commitAllowingStateLoss();
        }
    }

}
