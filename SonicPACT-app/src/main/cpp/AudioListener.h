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
    uint64_t lastListeningBroadcastTime = 0;

    void setFrequency(double d);

    void setOwnFrequency(double d);

    static int constexpr kSampleRate = 48000;
private:

    static int constexpr kChannelCount = 1;

    // Last time we saw a spike
    uint64_t lastListeningSpikeTime = 0;
    float mBroadcastFrequency = 14000;
    float mListenFrequency = 15000;

    /////////////////////////////////////////////////////////////////////////////////
    // IIR-based detection
    uint64_t lastSendingSpikeTime;
    uint64_t lastSendingBroadcastTime;

    // impulse control?
    float atten = .99f;

    //////
    /// Floats for Listening frequency

    float iir_out_0 = 0.0f;
    float iir_out_1 = 0.0f;
    float angle = static_cast<float>(2.0f * M_PI * mListenFrequency / atten);
    float pole_0 = cosf(angle)*atten;
    float pole_1 = sinf(angle)*atten;

    float junk1_iir_out_0 = 0.0f;
    float junk1_iir_out_1 = 0.0f;
    float angle1 = static_cast<float>(2.0f * M_PI * mListenFrequency - 200 / atten);
    float junk1_pole_0 = cosf(angle1)*atten;
    float junk1_pole_1 = sinf(angle1)*atten;

    float junk2_iir_out_0 = 0.0f;
    float junk2_iir_out_1 = 0.0f;
    float angle2 = static_cast<float>(2.0f * M_PI * mListenFrequency + 150 / atten);
    float junk2_pole_0 = cosf(angle2)*atten;
    float junk2_pole_1 = sinf(angle2)*atten;

    //////
    /// Floats for Broadcast frequency

    float broadcast_iir_out_0 = 0.0f;
    float broadcast_iir_out_1 = 0.0f;
    float broadcast_angle = static_cast<float>(2.0f * M_PI * mBroadcastFrequency / atten);
    float broadcast_pole_0 = cosf(broadcast_angle)*atten;
    float broadcast_pole_1 = sinf(broadcast_angle)*atten;

    float broadcast_junk1_iir_out_0 = 0.0f;
    float broadcast_junk1_iir_out_1 = 0.0f;
    float broadcast_angle1 = static_cast<float>(2.0f * M_PI * mBroadcastFrequency - 200 / atten);
    float broadcast_junk1_pole_0 = cosf(broadcast_angle1)*atten;
    float broadcast_junk1_pole_1 = sinf(broadcast_angle1)*atten;

    float broadcast_junk2_iir_out_0 = 0.0f;
    float broadcast_junk2_iir_out_1 = 0.0f;
    float broadcast_angle2 = static_cast<float>(2.0f * M_PI * mBroadcastFrequency + 150 / atten);
    float broadcast_junk2_pole_0 = cosf(broadcast_angle2)*atten;
    float broadcast_junk2_pole_1 = sinf(broadcast_angle2)*atten;

    /////////////////////////////////////////////////////////////////////////////////
    // FFT-based detection
    bool useFFT = false;
    int64_t currentBufferIndx = 0;
    kiss_fft_cpx *fft_result;
    kiss_fftr_cfg fft_config;
    int indexOwn;
    int compareOwn1;
    int compareOwn2;

    // Wave params, these could be instance variables in order to modify at runtime
    int index = (int)((double) (BUFFSIZE + 2) / kSampleRate * mListenFrequency);
    int compare1 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (mListenFrequency - 500));
    int compare2 = (int)( (double) (BUFFSIZE + 2) / kSampleRate * (mListenFrequency + 300));
    float audiobuffer[BUFFSIZE+2] = {0};
    bool shouldTakeMeasure = true;
    uint64_t startNS = 0;
    bool isMagnitudeAboveNoise(int index, int compare1, int compare2);
    double getMagnitudeAboveNoise(int index, int compare1, int compare2);
    oboe::DataCallbackResult do_fft_detect(void *audioData, int32_t numFrames);

};

static AudioListenerCallback toneListenerCallback;
#endif //SONICPACT_AUDIOLISTENER_H
