package com.MIT.sonicPACT;

import android.bluetooth.BluetoothAdapter;
import android.util.Log;

public class LeaderThread {
    // The leader will emit a chirp every N seconds to start the protocol

    private static final String TAG = LeaderThread.class.getSimpleName();
    private Thread mThread;
    private BluetoothHandler btHandler;

    private boolean shouldContinue;

    public LeaderThread(BluetoothAdapter bluetoothAdapter) {
        btHandler = new BluetoothHandler(bluetoothAdapter);
    }

    public void start() {
        btHandler.updateName(Utils.DEV_NAME_LEADER);
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
        NativeBridge.stopAudioChirpAtInterval();
    }

    private void run_protocol(){
        NativeBridge.setAudioBroadcastFreq(Utils.FREQ_LEADER);
        NativeBridge.setAudioListenFreq(Utils.FREQ_FOLLOWER);

        // Start a thread for chirping on a given interval:
        Thread chirpThread = new Thread(){
            @Override
            public void run() {
                NativeBridge.startAudioChirpAtInterval(3000); // half sec interval
            }
        };
        chirpThread.start();

        long T_1;
        long T_4;

        long last_recv_chirp_time = 0;

        while(shouldContinue){
            // check if there's a new chirp from the follower
            long new_recv_chirp_time = NativeBridge.getLastChirpRecvTime();
            if (new_recv_chirp_time - last_recv_chirp_time > 1000000) { // Ten miliseconds

                // Get last sent chirp time
                long last_sent_chirp_time = NativeBridge.getLastChirpSentTime();
                // T_1 = the time it was *actually* sent
                // TODO: get actual delay
                //T_1 = last_sent_chirp_time + 7500000; // Assume a 7.5 ms delay

                // T_4 == Time we Recv'd the actual payload
                //T_4 = new_recv_chirp_time + 7500000; // Assume a 7.5 ms delay

                // subtract
                long result = last_sent_chirp_time - new_recv_chirp_time;
                result *= -1;
                Log.d(TAG, "distance = " + result);
                /*

                // update bluetooth with t4-t1
                btHandler.updatePayload(Utils.longToBytes(result));
                while (!btHandler.mHasNewValue ) {
                    try {
                        Thread.sleep(5);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                long bluetoothresult = btHandler.getLastBTValue();

                Log.d(TAG, "distance = " + ((result - bluetoothresult - 7600000*4) / 2)/100000.0);

                 */
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
