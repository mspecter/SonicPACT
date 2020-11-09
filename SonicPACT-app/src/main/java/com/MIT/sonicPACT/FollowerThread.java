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
        //NativeBridge.setAudioListenFreq(Utils.FREQ_LEADER);
        //NativeBridge.setAudioBroadcastFreq(Utils.FREQ_FOLLOWER);
        NativeBridge.setLeader(false);

        long last_recv_chirp_time = 0;

        while(shouldContinue){
            // check if there's a new chirp from the follower

            // End of the recv broadcast as heard from the mic
            long new_recv_chirp_time = NativeBridge.getLastChirpRecvTime();
            if (new_recv_chirp_time - last_recv_chirp_time > 1000000) { // Ten miliseconds
                // We got a new chirp! Respond!

                // End of the sent broadcast as heard from the mic
                long current_chirp = NativeBridge.getLastChirpSentTime();
                long last_sent_chirp_time = current_chirp;
                NativeBridge.chirp();

                // Wait until the chirp finishes
                while (last_sent_chirp_time == current_chirp){
                    try {
                        Thread.sleep(1);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    last_sent_chirp_time = NativeBridge.getLastChirpSentTime();
                }

                // T2 = when it heard the chirp from the Leader
                // T3 = when it replied
                //long result = new_recv_chirp_time - last_sent_chirp_time;
                //long twiddle = NativeBridge.getLastChirpDelayNS() / 3;
                long twiddle = 0;
                long result = (last_sent_chirp_time+twiddle) - new_recv_chirp_time;

                btHandler.updatePayload(Utils.longToBytes(result));

                // update last recv time
                last_recv_chirp_time = new_recv_chirp_time;
            }

            try {
                Thread.sleep(1);
            } catch (Exception e){}
        }
    }
}
