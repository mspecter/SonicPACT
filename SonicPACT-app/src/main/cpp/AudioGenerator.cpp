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

        for (int i = 0; i < numFrames; i++) {
            int index = current_index + i;

            if (index < generated_wave.size())
                floatData[i] = generated_wave[index];
            else {
                floatData[i] = 0;
                if (!has_broadcast_preamble) {
                    last_broadcast_sent = getTimeNsec();
                    LOGE("FINISHED BROADCASTING %llu", last_broadcast_sent);
                    has_broadcast_preamble = true;
                }
            }
        }

        current_index += numFrames;
    }
    else {
        memset(audioData, 0, sizeof(float) * numFrames);
    }

    return oboe::DataCallbackResult::Continue;
}
