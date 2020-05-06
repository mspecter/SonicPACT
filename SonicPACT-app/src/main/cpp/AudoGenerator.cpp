//
// Created by Michael Specter on 5/2/20.
//

#include <vector>
#include "AudoGenerator.h"

oboe::DataCallbackResult
AudioGeneratorCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    // We requested AudioFormat::Float so we assume we got it.
    // For production code always check what format
    // the stream has and cast to the appropriate type.
    if (this->shouldBroadcast) {
        this->isBroadcasting = true;
        float *floatData = (float *) audioData;
        for (int i = 0; i < numFrames; ++i) {
            float sampleValue = kAmplitude * sinf(mPhase);
            //for (int j = 0; j < kChannelCount; j++)
            floatData[i] = sampleValue;

            mPhase += mPhaseIncrement;
            if (mPhase >= kTwoPi)
                mPhase -= kTwoPi;
        }
    } else {
        this->isBroadcasting = false;
        memset(audioData, 0, sizeof(float) * numFrames);
    }
    return oboe::DataCallbackResult::Continue;
}
