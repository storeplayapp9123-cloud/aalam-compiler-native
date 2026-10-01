#pragma once

/*
 * Aalam Compiler - Public C API
 *
 * Yeh header pure C hai (koi C++ type nahi: std::string, std::vector waghera
 * bahar nahi aate). Andar ka asli build logic C++ me hai (build_pipeline.h/.cpp),
 * lekin bahar se isko koi bhi C-compatible caller (JNI, dusri language, future
 * tools) isi header ke zariye use kar sakta hai.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Ek step shuru/khatam hone par bulaya jaata hai. */
typedef void (*AalamStepCallback)(int index, int total, const char *name, void *userdata);

/* Pura build khatam hone par ek baar bulaya jaata hai.
 * success != 0 par apkPath set hoga, error NULL hoga. Fail par ulta. */
typedef void (*AalamFinishCallback)(int success, const char *apkPath, const char *error, void *userdata);

/* Pipeline me kitne steps hain. */
int aalam_step_count(void);

/* index (0-based) step ka naam. Caller ko free nahi karna - internal static string hai. */
const char *aalam_step_name(int index);

/* Pura build chalata hai (abhi ke liye blocking call - caller apne background
 * thread se bulaye, jaise JNI side karta hai). */
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
