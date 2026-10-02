package com.aalam.compiler;

import android.app.Activity;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/** Landscape gold + black screen. Ab build logic C++ (NativeBridge se) chal rahi hai. */
public class MainActivity extends Activity {

    private static final int BG = 0xFF0A0A0A;
    private static final int SURFACE = 0xFF121212;
    private static final int GOLD = 0xFFD4AF37;
    private static final int GOLD_DIM = 0xFF5E5020;
    private static final int TEXT = 0xFFECE6CF;
    private static final int MUTED = 0xFF8C8467;
    private static final int RED = 0xFFE05555;

    private static final int PENDING = 0, RUNNING = 1, DONE = 2, FAILED = 3;

    private final List<TextView> stepDots = new ArrayList<>();
    private final List<TextView> stepLabels = new ArrayList<>();

    private TextView logView;
    private TextView statusView;
    private ScrollView logScroll;
    private Button buildBtn;
    private volatile int currentStep = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.HORIZONTAL);
        root.setBackgroundColor(BG);

        root.addView(buildLeftPanel(), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 4f));

        View divider = new View(this);
        divider.setBackgroundColor(GOLD_DIM);
        root.addView(divider, new LinearLayout.LayoutParams(dp(1), ViewGroup.LayoutParams.MATCH_PARENT));

        root.addView(buildRightPanel(), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 6f));

        setContentView(root);
    }

    private View buildLeftPanel() {
        LinearLayout left = new LinearLayout(this);
        left.setOrientation(LinearLayout.VERTICAL);
        left.setBackgroundColor(SURFACE);
        left.setPadding(dp(16), dp(12), dp(16), dp(12));

        TextView title = text("AALAM COMPILER", 17, GOLD, true);
        title.setLetterSpacing(0.08f);
        left.addView(title);
        left.addView(text("Native (C/C++) Core", 11, MUTED, false));

        LinearLayout steps = new LinearLayout(this);
        steps.setOrientation(LinearLayout.VERTICAL);
        String[] names = NativeBridge.stepNames();
        for (int i = 0; i < names.length; i++) {
            LinearLayout row = new LinearLayout(this);
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(0, dp(4), 0, dp(4));

            TextView dot = text(String.valueOf(i + 1), 11, MUTED, true);
            dot.setGravity(Gravity.CENTER);
            TextView label = text(names[i], 13, MUTED, false);
            label.setPadding(dp(10), 0, 0, 0);

            row.addView(dot, new LinearLayout.LayoutParams(dp(24), dp(24)));
            row.addView(label);
            steps.addView(row, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

            stepDots.add(dot);
            stepLabels.add(label);
            styleStep(i, PENDING);
        }

        ScrollView stepsScroll = new ScrollView(this);
        stepsScroll.addView(steps);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f);
        sp.topMargin = dp(16);
        left.addView(stepsScroll, sp);

        buildBtn = new Button(this);
        buildBtn.setText("BUILD SAMPLE PROJECT");
        buildBtn.setTextColor(BG);
        buildBtn.setTextSize(13);
        buildBtn.setTypeface(Typeface.DEFAULT_BOLD);
        buildBtn.setBackground(shape(GOLD, GOLD, 0, 10));
        buildBtn.setOnClickListener(v -> startBuild());
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(44));
        bp.topMargin = dp(10);
        left.addView(buildBtn, bp);

        return left;
    }

    private View buildRightPanel() {
        LinearLayout right = new LinearLayout(this);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(dp(16), dp(12), dp(16), dp(12));

        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        TextView logTitle = text("BUILD LOG", 12, GOLD, true);
        logTitle.setLetterSpacing(0.1f);
        statusView = text("READY", 12, MUTED, true);
        header.addView(logTitle, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        header.addView(statusView);
        right.addView(header);

        logView = text("", 13, TEXT, false);
        logView.setTypeface(Typeface.MONOSPACE);
        logView.setPadding(dp(12), dp(10), dp(12), dp(10));

        logScroll = new ScrollView(this);
        logScroll.setBackground(shape(0xFF0D0D0D, GOLD_DIM, 1, 10));
        logScroll.addView(logView);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f);
        lp.topMargin = dp(8);
        right.addView(logScroll, lp);

        return right;
    }

    private void startBuild() {
        buildBtn.setEnabled(false);
        buildBtn.setAlpha(0.5f);
        logView.setText("");
        currentStep = -1;
        for (int i = 0; i < stepDots.size(); i++) styleStep(i, PENDING);
        setStatus("BUILDING...", GOLD);

        new Thread(() -> {
            try {
                File project = new File(getFilesDir(), "sample");
                new File(project, "src/com/sample/app").mkdirs();
                try (FileOutputStream out = new FileOutputStream(new File(project, "AndroidManifest.xml"))) {
                    out.write("<manifest package=\"com.sample.app\"/>".getBytes("UTF-8"));
                }
                File outDir = new File(getFilesDir(), "out");

                NativeBridge.runBuild(
                        project.getAbsolutePath(),
                        outDir.getAbsolutePath(),
                        prepareAndroidJar().getAbsolutePath(),
                        new NativeBridge.Listener() {
                            @Override
                            public void onStepStart(int i, int total, String name) {
                                currentStep = i - 1;
                                runOnUiThread(() -> styleStep(i - 1, RUNNING));
                                append("[" + i + "/" + total + "] " + name + " ...");
                            }

                            @Override
                            public void onStepDone(int i, int total, String name) {
                                runOnUiThread(() -> styleStep(i - 1, DONE));
                                append("    done");
                            }

                            @Override
                            public void onFinished(boolean success, String apkPath, String error) {
                                append(success ? "\nBUILD SUCCESS: " + apkPath : "\nBUILD FAILED: " + error);
                                runOnUiThread(() -> {
                                    if (success) {
                                        setStatus("BUILD SUCCESS", GOLD);
                                    } else {
                                        styleStep(currentStep, FAILED);
                                        setStatus("BUILD FAILED", RED);
                                    }
                                    buildBtn.setEnabled(true);
                                    buildBtn.setAlpha(1f);
                                });
                            }
                        });
            } catch (Exception e) {
                append("ERROR: " + e);
                runOnUiThread(() -> {
                    setStatus("ERROR", RED);
                    buildBtn.setEnabled(true);
                    buildBtn.setAlpha(1f);
                });
            }
        }).start();
    }

    /** assets/android.jar ko files folder me copy karta hai (sirf pehli baar). */
    private File prepareAndroidJar() throws IOException {
        File dst = new File(getFilesDir(), "android.jar");
        if (dst.isFile() && dst.length() > 0) return dst;
        try (InputStream in = getAssets().open("android.jar");
             OutputStream out = new FileOutputStream(dst)) {
            byte[] buf = new byte[64 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
        return dst;
    }

    private void styleStep(int i, int state) {
        if (i < 0 || i >= stepDots.size()) return;
        TextView dot = stepDots.get(i);
        TextView label = stepLabels.get(i);
        dot.setText(String.valueOf(i + 1));
        switch (state) {
            case RUNNING:
                dot.setBackground(circle(BG, GOLD));
                dot.setTextColor(GOLD);
                label.setTextColor(GOLD);
                break;
            case DONE:
                dot.setBackground(circle(GOLD, GOLD));
                dot.setTextColor(BG);
                dot.setText("\u2713");
                label.setTextColor(TEXT);
                break;
            case FAILED:
                dot.setBackground(circle(BG, RED));
                dot.setTextColor(RED);
                dot.setText("!");
                label.setTextColor(RED);
                break;
            default:
                dot.setBackground(circle(BG, GOLD_DIM));
                dot.setTextColor(MUTED);
                label.setTextColor(MUTED);
        }
    }

    private void setStatus(String s, int color) {
        statusView.setText(s);
        statusView.setTextColor(color);
    }

    private void append(String line) {
        runOnUiThread(() -> {
            logView.append(line + "\n");
            logScroll.post(() -> logScroll.fullScroll(View.FOCUS_DOWN));
        });
    }

    private TextView text(String s, float sp, int color, boolean bold) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(sp);
        t.setTextColor(color);
        if (bold) t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private GradientDrawable shape(int fill, int stroke, int strokeDp, int radiusDp) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(fill);
        g.setCornerRadius(dp(radiusDp));
        if (strokeDp > 0) g.setStroke(dp(strokeDp), stroke);
        return g;
    }

    private GradientDrawable circle(int fill, int stroke) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(GradientDrawable.OVAL);
        g.setColor(fill);
        g.setStroke(dp(2), stroke);
        return g;
    }

    private int dp(int v) {
        return Math.round(v * getResources().getDisplayMetrics().density);
    }
}
