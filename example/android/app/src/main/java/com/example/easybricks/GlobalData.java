package com.example.easybricks;

import android.graphics.Bitmap;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

// Bad way to share data
// But i hate java, so be it
public class GlobalData {
    public static ReentrantLock gesture_lock = new ReentrantLock();
    public static Condition gesture_con = gesture_lock.newCondition();
    public static int click_x;
    public static int click_y;

    public static ReentrantLock image_lock = new ReentrantLock();
    public static Condition image_con = image_lock.newCondition();
    public static Bitmap image = null;
}
