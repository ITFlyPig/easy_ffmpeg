package com.wyl.ffmpegtest.permission;


import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

/**
 * 创建人   ：yuelinwang
 * 创建时间 ：2020/5/21
 * 描述     ：使用Fragment实现权限请求
 */
public class RequestFragment extends Fragment {
    private RequestFragmentListener listener;

    public void setListener(RequestFragmentListener listener) {
        this.listener = listener;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (listener != null) {
            listener.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    public interface RequestFragmentListener {
        void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults);
    }

}
