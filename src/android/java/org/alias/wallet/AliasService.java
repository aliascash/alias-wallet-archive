package org.alias.wallet;

/*
 * SPDX-FileCopyrightText: © 2020 Alias Developers
 * SPDX-License-Identifier: MIT
 */

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Icon;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtService;

import java.util.Objects;

public class AliasService extends QtService {

    private static final String TAG = "AliasService";

    public static String nativeLibraryDir;

    public static String CHANNEL_ID_SERVICE = "ALIAS_SERVICE";
    private static String CHANNEL_ID_WALLET = "ALIAS_WALLET";
    private static int NOTIFICATION_ID_SERVICE = 100;
    private static int NOTIFICATION_ID_WALLET = 1000;

    private static int SERVICE_NOTIFICATION_TYPE_INIT = 1;
    private static int SERVICE_NOTIFICATION_TYPE_NO_CONNECTION = 2;
    private static int SERVICE_NOTIFICATION_TYPE_NO_IMPORTING = 3;
    private static int SERVICE_NOTIFICATION_TYPE_SYNCING = 4;
    private static int SERVICE_NOTIFICATION_TYPE_SYNCED = 5;
    private static int SERVICE_NOTIFICATION_TYPE_STAKING = 6;
    private static int SERVICE_NOTIFICATION_TYPE_REWINDCHAIN = 7;

    private static String WALLET_NOTIFICATION_TYPE_TX_STAKED = "staked";
    private static String WALLET_NOTIFICATION_TYPE_TX_DONATED = "donated";
    private static String WALLET_NOTIFICATION_TYPE_TX_CONTRIBUTED = "contributed";
    private static String WALLET_NOTIFICATION_TYPE_TX_INPUT = "input";
    private static String WALLET_NOTIFICATION_TYPE_TX_OUTPUT = "output";
    private static String WALLET_NOTIFICATION_TYPE_TX_INOUT = "inout";

    public static final String ACTION_STOP = "ACTION_STOP";

    public boolean init = false;
    public boolean rescan = false;
    public String bip44key = "";

    private String lastWalletNotificationTitle;
    private String lastWalletNotificationText;
    private int lastServiceNotificationType = 0;
    private int sameNotificationCounter;

    private Notification.Builder notificationBuilder;
    private PowerManager.WakeLock wakeLock;
    private WifiManager.WifiLock wifiLock;

    public static void createServiceNotificationChannel(Service service) {
        NotificationManager notificationManager = service.getSystemService(NotificationManager.class);

        // Create the NotificationChannel for the permanent notification
        CharSequence serviceNotificationName = "Background Service"; //getString(R.string.channel_name);
        String serviceNotificationDescription = "The permanent notification which shows you the state of the Alias service."; //getString(R.string.channel_description);
        NotificationChannel channelSevice = new NotificationChannel(CHANNEL_ID_SERVICE, serviceNotificationName, NotificationManager.IMPORTANCE_LOW);
        channelSevice.setShowBadge(false);
        channelSevice.setDescription(serviceNotificationDescription);
        notificationManager.createNotificationChannel(channelSevice);
    }

    @Override
    public void onCreate() {
        nativeLibraryDir = getApplicationInfo().nativeLibraryDir;

        NotificationManager notificationManager = getSystemService(NotificationManager.class);

        // Create the NotificationChannel for the permanent notification
        createServiceNotificationChannel(this);

        // Create the NotificationChannel for notifications
        CharSequence walletNotificationName = "Wallet Notifications"; //getString(R.string.channel_name);
        String walletNotificationDescription = "Shows notifications regarding your wallet like incoming transactions."; //getString(R.string.channel_description);
        NotificationChannel channelWallet = new NotificationChannel(CHANNEL_ID_WALLET, walletNotificationName, NotificationManager.IMPORTANCE_DEFAULT);
        channelWallet.setDescription(walletNotificationDescription);
        notificationManager.createNotificationChannel(channelWallet);

        Intent notificationIntent = new Intent(this, AliasActivity.class);
        PendingIntent pendingIntent =  PendingIntent.getActivity(this, 0, notificationIntent, 0);

        Intent stopIntent = new Intent(this, AliasService.class);
        stopIntent.setAction(ACTION_STOP);
        PendingIntent stopPendingIntent = PendingIntent.getService(this, 0, stopIntent, 0);
        Notification.Action stopAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.baseline_stop_black_24), "Shutdown", stopPendingIntent).build();

        notificationBuilder = new Notification.Builder(this, CHANNEL_ID_SERVICE)
                        .setContentTitle("Core Service")//getText(R.string.notification_title))
                        .setContentText("Running...")//getText(R.string.notification_message))
                        .setOnlyAlertOnce(true)
                        .setSmallIcon(R.drawable.ic_alias_app_white)
                        .setColor(getColor(R.color.primary))
                        .setContentIntent(pendingIntent)
                        .addAction(stopAction);
                        //.setTicker(getText(R.string.ticker_text));
        Notification notification = notificationBuilder.build();
        notificationManager.notify(NOTIFICATION_ID_SERVICE, notification);
        startForeground(NOTIFICATION_ID_SERVICE, notification);


        PowerManager powerManager = (PowerManager) getApplicationContext().getSystemService(POWER_SERVICE);
        wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "AliasWallet::StakingWakeLockTag");

        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        wifiLock = wifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "AliasWallet::StakingWiFiLockTag");

        super.onCreate();
    }

    @Override
    public void onDestroy() {
        setBusyMode(false);
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if(action!=null)
                switch (action) {
                    case ACTION_STOP:
                        stopForeground(true);
                        this.stopService(new Intent(this, AliasService.class));
                        return START_NOT_STICKY;
                }

            Bundle bundle = intent.getExtras();
            if (bundle != null) {
                rescan = bundle.getBoolean("rescan", false);
                Log.d(TAG, "onStartCommand rescan=" + rescan);
                bip44key = bundle.getString("bip44key", "");
                //Log.d(TAG, "onStartCommand bip44key=" + bip44key);
            }
        }
        init = true;
        return super.onStartCommand(intent, flags, startId);
    }

    public void setBusyMode(boolean busy) {
        Log.d(TAG, "setBusyMode=" + busy);
        if (busy) {
            if (!wakeLock.isHeld()) {
                wakeLock.acquire();
            }
            if (!wifiLock.isHeld()) {
                wifiLock.acquire();
            }
        }
        else {
            if (wakeLock.isHeld()) {
                wakeLock.release();
            }
            if (wifiLock.isHeld()) {
                wifiLock.release();
            }
        }
    }

    public void stopCore() {
        Intent intent = new Intent(getApplicationContext(), AliasService.class);
        intent.setAction(AliasService.ACTION_STOP);
        getApplicationContext().startForegroundService(intent);
    }

    public void updateNotification(String title, String text, int type) {
        if (lastServiceNotificationType == SERVICE_NOTIFICATION_TYPE_REWINDCHAIN) {
            return; // don't update notification during rewindchain
        }
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationBuilder.setContentTitle(title);
        notificationBuilder.setContentText(text);
        notificationBuilder.setProgress(0, 0, false);
        notificationBuilder.setLargeIcon((Icon)null);
        if (SERVICE_NOTIFICATION_TYPE_STAKING == type) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_staking));
        }
        else if (SERVICE_NOTIFICATION_TYPE_REWINDCHAIN == type) {
            notificationBuilder.setProgress(0, 0, true);
        }
        lastServiceNotificationType = type;
        notificationManager.notify(NOTIFICATION_ID_SERVICE, notificationBuilder.build());
    }

    public void createWalletNotification(String type, String title, String text) {
        NotificationManager notificationManager = getSystemService(NotificationManager.class);

        Intent notificationIntent = new Intent(this, AliasActivity.class);
        PendingIntent pendingIntent =  PendingIntent.getActivity(this, 0, notificationIntent, 0);

        boolean sameNotification = false;
        if (Objects.equals(title, lastWalletNotificationTitle) && Objects.equals(text, lastWalletNotificationText)) {
            StatusBarNotification[] notifications = notificationManager.getActiveNotifications();
            for (StatusBarNotification notification : notifications) {
                if (notification.getId() == NOTIFICATION_ID_WALLET) {
                    sameNotification = true;
                    break;
                }
            }
        }
        sameNotificationCounter = sameNotification ? ++sameNotificationCounter : 0;
        String titleForNotification = sameNotificationCounter > 0 ? title + " [" + (sameNotificationCounter + 1) + "]" : title;

        Notification.Builder notificationBuilder = new Notification.Builder(this, CHANNEL_ID_WALLET)
                .setContentTitle(titleForNotification)//getText(R.string.notification_title))
                .setContentText(text)//getText(R.string.notification_message))
                .setSmallIcon(R.drawable.ic_alias_app_white)
                .setColor(getColor(R.color.primary))
                .setAutoCancel(true)
                .setContentIntent(pendingIntent);
        if (WALLET_NOTIFICATION_TYPE_TX_STAKED.equalsIgnoreCase(type) ||
                WALLET_NOTIFICATION_TYPE_TX_DONATED.equalsIgnoreCase(type) ||
                WALLET_NOTIFICATION_TYPE_TX_CONTRIBUTED.equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_staked));
        } else if (WALLET_NOTIFICATION_TYPE_TX_INPUT.equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_input));
        } else if (WALLET_NOTIFICATION_TYPE_TX_OUTPUT.equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_output));
        } else if (WALLET_NOTIFICATION_TYPE_TX_INOUT.equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_inout));
        }
        //.setTicker(getText(R.string.ticker_text));
        notificationManager.notify(NOTIFICATION_ID_WALLET, notificationBuilder.build());

        lastWalletNotificationTitle = title;
        lastWalletNotificationText = text;
    }
}
