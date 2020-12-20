package org.alias.wallet;

/*
 * SPDX-FileCopyrightText: © 2020 Alias Developers
 * SPDX-License-Identifier: MIT
 */

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.biometric.BiometricManager;
import androidx.biometric.BiometricPrompt;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentActivity;

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


public class BiometricActivity extends FragmentActivity {

    public static final int RESULT_ERROR   = 10;

    public enum BiometricAction {
        ACTION_UNDEFINED,
        ACTION_SETUP,
        ACTION_UNLOCK,
    }

    private static final String TAG = "BiometricActivity";

    private static final String PREF_WALLET_PASSWORD = "PREF_WALLET_PASSWORD";
    private static final String PREF_WALLET_PASSWORD_IV = "PREF_WALLET_PASSWORD_IV";
    private static final String BIOMETRIC_PREFERENCES = "BIOMETRIC_PREFERENCES";
    private static final Object BIOMETRIC_LOCK = new Object();
    private static volatile String walletPassword;
    private static volatile boolean biometricProcessRunning;
    private static volatile Cipher cipher;

    private Executor executor;
    private BiometricPrompt biometricPrompt;
    private BiometricPrompt.PromptInfo promptSetupInfo;
    private BiometricPrompt.PromptInfo promptUnlockInfo;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!initBiometric()) {
            Log.d(TAG, "onCreate(): Biometric authentication not available");
            finish();
        }
        if (BiometricAction.ACTION_SETUP.name().equalsIgnoreCase(getIntent().getAction())) {
            Log.d(TAG, "onCreate(): ACTION_SETUP: start authentication");
            biometricPrompt.authenticate(promptSetupInfo, new BiometricPrompt.CryptoObject(cipher));
        }
        else if (BiometricAction.ACTION_UNLOCK.name().equalsIgnoreCase(getIntent().getAction())) {
            Log.d(TAG, "onCreate(): ACTION_UNLOCK: start authentication");
            biometricPrompt.authenticate(promptUnlockInfo, new BiometricPrompt.CryptoObject(cipher));
        }
        else {
            Log.d(TAG, "onCreate(): intent has no valid action");
            finish();
        }
    }

    static public boolean canBiometricAuthenticate(Context context) {
        boolean biometricSupport = false;
        synchronized (BIOMETRIC_LOCK) {
            try {
                if (BiometricManager.from(context).canAuthenticate() == BiometricManager.BIOMETRIC_SUCCESS) {
                    // initialize Key
                    getSecretKey();
                    biometricSupport = true;
                }
            } catch (Exception ex) {
                Log.e(TAG, "canBiometricAuthenticate(): Exception!", ex);
            }
        }
        Log.d(TAG, "canBiometricAuthenticate(): " + biometricSupport);
        return biometricSupport;
    }

    public static boolean setupBiometricUnlock(Activity activity, String walletPassword) {
        synchronized (BIOMETRIC_LOCK) {
            if (biometricProcessRunning) {
                Log.d(TAG, "setupBiometricUnlock() return true because of biometricProcessRunning");
                return true;
            }
            if (!canBiometricAuthenticate(activity) || walletPassword == null || walletPassword.isEmpty()) {
                return false;
            }
            try {
                biometricProcessRunning = true;
                BiometricActivity.walletPassword = walletPassword;
                cipher = getCipher();
                SecretKey secretKey = getSecretKey();
                cipher.init(Cipher.ENCRYPT_MODE, secretKey);

                Intent intent = new Intent(activity, BiometricActivity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
                intent.setAction(BiometricAction.ACTION_SETUP.name());
                activity.startActivityForResult(intent, BiometricAction.ACTION_SETUP.ordinal());

                return true;
            } catch (Exception ex) {
                Log.e(TAG, "setupBiometricUnlock()", ex);
                biometricProcessRunning = false;
                clearBiometricUnlock(activity);
                return false;
            }
        }

    }

    public static boolean startBiometricUnlock(Activity activity)  {
        synchronized (BIOMETRIC_LOCK) {
            if (biometricProcessRunning) {
                Log.d(TAG, "startBiometricUnlock() return true because of biometricProcessRunning");
                return true;
            }
            if (!canBiometricAuthenticate(activity)) {
                return false;
            }
            SharedPreferences sharedPref = activity.getSharedPreferences(BIOMETRIC_PREFERENCES, Context.MODE_PRIVATE);
            String encryptedWalletPassword = sharedPref.getString(PREF_WALLET_PASSWORD, null);
            String encryptedWalletPasswordIv = sharedPref.getString(PREF_WALLET_PASSWORD_IV, null);
            if (encryptedWalletPassword == null || encryptedWalletPasswordIv == null) {
                Log.d(TAG, "startBiometricUnlock(): no encryptedWalletPassword");
                return false;
            }
            try {
                biometricProcessRunning = true;
                BiometricActivity.walletPassword = null;
                cipher = getCipher();
                SecretKey secretKey = getSecretKey();
                cipher.init(Cipher.DECRYPT_MODE, secretKey, new IvParameterSpec(Base64.decode(encryptedWalletPasswordIv, Base64.NO_WRAP)));

                Intent intent = new Intent(activity, BiometricActivity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
                intent.setAction(BiometricAction.ACTION_UNLOCK.name());
                activity.startActivityForResult(intent, BiometricAction.ACTION_UNLOCK.ordinal());

                return true;
            } catch (Exception ex) {
                Log.e(TAG, "startBiometricUnlock()", ex);
                biometricProcessRunning = false;
                clearBiometricUnlock(activity);
                return false;
            }
        }
    }

    public static void clearBiometricUnlock(Activity activity)  {
        synchronized (BIOMETRIC_LOCK) {
            if (biometricProcessRunning) {
                Log.d(TAG, "clearBiometricUnlock() aborted because of biometricProcessRunning");
                return;
            }
            Log.d(TAG, "clearBiometricUnlock()");
            walletPassword = null;
            SharedPreferences.Editor prefEditor = activity.getSharedPreferences(BIOMETRIC_PREFERENCES, Context.MODE_PRIVATE).edit();
            prefEditor.remove(PREF_WALLET_PASSWORD).remove(PREF_WALLET_PASSWORD_IV).apply();

            try {
                clearSecretKey();
            } catch (Exception ex) {
                Log.e(TAG, "clearBiometricUnlock: exception when removing secret key.", ex);
            }
        }
    }

    private boolean initBiometric() {
        if (!canBiometricAuthenticate(this)) {
            return false;
        }

        executor = ContextCompat.getMainExecutor(this);

        biometricPrompt = new BiometricPrompt(this, executor, new BiometricPrompt.AuthenticationCallback() {
            @Override
            public void onAuthenticationError(int errorCode, @NonNull CharSequence errString) {
                super.onAuthenticationError(errorCode, errString);
                Log.d(TAG, "onAuthenticationError: " + errString);
                synchronized (BIOMETRIC_LOCK) {
                    walletPassword = null;
                    biometricProcessRunning = false;
                    switch (errorCode) {
                        case BiometricPrompt.ERROR_HW_UNAVAILABLE:
                        case BiometricPrompt.ERROR_UNABLE_TO_PROCESS:
                        case BiometricPrompt.ERROR_TIMEOUT:
                        case BiometricPrompt.ERROR_NO_SPACE:
                        case BiometricPrompt.ERROR_CANCELED:
                        case BiometricPrompt.ERROR_LOCKOUT:
                        case BiometricPrompt.ERROR_LOCKOUT_PERMANENT:
                        case BiometricPrompt.ERROR_USER_CANCELED:
                        case BiometricPrompt.ERROR_NEGATIVE_BUTTON:
                            break;
                        default:
                            clearBiometricUnlock(BiometricActivity.this);
                    }
                }
                finish();
            }

            @Override
            public void onAuthenticationSucceeded(@NonNull BiometricPrompt.AuthenticationResult result) {
                super.onAuthenticationSucceeded(result);
                String encryptedWalletPassword =  getSharedPreferences(BIOMETRIC_PREFERENCES, Context.MODE_PRIVATE).getString(PREF_WALLET_PASSWORD, null);
                boolean error = false;
                if (walletPassword != null) {
                    // Encryption Mode
                    try {
                        byte[] encryptedInfo = result.getCryptoObject().getCipher().doFinal(walletPassword.getBytes(Charset.defaultCharset()));
                        SharedPreferences.Editor prefEditor = getSharedPreferences(BIOMETRIC_PREFERENCES, Context.MODE_PRIVATE).edit();
                        prefEditor.putString(PREF_WALLET_PASSWORD, Base64.encodeToString(encryptedInfo, Base64.NO_WRAP));
                        prefEditor.putString(PREF_WALLET_PASSWORD_IV, Base64.encodeToString(result.getCryptoObject().getCipher().getIV(), Base64.NO_WRAP));
                        prefEditor.apply();
                        Log.d(TAG, "onAuthenticationSucceeded(): walletPassword successful encrypted.");
                        setResult(RESULT_OK);
                     } catch (Exception e) {
                        Log.e(TAG, "onAuthenticationSucceeded(): encryption exception", e);
                        Intent data = new Intent();
                        data.putExtra("error", e.getLocalizedMessage());
                        setResult(RESULT_ERROR, data);
                        error = true;
                    }
                }
                else if (encryptedWalletPassword != null) {
                    // Decryption Mode
                    try {
                        byte[] decryptedInfo = result.getCryptoObject().getCipher().doFinal(Base64.decode(encryptedWalletPassword, Base64.NO_WRAP));
                        String decryptedInfoString = new String(decryptedInfo, Charset.defaultCharset());
                        Log.d(TAG, "onAuthenticationSucceeded(): walletPassword successful encrypted.");

                        Intent data = new Intent();
                        data.putExtra("walletPassword",decryptedInfoString);
                        setResult(RESULT_OK, data);
                    } catch (Exception e) {
                        Log.e(TAG, "onAuthenticationSucceeded(): decryption exception", e);
                        Intent data = new Intent();
                        data.putExtra("error", e.getLocalizedMessage());
                        setResult(RESULT_ERROR, data);
                        error = true;
                    }
                }
                else {
                    Log.w(TAG, "onAuthenticationSucceeded(): No walletPassword for encryption nor encryptedWalletPassword for decryption available!");
                }
                synchronized (BIOMETRIC_LOCK) {
                    walletPassword = null;
                    biometricProcessRunning = false;
                    if (error) {
                        clearBiometricUnlock(BiometricActivity.this);
                    }
                }
                finish();
            }

            @Override
            public void onAuthenticationFailed() {
                super.onAuthenticationFailed();
                Log.d(TAG, "onAuthenticationFailed()");
            }
        });

        promptSetupInfo = new BiometricPrompt.PromptInfo.Builder()
                .setTitle("Setup Biometric Wallet Unlock")
                .setSubtitle("Alternative for wallet password")
                .setNegativeButtonText("Cancel")
                .setDeviceCredentialAllowed(false)
                .build();

        promptUnlockInfo = new BiometricPrompt.PromptInfo.Builder()
                .setTitle("Unlock Alias Wallet Password")
                .setNegativeButtonText("Enter Password")
                .setDeviceCredentialAllowed(false)
                .build();

        return true;
    }
    private static SecretKey getSecretKey() throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException, UnrecoverableKeyException, NoSuchProviderException, InvalidAlgorithmParameterException {
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

    private static SecretKey generateSecretKey(KeyGenParameterSpec keyGenParameterSpec) throws InvalidAlgorithmParameterException, NoSuchProviderException, NoSuchAlgorithmException {
        KeyGenerator keyGenerator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore");
        keyGenerator.init(keyGenParameterSpec);
        return keyGenerator.generateKey();
    }

    private static void clearSecretKey() throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException {
        KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
        // Before the keystore can be accessed, it must be loaded.
        keyStore.load(null);
        if (keyStore.containsAlias("AliasWalletPassword")) {
            keyStore.deleteEntry("AliasWalletPassword");
        }
    }

    private static Cipher getCipher() throws NoSuchPaddingException, NoSuchAlgorithmException {
        return Cipher.getInstance(KeyProperties.KEY_ALGORITHM_AES + "/"  + KeyProperties.BLOCK_MODE_CBC + "/" + KeyProperties.ENCRYPTION_PADDING_PKCS7);
    }
}