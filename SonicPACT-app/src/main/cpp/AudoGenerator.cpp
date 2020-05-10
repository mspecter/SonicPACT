//
// Created by Michael Specter on 5/2/20.
//

#include <vector>
#include <android/log.h>
#include "AudoGenerator.h"
#include "AudioListener.h"

oboe::DataCallbackResult
AudioGeneratorCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    // We requested AudioFormat::Float so we assume we got it.
    // For production code always check what format
    // the stream has and cast to the appropriate type.
    float *floatData = (float *) audioData;
    bool takeTimestamp = false;

    if (shouldBroadcast) {
        if (!isBroadcasting){
            takeTimestamp = true;
            currentAmp = 1.0f;
        }
        isBroadcasting = true;

        //currentAmp *= 1.1;
        if (currentAmp > kAmplitude)
            currentAmp = kAmplitude;
    }
    else {
        memset(audioData, 0, sizeof(float) * numFrames);
        currentAmp = .002;
        isBroadcasting = false;
        mPhase = 0;
        return oboe::DataCallbackResult::Continue;
        //currentAmp = 0;
        if(currentAmp <.01) {
            memset(audioData, 0, sizeof(float) * numFrames);
            currentAmp = .002;
            return oboe::DataCallbackResult::Continue;
        }
        currentAmp /= 1.1f;
    }


    for (int i = 0; i < numFrames; i++) {
        // Actual sample value:
        // amplitude * sin(2 * pi * freq * i + phase)
        //floatData[i] = this->kAmplitude * sinf(mPhase);
        floatData[i] =  sinf(mPhase);

        // TODO: Flipping the phase!!!

        mPhase += mPhaseIncrement;
        if (mPhase >= kTwoPi)
            mPhase -= kTwoPi;
    }

    if(takeTimestamp)
        this->lastBroadcastTime = getTimeNsec();

    return oboe::DataCallbackResult::Continue;
}
