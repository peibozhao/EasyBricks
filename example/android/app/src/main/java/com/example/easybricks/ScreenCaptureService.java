package com.example.easybricks;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

import androidx.core.app.NotificationCompat;

import java.nio.ByteBuffer;

public class ScreenCaptureService extends Service {
    public ScreenCaptureService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate() {
        new Thread() {
            @Override
            public void run() {
                Log.i("ScreenCaptureService", "Thread start");
                Looper.prepare();
                handler = new Handler();
                Looper.loop();
                Log.i("ScreenCaptureService", "Thread stop");
            }
        }.start();

        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        Log.i("ScreenCaptureService", "onCreate");
        // Create notification
        String channel_id = "MyForeground";
        NotificationChannel noti_chan = new NotificationChannel(channel_id, "ForegroundService", NotificationManager.IMPORTANCE_NONE);
        noti_chan.setLightColor(Color.BLUE);
        noti_chan.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
        NotificationManager service = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
        service.createNotificationChannel(noti_chan);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, channel_id);
        Notification notification = builder.setOngoing(true)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setPriority(androidx.core.app.NotificationCompat.PRIORITY_MIN)
                .setCategory(Notification.CATEGORY_SERVICE)
                .build();
        startForeground(100, notification);

        int code = intent.getIntExtra("code", 0);
        Intent data = intent.getParcelableExtra("data");

        MediaProjectionManager mpm = (MediaProjectionManager)getSystemService(MEDIA_PROJECTION_SERVICE);
        MediaProjection mp = mpm.getMediaProjection(code, data);

        WindowManager wm = (WindowManager)getSystemService(Context.WINDOW_SERVICE);
        Display dis = wm.getDefaultDisplay();
        DisplayMetrics met = new DisplayMetrics();
        dis.getMetrics(met);

        image_reader = ImageReader.newInstance(met.widthPixels, met.heightPixels, PixelFormat.RGBA_8888, 5);
        mp.createVirtualDisplay("shot", met.widthPixels, met.heightPixels, met.densityDpi,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC,
                image_reader.getSurface(), null, null);

        image_reader.setOnImageAvailableListener(
                new ImageReader.OnImageAvailableListener() {
                    @Override
                    public void onImageAvailable(ImageReader imageReader) {
                        Log.i("ScreenCaptureService", "onImageAvailable");
                        try {
                            Image img = imageReader.acquireLatestImage();
                            Image.Plane[] planes = img.getPlanes();
                            ByteBuffer buffer = planes[0].getBuffer();
                            int pixel_stride = planes[0].getPixelStride();
                            int row_stride = planes[0].getRowStride();
                            int row_padding = row_stride - pixel_stride * img.getWidth();
                            Bitmap bt_img = Bitmap.createBitmap(img.getWidth() + row_padding / pixel_stride,
                                                             img.getHeight(), Bitmap.Config.ARGB_8888);
                            bt_img.copyPixelsFromBuffer(buffer);
                            GlobalData.image_lock.lock();
                            GlobalData.image = bt_img;
                            GlobalData.image_con.signal();
                            GlobalData.image_lock.unlock();;
                            img.close();
                        } catch (Exception e) {
                            Log.e("ScreenCaptureService", "Exception: " + e.toString());
                        }
                    }
                }, handler);

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        Log.i("ScreenCaptureService", "onDestroy");
        stopForeground(true);
        super.onDestroy();
    }

    private Handler handler;
    ImageReader image_reader;
}