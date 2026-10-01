#pragma once

#include <string>
#include <vector>
#include <functional>

namespace aalam {

// UI ko progress/log bhejne ke liye callbacks. Java side (JNI) inhe implement karega.
struct BuildListener {
    std::function<void(int index, int total, const std::string &name)> onStepStart;
    std::function<void(int index, int total, const std::string &name)> onStepDone;
    std::function<void(const std::string &message)> onLog;
};

// Saare steps ke beech shared state (jaisa Java wale BuildContext me tha).
struct BuildContext {
    std::string projectDir;
    std::string outDir;
    std::string workDir;
    std::string androidJar;
    std::string apk; // final result, abhi khaali
};

// Ek result: success ya error message.
struct StepResult {
    bool ok = true;
    std::string error;
};

struct BuildStep {
    std::string name;
    std::function<StepResult(BuildContext &)> run;
};

// Pipeline ke saare steps ko order me chalata hai. true/false return karta hai
// (success/fail), aur BuildListener ke zariye UI ko progress batata hai.
bool runPipeline(BuildContext &ctx, const std::vector<BuildStep> &steps,
                  const BuildListener &listener, std::string &outError);

std::vector<BuildStep> defaultSteps();

} // namespace aalam
