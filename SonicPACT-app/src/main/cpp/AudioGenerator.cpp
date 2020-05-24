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
        isBroadcasting = false;
        current_index_into_symbol = 0;
        has_broadcast_preamble = false;
        current_bit_index = 0;
        current_bit = PREAMBLE[0];
        // reset the phase
        mPhase = 0;
        return oboe::DataCallbackResult::Continue;
    }

    for (int i = 0; i < numFrames; i++) {

        // Updates i & q values
        if (!new_symbol)
            current_index_into_symbol++;


        // Change to the next symbol!
        if (current_index_into_symbol > SAMPLES_PER_SYMBOL) {
            current_index_into_symbol = 0;
            current_bit_index++;
            new_symbol = true;
            transition_time  = 0;

            if (current_bit_index >= PREAMBLE_LEN){
                current_bit_index  = 0;
                has_broadcast_preamble = true;
                new_symbol = true;
                should_ramp = true;
                current_bit = PREAMBLE[current_bit_index];
                new_i = 0;
            }
            else if (PREAMBLE[current_bit_index] != current_bit){
                current_bit = PREAMBLE[current_bit_index];
                if (current_bit)
                    new_i = 1;
                else
                    new_i = -1;
            }
        }


        float current_i = message_i;

        if (new_symbol) {
            // If we should change phase, we need to be able to follow a transition that won't click
            // This requires using a simple s-curve (which is provided by smootherstep)

            if (should_ramp){
                // We're ramping either up from 0 to +-1 or from 1 to 0

               if (new_i != 0){
                   // we're ramping to +/-1 from 0
                   current_i += smootherstep(0, 1, 1 / SAMPLES_PER_TRANSITION * transition_time) * new_i;
               }
               else {
                   // we're ramping down to 0
                   float direction = message_i * -1;
                   current_i += smootherstep(0, 1, 1 / SAMPLES_PER_TRANSITION * transition_time) * direction;
               }

            }
            else
                // Ramps from -1 to 1 or 1 to -1
                current_i += smootherstep(0, 1, 1 / SAMPLES_PER_TRANSITION * transition_time) * 2 * new_i;

            if ( current_i>= 1)
                 current_i= 1;
            if ( current_i<= -1)
                 current_i= -1;

            transition_time++;

            if (transition_time >= SAMPLES_PER_TRANSITION){
                message_i = new_i;
                current_i = new_i;
                new_symbol = false;
                should_ramp = false;
                transition_time = 0;
            }
        }
        // actual modulation!
        // note that messageq is always 0, so sinf kept for posterity
        floatData[i] = current_i * cosf(mPhase) + message_q * sinf(mPhase);

        // Increment current phase
        mPhase += mPhaseIncrement;
        if (mPhase >= kTwoPi)
           mPhase -= kTwoPi;
    }

    if(rampIsRequired)
        this->lastBroadcastTime = static_cast<long>(getTimeNsec());

    return oboe::DataCallbackResult::Continue;
}
