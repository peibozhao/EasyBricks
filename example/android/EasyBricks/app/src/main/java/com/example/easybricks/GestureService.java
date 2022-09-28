package com.example.easybricks;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.GestureDescription;
import android.graphics.Path;
import android.util.Log;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

public class GestureService extends AccessibilityService {
    @Override
    public void onCreate() {
        Log.i("GestureService", "onCreate");
        new Thread() {
            @Override
            public void run() {
                Log.i("GestureService", "Thread start");
                while (true) {
                    try {
                        GlobalData.gesture_lock.lock();
                        while (GlobalData.click_x == -1) {
                            GlobalData.gesture_con.await();
                        }
                        int x = GlobalData.click_x, y = GlobalData.click_y;
                        GlobalData.click_x = -1;
                        GlobalData.gesture_lock.unlock();
                        Click(x, y);
                    } catch (Exception e) {
                        Log.e("GestureService", "Exception: " + e.toString());
                    }
                }
            }
        }.start();
        super.onCreate();
    }

    private void Click(int x, int y) {
        AccessibilityNodeInfo root_node = getRootInActiveWindow();
        Path path = new Path();
        path.moveTo(x, y);
        GestureDescription.Builder ges_builder = new GestureDescription.Builder();
        ges_builder.addStroke(new GestureDescription.StrokeDescription(path, 0, 50));
        dispatchGesture(ges_builder.build(), null, null);
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent accessibilityEvent) {

    }

    @Override
    public void onInterrupt() {

    }
}