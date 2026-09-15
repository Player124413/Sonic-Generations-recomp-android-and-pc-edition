package com.rexauto.port;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;

import com.rexauto.port.gamepad.PadSettings;
import com.rexauto.port.gamepad.VirtualPadView;

import org.libsdl.app.SDLActivity;

/** SDL3 activity hosting the recompiled game (SDL is linked statically into libmain.so). */
public class MainActivity extends SDLActivity {
    private VirtualPadView mGamepad;
    private android.widget.TextView mFpsView;
    private android.os.Handler mFpsHandler;
    private long mLastPresents = -1;

    /** Present counter from the Vulkan presenter (librexruntime.so); -1 if unavailable. */
    private static native long nativeGetPresentCount();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        if (!GameFiles.hasValidGameRoot(this)) {
            startActivity(new Intent(this, SetupActivity.class));
            finish();
            return;
        }
        super.onCreate(savedInstanceState);
        applyOrientation();
        // Ask the OS for the sustained (thermally stable) clock profile instead of
        // the burst-then-throttle default, and never dim while playing.
        android.view.Window w = getWindow();
        w.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (android.os.Build.VERSION.SDK_INT >= 24) w.setSustainedPerformanceMode(true);
        if (android.os.Build.VERSION.SDK_INT >= 31) {
            android.view.WindowManager.LayoutParams lp = w.getAttributes();
            lp.preferredRefreshRate = 0;
            w.setAttributes(lp);
        }
        if (PadSettings.get(this).enabled()) {
            mGamepad = VirtualPadView.install(this);
        }
        if (new GraphicsSettings(this).showFps()) {
            installFpsMeter();
        }
    }

    /** Tiny FPS readout in the corner, fed by the native present counter. */
    private void installFpsMeter() {
        android.view.ViewGroup layout;
        try {
            layout = (android.view.ViewGroup) org.libsdl.app.SDLActivity.getContentView();
        } catch (Throwable t) {
            return;
        }
        if (layout == null) return;
        final MainActivity self = this;
        runOnUiThread(() -> {
            android.widget.TextView v = new android.widget.TextView(self);
            v.setText("FPS: --");
            v.setTextSize(13);
            v.setTypeface(android.graphics.Typeface.MONOSPACE);
            v.setTextColor(0xFF00FF88);
            v.setBackgroundColor(0x99000000);
            v.setPadding(14, 6, 14, 6);
            // Plain ViewGroup params (like the virtual gamepad): whatever layout
            // SDL uses, an unsized child lands in the top-left corner.
            android.view.ViewGroup.LayoutParams lp = new android.view.ViewGroup.LayoutParams(
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
            try {
                layout.addView(v, lp);
            } catch (Throwable t) {
                return;
            }
            mFpsView = v;
            mFpsHandler = new android.os.Handler(android.os.Looper.getMainLooper());
            mFpsHandler.post(mFpsTick);
        });
    }

    private final Runnable mFpsTick = new Runnable() {
        @Override
        public void run() {
            if (mFpsView != null) {
                long n;
                try {
                    n = nativeGetPresentCount();
                } catch (Throwable t) {
                    n = -1;
                }
                if (n >= 0 && mLastPresents >= 0) {
                    mFpsView.setText("FPS: " + (n - mLastPresents));
                } else if (n < 0) {
                    mFpsView.setText("FPS: --");
                }
                if (n >= 0) mLastPresents = n;
                if (mFpsHandler != null) mFpsHandler.postDelayed(this, 1000);
            }
        }
    };

    /** Orientation chosen in the launcher (Graphics dialog); landscape by default. */
    private void applyOrientation() {
        String o = new GraphicsSettings(this).orientation();
        int req;
        switch (o) {
            case "portrait": req = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT; break;
            case "auto": req = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_FULL_USER; break;
            default: req = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
        }
        setRequestedOrientation(req);
    }

    @Override
    protected void onPause() {
        if (mGamepad != null) mGamepad.onHostPause();
        if (mFpsHandler != null) mFpsHandler.removeCallbacks(mFpsTick);
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mFpsHandler != null && mFpsView != null) {
            mLastPresents = -1;
            mFpsHandler.post(mFpsTick);
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[]{"main"};
    }

    public static ParcelFileDescriptor openContentFd(String uri, String mode) {
        SDLActivity self = mSingleton;
        if (self == null || uri == null) return null;
        try {
            return self.getContentResolver().openFileDescriptor(Uri.parse(uri), mode == null ? "r" : mode);
        } catch (Exception e) {
            return null;
        }
    }
}
