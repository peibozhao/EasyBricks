package com.example.easybricks;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;

import java.io.File;
import java.io.FileOutputStream;

public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        dir = Environment.getExternalStorageDirectory().getAbsolutePath() + "/EasyBricks/";
        // Request read/write storage permissions
        requestPermissions(new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                100);
        File dir_file = new File(dir);
        if (!dir_file.exists() && !dir_file.mkdir()) {
            Log.e("MainActivity", "mkdir failed");
        }

        new Thread() {
            @Override
            public void run() {
                Log.i("MainActivity", "Thread start");
                while (true) {
                    ConsumeImage();
                }
            }
        }.start();
    }

    public void onScreenCaptureButton(View view) {
        Log.i("MainActivity", "onToogle");
        if (!started) {
            Intent mp_intent = ((MediaProjectionManager)getSystemService(MEDIA_PROJECTION_SERVICE)).createScreenCaptureIntent();
            lan.launch(mp_intent);
        } else {
            Intent screencap_intent = new Intent(this, ScreenCaptureService.class);
            stopService(screencap_intent);
            started = false;
        }
    }

    public void onClickButton(View view) {
        Log.i("MainActivity", "onClickButton");
        try {
            Thread.sleep(3000);
        } catch (Exception e) {

        }
        GlobalData.gesture_lock.lock();
        GlobalData.click_x = 500;
        GlobalData.click_y = 500;
        GlobalData.gesture_con.signal();
        GlobalData.gesture_lock.unlock();
    }

    private void ConsumeImage() {
        try {
            Thread.sleep(1000);

            GlobalData.image_lock.lock();
            while (GlobalData.image == null) {
                GlobalData.image_con.await();
            }
            Log.i("MainActivity", "Image new");
            Bitmap img = GlobalData.image;
            GlobalData.image = null;
            GlobalData.image_lock.unlock();

            File file = new File(dir + "latest.jpg");
            FileOutputStream out = new FileOutputStream(file);
            img.compress(Bitmap.CompressFormat.JPEG, 10, out);
            out.close();
        } catch (Exception e) {
            Log.e("MainActivity", "Exception: " + e.toString());
        }
    }

    private String dir;
    private boolean started = false;
    // Request MEDIA_PROJECTION_SERVICE. Start screen capture service
    ActivityResultLauncher<Intent> lan = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
        Log.i("MainActivity", "registerForActivityResult callback");
        if (result.getResultCode() != Activity.RESULT_OK) {
            Log.e("MainActivity", "Permission");
            return;
        }

        // Get media projection permission. Start custom screen capture service
        Intent screencap_intent = new Intent(this, ScreenCaptureService.class);
        screencap_intent.putExtra("code", result.getResultCode());
        screencap_intent.putExtra("data", result.getData());
        startService(screencap_intent);
        started = true;
    });
}