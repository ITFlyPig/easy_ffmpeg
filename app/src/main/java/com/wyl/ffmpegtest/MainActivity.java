package com.wyl.ffmpegtest;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.wyl.ffmpegtest.permission.PermissionListener;
import com.wyl.ffmpegtest.permission.PermissionRequestUtil;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public static final String TAG = MainActivity.class.getName();

    private TextView tvOpenGlEGLShow;
    private SurfaceView surfaceView;
    private Surface surface;
    private SurfaceView videoSurfaceView;
    private Surface videoSurface;
    private TextView tvRenderVideo;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        surfaceView = findViewById(R.id.surface);
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                surface = holder.getSurface();

            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PermissionRequestUtil.request(MainActivity.this, new PermissionListener() {
                    @Override
                    public void onResult(String[] permission, boolean[] grantedResults) {
//                        String path = "sdcard/img_video_1.png";
//                        BitmapFactory.Options options = new BitmapFactory.Options();
//                        options.inJustDecodeBounds = true;//这个参数设置为true才有效，
//                        BitmapFactory.decodeFile(path, options);
//                        Log.e(TAG, "onResult: 获取到的图片的宽和高：" + options.outWidth + ":" + options.outHeight);

//                        rgb2Yuv("sdcard/img_video_1.png", options.outWidth, options.outHeight, null);

//                        test22();
//                        makeMediaFile();
//                        Log.e("tag", getBaseContext().getFilesDir().getAbsolutePath());


                        String destFile = getFilesDir().getAbsolutePath() + "/test_media.mp4";
                        File file = new File(destFile);
                        Log.e(TAG, "onResult: " + destFile);
                        if (!file.exists()) {
                            try {
                                file.createNewFile();
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }

//                        decodeImages();
//                        testParse();
//                        makeMediaFile();
                        parseImageInfo("/sdcard/video_img/rank_title_bg1.png");

                    }
                }, Manifest.permission.READ_EXTERNAL_STORAGE);


            }
        });

        tvOpenGlEGLShow = findViewById(R.id.tv_opengl_egl_show);
        tvOpenGlEGLShow.setOnClickListener(this);
        tvRenderVideo = findViewById(R.id.tv_render_video);
        tvRenderVideo.setOnClickListener(this);
        videoSurfaceView = findViewById(R.id.video_surface);
        videoSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                videoSurface = holder.getSurface();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();
//
//    public native void rgb2Yuv(String imagePath, int srcW, int srcH, String yuvPath);
//
//    public native void test22();
    public native void makeMediaFile();

    public native void decodeImages();

    public native void testParse();

    public native void parseImageInfo(String path);

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.tv_opengl_egl_show:
                if (surface == null) {
                    Toast.makeText(this, "surface为空", Toast.LENGTH_SHORT).show();
                    return;
                }
                //解析得到Bitmap
                Bitmap bitmap = BitmapFactory.decodeResource(getResources(), R.mipmap.bg_english_ability_test);
                renderBitmap(surface, bitmap);
                break;
            case R.id.tv_render_video:
                PermissionRequestUtil.request(MainActivity.this, new PermissionListener() {
                    @Override
                    public void onResult(String[] permission, boolean[] grantedResults) {
                        if (grantedResults.length == 2 && grantedResults[0] && grantedResults[1]) {
                            int width = videoSurfaceView.getWidth();
                            int height = videoSurfaceView.getHeight();
//                renderVideo("/sdcard/mvtest.mp4", width, height, videoSurface);
                            //渲染视频
                            openVideo("/sdcard/mvtest.mp4", videoSurface, width, height);
//                            String targetPath = getFilesDir() + File.separator +  "mvtest22.mp4";
//                            File targetFile = new File(targetPath);
//                            if (!targetFile.exists()) {
//                                try {
//                                    if (!targetFile.createNewFile()) {
//                                        Log.e(TAG, "onClick: 新文件创建失败，退出：" + targetPath);
//                                        return;
//                                    }
//                                } catch (IOException e) {
//                                    e.printStackTrace();
//                                }
//                            }
////                            rmAudio("/sdcard/mvtest.mp4", targetPath);
//                            String targetPath = "/sdcard/encode_video.mp4";
//                            if (makeFile(targetPath)) {
//                                testEncodeVideo(targetPath);
//                            }

//                            String targetPath = "/sdcard/make_video.mp4";
//                            String name = "make_video.mp4";
//                            if (makeFile(targetPath)) {
//                                makeVideo(targetPath, name);
//                            }

                        } else {
                            Toast.makeText(MainActivity.this, "权限被拒绝", Toast.LENGTH_SHORT).show();
                        }

                    }
                }, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE);

                break;
            default:
                break;
        }
    }

    private boolean makeFile(String path) {
        File targetFile = new File(path);
        if (!targetFile.exists()) {
            try {
                if (!targetFile.createNewFile()) {
                    Log.e(TAG, "onClick: 新文件创建失败，退出：" + path);
                    return false;
                }
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        }
        return true;
    }


    public native void renderBitmap(Surface surface, Bitmap bitmap);

    public native void renderVideo(String path, int width, int height, Surface surface);

    public native long getAddr();

    public native byte[] getStringByteArr(long addr, int len);

    public native void openVideo(String path, Surface surface, int width, int height);

    public native void rmAudio(String srcPath, String outPath);

    public native void testEncodeVideo(String outPath);

    public native void makeVideo(String path, String name);

    public native void testAudioPlay(String path);
}
