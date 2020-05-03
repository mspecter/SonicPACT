package com.MIT.sonicPACT;

// Handles calls to/from JNI
public class NativeBridge {

    static {
        System.loadLibrary("native-lib");
    }


    public static native void InitPlaybackCallbacks();
    public static native void StartPlayback();
    public static native void StopPlayback();

    public static native void InitRecordCallbacks();
    public static native long GetLastSpikeNS();
    public static native void StartRecord();
    public static native void StopRecord();
}
