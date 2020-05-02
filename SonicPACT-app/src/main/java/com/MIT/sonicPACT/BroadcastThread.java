/*
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.MIT.sonicPACT;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import java.nio.ShortBuffer;

import static android.os.SystemClock.elapsedRealtimeNanos;

public class BroadcastThread {
    // Logging
    private static final String LOG_TAG = BroadcastThread.class.getSimpleName();
    private static final String TAG = LOG_TAG;

    private Thread mThread;
    private boolean mShouldContinue;
    private ShortBuffer mSamples;

    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    private AdvertiseCallback mAdvertiseCallback;

    public BroadcastThread(Context context) {
        // Init bluetooth
        BluetoothManager mBluetoothManager =
                (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);

        if (mBluetoothManager != null) {
            BluetoothAdapter mBluetoothAdapter = mBluetoothManager.getAdapter();
            if (mBluetoothAdapter != null) {
                mBluetoothLeAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
            }
            else {
                Log.v(TAG, "failed to init bt");
            }
        } else {
            Log.v(TAG, "failed to init bt");
        }

    }

    public boolean isPlaying() {
        return mThread != null;
    }

    public void startPlayback() {
        Log.v(LOG_TAG, "StartPlayback");

        if (mThread != null)
            return;

        // Start streaming in a thread
        mShouldContinue = true;
        mThread = new Thread(new Runnable() {
            @Override
            public void run() {
                play();
            }
        });
        mThread.start();
    }

    public void stopPlayback() {
        Log.v(LOG_TAG, "StopPlayback");
        if (mThread == null)
            return;

        mShouldContinue = false;
        mThread = null;
    }

    private void play() {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
        int duration = 1; // seconds

        int numSamples = (int)(.001 * Constants.SAMPLE_RATE);
        double sample[] = new double[numSamples];
        double freqOfTone = 21000; // hz

        byte generatedSnd[] = new byte[2 * numSamples];

        // fill out the array
        for (int i = 0; i < numSamples; ++i) {
            sample[i] = Math.sin(2 * Math.PI * i / (Constants.SAMPLE_RATE/freqOfTone));
        }

        // convert to 16 bit pcm sound array
        // assumes the sample buffer is normalised.
        int idx = 0;
        for (final double dVal : sample) {
            // scale to maximum amplitude
            final short val = (short) ((dVal * 32767));
            // in 16 bit wav PCM, first byte is the low order byte
            generatedSnd[idx++] = (byte) (val & 0x00ff);
            generatedSnd[idx++] = (byte) ((val & 0xff00) >>> 8);
        }

        final AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                Constants.SAMPLE_RATE,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                generatedSnd.length,
                AudioTrack.MODE_STATIC);

        audioTrack.write(generatedSnd, 0, generatedSnd.length);
        Thread thread = new Thread(){
            public void run(){
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
                audioTrack.play();
                Constants.nanosecondsSinceAudioSent = System.nanoTime();
            }
        };

        thread.start();


        // Broadcast BTLE here
        this.startBLE();
        this.stopBLE();

        try {
            thread.join();
        }
        catch (InterruptedException e) {
        }
    }

    /**
     * Starts BLE Advertising.
     */
    private void startBLE(){

        Log.d(TAG, "Service: Starting Advertising");

        if (mAdvertiseCallback == null) {

            AdvertiseSettings settings = buildAdvertiseSettings();
            AdvertiseData data = buildAdvertiseData();
            mAdvertiseCallback = new SampleAdvertiseCallback();

            if (mBluetoothLeAdvertiser != null)
                mBluetoothLeAdvertiser.startAdvertising(settings, data, mAdvertiseCallback);

        }
    }

    /**
     * Stops BLE Advertising.
     */
    private void stopBLE() {
        Log.d(TAG, "Service: Stopping Advertising");
        if (mBluetoothLeAdvertiser != null) {
            mBluetoothLeAdvertiser.stopAdvertising(mAdvertiseCallback);
            mAdvertiseCallback = null;
        }
    }

    /**
     * Returns an AdvertiseData object which includes the Service UUID and Device Name.
     */
    private AdvertiseData buildAdvertiseData() {

        AdvertiseData.Builder dataBuilder = new AdvertiseData.Builder();
        dataBuilder.addServiceUuid(Constants.Service_UUID);
        dataBuilder.addServiceData(Constants.Service_UUID, "hello!".getBytes());
        dataBuilder.setIncludeDeviceName(true);

        return dataBuilder.build();
    }

    private AdvertiseSettings buildAdvertiseSettings() {
        AdvertiseSettings.Builder settingsBuilder = new AdvertiseSettings.Builder();
        settingsBuilder.setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_POWER);
        //settingsBuilder.setTimeout(0);
        return settingsBuilder.build();
    }

    private class SampleAdvertiseCallback extends AdvertiseCallback {

        @Override
        public void onStartFailure(int errorCode) {
            super.onStartFailure(errorCode);
            Log.d(TAG, "Advertising failed");
        }

        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            super.onStartSuccess(settingsInEffect);
            Log.d(TAG, "Advertising successfully started");
        }
    }
}
