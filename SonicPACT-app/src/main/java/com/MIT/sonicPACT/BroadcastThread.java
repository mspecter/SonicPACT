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
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.AdvertisingSet;
import android.bluetooth.le.AdvertisingSetCallback;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.bluetooth.le.PeriodicAdvertisingParameters;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.ParcelUuid;
import android.util.Log;

import java.nio.ShortBuffer;
import java.util.Random;
import java.util.UUID;

import static android.os.SystemClock.elapsedRealtimeNanos;

public class BroadcastThread {
    // Logging
    private static final String LOG_TAG = BroadcastThread.class.getSimpleName();
    private static final String TAG = LOG_TAG;

    private Thread mThread;
    private boolean mShouldContinue;
    private ShortBuffer mSamples;

    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    private BluetoothAdapter mBluetoothAdapter;
    private AdvertiseCallback mAdvertiseCallback;

    public BroadcastThread(Context context) {
        // Init bluetooth
        BluetoothManager mBluetoothManager =
                (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);

        if (mBluetoothManager != null) {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
            mBluetoothAdapter.setName(Constants.DEV_NAME);

            Log.e(LOG_TAG, "BLUETOOTH ADDR: "+mBluetoothAdapter.getAddress());
            Log.e(LOG_TAG, "BLUETOOTH NAME: "+mBluetoothAdapter.getName());
            // Check if all features are supported
            if (mBluetoothAdapter.isLe2MPhySupported()) {
                Log.e(LOG_TAG, "2M PHY supported!");
            }
            else {
                Log.e(LOG_TAG, "2M PHY not supported!");
            }
            if (mBluetoothAdapter.isLeExtendedAdvertisingSupported()) {
                Log.e(LOG_TAG, "LE Extended Advertising supported!");
            }
            else {
                Log.e(LOG_TAG, "LE Extended Advertising not supported!");
            }
            if (mBluetoothAdapter.isLePeriodicAdvertisingSupported()) {
                Log.e(LOG_TAG, "LE PERIODIC Advertising supported!");
            }
            else {
                Log.e(LOG_TAG, "LE PERIODIC Advertising not supported!");
            }
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
        //Log.v(LOG_TAG, "StartPlayback");

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
        //Log.v(LOG_TAG, "StopPlayback");

        stopBLE();
        NativeBridge.StopPlayback();
        if (mThread == null)
            return;

        mShouldContinue = false;
        mThread = null;
    }

    private void play_java(){
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
        audioTrack.play();
        Constants.nanosecondsSinceAudioSent = System.nanoTime();

    }

    private void play_jni() {
        Constants.nanosecondsSinceAudioSent = System.nanoTime();
        NativeBridge.StartPlayback();
        Constants.beganPlayback = true;
    }

    private void play() {
        this.startBLE_periodic();
        /*
        for (int i = 0; i < 100; i++) {

            // Broadcast BTLE here
            this.startBLE();

            // wait 20 ms seconds
            while (!Constants.beganPlayback)
                try {
                    Thread.sleep(1);
                } catch (Exception e){}

            try {
                Thread.sleep(30);
            } catch (Exception e){}

            NativeBridge.StopPlayback();
            // Sleep another 20 ms
            try {
                Thread.sleep(300);
            } catch (Exception e){}

            // stops BLE playback
            this.stopPlayback();
            Constants.beganPlayback=false;

            // wait a second
            try {
                Thread.sleep(10000 );
            } catch (Exception e){}
        }*/


    }

    public AdvertisingSet currentAdvertisingSet = null;

    private void startBLE_periodic() {
        BluetoothAdapter adapter = this.mBluetoothAdapter;
        BluetoothLeAdvertiser advertiser = this.mBluetoothLeAdvertiser;

        // Check if all features are supported
        if (!adapter.isLe2MPhySupported()) {
            Log.e(LOG_TAG, "2M PHY not supported!");
            return;
        }
        if (!adapter.isLeExtendedAdvertisingSupported()) {
            Log.e(LOG_TAG, "LE Extended Advertising not supported!");
            return;
        }

        int maxDataLength = adapter.getLeMaximumAdvertisingDataLength();

        AdvertisingSetParameters.Builder parameters = (new AdvertisingSetParameters.Builder())
                .setLegacyMode(false)
                .setInterval(AdvertisingSetParameters.INTERVAL_MIN)
                .setTxPowerLevel(AdvertisingSetParameters.TX_POWER_HIGH)
                .setPrimaryPhy(BluetoothDevice.PHY_LE_1M)
                .setSecondaryPhy(BluetoothDevice.PHY_LE_2M);

        PeriodicAdvertisingParameters.Builder params = (new PeriodicAdvertisingParameters.Builder())
                .setIncludeTxPower(true)
                .setInterval(80);

        AdvertiseData data = (new AdvertiseData.Builder())
                .setIncludeDeviceName(true).build();

        AdvertisingSetCallback callback = new AdvertisingSetCallback() {
            @Override
            public void onAdvertisingSetStarted(AdvertisingSet advertisingSet, int txPower, int status) {
                Log.i(LOG_TAG, "onAdvertisingSetStarted(): txPower:" + txPower + " , status: "
                        + status);
                currentAdvertisingSet = advertisingSet;
            }

            @Override
            public void onAdvertisingSetStopped(AdvertisingSet advertisingSet) {
                Log.i(LOG_TAG, "onAdvertisingSetStopped():");
            }
        };

        advertiser.startAdvertisingSet(parameters.build(), data, null, params.build(), data, callback);

        while (currentAdvertisingSet == null){
            try{
                Thread.sleep(1);
            }catch (Exception e){ }
        }
        Log.i(LOG_TAG, "ENDING ADV");

        //currentAdvertisingSet.enableAdvertising(true, 0, 0);
    }

    /**
     * Starts BLE Advertising.
     */
    private void startBLE(){
        startBLE_periodic();
        if (true)
            return;

        //Log.d(TAG, "Service: Starting Advertising");

        if (mAdvertiseCallback == null) {

            AdvertiseSettings settings = new AdvertiseSettings.Builder()
                    .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
                    .setTxPowerLevel(AdvertisingSetParameters.TX_POWER_HIGH)
                    .build();

            AdvertiseData data = new AdvertiseData.Builder()
//                    .addServiceUuid(Constants.Service_UUID)
//                    .addServiceData(Constants.Service_UUID, "hello!".getBytes())
                    .setIncludeDeviceName(true)
  //                  .addManufacturerData(0x08A2, "derp".getBytes())
                    .build();

            PeriodicAdvertisingParameters periodic = new PeriodicAdvertisingParameters.Builder()
                    .setIncludeTxPower(true)
                    .setInterval(88)
                    .build();

            mAdvertiseCallback = new SampleAdvertiseCallback();

            AdvertisingSetParameters.Builder parameters = new AdvertisingSetParameters.Builder();
//                    .setInterval(AdvertisingSetParameters.INTERVAL_MEDIUM)
//                    .setTxPowerLevel(AdvertisingSetParameters.TX_POWER_HIGH)
//                    .setAnonymous(false);
                    //.setPrimaryPhy(BluetoothDevice.PHY_LE_1M)
                    //.setSecondaryPhy(BluetoothDevice.PHY_LE_2M);
                    //.setIncludeTxPower(true);

            AdvertisingSetCallback callback = new AdvertisingSetCallback() {
                @Override
                public void onAdvertisingSetStarted(AdvertisingSet advertisingSet, int txPower, int status) {
                    if (advertisingSet == null)
                        Log.i(LOG_TAG, "onAdvertisingSetStarted(): ERROR");
                    else {

                        Log.i(LOG_TAG, "onAdvertisingSetStarted(): txPower:" + txPower + " , status: "
                                + status);
                    }

                }

                public void onAdvertisingDataSet(AdvertisingSet advertisingSet, int status) {
                    if (advertisingSet == null)
                        Log.i(LOG_TAG, "onAdvertisingSetStarted(): ERROR " + status);

                    Log.i(LOG_TAG, "onAdvertisingSetStarted(): status: " + status);

                }

                @Override
                public void onAdvertisingSetStopped(AdvertisingSet advertisingSet) {
                    Log.i(LOG_TAG, "onAdvertisingSetStopped():");
                }

                @Override
                public void onPeriodicAdvertisingDataSet(AdvertisingSet advertisingSet, int status) {
                    if (advertisingSet == null)
                        Log.i(LOG_TAG, "onAdvertisingDataSet(): ERROR: "+ status);
                    else {
                        Log.i(LOG_TAG, "onPeriodicAdvertisingDataSet() status: " + status);
                    }
                }
            };

           //mBluetoothLeAdvertiser.startAdvertisingSet(parameters.build(), null, null, periodic, data, callback);

           if (mBluetoothLeAdvertiser != null)
               mBluetoothLeAdvertiser.startAdvertising(settings, data, mAdvertiseCallback);

        }
    }

    /**
     * Stops BLE Advertising.
     */
    private void stopBLE() {
        //Log.d(TAG, "Service: Stopping Advertising");
        if (mBluetoothLeAdvertiser != null && mAdvertiseCallback != null) {
            mBluetoothLeAdvertiser.stopAdvertising(mAdvertiseCallback);
            mAdvertiseCallback = null;
        }
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
            Thread thread = new Thread() {
                public void run() {
                    play_jni();
                }
            };
            thread.start();
            try {
                thread.join();
            } catch (InterruptedException e) {}
        }
    }
}
