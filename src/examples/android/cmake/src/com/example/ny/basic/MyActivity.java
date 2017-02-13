package com.example.ny.basic;

public class MyActivity extends android.app.NativeActivity {

    static {
       System.loadLibrary("ny-basic");
       System.loadLibrary("ny");
    }
 }
