#pragma once

#include <string>
#include <vector>
#include <functional>

namespace aalam {

struct BuildListener {
    std::function<void(int index, int total, const std::string &name)> onStepStart;
    std::function<void(int index, int total, const std::string &name)> onStepDone;
    std::function<void(const std::string &message)> onLog;
};

// Java compile hook: khaali string = success, warna error text.
using JavaCompileFn = std::function<std::string(const std::string &srcDir,
                                                const std::string &classesDir,
                                                const std::string &androidJar)>;

struct BuildContext {
    std::string projectDir;
    std::string outDir;
    std::string workDir;
    std::string classesDir; // Step 2 ka output
    std::string androidJar;
    std::string apk;
    JavaCompileFn javaCompile;
};

struct StepResult {
    bool ok = true;
    std::string error;
};

struct BuildStep {
    std::string name;
    std::function<StepResult(BuildContext &)> run;
};

bool runPipeline(BuildContext &ctx, const std::vector<BuildStep> &steps,
                  const BuildListener &listener, std::string &outError);

std::vector<BuildStep> defaultSteps();

} // namespace aalam
