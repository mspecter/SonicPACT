package com.MIT.sonicPACT;

import android.os.ParcelUuid;

import java.nio.ByteBuffer;

public class Utils {

    public static final int SAMPLE_RATE = 192000/2/2;
    public static final double freqOfTone = 21000; // hz

    public static final ParcelUuid Service_UUID = ParcelUuid
            .fromString("0000b81d-0000-1000-8000-AAAAAAAAAAAA");

    // Bluetooth Device Names
    public static final String DEV_NAME_LEADER = "SONIC_PACT_LEADER";
    public static final String DEV_NAME_FOLLOWER = "SONIC_PACT_FOLLOWER";

    // Are we the leader or follower?
    public static boolean IS_LEADER = false;

    //
    public static long nanosecondsSinceAudioSent = 0;

    // Frequencies for both leader & follower
    public static final double FREQ_LEADER   = 18000;
    public static final double FREQ_FOLLOWER = 18000;

    public static boolean beganPlayback = false;

    public static byte[] longToBytes(long x) {
        ByteBuffer buffer = ByteBuffer.allocate(Long.BYTES);
        buffer.putLong(x);
        return buffer.array();
    }

    public static long bytesToLong(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(Long.BYTES);
        buffer.put(bytes);
        buffer.flip();//need flip
        return buffer.getLong();
    }
}
