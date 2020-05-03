//
// Created by Michael Specter on 5/2/20.
//

#include <android/log.h>
#include "AudioListener.h"

oboe::DataCallbackResult
AudioListenerCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    // We requested AudioFormat::Float so we assume we got it.
    // For production code always check what format
    // the stream has and cast to the appropriate type.
    short *shortdata = (short *) audioData;
    uint64_t timestmp = getTimeNsec();

    // Convert shorts to float and copy
    for (int i = 0; i < BUFFSIZE; i++) {
        audiobuffer[i] = (float)shortdata[i];
    }

    kiss_fftr(cfg->config, audiobuffer, cfg->result);

    // Get frequency index
    int index = (int)( (double) (BUFFSIZE + 2) / kSampleRate * kFrequency);
    float real = cfg->result[index].r;
    float imaginary = cfg->result[index].i;
    double magnitude = sqrt((double)real*real + (double)imaginary*imaginary);

    if (magnitude > 100000){
        if (shouldTakeMeasure){
            uint64_t time = timestmp - startNS;
            __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "GOT SPIKE %f, at time %lu", magnitude, time);
            shouldTakeMeasure = false;
        }
    }

    return oboe::DataCallbackResult::Continue;
}