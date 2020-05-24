//
// Created by Michael Specter on 5/2/20.
//

#ifndef SONICPACT_AUDOGENERATOR_H
#include <oboe/Oboe.h>
#include <math.h>
#include <thread>
#include <android/log.h>
#include <Constants.h>
#include "Timing.h"

class AudioGeneratorCallback : public oboe::AudioStreamCallback {
public:
    ~AudioGeneratorCallback(){
        closeStream();
    }

    // override to handle AudioStreamCallbacks
    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);

    void init() {
        managedStream->requestStart();
    }

    void startPlayback() {
        current_bit_index = 0;
        current_bit = PREAMBLE[0];
        has_broadcast_preamble = false;
        current_index_into_symbol = 0;

        // Reset the flipping time
        transition_time   = 0;
        new_symbol = true;
        should_ramp = true;

        message_i = 0;

        if (current_bit){
            new_i = 1;
        }
        else {
            new_i = -1;
        }

        shouldBroadcast=true;
        mPhase = 0;

    }

    void stopPlayback() {
        shouldBroadcast=false;
    }

    void closeStream() {
        managedStream->close();
    }

    void setFrequency(double frequency) {
        kFrequency = frequency;
        updatePhaseIncrement();
    };


    oboe::ManagedStream managedStream;

    long lastBroadcastTime = 0;
    bool has_broadcast_preamble = false;

    std::atomic<float> mPhaseIncrement {kFrequency * kTwoPi /  SAMPLE_RATE};

private:

    /// Stream params
    float mPhase = 0.0;
    float kFrequency = 18000.0;

    bool shouldBroadcast = false;
    bool isBroadcasting = false;

    static float constexpr PI = static_cast<const float>(M_PI);
    static float constexpr kTwoPi = PI * 2;

    /////////////////////////////////////////////////////////////////////////////////
    /// BPSK implementation
    float message_i = 0;
    float message_q = 0;
    float new_i = 0;
    float new_q = 0; // todo: use new_q for qpsk or other changes

    int current_index_into_symbol = 0;

    int transition_time = 0;
    bool new_symbol = false;

    int current_bit = 0;
    int current_bit_index = 0;

    ///
    /// Provides a smooth s-curve for transition between -1 and 1
    /// \param edge0
    /// \param edge1
    /// \param x
    /// \return
    float smootherstep(float edge0, float edge1, float x) {
        // Scale, and clamp x to 0..2 range
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 2.0);
        // Evaluate polynomial
        return x * x * x * (x * (x * 6 - 15) + 10);
    }

    float clamp(float x, float lowerlimit, float upperlimit) {
        if (x < lowerlimit)
            x = lowerlimit;
        if (x > upperlimit)
            x = upperlimit;
        return x;
    }

    void updatePhaseIncrement(){
        mPhaseIncrement.store((kTwoPi * kFrequency) / SAMPLE_RATE);
    }

    bool should_ramp = false;
};

static AudioGeneratorCallback toneGeneratorCallback;

#define SONICPACT_AUDOGENERATOR_H

#endif //SONICPACT_AUDOGENERATOR_H
