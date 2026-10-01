#include <jni.h>
#include <string>
#include "aalam_compiler.h"

namespace {

std::string jstr(JNIEnv *env, jstring s) {
    if (s == nullptr) return {};
    const char *chars = env->GetStringUTFChars(s, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(s, chars);
    return result;
}

// aalam_run_build ko jo userdata milta hai, usi me JNIEnv* aur listener jobject
// pack karke bhejte hain, taaki C callbacks wapas Java tak pahunch sakein.
struct JniCallbackData {
    JNIEnv *env;
    jobject listener;
    jmethodID onStepStart;
    jmethodID onStepDone;
    jmethodID onFinished;
};

void stepStartThunk(int index, int total, const char *name, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    d->env->CallVoidMethod(d->listener, d->onStepStart, index, total, d->env->NewStringUTF(name));
}

void stepDoneThunk(int index, int total, const char *name, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    d->env->CallVoidMethod(d->listener, d->onStepDone, index, total, d->env->NewStringUTF(name));
}

void finishThunk(int success, const char *apkPath, const char *error, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    jstring apk = apkPath ? d->env->NewStringUTF(apkPath) : nullptr;
    jstring err = error ? d->env->NewStringUTF(error) : nullptr;
    d->env->CallVoidMethod(d->listener, d->onFinished, static_cast<jboolean>(success != 0), apk, err);
}

} // namespace

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_aalam_compiler_NativeBridge_stepNames(JNIEnv *env, jclass) {
    int count = aalam_step_count();
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(count, stringClass, nullptr);
    for (int i = 0; i < count; i++) {
        env->SetObjectArrayElement(result, i, env->NewStringUTF(aalam_step_name(i)));
    }
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_aalam_compiler_NativeBridge_runBuild(JNIEnv *env, jclass,
                                               jstring jProjectDir, jstring jOutDir,
                                               jstring jAndroidJar, jobject jListener) {
    std::string projectDir = jstr(env, jProjectDir);
    std::string outDir = jstr(env, jOutDir);
    std::string androidJar = jstr(env, jAndroidJar);

    jclass listenerClass = env->GetObjectClass(jListener);
    JniCallbackData data{
            env,
            jListener,
            env->GetMethodID(listenerClass, "onStepStart", "(IILjava/lang/String;)V"),
            env->GetMethodID(listenerClass, "onStepDone", "(IILjava/lang/String;)V"),
            env->GetMethodID(listenerClass, "onFinished", "(ZLjava/lang/String;Ljava/lang/String;)V"),
    };

    // JNI (C-style boundary) yahan sirf aalam_compiler.h ke C API ko bulata hai -
    // C++ core (build_pipeline.h) ko kabhi seedha nahi chhoota.
    aalam_run_build(projectDir.c_str(), outDir.c_str(), androidJar.c_str(),
                     stepStartThunk, stepDoneThunk, finishThunk, &data);
}
