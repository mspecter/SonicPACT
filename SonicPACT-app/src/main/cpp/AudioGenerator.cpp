//
// Created by Michael Specter on 5/2/20.
//

#include <vector>
#include "AudioGenerator.h"
#include "AudioListener.h"

// Simple BPSK sequence!

oboe::DataCallbackResult
AudioGeneratorCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    // We requested AudioFormat::Float so we assume we got it.
    // For production code always check what format
    // the stream has and cast to the appropriate type.
    float *floatData = (float *) audioData;
    bool rampIsRequired = false;

    if (shouldBroadcast) {
        if (!isBroadcasting){
            rampIsRequired = true;
        }
        isBroadcasting = true;
    }
    else {
        memset(audioData, 0, sizeof(float) * numFrames);
        return oboe::DataCallbackResult::Continue;
    }

    for (int i = 0; i < numFrames; i++){
        int index = current_index + i;

        if (index < generated_wave.size()){
            floatData[i] = generated_wave[index];
// __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %f\n", generated_wave[index]);
        }
        else {
            floatData[i] = 0;
            lastBroadcastTimestamp = getTimeNsec();
            has_broadcast_preamble = true;
        }
    }

    current_index += numFrames;

    if(rampIsRequired)
        this->lastBroadcastTime = static_cast<long>(getTimeNsec());

    return oboe::DataCallbackResult::Continue;
}
