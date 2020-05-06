#include <jni.h>
#include <string>
#include "AudoGenerator.h"
#include "../../../../../oboe/src/common/OboeDebug.h"
#include "AudioListener.h"
#include "Timing.h"
#define SAMPLE_RATE 192000;



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
    builder.setChannelCount(1);
    builder.setBufferCapacityInFrames(BUFFSIZE);
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
    return (jlong) toneListenerCallback.lastSpikeStartTime;

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