package com.example.easybricks;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;

import com.google.android.material.textfield.TextInputEditText;

import java.io.File;
import java.io.FileOutputStream;

import EasyBricks.EasyBricks;
import EasyBricks.PlayOperation;

public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void onScreenCaptureButton(View view) {
        Log.i("MainActivity", "onScreenCaptureButton");
        // Create directory and file
        requestPermissions(new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                100);

        dir = Environment.getExternalStorageDirectory().getAbsolutePath() + "/EasyBricks/";
        File dir_file = new File(dir);
        if (!dir_file.exists() && !dir_file.mkdir()) {
            Log.e("MainActivity", "mkdir failed");
            return;
        }
        AssetCopy.AssetToSD(getAssets(), "config.yaml", dir.toString() + "config.yaml");
        AssetCopy.AssetToSD(getAssets(), "player", dir.toString());

        // Initialize EasyBricks SDK
        System.loadLibrary("easy_bricks");
        easy_bricks = new EasyBricks(dir.toString() + "config.yaml");
        if (!easy_bricks.Init()) {
            Log.e("MainActivity", "EasyBricks init failed.");
            return;
        }

        // Create consume thread to process screen capture
        new Thread() {
            @Override
            public void run() {
                Log.i("MainActivity", "Thread start");
                while (true) {
                    ConsumeImage();
                }
            }
        }.start();

        if (!started) {
            Intent mp_intent = ((MediaProjectionManager)getSystemService(MEDIA_PROJECTION_SERVICE)).createScreenCaptureIntent();
            lan.launch(mp_intent);
        } else {
            Intent screencap_intent = new Intent(this, ScreenCaptureService.class);
            stopService(screencap_intent);
            started = false;
        }
    }

    public void onPlayerMode(View view) {
        String player = ((TextInputEditText)findViewById(R.id.player)).getText().toString();
        String mode = ((TextInputEditText)findViewById(R.id.mode)).getText().toString();
        if (!easy_bricks.SetPlayMode(player, mode)) {
            Log.e("MainActivity", "SetPlayMode failed");
        }
    }

    private void ConsumeImage() {
        try {
            Thread.sleep(1000);
            GlobalData.image_lock.lock();
            while (GlobalData.image_buffer == null) {
                GlobalData.image_con.await();
            }
            Log.i("MainActivity", "Image new");
            int width = GlobalData.width;
            int height = GlobalData.height;
            byte []buffer = GlobalData.image_buffer;
            GlobalData.image_buffer = null;
            GlobalData.image_lock.unlock();

            // Save image
            File file = new File(dir + width + "x" + height + ".rgba");
            FileOutputStream out = new FileOutputStream(file);
            out.write(buffer);
            out.close();

            PlayOperation []play_ops =  easy_bricks.InputRawImage(2, width, height, buffer);

            for (PlayOperation play_op : play_ops) {
                if (play_op.type == 1) {
                    Log.i("MainActivity", "Click " + play_op.x + " " + play_op.y);
                    GlobalData.gesture_lock.lock();
                    GlobalData.click_x = play_op.x;
                    GlobalData.click_y = play_op.y;
                    GlobalData.gesture_con.signal();
                    GlobalData.gesture_lock.unlock();
                } else if (play_op.type == 2) {
                    Thread.sleep(play_op.sleep_ms);
                } else if (play_op.type == 3) {
                    // Kill current process
                    int pid = android.os.Process.myPid();
                    android.os.Process.killProcess(pid);
                }
            }
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
    private EasyBricks easy_bricks;
}