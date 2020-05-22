package org.spectrecoin.wallet;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.graphics.drawable.Icon;

import org.qtproject.qt5.android.bindings.QtService;

public class SpectrecoinService extends QtService {

    public static String nativeLibraryDir;

    private static String CHANNEL_ID = "SPECTRECOIN_SERVICE";
    private static int NOTIFICATION_ID = 100;

    public static final String ACTION_STOP = "ACTION_STOP";

    @Override
    public void onCreate() {
        nativeLibraryDir = getApplicationInfo().nativeLibraryDir;

        // Create the NotificationChannel
        CharSequence name = "Service"; //getString(R.string.channel_name);
        String description = "Spectrecoin Background Service"; //getString(R.string.channel_description);
        int importance = NotificationManager.IMPORTANCE_DEFAULT;
        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
        channel.setDescription(description);
        // Register the channel with the system; you can't change the importance
        // or other notification behaviors after this
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);

        Intent notificationIntent = new Intent(this, SpectrecoinActivity.class);
        PendingIntent pendingIntent =  PendingIntent.getActivity(this, 0, notificationIntent, 0);

        Intent stopIntent = new Intent(this, SpectrecoinService.class);
        stopIntent.setAction(ACTION_STOP);
        PendingIntent stopPendingIntent = PendingIntent.getService(this, 0, stopIntent, 0);
        Notification.Action stopAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.baseline_stop_black_24), "Shutdown", stopPendingIntent).build();

        Notification notification =
                new Notification.Builder(this, CHANNEL_ID)
                        .setContentTitle("Spectrecoin")//getText(R.string.notification_title))
                        .setContentText("Syncing...")//getText(R.string.notification_message))
                        .setSmallIcon(R.drawable.icon)
                        .setContentIntent(pendingIntent)
                        .addAction(stopAction)
                        //.setTicker(getText(R.string.ticker_text))
                        .build();

        notificationManager.notify(NOTIFICATION_ID, notification);
        startForeground(NOTIFICATION_ID, notification);

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
        }
        return super.onStartCommand(intent, flags, startId);
    }
}
