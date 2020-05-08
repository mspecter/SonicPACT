//
// Created by Michael Specter on 5/2/20.
//

#include <android/log.h>
#include "AudioListener.h"


oboe::DataCallbackResult
AudioListenerCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    short *shortdata = (short *) audioData;
    uint64_t timestmp = getTimeNsec();
    // Complex resonator test
    /*
    // y(t) = x(t) + y(t-1)*(cos ω + i sin ω)
    var iir_out = [0,0];
    var angle = 2*Math.PI*16/512;
    var atten = 0.995;
    var pole = [Math.cos(angle)*atten, Math.sin(angle)*atten];

    function complexResonator(x, y, pole) {
        return [y[0]*pole[0] - y[1]*pole[1] + x,
                y[0]*pole[1] + y[1]*pole[0]];
    }

    iir_out = complexResonator(sample, iir_out, pole);
    var iir_mag = iir_out[0]*iir_out[0] + iir_out[1]*iir_out[1];
    */
    float angle = static_cast<float>(2.0f * M_PI * kFrequency / atten);
    float pole_0 = cosf(angle)*atten;
    float pole_1 = sinf(angle)*atten;

    // Convert shorts to float and copy
    for (int i = 0; i < numFrames ; i++) {
        float data = (float) shortdata[i];
        float iir_out_0_old = iir_out_0;
        float iir_out_1_old = iir_out_1;
        iir_out_0 = iir_out_0_old*pole_0 - iir_out_1_old*pole_1 + data;
        iir_out_1 = iir_out_0_old*pole_1 - iir_out_1_old*pole_0;
        audiobuffer[currentBufferIndx] = (float)shortdata[i];
        currentBufferIndx ++;
        if (currentBufferIndx >= BUFFSIZE)
            break;
    }
    float iir_mag = iir_out_0*iir_out_0 + iir_out_1*iir_out_1;
    if (iir_mag > 50000000)
        __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT IIR mag %f, at time %lu", iir_mag, timestmp);



    // THe bufer may not be full
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

    return oboe::DataCallbackResult::Continue;
}

void AudioListenerCallback::setFrequency(double d) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", d);
    kFrequency = d;
    index = (int)( (double) (BUFFSIZE + 2) / kSampleRate * kFrequency);
    compare1 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kFrequency-1500));
    compare2 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kFrequency+1120));
}

void AudioListenerCallback::setOwnFrequency(double d) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", d);
    kOwnFrequency = d;
    indexOwn = (int)( (double) (BUFFSIZE + 2) / kSampleRate * kOwnFrequency);
    compareOwn1 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kOwnFrequency-1500));
    compareOwn2 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kOwnFrequency+1120));
}

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


    return 0;
}

