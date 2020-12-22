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
import android.os.PowerManager;
import android.provider.MediaStore;
import android.provider.Settings;
import android.util.Log;
import android.view.WindowManager;
import android.webkit.WebView;
import android.widget.Toast;

import org.qtproject.qt5.android.bindings.QtActivity;


public class AliasActivity extends QtActivity {

    private static final String TAG = "AliasActivity";

    private int softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
    private int screenOrientationMode = ActivityInfo.SCREEN_ORIENTATION_LOCKED;
    private int screenOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

    // native method to handle 'alias:' URIs
    public static native void receiveURI(String url);

    // native method to handle boostrap service events
    public static native void updateBootstrapState(int state, int errorCode, int progress, int indexOfItem, int numOfItems, boolean indeterminate);

    // native method to pass via biometric protected walletPassword
    public static native void serveWalletPassword(String walletPassword);

    private volatile boolean qtInitialized = false; // will be accessed from android UI and Qt thread
    private boolean hasPendingIntent = false; // accessed only from android UI thread

    private BoostrapBroadcastReceiver mBoostrapBroadcastReceiver;

    private long backPressedTime;
    private Toast backToast;

    private volatile ActivityResult lastActivityResult;

    private class ActivityResult {
        public ActivityResult(int requestCode, int resultCode, Intent data) {
            this.requestCode = requestCode;
            this.resultCode = resultCode;
            this.data = data;
        }
        public int requestCode;
        public int resultCode;
        public Intent data;
    }


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
        lastActivityResult = null;
        unregisterReceiver(mBoostrapBroadcastReceiver);
    }

    @Override
    protected void onPause() {
        super.onPause();
        informCoreUIpause();
        if (isFinishing()) {
            // fix the unintended 'go back to camera app after qrcode scan' behavior by explictly go the home screen
            Intent mainActivity = new Intent(Intent.ACTION_MAIN);
            mainActivity.addCategory(Intent.CATEGORY_HOME);
            mainActivity.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(mainActivity);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        informCoreUIresume();
        getWindow().setSoftInputMode(softInputMode);
        if (screenOrientationMode == ActivityInfo.SCREEN_ORIENTATION_LOCKED) {
            setRequestedOrientation(screenOrientation);
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LOCKED);
        }
        if (lastActivityResult != null) {
            handleOnActivityResult(lastActivityResult.requestCode, lastActivityResult.resultCode, lastActivityResult.data);
            lastActivityResult = null;
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

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.d(TAG, "onActivityResult() requestCode= " +requestCode);
        lastActivityResult = new ActivityResult(requestCode, resultCode, data);
    }

    private void handleOnActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "handleOnActivityResult() requestCode=" + requestCode);
        if (requestCode == BiometricActivity.BiometricAction.ACTION_SETUP.ordinal()) {
            if (resultCode == RESULT_OK) {
                Toast.makeText(this, "Biometric setup successful!", Toast.LENGTH_SHORT).show();
            }
            else if (resultCode == BiometricActivity.RESULT_ERROR) {
                Toast.makeText(this, "Biometric setup failed: " + data.getStringExtra("error"), Toast.LENGTH_LONG).show();
            }
        }
        else if (requestCode == BiometricActivity.BiometricAction.ACTION_UNLOCK.ordinal()) {
            if (resultCode == RESULT_OK) {
                serveWalletPassword(data.getStringExtra("walletPassword"));
            }
            else if (resultCode == BiometricActivity.RESULT_ERROR) {
                Toast.makeText(this, "Biometric unlock failed: " + data.getStringExtra("error"), Toast.LENGTH_LONG).show();
            }
        }
    }

    public void onBackPressedQt() {
        runOnUiThread(() -> {
            if (backPressedTime + 2000 > System.currentTimeMillis()) {
                backToast.cancel();
                finish();
            } else {
                backToast = Toast.makeText(getBaseContext(), "Press back again to exit", Toast.LENGTH_SHORT);
                backToast.show();
                backPressedTime = System.currentTimeMillis();
            }
        });
    }

    public void startCore(boolean rescan, String bip44key) {
        Intent intent = new Intent(getApplicationContext(), AliasService.class);
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
        intent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        startActivity(intent);
    }

    public boolean hasQRCodeScanner() {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P);
    }

    public boolean isIgnoringBatteryOptimizations() {
        String packageName = getPackageName();
        PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
        return pm.isIgnoringBatteryOptimizations(packageName);
    }

    public void disableBatteryOptimizations() {
        Intent intent = new Intent();
        intent.setAction(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS);
        startActivity(intent);
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

    public boolean isAliasServiceRunning() {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (AliasService.class.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    public void informCoreUIpause() {
        if (isAliasServiceRunning()) {
            Log.d(TAG, "informCoreUIpause()");
            Intent intent = new Intent(getApplicationContext(), AliasService.class);
            intent.setAction(AliasService.ServiceAction.ACTION_UI_PAUSE.name());
            getApplicationContext().startForegroundService(intent);
        }
    }

    public void informCoreUIresume() {
        if (isAliasServiceRunning()) {
            Log.d(TAG, "informCoreUIresume()");
            Intent intent = new Intent(getApplicationContext(), AliasService.class);
            intent.setAction(AliasService.ServiceAction.ACTION_UI_RESUME.name());
            getApplicationContext().startForegroundService(intent);
        }
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


    // ---------------------------------------------------------------------------------------------
    //
    // Biometric Unlock Support
    //
    public boolean setupBiometricUnlock(String walletPassword)  {
        return BiometricActivity.setupBiometricUnlock(this, walletPassword);
    }

    public boolean startBiometricUnlock()  {
        return BiometricActivity.startBiometricUnlock(this);
    }

    public void clearBiometricUnlock()  {
        BiometricActivity.clearBiometricUnlock(this);
    }
}