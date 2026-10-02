package com.aalam.compiler;

/**
 * Java se C++ pipeline tak ka pul. Asli build logic ab native-lib.cpp /
 * build_pipeline.cpp me hai - ye class sirf JNI calls karti hai.
 */
public class NativeBridge {

    static {
        System.loadLibrary("aalamcompiler");
    }

    public interface Listener {
        void onStepStart(int index, int total, String name);

        void onStepDone(int index, int total, String name);

        void onFinished(boolean success, String apkPath, String error);
    }

    public static native String[] stepNames();

    public static native void runBuild(String projectDir, String outDir, String androidJar, Listener listener);
}
