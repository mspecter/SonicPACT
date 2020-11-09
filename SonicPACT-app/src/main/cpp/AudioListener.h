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

    // override to handle AudioStreamCallbacks
    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);

    void startRecord() {
        stopped = false;
        managedStream->requestStart();
    }

    void stopRecord() {
        stopped = true;
        managedStream->requestStop();
    }

    void closeStream() {
        managedStream->close();
    }

    void beginTimer() {
        shouldTakeMeasure = true;
    }

    uint64_t getLastBroadcastSeen(){
        return detector.last_broadcast_seen_finished;
    }

    uint64_t getLastRecvSeen(){
        return detector.last_recv_seen;
    }

    oboe::ManagedStream managedStream;
    uint64_t last_broadcast_seen = 0;
    uint64_t last_recv_seen = 0;

    void setFrequency(float frequency);
    void setOwnFrequency(float frequency);

    void isLeader(bool isLeader){

        //this->stopped = true;
        detector.setIsLeader(isLeader);
        //startRecord();
    }

    static int constexpr kSampleRate = 48000;
private:

    static int constexpr kChannelCount = 1;
    std::mutex detectorMTX;

    // Last time we saw a spike
    float mBroadcastFrequency = 18000;
    float mListenFrequency = 18000;


    MatchedFilterDetector &detector = MatchedFilterDetector::getInstance(kSampleRate, true);


    bool dumpBuffer = false;
    bool stopped = false;
};

#endif //SONICPACT_AUDIOLISTENER_H
