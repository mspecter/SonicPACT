#include <jni.h>
#include <string>
#include "AudioGenerator.h"
#include "Logging.h"
#include "AudioListener.h"
#include "Timing.h"
#include <opencv2/core/core.hpp>

using namespace cv;

std::atomic<bool> shouldContinueChirpingAtInterval {true};
std::atomic<bool> isCurrentlyLeader {true};

static uint64_t chirp_delay;

static AudioListenerCallback* toneListenerCallback = nullptr;
static AudioGeneratorCallback* toneGeneratorCallback = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StartPlayback(JNIEnv *env, jclass clazz) {
    LOGE("BEGIN PLAYBACK NATIVE");
    // Starts the playback of the tone
    toneGeneratorCallback->startPlayback();
    toneListenerCallback->beginTimer();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StopPlayback(JNIEnv *env, jclass clazz) {
    // Stops the playback of the tone
    LOGE("STOP PLAYBACK NATIVE");
    toneGeneratorCallback->stopPlayback();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_InitPlaybackCallbacks(JNIEnv *env, jclass clazz) {

    LOGE("INIT PLAYBACK CALLBACKS");
    toneGeneratorCallback = new AudioGeneratorCallback();
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    //builder.setBufferCapacityInFrames(128);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(1);
    builder.setSampleRate(SAMPLE_RATE);
    builder.setCallback(toneGeneratorCallback);
    oboe::Result result = builder.openManagedStream(toneGeneratorCallback->managedStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }
    result = toneGeneratorCallback->managedStream->setBufferSizeInFrames(
            toneGeneratorCallback->managedStream->getFramesPerBurst());
    if (result != oboe::Result::OK) {
        LOGE("Failed to set the frames per burst to min: %s", oboe::convertToText(result));
    }
    toneGeneratorCallback->init();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_InitRecordCallbacks(JNIEnv *env, jclass clazz) {
    LOGE("INIT RECORD CALLBACKS");

    // Adding a signal handler for debugging
    toneListenerCallback = new AudioListenerCallback();
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Input);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    // valid options for this device are 18 and 19. worth testing both?
    builder.setDeviceId(18);
    builder.setChannelCount(1);
    //builder.setFramesPerCallback(400);
    builder.setSampleRate(toneListenerCallback->kSampleRate);
    builder.setCallback(toneListenerCallback);

    oboe::Result result = builder.openManagedStream(toneListenerCallback->managedStream);

    if (result != oboe::Result::OK) {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_GetLastSpikeNS(JNIEnv *env, jclass clazz) {
    // TODO: implement GetLastSpikeNS()
    return (jlong) toneListenerCallback->last_broadcast_seen;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StartRecord(JNIEnv *env, jclass clazz) {
    LOGE("START RECORD");
    toneListenerCallback->startRecord();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StopRecord(JNIEnv *env, jclass clazz) {
    LOGE("STOP RECORD");
    toneListenerCallback->stopRecord();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_stopAudioChirpAtInterval(JNIEnv *env, jclass clazz) {
    LOGE("STOP");
    toneGeneratorCallback->stopPlayback();
    shouldContinueChirpingAtInterval = false;
}

extern "C"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_startAudioChirpAtInterval(JNIEnv *env, jclass clazz, jint i) {
    LOGE("AUDIO CHIRP INT");
    // TODO: Add playback tuning to ensure the chirp happens every i ms
    std::chrono::milliseconds pause_between_chirp_starts(i*2);
    std::chrono::nanoseconds chirp_duration(CHIRP_LEN_NS*10 );
    std::chrono::nanoseconds wait(1);
    shouldContinueChirpingAtInterval = true;
    std::chrono::milliseconds one(1 );

    while (shouldContinueChirpingAtInterval) {


        LOGE("_________________________________________________________________________________");
        LOGE("begin playback");
        auto last_bcast = toneListenerCallback->getLastBroadcastSeen() ;
        toneGeneratorCallback->startPlayback();

        while (!toneGeneratorCallback->hasFinishedBroadcasting)
            std::this_thread::sleep_for(wait);

        auto bcast_sent = toneGeneratorCallback->last_broadcast_sent;

        LOGE("CHIRP SENT AT %llu", bcast_sent);

        toneGeneratorCallback->stopPlayback();

        auto current_bcast = toneListenerCallback->getLastBroadcastSeen();

        while (current_bcast == last_bcast){
            std::this_thread::sleep_for(wait);
            current_bcast = toneListenerCallback->getLastBroadcastSeen();
        }

        chirp_delay = (toneListenerCallback->getLastBroadcastSeen() - bcast_sent) / 2;
        auto print_delay = chirp_delay / double(1e+6);
        LOGE("CHIRP DELAY: %f", print_delay);

        std::this_thread::sleep_for(pause_between_chirp_starts);


    }

}
#pragma clang diagnostic pop

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_chirp(JNIEnv *env, jclass clazz) {
    // Chirps one time
    std::chrono::nanoseconds one(1);
    std::chrono::milliseconds mili(3);

    auto last_bcast = toneListenerCallback->getLastBroadcastSeen() ;
    toneGeneratorCallback->startPlayback();
    while (!toneGeneratorCallback->hasFinishedBroadcasting)
        std::this_thread::sleep_for(one);

    auto bcast_sent = toneGeneratorCallback->last_broadcast_sent;
    LOGE("CHIRP SENT AT %llu", toneGeneratorCallback->last_broadcast_sent);

    toneGeneratorCallback->stopPlayback();

    auto current_bcast = toneListenerCallback->getLastBroadcastSeen();
    while (current_bcast == last_bcast){
        std::this_thread::sleep_for(one);
        current_bcast = toneListenerCallback->getLastBroadcastSeen();
    }

    chirp_delay = (toneListenerCallback->getLastBroadcastSeen() - bcast_sent) / 2;
    auto print_delay = chirp_delay / double(1e+6);
    LOGE("CHIRP DELAY: %f", print_delay);
    LOGE("_________________________________________________________________________________");

    std::this_thread::sleep_for(mili);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_getLastChirpRecvTime(JNIEnv *env, jclass clazz) {
    return (jlong) toneListenerCallback->getLastRecvSeen();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_getLastChirpSentTime(JNIEnv *env, jclass clazz) {
    LOGE("GetLASTCHIRPTIME");
    std::chrono::nanoseconds one(1);
    // Wait until broadcasting has finished

    //return (jlong) toneGeneratorCallback->last_broadcast_sent;
    return (jlong) toneListenerCallback->getLastBroadcastSeen();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_setAudioListenFreq(JNIEnv *env, jclass clazz, jdouble freq) {
    LOGE("UPDATING FREQ %f", (double)freq);
    toneListenerCallback->setFrequency(static_cast<float>((double) freq));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_setAudioBroadcastFreq(JNIEnv *env, jclass clazz, jdouble freq) {
    //toneGeneratorCallback.setFrequency((double)freq);
    LOGE("SET OWN FREQUENCY");
    toneListenerCallback->setOwnFrequency(static_cast<float>((double) freq));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_setLeader(JNIEnv *env, jclass clazz, jboolean is_leader) {
    LOGE("SETTING IS LEADER %d",is_leader);
    bool shouldChangeToLeader = (bool)is_leader;

    //if (shouldChangeToLeader == isCurrentlyLeader.load(std::memory_order_acquire))
    //return;

    toneListenerCallback->isLeader(shouldChangeToLeader);
    toneGeneratorCallback->setLeader(shouldChangeToLeader);
    isCurrentlyLeader.store(shouldChangeToLeader);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_getLastChirpDelayNS(JNIEnv *env, jclass clazz) {
    return (jlong) chirp_delay;
}