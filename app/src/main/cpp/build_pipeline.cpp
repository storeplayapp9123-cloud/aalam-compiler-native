#include "build_pipeline.h"
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace aalam {

bool runPipeline(BuildContext &ctx, const std::vector<BuildStep> &steps,
                  const BuildListener &listener, std::string &outError) {
    int total = static_cast<int>(steps.size());
    for (int i = 0; i < total; i++) {
        const BuildStep &step = steps[i];
        if (listener.onStepStart) listener.onStepStart(i + 1, total, step.name);

        StepResult result = step.run(ctx);

        if (!result.ok) {
            outError = result.error;
            return false;
        }
        if (listener.onStepDone) listener.onStepDone(i + 1, total, step.name);
    }
    return true;
}

// Step 1: project check karna aur work folder saaf banana.
static StepResult prepareStep(BuildContext &ctx) {
    StepResult r;

    std::error_code ec;
    if (ctx.projectDir.empty() || !fs::is_directory(ctx.projectDir, ec)) {
        r.ok = false;
        r.error = "Project folder nahi mila: " + ctx.projectDir;
        return r;
    }

    fs::path manifest = fs::path(ctx.projectDir) / "AndroidManifest.xml";
    if (!fs::is_regular_file(manifest, ec)) {
        r.ok = false;
        r.error = "AndroidManifest.xml nahi mila: " + manifest.string();
        return r;
    }

    fs::path src = fs::path(ctx.projectDir) / "src";
    if (!fs::is_directory(src, ec)) {
        r.ok = false;
        r.error = "src folder nahi mila: " + src.string();
        return r;
    }

    ctx.workDir = (fs::path(ctx.outDir) / "work").string();
    fs::remove_all(ctx.workDir, ec);
    if (!fs::create_directories(ctx.workDir, ec)) {
        if (!fs::is_directory(ctx.workDir, ec)) {
            r.ok = false;
            r.error = "Work folder nahi ban paya: " + ctx.workDir;
            return r;
        }
    }

    return r;
}

// Step 2: Java source ko ECJ se compile karna (hook ke through Java ko call).
static StepResult compileJavaStep(BuildContext &ctx) {
    StepResult r;
    std::error_code ec;

    if (!ctx.javaCompile) {
        r.ok = false;
        r.error = "Java compile hook set nahi hai";
        return r;
    }
    if (!fs::is_regular_file(ctx.androidJar, ec)) {
        r.ok = false;
        r.error = "android.jar nahi mila: " + ctx.androidJar;
        return r;
    }

    fs::path src = fs::path(ctx.projectDir) / "src";
    int javaFiles = 0;
    for (auto it = fs::recursive_directory_iterator(src, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->is_regular_file() && it->path().extension() == ".java") javaFiles++;
    }
    if (javaFiles == 0) {
        r.ok = false;
        r.error = "src me koi .java file nahi mili: " + src.string();
        return r;
    }

    ctx.classesDir = (fs::path(ctx.workDir) / "classes").string();
    std::string err = ctx.javaCompile(src.string(), ctx.classesDir, ctx.androidJar);
    if (!err.empty()) {
        r.ok = false;
        r.error = "Java compile fail:\n" + err;
    }
    return r;
}

// Jo step abhi likha nahi gaya, wo saaf error deta hai.
static BuildStep pending(const std::string &name) {
    BuildStep step;
    step.name = name;
    step.run = [name](BuildContext &) -> StepResult {
        StepResult r;
        r.ok = false;
        r.error = "Step abhi native me implement nahi hua: " + name;
        return r;
    };
    return step;
}

std::vector<BuildStep> defaultSteps() {
    std::vector<BuildStep> steps;

    BuildStep prepare;
    prepare.name = "Preparing";
    prepare.run = prepareStep;
    steps.push_back(prepare);

    BuildStep ecj;
    ecj.name = "Compiling Java (ECJ)";
    ecj.run = compileJavaStep;
    steps.push_back(ecj);

    steps.push_back(pending("Compiling resources (AAPT2)"));
    steps.push_back(pending("Linking resources (AAPT2)"));
    steps.push_back(pending("Dexing (D8)"));
    steps.push_back(pending("Packaging APK"));
    steps.push_back(pending("Aligning and signing"));

    return steps;
}

} // namespace aalam
