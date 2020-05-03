package com.MIT.sonicPACT;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import com.paramsen.noise.Noise;

import android.util.Log;

import java.nio.FloatBuffer;

import static android.os.SystemClock.elapsedRealtimeNanos;

public class ListenThread {
    private static final String LOG_TAG = ListenThread.class.getSimpleName();

    public ListenThread(AudioDataReceivedListener listener) {
        mListener = listener;
    }

    private boolean mShouldContinue;
    private AudioDataReceivedListener mListener;
    private Thread mThread;

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
        if (mThread == null)
            return;

        mShouldContinue = false;
        mThread = null;
    }

    private void record() {
        Log.v(LOG_TAG, "Start");
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);

        // buffer size in bytes
        final int bufferSize = 4096/2;

                /*AudioRecord.getMinBufferSize(Constants.SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT);*/

        //if (bufferSize == AudioRecord.ERROR || bufferSize == AudioRecord.ERROR_BAD_VALUE) {
        //}

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
            final long currentTimestamp = System.nanoTime();
            final short[] audioBuffer = new short[bufferSize];
            int numberOfShort = record.read(audioBuffer, 0, bufferSize);
            /*
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
            mListener.onAudioDataReceived(audioBuffer);
        }

        record.stop();
        record.release();

        Log.v(LOG_TAG, String.format("Recording stopped. Samples read: %d", shortsRead));
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
}
