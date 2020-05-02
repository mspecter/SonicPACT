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
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import com.MIT.waveform.WaveformView;


public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    private WaveformView mRealtimeWaveformView;
    private ListenThread mListenThread;
    private BroadcastThread mBroadcastThread;
    private BluetoothAdapter mBluetoothAdapter;

    public static final int REQUEST_ENABLE_BT = 1;

    private static final int REQUEST_RECORD_AUDIO = 13;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        mRealtimeWaveformView = findViewById(R.id.waveformView);
        mListenThread = new ListenThread(new AudioDataReceivedListener() {
            @Override
            public void onAudioDataReceived(short[] data) {
                mRealtimeWaveformView.setSamples(data);
            }
        });

        final WaveformView mPlaybackView = findViewById(R.id.playbackWaveformView);

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!mListenThread.recording()) {
                    startAudioRecordingSafe();
                } else {
                    mListenThread.stopRecording();
                }
            }
        });

        final FloatingActionButton playFab = findViewById(R.id.playFab);

        mBroadcastThread = new BroadcastThread(this.getApplicationContext());

        mPlaybackView.setChannels(1);
        mPlaybackView.setSampleRate(Constants.SAMPLE_RATE);

        playFab.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (!mBroadcastThread.isPlaying()) {
                            mBroadcastThread.startPlayback();
                            playFab.setImageResource(android.R.drawable.ic_media_pause);
                        }
                        else {
                            mBroadcastThread.stopPlayback();
                            playFab.setImageResource(android.R.drawable.ic_media_play);
                        }
                    }
                });

        // enable and test bluetooth
        // Initializes Bluetooth adapter.
        final BluetoothManager bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);

        mBluetoothAdapter = bluetoothManager.getAdapter();

        showErrorText("Enabling bt");

        // Is Bluetooth supported on this device?
        if (mBluetoothAdapter != null) {

            // Is Bluetooth turned on?
            if (mBluetoothAdapter.isEnabled()) {

                // Are Bluetooth Advertisements supported on this device?
                if (mBluetoothAdapter.isMultipleAdvertisementSupported()) {

                    // Everything is supported and enabled, load the fragments.
                    showErrorText("BLUETOOTH ENABLED");

                } else {

                    // Bluetooth Advertisements are not supported.
                    showErrorText("bt_ads_not_supported");
                }
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

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case REQUEST_ENABLE_BT:
                if (resultCode == RESULT_OK) {
                    if (mBluetoothAdapter.isMultipleAdvertisementSupported()) {
                        // Everything is supported and enabled, load the fragments.
                        //setupFragments();

                    } else {
                        // Bluetooth Advertisements are not supported.
                        showErrorText("bt_ads_not_supported");
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
        Log.println(1, TAG, "WIN");
    }

    @Override
    protected void onStop() {
        super.onStop();
        mListenThread.stopRecording();
        mBroadcastThread.stopPlayback();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement

        return super.onOptionsItemSelected(item);
    }

    private void startAudioRecordingSafe() {
        if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO)
                == PackageManager.PERMISSION_GRANTED) {
            mListenThread.startRecording();
        } else {
            requestMicrophonePermission();
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


}
