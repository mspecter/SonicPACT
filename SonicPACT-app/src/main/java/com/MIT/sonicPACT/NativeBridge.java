package com.MIT.sonicPACT;

// Handles calls to/from JNI
public class NativeBridge {

    static {
        System.loadLibrary("native-lib");
    }
    
    private native String test();

}
