package com.wyl.ffmpegtest;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.wyl.ffmpegtest.permission.PermissionListener;
import com.wyl.ffmpegtest.permission.PermissionRequestUtil;

import java.io.File;
import java.io.IOException;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public static final String TAG = MainActivity.class.getName();


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
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
                        makeMediaFile();

                    }
                }, Manifest.permission.READ_EXTERNAL_STORAGE);



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
}
