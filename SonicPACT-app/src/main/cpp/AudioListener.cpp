//
// Created by Michael Specter on 5/2/20.
//

#include <android/log.h>
#include <__locale>
#include "AudioListener.h"
//#include "Buffer.h"
#include "readerwriterqueue/readerwriterqueue.h"
#include "readerwriterqueue/atomicops.h"

using namespace moodycamel;

bool AudioListenerCallback::isMagnitudeAboveNoise(int index, int compare1, int compare2) {

    // Get frequency index
    float real = fft_result[index].r;
    float imaginary = fft_result[index].i;
    double magnitude = sqrt((double)real*real + (double)imaginary*imaginary);

    real = fft_result[compare1].r;
    imaginary = fft_result[compare1].i;
    double magnitude2 = sqrt((double)real*real + (double)imaginary*imaginary);

    real = fft_result[compare2].r;
    imaginary = fft_result[compare2].i;
    magnitude2 = (sqrt((double)real*real + (double)imaginary*imaginary) + magnitude2)/2 ;

    return (magnitude > magnitude2) && (magnitude - magnitude2 > 100000);
}

oboe::DataCallbackResult
AudioListenerCallback::do_fft_detect(void *audioData, int32_t numFrames) {
    short *shortdata = (short *) audioData;
    uint64_t timestmp = static_cast<uint64_t>(getTimeNsec());

    // Convert shorts to float and copy
    for (int i = 0; i < numFrames ; i++) {
        audiobuffer[currentBufferIndx] = (float)shortdata[i];
        currentBufferIndx ++;
        if (currentBufferIndx >= BUFFSIZE)
            break;
    }

    // THe bufer may not be isFull
    if (currentBufferIndx < BUFFSIZE)
        return oboe::DataCallbackResult::Continue;

    currentBufferIndx = 0;

    kiss_fftr(fft_config, audiobuffer, fft_result);

    bool isAboveNoise = isMagnitudeAboveNoise(index, compare1, compare2);
    if (isAboveNoise){
        if (timestmp - lastListeningSpikeTime > 30000000) {// 30 ms
            //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT mag %f, at time %lu", magnitude -magnitude2, timestmp);
            __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT mag at time %lu", timestmp);
            lastListeningBroadcastTime = timestmp;
        }
        lastListeningSpikeTime = timestmp;
    }

    isAboveNoise = isMagnitudeAboveNoise(indexOwn, compareOwn1, compareOwn2);
    if (isAboveNoise){
        if (timestmp - lastSendingSpikeTime > 30000000) {// 30 ms
            //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT mag %f, at time %lu", magnitude -magnitude2, timestmp);
            lastSendingBroadcastTime = timestmp;
        }
        lastSendingSpikeTime = timestmp;
    }
}

float ExpoAvg(float sample, float avg, float w) {
    return w*sample + (1-w)*avg;
}

oboe::DataCallbackResult
AudioListenerCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    float *shortdata = (float *) audioData;
    uint64_t timestmp = getTimeNsec();

    // Complex resonator test
    bool prevDifference;
    float currentTotal = 0;

    // Convert shorts to float and copy
    for (int i = 0; i < numFrames; i++) {
        float data = shortdata[i];
        // put into the buffer to be decoded by the decoder thread
        //this->readerWriterQueue.enqueue(std::make_pair(timestmp, data));
        __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %llu, %f\n", timestmp, data);
        //broadcastFreqDetector->update(data, timestmp);
        //listenFreqDetector->update(data, timestmp);

        continue;
        __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %llu, %f\n", timestmp, data);

        if (this->dumpBuffer){
            __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %f\n",
                            data);
        }
        ////////
        /// listening floats
        float iir_out_0_old = iir_out_0;
        float iir_out_1_old = iir_out_1;
        iir_out_0 = iir_out_0_old*pole_0 - iir_out_1_old*pole_1 + data;
        iir_out_1 = iir_out_0_old*pole_1 - iir_out_1_old*pole_0;

        iir_out_0_old = junk1_iir_out_0;
        iir_out_1_old = junk1_iir_out_1;
        junk1_iir_out_0 = iir_out_0_old * junk1_pole_0 - iir_out_1_old * junk1_pole_1 + data;
        junk1_iir_out_1 = iir_out_0_old * junk1_pole_1 - iir_out_1_old * junk1_pole_0;

        iir_out_0_old = junk2_iir_out_0;
        iir_out_1_old = junk2_iir_out_1;
        junk2_iir_out_0 = iir_out_0_old * junk2_pole_0 - iir_out_1_old * junk2_pole_1 + data;
        junk2_iir_out_1 = iir_out_0_old * junk2_pole_1 - iir_out_1_old * junk2_pole_0;

        ////////
        /// broadcast floats
        float broadcast_iir_out_0_old = broadcast_iir_out_0;
        float broadcast_iir_out_1_old = broadcast_iir_out_1;
        broadcast_iir_out_0 = broadcast_iir_out_0_old * broadcast_pole_0 - broadcast_iir_out_1_old * broadcast_pole_1 + data;
        broadcast_iir_out_1 = broadcast_iir_out_0_old * broadcast_pole_1 - broadcast_iir_out_1_old * broadcast_pole_0;

        broadcast_iir_out_0_old = broadcast_junk1_iir_out_0;
        broadcast_iir_out_1_old = broadcast_junk1_iir_out_1;
        broadcast_junk1_iir_out_0 = broadcast_iir_out_0_old * broadcast_junk1_pole_0 - broadcast_iir_out_1_old * broadcast_junk1_pole_1 + data;
        broadcast_junk1_iir_out_1 = broadcast_iir_out_0_old * broadcast_junk1_pole_1 - broadcast_iir_out_1_old * broadcast_junk1_pole_0;

        broadcast_iir_out_0_old = broadcast_junk2_iir_out_0;
        broadcast_iir_out_1_old = broadcast_junk2_iir_out_1;
        broadcast_junk2_iir_out_0 = broadcast_iir_out_0_old * broadcast_junk2_pole_0 - broadcast_iir_out_1_old * broadcast_junk2_pole_1 + data;
        broadcast_junk2_iir_out_1 = broadcast_iir_out_0_old * broadcast_junk2_pole_1 - broadcast_iir_out_1_old * broadcast_junk2_pole_0;

        ////
        /// Edge Detect
        float iir_mag = iir_out_0*iir_out_0 + iir_out_1*iir_out_1;

        float iir_mag_max = .1;
        if (iir_mag > iir_mag_max){
            iir_out_0 = sqrtf(iir_mag_max/2);
            iir_out_1 = sqrtf(iir_mag_max/2);
            iir_mag = iir_out_0*iir_out_0 + iir_out_1*iir_out_1;
        }
        if (uninit){
            fastAvg = iir_mag;
            slowAvg = iir_mag;
            uninit = false;
        }
        fastAvg = ExpoAvg(iir_mag, fastAvg, 0.025);
        slowAvg = ExpoAvg(iir_mag, slowAvg, 0.0025);

        float difference = fabs(fastAvg - slowAvg);
        float threshold = .002;
        //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "fastAvg %lu, %f, %d\n",
        //                    timestmp, fastAvg, fast_avg_duration);
        /*
        __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "FAST_AVG %llu, %f\n",
                            timestmp, fastAvg);

        __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "SLOW_AVG %llu, %f\n",
                            timestmp, slowAvg);
        */

        bool isEdge = prevDifference < threshold && difference >= threshold;
        if (isEdge){
            //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "FOUND EDGE AT TIME %lu, %f\n",
            //                    timestmp, difference);
        }
        if(slowAvg > 0.00005){
            fast_avg_duration ++;
            currentTotal += slowAvg;
            //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "fastAvg %lu, %f, %d\n",
            //                    timestmp, fastAvg, fast_avg_duration);
            //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "fastAvg %lu, %f, %d\n",
            //                    timestmp, fastAvg, fast_avg_duration);
            //if (fast_avg_duration == 900){
            //    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "fastAvg %lu, %f, %d\n",
            //                       timestmp, fastAvg, fast_avg_duration);
                //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT",
                //        "Frequency %f, %lu, %f, %d\n", this->mListenFrequency,
                //                    timestmp, fastAvg, fast_avg_duration);
            //}

        }
        else {
            if (fast_avg_duration > 1000 && fast_avg_duration < 3000)
                //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "Threshold Lower %lu, %f, %d, %f\n",
                //                    timestmp, slowAvg, fast_avg_duration, currentTotal/fast_avg_duration);
            fast_avg_duration = 0;
        }
        prevDifference = difference;


    }
    this->broadcastFreqDetector->reset();
    this->listenFreqDetector->reset();
    dumpBuffer = false;
    return oboe::DataCallbackResult::Continue;


    float iir_mag = iir_out_0*iir_out_0 + iir_out_1*iir_out_1;
    float iir_noise_avg = (junk1_iir_out_0 * junk1_iir_out_0 + junk1_iir_out_1 * junk1_iir_out_1) +
                          (junk2_iir_out_0 * junk2_iir_out_0 + junk2_iir_out_1 * junk2_iir_out_1);

    iir_noise_avg /= 2;

    //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT IIR mag %f, at time %lu\n",
    //                    iir_mag , timestmp);

    if (iir_mag > 50000000){

        if (timestmp - lastListeningSpikeTime > 30000000) {// 30 ms
            __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT IIR mag %f, at time %lu\n",
                    iir_mag , timestmp);
            lastListeningBroadcastTime = timestmp;
        }
        lastListeningSpikeTime = timestmp;
    }

    float broadcast_iir_mag = broadcast_iir_out_0 * broadcast_iir_out_0 +
                              broadcast_iir_out_1 * broadcast_iir_out_1;

    float broadcast_iir_noise_avg =
            (broadcast_junk1_iir_out_0 * broadcast_junk1_iir_out_0 + broadcast_junk1_iir_out_1 * broadcast_junk1_iir_out_1) +
            (broadcast_junk2_iir_out_0 * broadcast_junk2_iir_out_0 + broadcast_junk2_iir_out_1 * broadcast_junk2_iir_out_1);

    broadcast_iir_noise_avg /= 2;

    if ( broadcast_iir_mag > 50000000){
        if (timestmp - lastSendingSpikeTime > 30000000) {// 30 ms
            __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT Broadcast IIR mag %f, at time %lu", broadcast_iir_mag, timestmp);
            lastSendingBroadcastTime = timestmp;
        }
        lastSendingSpikeTime = timestmp;
    }


    return oboe::DataCallbackResult::Continue;
}

void AudioListenerCallback::setFrequency(float d) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", d);
    mListenFrequency = static_cast<float>(d);
    index = (int)((double) (BUFFSIZE + 2) / kSampleRate * mListenFrequency);
    compare1 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (mListenFrequency - 1500));
    compare2 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (mListenFrequency + 1120));

    PI = 3.14159265358979323846f;
    this->listenFreqDetector = new AmpDetector(static_cast<float>(d), this->kSampleRate);

    this->broadcastFreqDetector = new AmpDetector(d,kSampleRate);

    dumpBuffer = true;

    //////
    /// Floats for Listening frequency
    iir_out_0 = 0.0f;
    iir_out_1 = 0.0f;
    angle = 2.0f * PI * mListenFrequency / atten;
    pole_0 = cosf(angle)*atten;
    pole_1 = sinf(angle)*atten;

    junk1_iir_out_0 = 0.0f;
    junk1_iir_out_1 = 0.0f;
    angle1 = static_cast<float>(2.0f * M_PI * (mListenFrequency - 50) / atten);
    junk1_pole_0 = cosf(angle1)*atten;
    junk1_pole_1 = sinf(angle1)*atten;

    junk2_iir_out_0 = 0.0f;
    junk2_iir_out_1 = 0.0f;
    angle2 = static_cast<float>(2.0f * M_PI * (mListenFrequency + 50) / atten);
    junk2_pole_0 = cosf(angle2)*atten;
    junk2_pole_1 = sinf(angle2)*atten;

}

void AudioListenerCallback::setOwnFrequency(float d) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", d);
    mBroadcastFrequency = d;

    this->broadcastFreqDetector = new AmpDetector(d,kSampleRate);
    indexOwn = (int)((double) (BUFFSIZE + 2) / kSampleRate * mBroadcastFrequency);
    compareOwn1 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (mBroadcastFrequency - 1500));
    compareOwn2 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (mBroadcastFrequency + 1120));

    //////
    /// Floats for Broadcast frequency
    broadcast_iir_out_0 = 0.0f;
    broadcast_iir_out_1 = 0.0f;
    broadcast_angle = static_cast<float>(2.0f * M_PI * mBroadcastFrequency / atten);
    broadcast_pole_0 = cosf(broadcast_angle)*atten;
    broadcast_pole_1 = sinf(broadcast_angle)*atten;

    broadcast_junk1_iir_out_0 = 0.0f;
    broadcast_junk1_iir_out_1 = 0.0f;
    broadcast_angle1 = static_cast<float>(2.0f * M_PI * (mBroadcastFrequency - 50) / atten);
    broadcast_junk1_pole_0 = cosf(broadcast_angle1)*atten;
    broadcast_junk1_pole_1 = sinf(broadcast_angle1)*atten;

    broadcast_junk2_iir_out_0 = 0.0f;
    broadcast_junk2_iir_out_1 = 0.0f;
    broadcast_angle2 = static_cast<float>(2.0f * M_PI * (mBroadcastFrequency + 50) / atten);
    broadcast_junk2_pole_0 = cosf(broadcast_angle2)*atten;
    broadcast_junk2_pole_1 = sinf(broadcast_angle2)*atten;
}

/// The decoder thread reads from the ring buffer continuously to attempt to decode
/// incoming messages
void AudioListenerCallback::decoderThread() {
    int result;
    std::pair<uint64_t, float> item;

    while (this->shouldContinueDecoding) {
        this->readerWriterQueue.wait_dequeue(item);

        //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT",
        //                    "POPPED OFF: %f, %llu", item.second, item.first);
    }


}


