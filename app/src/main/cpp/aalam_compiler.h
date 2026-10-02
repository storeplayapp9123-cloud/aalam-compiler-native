#pragma once

/*
 * Aalam Compiler - Public C API
 * Pure C header: koi C++ type bahar nahi aata.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Ek step shuru/khatam hone par bulaya jaata hai. */
typedef void (*AalamStepCallback)(int index, int total, const char *name, void *userdata);

/* Pura build khatam hone par ek baar. success != 0 par apkPath set, error NULL. Fail par ulta. */
typedef void (*AalamFinishCallback)(int success, const char *apkPath, const char *error, void *userdata);

/* Java compile hook: success pe NULL, fail pe error text (agle call tak valid). */
typedef const char *(*AalamJavaCompileHook)(const char *srcDir, const char *classesDir,
                                             const char *androidJar, void *userdata);

/* Pipeline me kitne steps hain. */
int aalam_step_count(void);

/* index (0-based) step ka naam. Free nahi karna. */
const char *aalam_step_name(int index);

/* Step 2 ke liye Java compile hook set karta hai. */
void aalam_set_java_compile_hook(AalamJavaCompileHook hook, void *userdata);

/* Pura build chalata hai (blocking - background thread se bulao). */
void aalam_run_build(const char *projectDir,
                      const char *outDir,
                      const char *androidJar,
                      AalamStepCallback onStepStart,
                      AalamStepCallback onStepDone,
                      AalamFinishCallback onFinish,
                      void *userdata);

#ifdef __cplusplus
}
#endif
