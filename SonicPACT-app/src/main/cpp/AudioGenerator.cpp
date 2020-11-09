//
// Created by Michael Specter on 5/2/20.
//

#include <vector>
#include "AudioGenerator.h"
#include "AudioListener.h"
#include "Logging.h"

oboe::DataCallbackResult
AudioGeneratorCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    float *floatData = (float *) audioData;

    if (shouldBroadcast) {

        isBroadcasting = true;
        if (current_index + numFrames < generated_wave.size()) {
            memcpy(floatData, &generated_wave[current_index], numFrames*sizeof(float));
            current_index += numFrames;
        }
        else {
            auto len = current_index + numFrames - generated_wave.size();
            memcpy(floatData, &generated_wave[current_index], len*sizeof(float));
            last_broadcast_sent = getTimeNsec();
            memset(floatData+len, 0, sizeof(float) * len);
            LOGE("FINISHED BROADCASTING %llu", last_broadcast_sent);
            shouldBroadcast = false;
            hasFinishedBroadcasting = true;
            current_index = 0;
        }

    }
    else {
        isBroadcasting = false;
        memset(audioData, 0, sizeof(float) * numFrames);
    }

    return oboe::DataCallbackResult::Continue;
}
