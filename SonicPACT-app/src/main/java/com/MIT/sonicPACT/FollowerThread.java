package com.MIT.sonicPACT;

import android.bluetooth.BluetoothAdapter;
import android.util.Log;

public class FollowerThread {
    // The leader will emit a chirp every N seconds to start the protocol

    private static final String TAG = FollowerThread.class.getSimpleName();
    private Thread mThread;
    private BluetoothHandler btHandler;

    private boolean shouldContinue;

    public FollowerThread(BluetoothAdapter bluetoothAdapter) {
        btHandler = new BluetoothHandler(bluetoothAdapter);
    }

    public void start() {
        btHandler.updateName(Utils.DEV_NAME_FOLLOWER);
        shouldContinue = true;
        Thread broadcastThread = new Thread(new Runnable() {
            @Override
            public void run() {
                btHandler.startBroadcast();
            }
        });
        broadcastThread.start();

        Thread scanThread = new Thread(new Runnable() {
            @Override
            public void run() {
                btHandler.startScanning();
            }
        });
        scanThread.start();

        mThread = new Thread(new Runnable() {
            @Override
            public void run() {
                run_protocol();
            }
        });

        mThread.start();
    }

    public void stop() {
        if (mThread == null)
            return;

        shouldContinue = false;

        // Stop bluetooth scanning
        btHandler.stopScanning();
        btHandler.stopBroadcast();
    }

    private void run_protocol(){
        NativeBridge.setAudioListenFreq(Utils.FREQ_LEADER);
        NativeBridge.setAudioBroadcastFreq(Utils.FREQ_FOLLOWER);

        long last_recv_chirp_time = 0;

        while(shouldContinue){
            // check if there's a new chirp from the follower
            long new_recv_chirp_time = NativeBridge.getLastChirpRecvTime();
            if (new_recv_chirp_time - last_recv_chirp_time > 1000000) { // Ten miliseconds
                // We got a new chirp! Respond!
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                NativeBridge.chirp();
                long last_sent_chirp_time = NativeBridge.getLastChirpSentTime();

                // T2 = when it heard the chirp from the Leader
                // T3 = when it replied
                long result = new_recv_chirp_time - last_sent_chirp_time;
                result *= -1;

                btHandler.updatePayload(Utils.longToBytes(result));

                // update last recv time
                last_recv_chirp_time = new_recv_chirp_time;
            }
            //Log.d(TAG, "LeaderThread running!");

            try {
                Thread.sleep(1);
            } catch (Exception e){}
        }
    }
}
