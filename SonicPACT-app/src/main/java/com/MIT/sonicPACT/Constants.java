package com.MIT.sonicPACT;

import android.os.ParcelUuid;

public class Constants {

    public static final int SAMPLE_RATE = 192000/2/2;
    public static final double freqOfTone = 21000; // hz
    public static final ParcelUuid Service_UUID = ParcelUuid
            .fromString("0000b81d-0000-1000-8000-00805f9b34fb");

    public static long nanosecondsSinceAudioSent = 0;

}
