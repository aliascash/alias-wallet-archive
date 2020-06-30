package org.spectrecoin.wallet;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.Bundle;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtService;

public class SpectrecoinService extends QtService {

    private static final String TAG = "SpectrecoinService";

    public static String nativeLibraryDir;

    private static String CHANNEL_ID_SERVICE = "SPECTRECOIN_SERVICE";
    private static String CHANNEL_ID_WALLET = "SPECTRECOIN_WALLET";
    private static int NOTIFICATION_ID_SERVICE = 100;
    private static int NOTIFICATION_ID_WALLET = 1000;

    public static final String ACTION_STOP = "ACTION_STOP";

    public boolean init = false;
    public boolean rescan = false;
    public String bip44key = "";

    private Notification.Builder notificationBuilder;

    @Override
    public void onCreate() {
        nativeLibraryDir = getApplicationInfo().nativeLibraryDir;

        NotificationManager notificationManager = getSystemService(NotificationManager.class);

        // Create the NotificationChannel for the permanent notification
        CharSequence serviceNotificationName = "Background Service"; //getString(R.string.channel_name);
        String serviceNotificationDescription = "The permanent notification which shows you the state of the Spectrecoin service."; //getString(R.string.channel_description);
        NotificationChannel channelSevice = new NotificationChannel(CHANNEL_ID_SERVICE, serviceNotificationName, NotificationManager.IMPORTANCE_DEFAULT);
        channelSevice.setDescription(serviceNotificationDescription);
        notificationManager.createNotificationChannel(channelSevice);

        // Create the NotificationChannel for notifications
        CharSequence walletNotificationName = "Wallet Notifications"; //getString(R.string.channel_name);
        String walletNotificationDescription = "Shows notifications regarding your wallet like incoming transactions."; //getString(R.string.channel_description);
        NotificationChannel channelWallet = new NotificationChannel(CHANNEL_ID_WALLET, walletNotificationName, NotificationManager.IMPORTANCE_DEFAULT);
        channelWallet.setDescription(walletNotificationDescription);
        notificationManager.createNotificationChannel(channelWallet);

        Intent notificationIntent = new Intent(this, SpectrecoinActivity.class);
        PendingIntent pendingIntent =  PendingIntent.getActivity(this, 0, notificationIntent, 0);

        Intent stopIntent = new Intent(this, SpectrecoinService.class);
        stopIntent.setAction(ACTION_STOP);
        PendingIntent stopPendingIntent = PendingIntent.getService(this, 0, stopIntent, 0);
        Notification.Action stopAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.baseline_stop_black_24), "Shutdown", stopPendingIntent).build();

        notificationBuilder = new Notification.Builder(this, CHANNEL_ID_SERVICE)
                        .setContentTitle("Core Service")//getText(R.string.notification_title))
                        .setContentText("Running...")//getText(R.string.notification_message))
                        .setOnlyAlertOnce(true)
                        .setSmallIcon(R.drawable.icon)
                        .setContentIntent(pendingIntent)
                        .addAction(stopAction);
                        //.setTicker(getText(R.string.ticker_text));
        Notification notification = notificationBuilder.build();
        notificationManager.notify(NOTIFICATION_ID_SERVICE, notification);
        startForeground(NOTIFICATION_ID_SERVICE, notification);

        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if(action!=null)
                switch (action) {
                    case ACTION_STOP:
                        this.stopService(new Intent(this, SpectrecoinService.class));
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

    public void updateNotification(String title, String text) {
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationBuilder.setContentTitle(title);
        notificationBuilder.setContentText(text);
        notificationManager.notify(NOTIFICATION_ID_SERVICE, notificationBuilder.build());
    }

    public void createNotification(String title, String text) {
        NotificationManager notificationManager = getSystemService(NotificationManager.class);

        Intent notificationIntent = new Intent(this, SpectrecoinActivity.class);
        PendingIntent pendingIntent =  PendingIntent.getActivity(this, 0, notificationIntent, 0);

        Notification.Builder notificationBuilder = new Notification.Builder(this, CHANNEL_ID_WALLET)
                .setContentTitle(title)//getText(R.string.notification_title))
                .setContentText(text)//getText(R.string.notification_message))
                .setSmallIcon(R.drawable.icon)
                .setContentIntent(pendingIntent);
        //.setTicker(getText(R.string.ticker_text));
        notificationManager.notify(NOTIFICATION_ID_WALLET, notificationBuilder.build());
    }
}
