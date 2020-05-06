package com.MIT.sonicPACT;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import com.paramsen.noise.Noise;

import android.os.Handler;
import android.os.ParcelUuid;
import android.os.SystemClock;
import android.util.Log;

import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import static android.os.SystemClock.elapsedRealtimeNanos;
import static com.MIT.sonicPACT.NativeBridge.GetLastSpikeNS;

public class ListenThread {
    private static final String LOG_TAG = ListenThread.class.getSimpleName();
    private static final String TAG = ListenThread.class.getSimpleName();

    private static final long SCAN_PERIOD = 5000;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothLeScanner mBluetoothLeScanner;
    private ScanCallback mScanCallback;
    private boolean mShouldContinue;
    private AudioDataReceivedListener mListener;
    private Thread mThread;
    private Handler mHandler;

    public ListenThread(AudioDataReceivedListener listener, BluetoothAdapter bluetoothAdapter) {
        mListener = listener;
        mHandler = new Handler();
        mBluetoothAdapter = bluetoothAdapter;
        mBluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();
    }


    public boolean recording() {
        return mThread != null;
    }

    public void startRecording() {
        Log.v(LOG_TAG, "StartRecording");
        if (mThread != null)
            return;

        mShouldContinue = true;
        mThread = new Thread(new Runnable() {
            @Override
            public void run() {
                record();
            }
        });
        mThread.start();
    }

    public void stopRecording() {
        Log.v(LOG_TAG, "StopRecording");

        this.stopScanning();
        if (mThread == null)
            return;

        mShouldContinue = false;
        mThread = null;
    }

    private void record() {

        startScanning();



        /*
        Log.v(LOG_TAG, "Start");
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);

        // buffer size in bytes
        final int bufferSize = 4096/2;
        final Noise foo = Noise.real(bufferSize);

        AudioRecord record = new AudioRecord(MediaRecorder.AudioSource.DEFAULT,
                Constants.SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize);

        if (record.getState() != AudioRecord.STATE_INITIALIZED) {
            Log.e(LOG_TAG, "Audio Record can't initialize!");
            return;
        }
        record.startRecording();

        Log.v(LOG_TAG, "Start recording, buffersize = "+bufferSize);

        long shortsRead = 0;

        while (mShouldContinue) {
            final short[] audioBuffer = new short[bufferSize];
            //int numberOfShort = record.read(audioBuffer, 0, bufferSize);
            /*
            final long currentTimestamp = System.nanoTime();
            shortsRead += numberOfShort;

            Thread calcthread = new Thread(new Runnable() {
                @Override
                public void run() {
                    final FloatBuffer f = FloatBuffer.allocate(bufferSize);
                    for (Short element: audioBuffer) {
                        f.put(element.floatValue());
                        if (!f.hasRemaining()) {
                            break;
                        }
                    }
                    final float[] floatBuffer = new float[bufferSize +2];
                    foo.fft(f.array(), floatBuffer);
                    calc(floatBuffer, currentTimestamp);
                }
            });
            calcthread.start();

            */
            // Notify waveform
            //mListener.onAudioDataReceived(audioBuffer);
        //}

        //record.stop();
        //record.release();


        // Log.v(LOG_TAG, String.format("Recording stopped. Samples read: %d", shortsRead));
    }

    void calc(float[] fft, long time){
        // freq = index * Fs / N;
        // N == size of the FFT
        // F_s = sample rate

        // index = (fftSize / SampleRate) * freq
        int index = (int)( (double) fft.length / Constants.SAMPLE_RATE * Constants.freqOfTone);
        float real = fft[index * 2];
        float imaginary = fft[index * 2+1];
        double magnitude = Math.sqrt((double)real*real + (double)imaginary*imaginary);
        //Log.v(LOG_TAG, "MAGNITUDE DETECTED:" + magnitude );
        if (magnitude > 9000) {
            long timedist = time - Constants.nanosecondsSinceAudioSent;
            Log.v(LOG_TAG, "HIGH MAGNITUDE DETECTED:" + magnitude + ", TIME SINCE SENT" + timedist);
        }
    }

    /**
     * Start scanning for BLE Advertisements (& set it up to stop after a set period of time).
     */
    public void startScanning() {
        if (mScanCallback == null) {
            Log.d(TAG, "Starting Scanning");

            // Kick off a new scan.
            mScanCallback = new SampleScanCallback();
            mBluetoothLeScanner.startScan(buildScanFilters(), buildScanSettings(), mScanCallback);
        }
    }

    /**
     * Stop scanning for BLE Advertisements.
     */
    public void stopScanning() {
        Log.d(TAG, "Stopping Scanning");

        // Stop the scan, wipe the callback.
        mBluetoothLeScanner.stopScan(mScanCallback);
        mScanCallback = null;
    }

    /**
     * Return a List of {@link ScanFilter} objects to filter by Service UUID.
     */
    private List<ScanFilter> buildScanFilters() {
        List<ScanFilter> scanFilters = new ArrayList<>();

        ScanFilter.Builder builder = new ScanFilter.Builder();
        // Comment out the below line to see all BLE devices around you
        //builder.setServiceUuid(Constants.Service_UUID);
        builder.setDeviceName(Constants.DEV_NAME);
        scanFilters.add(builder.build());

        return scanFilters;
    }

    /**
     * Return a {@link ScanSettings} object set to use low power (to preserve battery life).
     */
    private ScanSettings buildScanSettings() {
        ScanSettings.Builder builder = new ScanSettings.Builder();
        builder.setLegacy(false);
        builder.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY);
        builder.setMatchMode(ScanSettings.MATCH_MODE_AGGRESSIVE);
        builder.setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES);
        builder.setReportDelay(0);
        return builder.build();
    }

    /**
     * Custom ScanCallback object - adds to adapter on success, displays error on failure.
     */
    private class SampleScanCallback extends ScanCallback {
        ArrayList<ParcelUuid> uuds = new ArrayList<ParcelUuid>();
        ArrayList<String> devices = new ArrayList<String>();

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            super.onBatchScanResults(results);

            for (ScanResult result : results) {
                String res = "";
                res += result.getScanRecord().getDeviceName() + ", ";
                res += result.getScanRecord().getTxPowerLevel() + ", ";
                res += result.getAdvertisingSid() + ", ";
                res += result.getTimestampNanos() + ", ";
                Log.d("Scan:", "RESULT SCAN: " + res);
            }
        }

        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);


            if (true || !devices.contains(result.getDevice().getAddress())){
                devices.add(result.getDevice().getAddress());
                String res = "";
                res += result.getDevice().getAddress() + ", ";
                res += result.getDevice().getName() + ", ";
                res += result.getTimestampNanos() + ", ";
                res += result.getDevice().getUuids() + ", ";
                res += result.getScanRecord().getServiceUuids() + ", ";
                //res += svcData + ", ";

                if (result.getDevice().getName() != null && result.getDevice().getName().contains(Constants.DEV_NAME)) {

                    Log.d("Scan:", "RESULT SCAN: " + res);
                    try {
                        Thread.sleep(5);
                    } catch(Exception e){}
                    //Log.d("SCAN:", "BLUETOOTH - AUDIO TIME = " + (result.getTimestampNanos()- GetLastSpikeNS()) );


                }
            }
            /*
            Map<ParcelUuid, byte[]> svcData = result.getScanRecord().getServiceData();
            if (svcData == null)
                return;
            for (ParcelUuid id: svcData.keySet()) {
                if (!uuds.contains(id)){
                    uuds.add(id);
                    String res = "";
                    res += result.getDevice().getAddress() + ", ";
                    res += result.getDevice().getName() + ", ";
                    res += result.getTimestampNanos() + ", ";
                    res += svcData + ", ";

                    Log.d("Scan:", "RESULT SCAN: " + res);
                }

            }
            */
          //Log.d(LOG_TAG, "RESULT SCAN: " + result.toString());
        }

        @Override
        public void onScanFailed(int errorCode) {
            super.onScanFailed(errorCode);
            Log.d(LOG_TAG, "SCAN FAILED: " + errorCode);

        }
    }
}
