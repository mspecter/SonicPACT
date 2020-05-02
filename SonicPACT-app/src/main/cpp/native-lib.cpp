#include <jni.h>
#include <string>
//#include <oboe/Oboe.h>


extern "C" JNIEXPORT jstring JNICALL
Java_com_mit_myapplication_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_MIT_sonicPACT_NativeBridge_test(JNIEnv *env, jobject thiz) {
    // TODO: implement test()
}