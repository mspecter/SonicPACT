//
// Created by Michael Specter on 5/2/20.
//

#ifndef SONICPACT_AUDIOLISTENER_H
#define SONICPACT_AUDIOLISTENER_H
#include <oboe/Oboe.h>
#include <math.h>

#include "kissfft/tools/kiss_fftr.h"
#include "kissfft/kiss_fft.h"

#define TONE_FREQ 21000; // hz

#define BUFFSIZE 256

static int64_t getTimeNsec() {
    struct timespec now;
    clock_gettime(CLOCK_BOOTTIME, &now);
    return (int64_t) now.tv_sec*1000000000LL + now.tv_nsec;
}

class AudioListenerCallback : public oboe::AudioStreamCallback {
public:
    ~AudioListenerCallback(){
        closeStream();
    }
    AudioListenerCallback(){
        //cfg = (KissRealConfig *) malloc(sizeof(KissRealConfig));
        fft_config = kiss_fftr_alloc(BUFFSIZE, 0, 0, 0);
        fft_result = (kiss_fft_cpx *) malloc(sizeof(kiss_fft_cpx) * BUFFSIZE + 2);
    }
    // override to handle AudioStreamCallbacks
    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);

    void startRecord() {
        managedStream->requestStart();
    }

    void stopRecord() {
        managedStream->requestStop();
    }

    void closeStream() {
        managedStream->close();
    }

    void beginTimer() {
        startNS = getTimeNsec();
        shouldTakeMeasure = true;
    }

    void getFFT(float* src, float* dest);

    oboe::ManagedStream managedStream;
    static int constexpr kChannelCount = 1;
    static int constexpr kSampleRate = 48000;
    float kFrequency = 15000;
    uint64_t lastListeningBroadcastTime = 0;

    void setFrequency(double d);

    void setOwnFrequency(double d);

private:
    kiss_fft_cpx *fft_result;
    kiss_fftr_cfg fft_config;
    
    int64_t currentBufferIndx = 0;
    // Last time we saw a spike
    uint64_t lastListeningSpikeTime = 0;

    // Wave params, these could be instance variables in order to modify at runtime
    int index = (int)( (double) (BUFFSIZE + 2) / kSampleRate * kFrequency);
    int compare1 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kFrequency-500));
    int compare2 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kFrequency+300));
    float audiobuffer[BUFFSIZE+2] = {0};
    bool shouldTakeMeasure = true;
    uint64_t startNS = 0;

    double kOwnFrequency;
    int indexOwn;
    int compareOwn1;
    int compareOwn2;

    double getMagnitudeAboveNoise(int index, int compare1, int compare2);

    bool isMagnitudeAboveNoise(int index, int compare1, int compare2);

    uint64_t lastSendingSpikeTime;
    uint64_t lastSendingBroadcastTime;

    // impulse control?
    float atten = .99f;
    float iir_out_0 = 0.0f;
    float iir_out_1 = 0.0f;

};

static AudioListenerCallback toneListenerCallback;
#endif //SONICPACT_AUDIOLISTENER_H
