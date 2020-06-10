package org.spectrecoin.wallet;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.webkit.WebView;

public class SpectrecoinActivity extends org.qtproject.qt5.android.bindings.QtActivity {

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            WebView.setWebContentsDebuggingEnabled(true);
        }
    }

    public void startCore() {
        Intent intent = new Intent(getApplicationContext(), SpectrecoinService.class);
        getApplicationContext().startForegroundService(intent);
    }
}
