#include "aalam_compiler.h"
#include "build_pipeline.h"
#include <vector>
#include <string>

using aalam::BuildContext;
using aalam::BuildListener;
using aalam::BuildStep;
using aalam::defaultSteps;
using aalam::runPipeline;

namespace {

const std::vector<BuildStep> &steps() {
    static std::vector<BuildStep> s = defaultSteps();
    return s;
}

AalamJavaCompileHook g_javaHook = nullptr;
void *g_javaHookData = nullptr;

} // namespace

extern "C" int aalam_step_count(void) {
    return static_cast<int>(steps().size());
}

extern "C" const char *aalam_step_name(int index) {
    const auto &s = steps();
    if (index < 0 || static_cast<size_t>(index) >= s.size()) return "";
    return s[index].name.c_str();
}

extern "C" void aalam_set_java_compile_hook(AalamJavaCompileHook hook, void *userdata) {
    g_javaHook = hook;
    g_javaHookData = userdata;
}

extern "C" void aalam_run_build(const char *projectDir,
                                 const char *outDir,
                                 const char *androidJar,
                                 AalamStepCallback onStepStart,
                                 AalamStepCallback onStepDone,
                                 AalamFinishCallback onFinish,
                                 void *userdata) {
    BuildContext ctx;
    ctx.projectDir = projectDir ? projectDir : "";
    ctx.outDir = outDir ? outDir : "";
    ctx.androidJar = androidJar ? androidJar : "";

    if (g_javaHook) {
        AalamJavaCompileHook hook = g_javaHook;
        void *hd = g_javaHookData;
        ctx.javaCompile = [hook, hd](const std::string &s, const std::string &c,
                                     const std::string &a) -> std::string {
            const char *e = hook(s.c_str(), c.c_str(), a.c_str(), hd);
            return e ? std::string(e) : std::string();
        };
    }

    BuildListener listener;
    if (onStepStart) {
        listener.onStepStart = [=](int i, int total, const std::string &name) {
            onStepStart(i, total, name.c_str(), userdata);
        };
    }
    if (onStepDone) {
        listener.onStepDone = [=](int i, int total, const std::string &name) {
            onStepDone(i, total, name.c_str(), userdata);
        };
    }

    std::string error;
    bool success = runPipeline(ctx, steps(), listener, error);

    if (onFinish) {
        onFinish(success ? 1 : 0,
                 success ? ctx.apk.c_str() : nullptr,
                 success ? nullptr : error.c_str(),
                 userdata);
    }
}
