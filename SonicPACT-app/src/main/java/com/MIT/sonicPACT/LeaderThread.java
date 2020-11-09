package com.MIT.sonicPACT;

import android.bluetooth.BluetoothAdapter;
import android.util.Log;

import static com.MIT.sonicPACT.NativeBridge.setLeader;

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
        NativeBridge.setLeader(true);
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
        //NativeBridge.setAudioBroadcastFreq(Utils.FREQ_LEADER);
        //NativeBridge.setAudioListenFreq(Utils.FREQ_FOLLOWER);

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

        while (shouldContinue) {
            // check if there's a new chirp from the follower
            long test = btHandler.getLastBTValue();

            // When the end of the last recv broadcast was seen
            long new_recv_chirp_time = NativeBridge.getLastChirpRecvTime();
            if (new_recv_chirp_time - last_recv_chirp_time > 1000000) { // Ten miliseconds

                // Get last sent chirp time
                // End of the broadcast as it is confirmed sent from the mic
                long last_sent_chirp_time = NativeBridge.getLastChirpSentTime();
                //last_sent_chirp_time -= 7500000/4;

                // subtract
                long result = new_recv_chirp_time - last_sent_chirp_time ;
                Log.d(TAG, "T4-T1= " + result);

                // update bluetooth with t4-t1
                btHandler.updatePayload(Utils.longToBytes(result));
                while (!btHandler.mHasNewValue ) {
                    try {
                        Thread.sleep(1);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                long bluetoothresult = btHandler.getLastBTValue();
                Log.d(TAG, "T3-T2= " + bluetoothresult );
                long chirpdelay = NativeBridge.getLastChirpDelayNS();
                Log.d(TAG, "Chirp Delay= " + chirpdelay);

                float distance =((result - bluetoothresult ) / (2.0f))/1000000.0f / 1.12533f;
                Log.d(TAG, "distance = " + distance);
                distance =((result - bluetoothresult) / (2.0f))/1000000.0f ;
                Log.d(TAG, "distance (weak)= " + distance);
                //distance =((result - bluetoothresult + chirpdelay/4.0f) / (2.0f))/1000000.0f/ 1.12533f ;
                //Log.d(TAG, "distance (delay)= " + distance);
                // update last recv time
                last_recv_chirp_time = NativeBridge.getLastChirpRecvTime();
            }
            //Log.d(TAG, "LeaderThread running!");


            try {
                Thread.sleep(1);
            } catch (Exception e){}

        }

    }
}
