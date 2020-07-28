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
#include "BPSKSignalGenerator.h"
#include "RandomNoise.h"

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
        current_index   = 0;
        shouldBroadcast = true;
        has_broadcast_preamble = false;
    }

    void stopPlayback() {
        current_index   = 0;
        shouldBroadcast = false;
        has_broadcast_preamble = false;
    }

    void closeStream() {
        managedStream->close();
    }

    void setFrequency(double frequency) {
        shouldBroadcast = false;

        kFrequency = static_cast<float>(frequency);
        //generated_wave = GenerateBPSKPreamble(kFrequency);
        //generated_wave = GenerateRandom("leader");
        updatePhaseIncrement();
    };

    void setLeader(bool isLeader) {
        shouldBroadcast = false;
        if (isLeader)
            generated_wave = GenerateRandom("follower");
        else
            generated_wave = GenerateRandom("leader");

        updatePhaseIncrement();
    };


    oboe::ManagedStream managedStream;

    long lastBroadcastTime = 0;
    bool has_broadcast_preamble = false;
    uint64_t last_broadcast_sent = 0;

    std::atomic<float> mPhaseIncrement {kFrequency * kTwoPi /  SAMPLE_RATE};

private:
    /// Stream params
    float kFrequency = 18000.0;
    int current_index = 0;

    std::vector<float> generated_wave = GenerateRandom("follower");

    bool shouldBroadcast = false;
    bool isBroadcasting = false;

    static float constexpr PI = static_cast<const float>(M_PI);
    static float constexpr kTwoPi = PI * 2;

    void updatePhaseIncrement(){
        mPhaseIncrement.store((kTwoPi * kFrequency) / SAMPLE_RATE);
    }

};


#define SONICPACT_AUDOGENERATOR_H

#endif //SONICPACT_AUDOGENERATOR_H
