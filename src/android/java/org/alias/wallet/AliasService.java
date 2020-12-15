package org.alias.wallet;

/*
 * SPDX-FileCopyrightText: © 2020 Alias Developers
 * SPDX-License-Identifier: MIT
 */

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Icon;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.SystemClock;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtService;

import java.util.Objects;
import java.util.Timer;
import java.util.TimerTask;

public class AliasService extends QtService {

    private static final String TAG = "AliasService";

    public static String nativeLibraryDir;

    // native method to set the core running mode
    public static native void setCoreRunningMode(int coreRunningMode);
    private enum CoreRunningMode {
        NORMAL,
        UI_PAUSED,
        REQUEST_SYNC_SLEEP,
        SLEEP
    }

    public enum ServiceAction {
        ACTION_STOP,
        ACTION_UI_PAUSE,
        ACTION_UI_RESUME,
        ACTION_SYNC,
        ACTION_TURN_OFF_BATTERY_SAVE,
        ACTION_TURN_ON_BATTERY_SAVE
    }

    private enum ServiceNotificationType {
        UNDEFINED,
        INIT,
        NO_CONNECTION,
        NO_IMPORTING,
        SYNCING,
        SYNCED,
        STAKING,
        REWINDCHAIN,
        SLEEP,
        SHUTDOWN;
    }
    
    private enum WalletNotificationType {
        STAKED,
        DONATED,
        CONTRIBUTED,
        INPUT,
        OUTPUT,
        INOUT
    }

    public static final String CHANNEL_ID_SERVICE = "ALIAS_SERVICE";
    private static final String CHANNEL_ID_WALLET = "ALIAS_WALLET";
    private static final int NOTIFICATION_ID_SERVICE = 100;
    private static final int NOTIFICATION_ID_WALLET = 1000;

    private static final int SERVICE_SLEEP_DELAY = 60000; // delay in milliseconds

    public boolean init = false;
    public boolean rescan = false;

    private String lastWalletNotificationTitle;
    private String lastWalletNotificationText;
    private int lastServiceNotificationType = 0;
    private int sameNotificationCounter;

    private NotificationManager notificationManager;
    private Notification.Builder notificationBuilder;
    private Notification.Action stopAction;
    private Notification.Action turnOffBatterySaveAction;
    private Notification.Action turnOnBatterySaveAction;

    private PowerManager.WakeLock wakeLock;
    private WifiManager.WifiLock wifiLock;
    private BatteryManager batteryManager;
    private AlarmManager alarmManager;

    private volatile boolean serviceIsStopping = false;
    private volatile boolean uiPaused = false;
    private volatile boolean busy = false;
    private volatile boolean batterySaveModeEnabled = true;
    private Timer longTimer;

    private BroadcastReceiver powerConnectedReceiver;
    private BroadcastReceiver powerDisconnectedReceiver;

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
        Log.d(TAG, "onCreate()");

        nativeLibraryDir = getApplicationInfo().nativeLibraryDir;

        notificationManager = getApplicationContext().getSystemService(NotificationManager.class);

        // Set batterySaveModeEnabled depending on if phone is currently charged
        batteryManager = (BatteryManager) getApplicationContext().getSystemService(BATTERY_SERVICE);
        batterySaveModeEnabled = !batteryManager.isCharging();

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
        stopIntent.setAction(ServiceAction.ACTION_STOP.name());
        PendingIntent stopPendingIntent = PendingIntent.getService(this, 0, stopIntent, 0);
        stopAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.baseline_stop_black_24), "Shutdown", stopPendingIntent).build();

        Intent turnOffPowerSaveIntent = new Intent(this, AliasService.class);
        turnOffPowerSaveIntent.setAction(ServiceAction.ACTION_TURN_OFF_BATTERY_SAVE.name());
        PendingIntent turnOffPowerSavePendingIntent = PendingIntent.getService(this, 0, turnOffPowerSaveIntent, 0);
        turnOffBatterySaveAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.ic_battery_off), "Turn power save off", turnOffPowerSavePendingIntent).build();

        Intent turnOnPowerSaveIntent = new Intent(this, AliasService.class);
        turnOnPowerSaveIntent.setAction(ServiceAction.ACTION_TURN_ON_BATTERY_SAVE.name());
        PendingIntent turnOnPowerSavePendingIntent = PendingIntent.getService(this, 0, turnOnPowerSaveIntent, 0);
        turnOnBatterySaveAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.ic_battery_on), "Turn power save on", turnOnPowerSavePendingIntent).build();

        notificationBuilder = new Notification.Builder(this, CHANNEL_ID_SERVICE)
                        .setContentTitle("Core Service")//getText(R.string.notification_title))
                        .setContentText("Running...")//getText(R.string.notification_message))
                        .setOnlyAlertOnce(true)
                        .setSmallIcon(R.drawable.ic_alias_app_white)
                        .setColor(getColor(R.color.primary))
                        .setContentIntent(pendingIntent)
                        .addAction(stopAction)
                        .addAction(batterySaveModeEnabled ? turnOffBatterySaveAction : turnOnBatterySaveAction);
                        //.setTicker(getText(R.string.ticker_text));
        Notification notification = notificationBuilder.build();
        notificationManager.notify(NOTIFICATION_ID_SERVICE, notification);
        startForeground(NOTIFICATION_ID_SERVICE, notification);

        PowerManager powerManager = (PowerManager) getApplicationContext().getSystemService(POWER_SERVICE);
        wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "AliasWallet::StakingWakeLockTag");

        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        wifiLock = wifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "AliasWallet::StakingWiFiLockTag");

        alarmManager = (AlarmManager) getApplicationContext().getSystemService(Context.ALARM_SERVICE);

        // ------
        // Register BroadcastReceivers to handle POWER_CONNECTED and POWER_DISCONNECTED
        powerConnectedReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                turnOffPowerSave();
            }
        };
        powerDisconnectedReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                turnOnPowerSave();
            }
        };
        IntentFilter powerConnectedFilter = new IntentFilter(Intent.ACTION_POWER_CONNECTED);
        registerReceiver(powerConnectedReceiver, powerConnectedFilter);
        IntentFilter powerDisconnectedFilter = new IntentFilter(Intent.ACTION_POWER_DISCONNECTED);
        registerReceiver(powerDisconnectedReceiver, powerDisconnectedFilter);

        super.onCreate();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy()");
        setBusyMode(false);
        if (longTimer != null) {
            longTimer.cancel();
            longTimer = null;
        }
        removeSyncAlarm();
        unregisterReceiver(powerConnectedReceiver);
        unregisterReceiver(powerDisconnectedReceiver);
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand(): " + intent);
        if (intent != null) {
            String action = intent.getAction();
            if(action != null) {
                ServiceAction serviceAction = ServiceAction.valueOf(action);
                switch (serviceAction) {
                    case ACTION_STOP:
                        handleShutdown();
                        return START_NOT_STICKY;
                    case ACTION_TURN_OFF_BATTERY_SAVE:
                        turnOffPowerSave();
                        return START_STICKY;
                    case ACTION_TURN_ON_BATTERY_SAVE:
                        turnOnPowerSave();
                        return START_STICKY;
                    case ACTION_UI_PAUSE:
                        handleUIPause();
                        return START_STICKY;
                    case ACTION_UI_RESUME:
                        handleUIResume();
                        return START_STICKY;
                    case ACTION_SYNC:
                        if (uiPaused) {
                            setCoreRunningMode(CoreRunningMode.REQUEST_SYNC_SLEEP.ordinal());
                        }
                        return START_STICKY;
                }
            }

            Bundle bundle = intent.getExtras();
            if (bundle != null) {
                rescan = bundle.getBoolean("rescan", false);
                Log.d(TAG, "onStartCommand rescan=" + rescan);
            }
        }
        init = true;
        return super.onStartCommand(intent, flags, startId);
    }

    public void setBusyMode(boolean busy) {
        Log.d(TAG, "setBusyMode=" + busy);
        this.busy = busy;
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
        notificationBuilder.setActions(stopAction);
        if (!busy) {
            notificationBuilder.addAction(batterySaveModeEnabled ? turnOffBatterySaveAction : turnOnBatterySaveAction);
        }
        updateServiceNotification(notificationBuilder.build());
    }

    public void stopCore() {
        Intent intent = new Intent(getApplicationContext(), AliasService.class);
        intent.setAction(ServiceAction.ACTION_STOP.name());
        getApplicationContext().startForegroundService(intent);
    }

    public void updateNotification(String title, String text, int type) {
        if (lastServiceNotificationType == ServiceNotificationType.REWINDCHAIN.ordinal()) {
            return; // don't update notification during rewindchain
        }
        notificationBuilder.setContentTitle(title);
        notificationBuilder.setContentText(text);
        notificationBuilder.setProgress(0, 0, false);
        notificationBuilder.setLargeIcon((Icon)null);
        if (ServiceNotificationType.STAKING.ordinal() == type) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_staking));
        }
        else if (ServiceNotificationType.REWINDCHAIN.ordinal() == type) {
            notificationBuilder.setProgress(0, 0, true);
        }
        else if (ServiceNotificationType.SHUTDOWN.ordinal() == type) {
            notificationBuilder.setActions();
            notificationBuilder.setProgress(0, 0, true);
        }
        lastServiceNotificationType = type;
        updateServiceNotification(notificationBuilder.build());
    }

    public void createWalletNotification(String type, String title, String text) {
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
        if (WalletNotificationType.STAKED.name().equalsIgnoreCase(type) ||
                WalletNotificationType.DONATED.name().equalsIgnoreCase(type) ||
                WalletNotificationType.CONTRIBUTED.name().equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_staked));
        } else if (WalletNotificationType.INPUT.name().equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_input));
        } else if (WalletNotificationType.OUTPUT.name().equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_output));
        } else if (WalletNotificationType.INOUT.name().equalsIgnoreCase(type)) {
            notificationBuilder.setLargeIcon(Icon.createWithResource(this, R.drawable.ic_tx_inout));
        }
        //.setTicker(getText(R.string.ticker_text));
        notificationManager.notify(NOTIFICATION_ID_WALLET, notificationBuilder.build());

        lastWalletNotificationTitle = title;
        lastWalletNotificationText = text;
    }

    private void handleShutdown() {
        updateNotification("Alias is shutting down...", "", ServiceNotificationType.SHUTDOWN.ordinal());
        serviceIsStopping = true;
        stopForeground(false);
        this.stopService(new Intent(this, AliasService.class));
    }

    private synchronized void handleUIPause() {
        Log.d(TAG, "handleUIPause()");
        uiPaused = true;
        setCoreRunningMode(CoreRunningMode.UI_PAUSED.ordinal());
        if (longTimer != null) {
            longTimer.cancel();
            longTimer = null;
        }
        if (longTimer == null) {
            longTimer = new Timer();
            longTimer.schedule(new TimerTask() {
                public void run() {
                    synchronized (AliasService.this) {
                        if (longTimer != null) {
                            cancel();
                            longTimer = null;
                            if (uiPaused && batterySaveModeEnabled) {
                                setCoreRunningMode(CoreRunningMode.REQUEST_SYNC_SLEEP.ordinal());
                                createSyncAlarm();
                            }
                        }
                    }
                }
            }, SERVICE_SLEEP_DELAY);
        }
    }

    private synchronized void handleUIResume() {
        Log.d(TAG, "handleUIResume()");
        uiPaused = false;
        setCoreRunningMode(CoreRunningMode.NORMAL.ordinal());
        if (longTimer != null) {
            longTimer.cancel();
            longTimer = null;
        }
        removeSyncAlarm();
    }

    private synchronized void turnOffPowerSave() {
        Log.d(TAG, "turnOffPowerSave()");
        batterySaveModeEnabled = false;
        removeSyncAlarm();
        setCoreRunningMode(uiPaused ? CoreRunningMode.UI_PAUSED.ordinal() : CoreRunningMode.NORMAL.ordinal());
        if (!busy) {
            notificationBuilder.setActions(stopAction, turnOnBatterySaveAction);
            updateServiceNotification(notificationBuilder.build());
        }
    }

    private synchronized void turnOnPowerSave() {
        Log.d(TAG, "turnOnPowerSave()");
        batterySaveModeEnabled = true;
        if (uiPaused) {
            setCoreRunningMode(CoreRunningMode.REQUEST_SYNC_SLEEP.ordinal());
            createSyncAlarm();
        }
        if (!busy) {
            notificationBuilder.setActions(stopAction, turnOffBatterySaveAction);
            updateServiceNotification(notificationBuilder.build());
        }
    }

    private void createSyncAlarm() {
        Log.d(TAG, "createSyncAlarm()");
        Intent syncIntent = new Intent(this, AliasService.class);
        syncIntent.setAction(ServiceAction.ACTION_SYNC.name());
        PendingIntent syncPendingIntent = PendingIntent.getService(this, 0, syncIntent, 0);

        alarmManager.setInexactRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + AlarmManager.INTERVAL_HOUR,
                AlarmManager.INTERVAL_HOUR, syncPendingIntent);
    }

    private void removeSyncAlarm() {
        Intent syncIntent = new Intent(this, AliasService.class);
        syncIntent.setAction(ServiceAction.ACTION_SYNC.name());
        PendingIntent pendingIntent = PendingIntent.getService(this, 0, syncIntent, PendingIntent.FLAG_NO_CREATE);
        if (pendingIntent != null) {
            Log.d(TAG, "removeSyncAlarm(): cancel sync intent");
            alarmManager.cancel(pendingIntent);
        }
    }

    private void updateServiceNotification(Notification notification) {
        if (!serviceIsStopping) {
            notificationManager.notify(NOTIFICATION_ID_SERVICE, notification);
        }
    }
}
