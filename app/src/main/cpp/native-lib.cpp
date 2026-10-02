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

struct JniCallbackData {
    JNIEnv *env;
    jobject listener;
    jmethodID onStepStart;
    jmethodID onStepDone;
    jmethodID onFinished;
    jclass bridgeClass;
    jmethodID compileJava;
    std::string javaError;
};

void stepStartThunk(int index, int total, const char *name, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    jstring n = d->env->NewStringUTF(name);
    d->env->CallVoidMethod(d->listener, d->onStepStart, index, total, n);
    d->env->DeleteLocalRef(n);
}

void stepDoneThunk(int index, int total, const char *name, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    jstring n = d->env->NewStringUTF(name);
    d->env->CallVoidMethod(d->listener, d->onStepDone, index, total, n);
    d->env->DeleteLocalRef(n);
}

void finishThunk(int success, const char *apkPath, const char *error, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    jstring apk = apkPath ? d->env->NewStringUTF(apkPath) : nullptr;
    jstring err = error ? d->env->NewStringUTF(error) : nullptr;
    d->env->CallVoidMethod(d->listener, d->onFinished, static_cast<jboolean>(success != 0), apk, err);
}

// Step 2: Java ka ECJ wala compileJava() bulata hai.
const char *javaCompileThunk(const char *src, const char *classes, const char *jar, void *userdata) {
    auto *d = static_cast<JniCallbackData *>(userdata);
    JNIEnv *env = d->env;
    jstring js = env->NewStringUTF(src);
    jstring jc = env->NewStringUTF(classes);
    jstring ja = env->NewStringUTF(jar);
    jstring res = static_cast<jstring>(
            env->CallStaticObjectMethod(d->bridgeClass, d->compileJava, js, jc, ja));
    env->DeleteLocalRef(js);
    env->DeleteLocalRef(jc);
    env->DeleteLocalRef(ja);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        d->javaError = "Java exception aayi compileJava me";
        return d->javaError.c_str();
    }
    if (res == nullptr) return nullptr;
    d->javaError = jstr(env, res);
    env->DeleteLocalRef(res);
    return d->javaError.c_str();
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
Java_com_aalam_compiler_NativeBridge_runBuild(JNIEnv *env, jclass clazz,
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
            clazz,
            env->GetStaticMethodID(clazz, "compileJava",
                                   "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
            std::string(),
    };

    aalam_set_java_compile_hook(javaCompileThunk, &data);
    aalam_run_build(projectDir.c_str(), outDir.c_str(), androidJar.c_str(),
                     stepStartThunk, stepDoneThunk, finishThunk, &data);
    aalam_set_java_compile_hook(nullptr, nullptr);
}
