#include <jni.h>
#include <string>
#include "AudoGenerator.h"
#include "../../../../../oboe/src/common/OboeDebug.h"
#include "AudioListener.h"
#include "Timing.h"


std::atomic<bool> shouldContinueChirpingAtInterval {true};

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StartPlayback(JNIEnv *env, jclass clazz) {
    // Starts the playback of the tone
    toneGeneratorCallback.startPlayback();
    toneListenerCallback.beginTimer();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StopPlayback(JNIEnv *env, jclass clazz) {
    // Stops the playback of the tone
    toneGeneratorCallback.stopPlayback();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_InitPlaybackCallbacks(JNIEnv *env, jclass clazz) {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setBufferCapacityInFrames(128);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(1);
    builder.setSampleRate(48000);
    builder.setCallback(&toneGeneratorCallback);
    oboe::Result result = builder.openManagedStream(toneGeneratorCallback.managedStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }
    result = toneGeneratorCallback.managedStream->setBufferSizeInFrames(
            toneGeneratorCallback.managedStream->getFramesPerBurst());
    if (result != oboe::Result::OK) {
        LOGE("Failed to set the frames per burst to min: %s", oboe::convertToText(result));
    }
    toneGeneratorCallback.init();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_InitRecordCallbacks(JNIEnv *env, jclass clazz) {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Input);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::I16);
    // valid options for this device are 18 and 19. worth testing both?
    builder.setDeviceId(18);
    builder.setChannelCount(1);
    //builder.setFramesPerCallback(BUFFSIZE);
    builder.setSampleRate(toneListenerCallback.kSampleRate);
    builder.setCallback(&toneListenerCallback);

    oboe::Result result = builder.openManagedStream(toneListenerCallback.managedStream);

    if (result != oboe::Result::OK) {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_GetLastSpikeNS(JNIEnv *env, jclass clazz) {
    // TODO: implement GetLastSpikeNS()
    return (jlong) toneListenerCallback.lastListeningBroadcastTime;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StartRecord(JNIEnv *env, jclass clazz) {
    toneListenerCallback.startRecord();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_StopRecord(JNIEnv *env, jclass clazz) {
    toneListenerCallback.stopRecord();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_stopAudioChirpAtInterval(JNIEnv *env, jclass clazz) {
    shouldContinueChirpingAtInterval = false;
}

extern "C"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_startAudioChirpAtInterval(JNIEnv *env, jclass clazz, jint i) {
    // TODO: Add playback tuning to ensure the chirp happens every i ms
    std::chrono::milliseconds pause_between_chirp_starts(i);
    std::chrono::nanoseconds chirp_duration(CHIRP_LEN_NS);
    shouldContinueChirpingAtInterval = true;

    while (shouldContinueChirpingAtInterval){
        toneGeneratorCallback.startPlayback();

        std::this_thread::sleep_for(chirp_duration);
        toneGeneratorCallback.stopPlayback();
        std::this_thread::sleep_for(pause_between_chirp_starts - chirp_duration);
    }

}
#pragma clang diagnostic pop

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_chirp(JNIEnv *env, jclass clazz) {
    // TODO: implement chirp(), sends a chirp for N milliseconds
    std::chrono::nanoseconds chirp_duration(CHIRP_LEN_NS);
    toneGeneratorCallback.startPlayback();
    std::this_thread::sleep_for(chirp_duration);
    toneGeneratorCallback.stopPlayback();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_getLastChirpRecvTime(JNIEnv *env, jclass clazz) {
    return (jlong) toneListenerCallback.lastListeningBroadcastTime;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_MIT_sonicPACT_NativeBridge_getLastChirpSentTime(JNIEnv *env, jclass clazz) {
    return (jlong) toneGeneratorCallback.lastBroadcastTime;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_setAudioListenFreq(JNIEnv *env, jclass clazz, jdouble freq) {
    __android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "UPDATING FREQ %f", (double)freq);
    toneListenerCallback.setFrequency((double) freq);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_MIT_sonicPACT_NativeBridge_setAudioBroadcastFreq(JNIEnv *env, jclass clazz, jdouble freq) {
    toneGeneratorCallback.setFrequency((double)freq);
    toneListenerCallback.setOwnFrequency((double)freq);
}