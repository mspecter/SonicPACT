//
// Created by Michael Specter on 5/2/20.
//

#include <android/log.h>
#include <__locale>
#include "AudioListener.h"
//#include "Buffer.h"
#include "readerwriterqueue/readerwriterqueue.h"
#include "readerwriterqueue/atomicops.h"

using namespace moodycamel;

oboe::DataCallbackResult
AudioListenerCallback::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    float *shortdata = (float *) audioData;
    uint64_t timestmp = getTimeNsec();

    detector->update(shortdata, numFrames, timestmp);

    // Convert shorts to float and copy
    for (int i = 0; i < numFrames; i++) {
        float data = shortdata[i];
        // put into the buffer to be decoded by the decoder thread
        ///__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %llu, %f\n", timestmp, data);
        //broadcastFreqDetector->update(data, timestmp);
        //listenFreqDetector->update(data, timestmp);

        if (dumpBuffer){
            __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %f\n",
                            data);
        }


    }
    this->broadcastFreqDetector->reset();
    this->listenFreqDetector->reset();
    dumpBuffer = false;
    return oboe::DataCallbackResult::Continue;
}

void AudioListenerCallback::setFrequency(float frequency) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", frequency);

    mListenFrequency = frequency;

    detector = new MatchedFilterDetector(mListenFrequency, kSampleRate);
    listenFreqDetector    = new AmpDetector(frequency, kSampleRate);
    broadcastFreqDetector = new AmpDetector(frequency, kSampleRate);

    dumpBuffer = true;

}

void AudioListenerCallback::setOwnFrequency(float frequency) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", frequency);
    mBroadcastFrequency = frequency;
    detector = new MatchedFilterDetector(mListenFrequency, kSampleRate);
    broadcastFreqDetector = new AmpDetector(frequency , kSampleRate);
}

