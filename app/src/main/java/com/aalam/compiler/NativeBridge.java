package com.aalam.compiler;

import java.io.File;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.eclipse.jdt.core.compiler.batch.BatchCompiler;

/**
 * Java se C++ pipeline tak ka pul. Asli build logic native-lib.cpp /
 * build_pipeline.cpp me hai. Sirf ECJ step Java me hai, kyunki ECJ Java library hai.
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

    /** Native se bulaya jata hai (Step 2). Success pe null, fail pe error text return karta hai. */
    public static String compileJava(String srcDir, String classesDir, String androidJar) {
        try {
            new File(classesDir).mkdirs();
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            boolean ok = BatchCompiler.compile(
                    new String[]{"-1.8", "-nowarn", "-cp", androidJar, "-d", classesDir, srcDir},
                    pw, pw, null);
            pw.flush();
            return ok ? null : sw.toString();
        } catch (Throwable t) {
            return "ECJ crash: " + t;
        }
    }
}
