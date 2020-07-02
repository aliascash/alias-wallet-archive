package org.spectrecoin.wallet;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.webkit.WebView;

import java.util.Map;

public class SpectrecoinActivity extends org.qtproject.qt5.android.bindings.QtActivity {

    private static final String TAG = "SpectrecoinService";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            WebView.setWebContentsDebuggingEnabled(true);
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
    }

    public void setSoftInputModeAdjustResize() {
        runOnUiThread(new Runnable() {
            public void run() {
                getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
            }
        });
    }
    public void setSoftInputModeAdjustPan() {
        runOnUiThread(new Runnable() {
            public void run() {
                getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
            }
        });
    }
}
