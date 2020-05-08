package com.MIT.sonicPACT;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertisingSet;
import android.bluetooth.le.AdvertisingSetCallback;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.PeriodicAdvertisingParameters;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class BluetoothHandler {
    private static String TAG = "BLUETOOTH_HANDLER";

    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothLeScanner mBluetoothLeScanner;
    private ScanCallback mScanCallback;
    BluetoothLeAdvertiser mBluetoothLeAdvertiser;

    private AdvertisingSet currentAdvertisingSet = null;
    private AdvertisingSetCallback mAdvertiseCallback = null;
    private boolean advertisingStopped = false;
    private long mLastBluetoothValue;
    public boolean mHasNewValue = false;

    public BluetoothHandler(BluetoothAdapter bluetoothAdapter) {
        mBluetoothAdapter = bluetoothAdapter;
        mBluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();
        mBluetoothLeAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
    }

    public void updateName(String name){
        mBluetoothAdapter.setName(name);
    }

    // Update Manufacturer's data
    public void updatePayload(byte[] payload){
        AdvertiseData data = (new AdvertiseData.Builder())
                .addManufacturerData(1234, payload)
                .setIncludeDeviceName(true).build();
        while (currentAdvertisingSet == null){
            try{
                Thread.sleep(1);
            }catch (Exception e){ }
        }
        currentAdvertisingSet.setAdvertisingData(data);
    }

    /**
     * Start BLE Broadcast
     */
    public void startBroadcast() {
        startBLE_periodic();
    }

    private void startBLE_periodic() {
        // Check if all features are supported
        if (!mBluetoothAdapter.isLe2MPhySupported()) {
            Log.e(TAG, "2M PHY not supported!");
            return;
        }
        if (!mBluetoothAdapter.isLeExtendedAdvertisingSupported()) {
            Log.e(TAG, "LE Extended Advertising not supported!");
            return;
        }

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

        mAdvertiseCallback = new BluetoothHandler.AdvertisingCallback();

        mBluetoothLeAdvertiser.startAdvertisingSet(parameters.build(), data, null, null, null, mAdvertiseCallback);

        // Wait until the advertising is over:
        while (currentAdvertisingSet == null){
            try{
                Log.e(TAG, "Still waiting.");
                Thread.sleep(1);
            }catch (Exception e){ }
        }
    }


    /**
     * Stops Advertising.
     */
    public void stopBroadcast() {
        if (mBluetoothLeAdvertiser != null &&
                mAdvertiseCallback != null &&
                currentAdvertisingSet != null) {

            Log.e(TAG, "Service: Halt Advertising");
            currentAdvertisingSet.enableAdvertising (false, 0, 0 );
            currentAdvertisingSet.setPeriodicAdvertisingEnabled(false);
            mBluetoothLeAdvertiser.stopAdvertisingSet(mAdvertiseCallback);
            mAdvertiseCallback = null;
        }
    }

    /**
     * Start scanning for BLE Advertisements (& set it up to stop after a set period of time).
     */
    public void startScanning() {
        if (mScanCallback == null) {
            Log.d(TAG, "Starting Scanning");
            mScanCallback = new BluetoothHandler.SampleScanCallback();
            Thread t = new Thread(){
                @Override
                public void run() {
                    super.interrupt();
                }
            };
            t.start();
            mBluetoothLeScanner.startScan(buildScanFilters(), buildScanSettings(), mScanCallback);
        }
    }

    /**
     * Stop scanning for BLE Advertisements.
     */
    public void stopScanning() {
        Log.d(TAG, "Stopping Scanning");
        if (mScanCallback!=null) {
            // Stop the scan, wipe the callback.
            mBluetoothLeScanner.stopScan(mScanCallback);
            mScanCallback = null;
        }
    }

    /**
     * Return a List of {@link ScanFilter} objects to filter by Service UUID.
     */
    private List<ScanFilter> buildScanFilters() {
        List<ScanFilter> scanFilters = new ArrayList<>();
        ScanFilter.Builder builder = new ScanFilter.Builder();

        if (Utils.IS_LEADER)
            builder.setDeviceName(Utils.DEV_NAME_FOLLOWER);
        else
            builder.setDeviceName(Utils.DEV_NAME_LEADER);

        scanFilters.add(builder.build());

        return scanFilters;
    }

    /**
     * Return a {@link ScanSettings} object set to use low power (to preserve battery life).
     */
    private ScanSettings buildScanSettings() {
        //TODO: we probably don't need to be so aggressive here
        return new ScanSettings.Builder()
                .setLegacy(false)
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .setMatchMode(ScanSettings.MATCH_MODE_AGGRESSIVE)
                .setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                .setReportDelay(0)
                .build();
    }

    public long getLastBTValue() {
        mHasNewValue = false;
        return mLastBluetoothValue;
    }

    /**
     * Custom AdvertisingSetCallback object
     */
    private class AdvertisingCallback extends AdvertisingSetCallback {
        @Override
        public void onAdvertisingSetStarted(AdvertisingSet advertisingSet, int txPower, int status) {
            Log.i(TAG, "onAdvertisingSetStarted(): txPower:" + txPower + " , status: "
                    + status);
            currentAdvertisingSet = advertisingSet;
        }

        @Override
        public void onAdvertisingSetStopped(AdvertisingSet advertisingSet) {
            Log.i(TAG, "onAdvertisingSetStopped():");
            advertisingStopped = true;
        }
    }

    /**
     * Custom ScanCallback object - adds to adapter on success, displays error on failure.
     */
    private class SampleScanCallback extends ScanCallback {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);
            String res = "";
            res += result.getDevice().getAddress() + ", ";
            res += result.getDevice().getName() + ", ";
            res += result.getTimestampNanos() + ", ";
            res += result.getDevice().getUuids() + ", ";
            res += result.getScanRecord().getServiceUuids() + ", ";

            byte[] foo = result.getScanRecord().getManufacturerSpecificData().get(1234);
            if (foo != null) {
                long newValue = Utils.bytesToLong(foo);
                if (newValue != mLastBluetoothValue) {
                    mLastBluetoothValue = newValue;
                    mHasNewValue = true;
                    res += mLastBluetoothValue + ", ";
                    Log.d("Bluetooth:", "Broadcast Recv: " + res);
                }
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            super.onScanFailed(errorCode);
            Log.d(TAG, "SCAN FAILED: " + errorCode);
        }
    }
}
