//
// Created by Michael Specter on 5/2/20.
//

#ifndef SONICPACT_AUDOGENERATOR_H
#include <oboe/Oboe.h>
#include <math.h>
#include <thread>

#define TONE_FREQ 21000; // hz

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
        shouldBroadcast=true;
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
private:
    // Stream params
    // Keeps track of where the wave is
    float mPhase = 0.0;
    float kFrequency = 15000;
    std::atomic<float> mPhaseIncrement {kFrequency * kTwoPi /  kSampleRate};

    static int constexpr kChannelCount = 1;
    static int constexpr kSampleRate = 48000;
    // Wave params, these could be instance variables in order to modify at runtime
    static float constexpr kAmplitude = 0.5f;
//    static float constexpr mListenFrequency = 21000;
    static float constexpr kPI = static_cast<const float>(M_PI);
    static float constexpr kTwoPi = kPI * 2;
    bool shouldBroadcast = false;
    bool isBroadcasting = false;
    float rampInc = .0001;
    float currentAmp = .01;

    void updatePhaseIncrement(){
        mPhaseIncrement.store((kTwoPi * kFrequency) / kSampleRate);
    };
};

static AudioGeneratorCallback toneGeneratorCallback;

#define SONICPACT_AUDOGENERATOR_H

#endif //SONICPACT_AUDOGENERATOR_H
