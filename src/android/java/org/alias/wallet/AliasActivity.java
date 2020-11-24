package org.alias.wallet;

/*
 * SPDX-FileCopyrightText: © 2020 Alias Developers
 * SPDX-License-Identifier: MIT
 */

import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.WindowManager;
import android.webkit.WebView;

public class AliasActivity extends org.qtproject.qt5.android.bindings.QtActivity {

    private static final String TAG = "AliasActivity";

    private int softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
    private int screenOrientationMode = ActivityInfo.SCREEN_ORIENTATION_LOCKED;
    private int screenOrientation = ActivityInfo.SCREEN_ORIENTATION_LOCKED;

    // native method to handle 'alias:' URIs
    public static native void receiveURI(String url);
    // native method to handle boostrap service events
    public static native void updateBootstrapState(int state, int errorCode, int progress, int indexOfItem, int numOfItems, boolean indeterminate);

    private volatile boolean qtInitialized = false; // will be accessed from android UI and Qt thread
    private boolean hasPendingIntent = false; // accessed only from android UI thread

    private BoostrapBroadcastReceiver mBoostrapBroadcastReceiver;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            WebView.setWebContentsDebuggingEnabled(true);
        }
        if (getIntent() != null && getIntent().getDataString() != null) {
            hasPendingIntent = true;
        }

        // Create and register Broadcast Receiver for Bootstrap Service
        mBoostrapBroadcastReceiver = new BoostrapBroadcastReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BootstrapService.BOOTSTRAP_BROADCAST_ACTION);
        registerReceiver(mBoostrapBroadcastReceiver, intentFilter);

        screenOrientation = getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE ?
                ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mBoostrapBroadcastReceiver);
    }

    @Override
    protected void onResume() {
        super.onResume();
        getWindow().setSoftInputMode(softInputMode);
        if (screenOrientationMode == ActivityInfo.SCREEN_ORIENTATION_LOCKED) {
            setRequestedOrientation(screenOrientation);
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LOCKED);
        }
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
        Intent intent = new Intent(getApplicationContext(), AliasService.class);
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

    public void scanQRCode() {
        Intent intent = new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
        intent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
        startActivity(intent);
    }

    public boolean hasQRCodeScanner() {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P);
    }

    protected void handleIntent() {
        hasPendingIntent = false;
        Intent intent = getIntent();
        if (intent != null && intent.getData() != null && intent.getDataString().startsWith("alias:")) {
            // process payment URI
            Log.d(TAG, "onStartCommand process URI: " + intent.getDataString());
            receiveURI(intent.getDataString());
        }
    }

    public void setSoftInputModeAdjustResize() {
        runOnUiThread(() -> {
            softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;
            getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        });
    }

    public void setSoftInputModeAdjustPan() {
        runOnUiThread(() -> {
            softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
            getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        });
    }

    public void setRequestedOrientationUnspecified() {
        runOnUiThread(() -> {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
            screenOrientationMode = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        });
    }

    public void downloadBootstrap() {
        Intent intent = new Intent(getApplicationContext(), BootstrapService.class);
        getApplicationContext().startForegroundService(intent);
    }

    public boolean isBootstrapDownloadRunning() {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (BootstrapService.class.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    /**
     * BroadcastReceiver for handling BootstrapService broadcasts
     */
    private class BoostrapBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            int state = intent.getIntExtra("state", 0);
            int errorCode = intent.getIntExtra("errorCode", 0);
            int progress = intent.getIntExtra("progress", 0);
            int indexOfItem = intent.getIntExtra("indexOfItem", 0);
            int numOfItems = intent.getIntExtra("numOfItems", 0);
            boolean indeterminate = intent.getBooleanExtra("indeterminate", false);
            updateBootstrapState(state, errorCode, progress, indexOfItem, numOfItems, indeterminate);
        }
    }
}