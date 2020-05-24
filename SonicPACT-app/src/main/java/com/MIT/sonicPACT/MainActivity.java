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

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.MicrophoneInfo;
import android.os.Build;
import android.os.Bundle;
/*import android.support.annotation.NonNull;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.design.widget.TextInputEditText;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;

 */
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.MIT.waveform.WaveformView;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import java.io.IOException;

import static android.media.AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER;
import static android.media.AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE;
import static android.media.AudioManager.PROPERTY_SUPPORT_AUDIO_SOURCE_UNPROCESSED;
import static android.media.AudioManager.PROPERTY_SUPPORT_SPEAKER_NEAR_ULTRASOUND;
import static android.media.AudioManager.PROPERTY_SUPPORT_MIC_NEAR_ULTRASOUND;


public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    // Views the waveform
    private WaveformView mRealtimeWaveformView;

    // Threads
    private ListenThread mListenThread;
    private BroadcastThread mBroadcastThread;
    private LeaderThread mLeaderThread;
    private FollowerThread mFollowerThread;

    // Bluetooth
    private BluetoothAdapter mBluetoothAdapter;

    // Enable Intent Static #'s
    private static final int REQUEST_ENABLE_BT = 1;
    private static final int REQUEST_RECORD_AUDIO = 13;
    private static final int REQUEST_LOC = 10;

    @RequiresApi(api = Build.VERSION_CODES.P)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Handle permissions etc
        enableBluetooth();
        requestLocationPermission();

        // Initialize the native audio callbacks for playback
        NativeBridge.InitPlaybackCallbacks();
        NativeBridge.InitRecordCallbacks();

        // Start recording chirps!
        NativeBridge.StartRecord();

        setContentView(R.layout.activity_main);

        // Printing out various information
        AudioManager myAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        try {
            for(MicrophoneInfo info : myAudioManager.getMicrophones()){
                Log.e(TAG, info.getDescription());
                Log.e(TAG, "" + info.getId());
                for (Pair<Float, Float> respones : info.getFrequencyResponse())
                    Log.e(TAG, "response: freq = " + respones.first + ", db = " + respones.second);

                Log.e(TAG, "END : "+ info.getDescription());
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {
            for(AudioDeviceInfo info : myAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS)){

                Log.e(TAG, "devname"+ info.getProductName());
                for (int rate : info.getSampleRates())
                    Log.e(TAG, "SAMPLERATE"+ rate);
                Log.e(TAG, "SAMPLERATE"+ info.getSampleRates().length);

            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.e(TAG, "SAMPLERATE"+
                myAudioManager.getProperty(PROPERTY_OUTPUT_SAMPLE_RATE));
        Log.e(TAG, "SUPPORT ULTRASOUND SPEAKER"+
                myAudioManager.getProperty(PROPERTY_SUPPORT_SPEAKER_NEAR_ULTRASOUND));
        Log.e(TAG, "SUPPORT ULTRASOUND MIC"+
                myAudioManager.getProperty(PROPERTY_SUPPORT_MIC_NEAR_ULTRASOUND));
        Log.e(TAG, "SUPPORT SOUND UNPROC "+
                myAudioManager.getProperty(PROPERTY_SUPPORT_AUDIO_SOURCE_UNPROCESSED));
        Log.e(TAG, "SOUND FRAMES PER BUFFER "+
                myAudioManager.getProperty(PROPERTY_OUTPUT_FRAMES_PER_BUFFER));

        mRealtimeWaveformView = findViewById(R.id.waveformView);
        mLeaderThread = new LeaderThread(mBluetoothAdapter);
        mFollowerThread = new FollowerThread(mBluetoothAdapter);
        final boolean[] isPlaying = {false};

        final FloatingActionButton aButton = findViewById(R.id.aButton);

        aButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isPlaying[0]) {
                    isPlaying[0] = false;
                    aButton.setImageResource(android.R.drawable.ic_media_play);

                    if (Utils.IS_LEADER)
                        mLeaderThread.stop();
                    else
                        mFollowerThread.stop();
                }
                else {
                    isPlaying[0] = true;
                    aButton.setImageResource(android.R.drawable.ic_media_pause);

                    if (Utils.IS_LEADER)
                        mLeaderThread.start();
                    else
                        mFollowerThread.start();
                }
            }
        });

        final MaterialButton toggle  = findViewById(R.id.toggleButton);
        final TextView roleTextLabel = findViewById(R.id.DevName);
        toggle.setText("SWITCH TO FOLLOWER");

        toggle.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                EditText frequency = findViewById(R.id.Frequency);
                if (!Utils.IS_LEADER) {
                    mLeaderThread.stop();
                    mFollowerThread.stop();
                    isPlaying[0] = false;

                    Utils.IS_LEADER = true;

                    toggle.setText("SWITCH TO FOLLOWER");
                    roleTextLabel.setText("Currently Leader");

                    aButton.setImageResource(android.R.drawable.ic_media_play);
                    frequency.setText(""+ Utils.FREQ_LEADER);

                } else {
                    // stop the leader thread
                    mLeaderThread.stop();
                    mFollowerThread.stop();
                    isPlaying[0] = false;

                    Utils.IS_LEADER = false;

                    roleTextLabel.setText("Currently Follower");
                    toggle.setText("SWITCH TO LEADER");

                    frequency.setText("" + Utils.FREQ_FOLLOWER);
                    aButton.setImageResource(android.R.drawable.ic_media_play);
                }
            }
        });

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case REQUEST_ENABLE_BT:
                if (resultCode == RESULT_OK) {
                    if (!mBluetoothAdapter.isMultipleAdvertisementSupported()) {
                        // Bluetooth Advertisements are not supported.
                        Toast.makeText(this, "We can't advertise!",
                                Toast.LENGTH_SHORT).show();
                        finish();
                    }
                } else {
                    // User declined to enable Bluetooth, exit the app.
                    Toast.makeText(this, "Bluetooth isn't enabled, bye!",
                                    Toast.LENGTH_SHORT).show();
                    finish();
                }
            default:
                super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private void showErrorText(String text) {
        Log.i(TAG, "Error:" + text);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    private void startAudioRecordingSafe() {
        if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO)
                == PackageManager.PERMISSION_GRANTED) {
            mListenThread.startRecording();
        } else {
            requestMicrophonePermission();
        }
    }

    private void requestLocationPermission() {
        if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.ACCESS_COARSE_LOCATION)) {
            // Show dialog explaining why we need record audio
            Snackbar.make(mRealtimeWaveformView, "LOCATION!!!",
                    Snackbar.LENGTH_INDEFINITE).setAction("OK", new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    ActivityCompat.requestPermissions(MainActivity.this, new String[]{
                            Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_LOC);
                }
            }).show();
        } else {
            ActivityCompat.requestPermissions(MainActivity.this, new String[]{
                    Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_LOC);
        }
    }

    private void requestMicrophonePermission() {
        if (ActivityCompat.shouldShowRequestPermissionRationale(this, android.Manifest.permission.RECORD_AUDIO)) {
            // Show dialog explaining why we need record audio
            Snackbar.make(mRealtimeWaveformView, "Microphone access is required in order to record audio",
                    Snackbar.LENGTH_INDEFINITE).setAction("OK", new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    ActivityCompat.requestPermissions(MainActivity.this, new String[]{
                            android.Manifest.permission.RECORD_AUDIO}, REQUEST_RECORD_AUDIO);
                }
            }).show();
        } else {
            ActivityCompat.requestPermissions(MainActivity.this, new String[]{
                    android.Manifest.permission.RECORD_AUDIO}, REQUEST_RECORD_AUDIO);
        }
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        if (requestCode == REQUEST_RECORD_AUDIO && grantResults.length > 0 &&
                grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            mListenThread.stopRecording();
        }
    }


    // Enables bluetooth
    private void enableBluetooth(){
        // Initializes Bluetooth adapter.
        final BluetoothManager bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);

        mBluetoothAdapter = bluetoothManager.getAdapter();

        Log.v(TAG, "Enabling BT");
        // Is Bluetooth supported on this device?
        if (mBluetoothAdapter != null) {
            // Is Bluetooth turned on?
            if (mBluetoothAdapter.isEnabled()) {
                // Are Bluetooth Advertisements supported on this device?
                if (mBluetoothAdapter.isMultipleAdvertisementSupported())
                    // Everything is supported and enabled, load the fragments.
                    Log.v(TAG, "BLUETOOTH ENABLED");
                else
                    // Bluetooth Advertisements are not supported.
                    showErrorText("bt_ads_not_supported");
            } else {
                // Prompt user to turn on Bluetooth (logic continues in onActivityResult()).
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
        } else {
            // Bluetooth is not supported.
            showErrorText("Bluetooth Not Enabled");
        }
    }

}
