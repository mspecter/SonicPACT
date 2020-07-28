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

    public static native void startAudioChirpAtInterval(int i);
    public static native void stopAudioChirpAtInterval();
    public static native void chirp();

    public static native void setAudioListenFreq(double freq);

    public static native void setAudioBroadcastFreq(double freq);

    public static native long getLastChirpRecvTime();

    public static native long getLastChirpSentTime();

    public static native long getLastChirpDelayNS();

    public static native long setLeader(boolean isLeader);
}
