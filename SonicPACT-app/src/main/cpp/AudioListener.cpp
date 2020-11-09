//
// Created by Michael Specter on 5/2/20.
//

#include <android/log.h>
#include <__locale>
#include "AudioListener.h"
//#include "Buffer.h"
#include "readerwriterqueue/readerwriterqueue.h"
#include "readerwriterqueue/atomicops.h"
#include "Logging.h"

using namespace moodycamel;

oboe::DataCallbackResult
AudioListenerCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    float *shortdata = (float *) audioData;
    if(!stopped){
        uint64_t timestmp = getTimeNsec();

        detector.update(shortdata, numFrames, timestmp);
        last_broadcast_seen = detector.last_broadcast_seen_finished;
        last_recv_seen = detector.last_recv_seen;
    }

    return oboe::DataCallbackResult::Continue;
}

void AudioListenerCallback::setFrequency(float frequency) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", frequency);
    mListenFrequency = frequency;
    //detector = new MatchedFilterDetector(mBroadcastFrequency, mListenFrequency, kSampleRate);
    dumpBuffer = true;
}

void AudioListenerCallback::setOwnFrequency(float frequency) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", frequency);
    mBroadcastFrequency = frequency;
    //detector = new MatchedFilterDetector(mBroadcastFrequency, mListenFrequency, kSampleRate);
}

