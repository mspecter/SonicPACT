//
// Created by Michael Specter on 5/2/20.
//

#include <android/log.h>
#include "AudioListener.h"

oboe::DataCallbackResult
AudioListenerCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {

    short *shortdata = (short *) audioData;
    uint64_t timestmp = getTimeNsec();

    // Convert shorts to float and copy
    for (int i = 0; i < numFrames ; i++) {
        audiobuffer[currentBufferIndx] = (float)shortdata[i];
        currentBufferIndx ++;
        if (currentBufferIndx >= BUFFSIZE)
            break;
    }

    // THe bufer may not be full
    if (currentBufferIndx < BUFFSIZE)
        return oboe::DataCallbackResult::Continue;

    currentBufferIndx = 0;

    kiss_fftr(cfg->config, audiobuffer, cfg->result);
    //__android_log_print(ANDROID_LOG_DEBUG, "TEST", "index %d, magnitude %lu",
    //                    current_max_index, max_magnitude);
    // Get frequency index
    float real = cfg->result[index].r;
    float imaginary = cfg->result[index].i;
    double magnitude = sqrt((double)real*real + (double)imaginary*imaginary);

    real = cfg->result[compare].r;
    imaginary = cfg->result[compare].i;
    double magnitude2 = sqrt((double)real*real + (double)imaginary*imaginary);


    //__android_log_print(ANDROID_LOG_DEBUG, "TEST", "GOT mag %f, at time %lu",
    //                   magnitude, timestmp);

    //__android_log_print(ANDROID_LOG_DEBUG, "NATIVE_PACT", "GOT mag %f, at time %lu", magnitude, timestmp);

    if (magnitude - magnitude2 > 30000){
        uint64_t time = timestmp - startNS;
         if (timestmp - lastSpikeTime > 30000000) {// 5 ms
             lastSpikeStartTime = timestmp;
             __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT mag %f, at time %lu", magnitude, timestmp);
         }
         lastSpikeTime = timestmp;
    }
    else{
        isContiuing = false;
    }

    return oboe::DataCallbackResult::Continue;
}