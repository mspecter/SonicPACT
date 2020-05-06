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

typedef struct {
    kiss_fft_cpx *result;
    kiss_fftr_cfg config;
} KissRealConfig;

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
        cfg = (KissRealConfig *) malloc(sizeof(KissRealConfig));
        cfg->config = kiss_fftr_alloc(BUFFSIZE, 0, 0, 0);
        cfg->result = (kiss_fft_cpx *) malloc(sizeof(kiss_fft_cpx) * BUFFSIZE + 2);
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
    static float constexpr kFrequency = 15000;
    uint64_t lastSpikeStartTime = 0;
private:
    int64_t currentBufferIndx = 0;
    // Last time we saw a spike
    uint64_t lastSpikeTime = 0;

    // Wave params, these could be instance variables in order to modify at runtime
    static int constexpr index = (int)( (double) (BUFFSIZE + 2) / kSampleRate * kFrequency);
    static int constexpr compare = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (kFrequency-1000));
    float audiobuffer[BUFFSIZE+2] = {0};
    float fftbuffer[4098+2] = {0};
    KissRealConfig *cfg;
    bool shouldTakeMeasure = true;
    bool isContiuing = false;
    uint64_t startNS = 0;

};

static AudioListenerCallback toneListenerCallback;
#endif //SONICPACT_AUDIOLISTENER_H
