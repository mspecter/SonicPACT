//
// Created by Michael Specter on 5/2/20.
//

#ifndef SONICPACT_AUDIOLISTENER_H
#define SONICPACT_AUDIOLISTENER_H
#include <oboe/Oboe.h>
#include <math.h>
#include <thread>

#include "kissfft/tools/kiss_fftr.h"
#include "kissfft/kiss_fft.h"
#include "AmpDetector.h"
#include "readerwriterqueue/readerwriterqueue.h"
#include "Timing.h"
#include "MatchedFilterDetector.h"

using namespace moodycamel;

#define TONE_FREQ 21000; // hz

#define BUFFSIZE 256


// Preamble for the char

class AudioListenerCallback : public oboe::AudioStreamCallback {
public:
    ~AudioListenerCallback(){
        closeStream();
    }

    AudioListenerCallback(){
        //cfg = (KissRealConfig *) malloc(sizeof(KissRealConfig));
        //fft_config = kiss_fftr_alloc(BUFFSIZE, 0, 0, 0);
        //fft_result = (kiss_fft_cpx *) malloc(sizeof(kiss_fft_cpx) * BUFFSIZE + 2);

        // generate the matched listener
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


    oboe::ManagedStream managedStream;
    uint64_t lastListeningBroadcastTime = 0;

    void setFrequency(float frequency);
    void setOwnFrequency(float frequency);

    static int constexpr kSampleRate = 48000;
private:

    static int constexpr kChannelCount = 1;

    // Last time we saw a spike
    uint64_t lastListeningSpikeTime = 0;
    float mBroadcastFrequency = 18000;
    float mListenFrequency = 18000;

    AmpDetector *listenFreqDetector    = new AmpDetector(mListenFrequency, kSampleRate);
    AmpDetector *broadcastFreqDetector = new AmpDetector(mBroadcastFrequency, kSampleRate);

    MatchedFilterDetector *detector = new MatchedFilterDetector(mListenFrequency, kSampleRate);


    bool shouldTakeMeasure = true;
    uint64_t startNS = 0;

    bool dumpBuffer = false;
};

static AudioListenerCallback toneListenerCallback;
#endif //SONICPACT_AUDIOLISTENER_H
