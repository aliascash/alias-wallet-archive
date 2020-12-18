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
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.provider.MediaStore;
import android.provider.Settings;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import android.util.Log;
import android.view.WindowManager;
import android.webkit.WebView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.biometric.BiometricManager;
import androidx.biometric.BiometricPrompt;
import androidx.core.content.ContextCompat;

import org.qtproject.qt5.android.bindings.QtFragmentActivity;

import java.io.IOException;
import java.nio.charset.Charset;
import java.security.InvalidAlgorithmParameterException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.util.concurrent.Executor;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;


public class AliasActivity extends QtFragmentActivity {

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

    private static final String PREF_WALLET_PASSWORD = "PREF_WALLET_PASSWORD";
    private static final String PREF_WALLET_PASSWORD_IV = "PREF_WALLET_PASSWORD_IV";
    private boolean biometricSupport = false;
    private Executor executor;
    private BiometricPrompt biometricPrompt;
    private BiometricPrompt.PromptInfo promptSetupInfo;
    private BiometricPrompt.PromptInfo promptUnlockInfo;
    private volatile String walletPassword;


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

        // Biometric Unlock Support
        try {
            biometricSupport = initBiometric();
            Log.d(TAG, "initBiometric(): " + biometricSupport);
        } catch (Exception ex) {
            Log.e(TAG, "initBiometric(): Exception!", ex);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
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
    public boolean setupBiometricUnlock(String walletPassword) {
        if (!biometricSupport || walletPassword == null || walletPassword.isEmpty()) {
            return false;
        }
        try {
            this.walletPassword = walletPassword;
            Cipher cipher = getCipher();
            SecretKey secretKey = getSecretKey();
            cipher.init(Cipher.ENCRYPT_MODE, secretKey);
            runOnUiThread(() -> {
                biometricPrompt.authenticate(promptSetupInfo, new BiometricPrompt.CryptoObject(cipher));
            });
            return true;
        } catch (Exception ex) {
            Log.e(TAG, "setupBiometricUnlock()", ex);
            clearBiometricUnlock();
            return false;
        }
    }

    public boolean startBiometricUnlock()  {
        if (!biometricSupport) {
            return false;
        }
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        String encryptedWalletPassword = sharedPref.getString(PREF_WALLET_PASSWORD, null);
        String encryptedWalletPasswordIv = sharedPref.getString(PREF_WALLET_PASSWORD_IV, null);
        if (encryptedWalletPassword == null || encryptedWalletPasswordIv == null) {
            Log.d(TAG, "startBiometricUnlock(): no encryptedWalletPassword");
            return false;
        }
        try {
            this.walletPassword = null;
            Cipher cipher = getCipher();
            SecretKey secretKey = getSecretKey();
            cipher.init(Cipher.DECRYPT_MODE, secretKey, new IvParameterSpec(Base64.decode(encryptedWalletPasswordIv, Base64.NO_WRAP)));
            runOnUiThread(() -> {
                biometricPrompt.authenticate(promptUnlockInfo, new BiometricPrompt.CryptoObject(cipher));
            });
            return true;
        }
        catch (Exception ex) {
            Log.e(TAG, "startBiometricUnlock()", ex);
            clearBiometricUnlock();
            return false;
        }
    }

    public void clearBiometricUnlock()  {
        Log.d(TAG, "clearBiometricUnlock()");
        this.walletPassword = null;
        SharedPreferences.Editor prefEditor = getPreferences(Context.MODE_PRIVATE).edit();
        prefEditor.remove(PREF_WALLET_PASSWORD).remove(PREF_WALLET_PASSWORD_IV).apply();

        try {
            clearSecretKey();
        }
        catch (Exception ex) {
            Log.e(TAG, "clearBiometricUnlock: exception when removing secret key.", ex);
        }
    }

    private boolean initBiometric() throws InvalidAlgorithmParameterException, NoSuchAlgorithmException, NoSuchProviderException, UnrecoverableKeyException, CertificateException, KeyStoreException, IOException {
        if (BiometricManager.from(getApplicationContext()).canAuthenticate(BiometricManager.Authenticators.BIOMETRIC_STRONG) != BiometricManager.BIOMETRIC_SUCCESS) {
            return false;
        }

        // initialize Key
        getSecretKey();

        executor = ContextCompat.getMainExecutor(this);

        biometricPrompt = new BiometricPrompt(this, executor, new BiometricPrompt.AuthenticationCallback() {
            @Override
            public void onAuthenticationError(int errorCode, @NonNull CharSequence errString) {
                super.onAuthenticationError(errorCode, errString);
                Log.d(TAG, "BiometricPrompt: onAuthenticationError: " + errString);
                walletPassword = null;
                switch (errorCode) {
                    case BiometricPrompt.ERROR_CANCELED:
                    case BiometricPrompt.ERROR_USER_CANCELED:
                    case BiometricPrompt.ERROR_NEGATIVE_BUTTON:
                    case BiometricPrompt.ERROR_TIMEOUT:
                    case BiometricPrompt.ERROR_LOCKOUT:
                        break;
                    default:
                        clearBiometricUnlock();
                }
            }

            @Override
            public void onAuthenticationSucceeded(@NonNull BiometricPrompt.AuthenticationResult result) {
                super.onAuthenticationSucceeded(result);
                String encryptedWalletPassword = getPreferences(Context.MODE_PRIVATE).getString(PREF_WALLET_PASSWORD, null);
                if (walletPassword != null) {
                    // Encryption Mode
                    try {
                        byte[] encryptedInfo = result.getCryptoObject().getCipher().doFinal(walletPassword.getBytes(Charset.defaultCharset()));
                        SharedPreferences.Editor prefEditor = getPreferences(Context.MODE_PRIVATE).edit();
                        prefEditor.putString(PREF_WALLET_PASSWORD, Base64.encodeToString(encryptedInfo, Base64.NO_WRAP));
                        prefEditor.putString(PREF_WALLET_PASSWORD_IV, Base64.encodeToString(result.getCryptoObject().getCipher().getIV(), Base64.NO_WRAP));
                        prefEditor.apply();
                        Toast.makeText(getApplicationContext(), "Biometric setup successful!", Toast.LENGTH_SHORT).show();
                    } catch (Exception e) {
                        Log.e(TAG, "BiometricPrompt: onAuthenticationSucceeded() Encryption Exception", e);
                        Toast.makeText(getApplicationContext(), "Biometric setup failed: " + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
                        clearBiometricUnlock();
                    }
                }
                else if (encryptedWalletPassword != null) {
                    // Decryption Mode
                    try {
                        byte[] decryptedInfo = result.getCryptoObject().getCipher().doFinal(Base64.decode(encryptedWalletPassword, Base64.NO_WRAP));
                        String decryptedInfoString = new String(decryptedInfo, Charset.defaultCharset());
                        serveWalletPassword(decryptedInfoString);
                    } catch (Exception e) {
                        Log.e(TAG, "UnlockBiometricPrompt: onAuthenticationSucceeded() Decryption Exception", e);
                        Toast.makeText(getApplicationContext(), "Biometric unlock failed: " + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
                        clearBiometricUnlock();
                    }
                }
                else {
                    Log.w(TAG, "UnlockBiometricPrompt: onAuthenticationSucceeded() No walletPassword for encryption nor encryptedWalletPassword for decryption available!");
                }
                walletPassword = null;
            }

            @Override
            public void onAuthenticationFailed() {
                super.onAuthenticationFailed();
                Log.d(TAG, "BiometricPrompt: onAuthenticationFailed");
            }
        });

        promptSetupInfo = new BiometricPrompt.PromptInfo.Builder()
                .setTitle("Setup Biometric Wallet Unlock")
                .setSubtitle("Alternative for wallet password")
                .setNegativeButtonText("Cancel")
                .setAllowedAuthenticators(BiometricManager.Authenticators.BIOMETRIC_STRONG)
                .build();

        promptUnlockInfo = new BiometricPrompt.PromptInfo.Builder()
                .setTitle("Unlock Alias Wallet Password")
                .setNegativeButtonText("Enter Password")
                .setAllowedAuthenticators(BiometricManager.Authenticators.BIOMETRIC_STRONG)
                .build();

        return true;
    }
    private SecretKey getSecretKey() throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException, UnrecoverableKeyException, NoSuchProviderException, InvalidAlgorithmParameterException {
        KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
        // Before the keystore can be accessed, it must be loaded.
        keyStore.load(null);
        SecretKey secretKey = ((SecretKey) keyStore.getKey("AliasWalletPassword", null));

        if (secretKey == null) {
            secretKey = generateSecretKey(new KeyGenParameterSpec.Builder(
                    "AliasWalletPassword",
                    KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
                    .setBlockModes(KeyProperties.BLOCK_MODE_CBC)
                    .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_PKCS7)
                    .setUserAuthenticationRequired(true)
                    .setInvalidatedByBiometricEnrollment(true)
                    .build());
        }
        return secretKey;
    }

    private SecretKey generateSecretKey(KeyGenParameterSpec keyGenParameterSpec) throws InvalidAlgorithmParameterException, NoSuchProviderException, NoSuchAlgorithmException {
        KeyGenerator keyGenerator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore");
        keyGenerator.init(keyGenParameterSpec);
        return keyGenerator.generateKey();
    }

    private void clearSecretKey() throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException {
        KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
        // Before the keystore can be accessed, it must be loaded.
        keyStore.load(null);
        if (keyStore.containsAlias("AliasWalletPassword")) {
            keyStore.deleteEntry("AliasWalletPassword");
        }
    }

    private Cipher getCipher() throws NoSuchPaddingException, NoSuchAlgorithmException {
        return Cipher.getInstance(KeyProperties.KEY_ALGORITHM_AES + "/"  + KeyProperties.BLOCK_MODE_CBC + "/" + KeyProperties.ENCRYPTION_PADDING_PKCS7);
    }
}