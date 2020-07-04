package org.spectrecoin.wallet;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.webkit.WebView;

public class SpectrecoinActivity extends org.qtproject.qt5.android.bindings.QtActivity {

    private static final String TAG = "SpectrecoinService";

    private int softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;

    // native method to handle 'spectrecoin:' URIs
    public static native void receiveURI(String url);

    volatile boolean qtInitialized = false; // will be accessed from android UI and Qt thread
    boolean hasPendingIntent = false; // accessed only from android UI thread

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            WebView.setWebContentsDebuggingEnabled(true);
        }
        if (getIntent() != null && getIntent().getDataString() != null) {
            hasPendingIntent = true;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        getWindow().setSoftInputMode(softInputMode);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        hasPendingIntent = true;
        if (qtInitialized) {
            handleIntent();
        }
    }

    public void startCore(boolean rescan, String bip44key) {
        Intent intent = new Intent(getApplicationContext(), SpectrecoinService.class);
        Bundle bundle = intent.getExtras();
        if (rescan) {
            intent.putExtra("rescan", true);
        }
        if (bip44key != null && !bip44key.isEmpty()) {
            intent.putExtra("bip44key", bip44key);
        }
        getApplicationContext().startForegroundService(intent);

        // use start core to identify QT is up and running
        qtInitialized = true;
        runOnUiThread(new Runnable() {
            public void run() {
                if (hasPendingIntent) {
                    handleIntent();
                }
            }
        });
    }

    protected void handleIntent() {
        hasPendingIntent = false;
        Intent intent = getIntent();
        if (intent != null && intent.getData() != null && intent.getDataString().startsWith("spectrecoin:")) {
            // process payment URI
            Log.d(TAG, "onStartCommand process URI: " +intent.getDataString());
            receiveURI(intent.getDataString());
        }
    }

    public void setSoftInputModeAdjustResize() {
        runOnUiThread(new Runnable() {
            public void run() {
                softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;
                getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
            }
        });
    }
    public void setSoftInputModeAdjustPan() {
        runOnUiThread(new Runnable() {
            public void run() {
                softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
                getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
            }
        });
    }
}
