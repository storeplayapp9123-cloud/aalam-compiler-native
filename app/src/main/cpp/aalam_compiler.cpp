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

// Steps ek baar banaye jaate hain aur process ke jeete-ji zinda rehte hain,
// taaki aalam_step_name() ke diye hue const char* pointers hamesha valid rahein.
const std::vector<BuildStep> &steps() {
    static std::vector<BuildStep> s = defaultSteps();
    return s;
}

} // namespace

extern "C" int aalam_step_count(void) {
    return static_cast<int>(steps().size());
}

extern "C" const char *aalam_step_name(int index) {
    const auto &s = steps();
    if (index < 0 || static_cast<size_t>(index) >= s.size()) return "";
    return s[index].name.c_str();
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
