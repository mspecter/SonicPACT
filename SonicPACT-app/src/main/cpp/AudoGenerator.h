//
// Created by Michael Specter on 5/2/20.
//

#ifndef SONICPACT_AUDOGENERATOR_H
#include <oboe/Oboe.h>
#include <math.h>

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
        //managedStream->requestStart();
        this->shouldBroadcast=true;
    }

    void stopPlayback() {
        this->shouldBroadcast=false;
        //managedStream->requestStop();
    }

    void closeStream() {
        managedStream->close();
    }

    oboe::ManagedStream managedStream;
private:
    // Stream params
    // Keeps track of where the wave is
    float mPhase = 0.0;
    static int constexpr kChannelCount = 1;
    static int constexpr kSampleRate = 48000;
    // Wave params, these could be instance variables in order to modify at runtime
    static float constexpr kAmplitude = 0.99f;
//    static float constexpr kFrequency = 21000;
    static float constexpr kFrequency = 15000;
    static float constexpr kPI = M_PI;
    static float constexpr kTwoPi = kPI * 2;
    static double constexpr mPhaseIncrement = kFrequency * kTwoPi / (double) kSampleRate;
    bool shouldBroadcast = false;
};

static AudioGeneratorCallback toneGeneratorCallback;

#define SONICPACT_AUDOGENERATOR_H

#endif //SONICPACT_AUDOGENERATOR_H
