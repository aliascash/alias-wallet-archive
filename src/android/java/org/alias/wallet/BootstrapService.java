package org.alias.wallet;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.util.Log;

import org.chromium.net.CronetEngine;
import org.chromium.net.impl.JavaCronetProvider;

import java.io.BufferedInputStream;
import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class BootstrapService extends Service {

    private static final String TAG = "BootstrapService";

    public static final String BOOTSTRAP_BROADCAST_ACTION = "org.alias.wallet.boostrap";

    public static final String ACTION_STOP = "ACTION_STOP";

    public static final int STATE_CANCEL = -2;
    public static final int STATE_ERROR = -1;
    public static final int STATE_DOWNLOAD = 1;
    public static final int STATE_EXTRACTION = 2;
    public static final int STATE_FINISHED = 3;

    private static int NOTIFICATION_ID_SERVICE_PROGRESS = 100;
    private static int NOTIFICATION_ID_SERVICE_RESULT = 101;

    private Notification.Builder notificationBuilder;
    private BootstrapTask bootstrapTask;

    private CronetEngine engine;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        // Create the NotificationChannel for the permanent notification
        AliasService.createServiceNotificationChannel(this);

        Intent notificationIntent = new Intent(this, AliasActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);

        Intent stopIntent = new Intent(this, BootstrapService.class);
        stopIntent.setAction(ACTION_STOP);
        PendingIntent stopPendingIntent = PendingIntent.getService(this, 0, stopIntent, 0);
        Notification.Action stopAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.baseline_stop_black_24), "Abort", stopPendingIntent).build();

        notificationBuilder = new Notification.Builder(this, AliasService.CHANNEL_ID_SERVICE)
                .setContentTitle("Blockchain Bootstrap")//getText(R.string.notification_title))
                .setContentText("Downloading...")//getText(R.string.notification_message))
                .setOnlyAlertOnce(true)
                .setSmallIcon(R.drawable.ic_alias_app_white)
                .setColor(ContextCompat.getColor(this, R.color.primary))
                .setContentIntent(pendingIntent)
                .setProgress(100, 0, false)
                .addAction(stopAction);
        //.setTicker(getText(R.string.ticker_text));
        Notification notification = notificationBuilder.build();

        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.cancel(NOTIFICATION_ID_SERVICE_RESULT);
        notificationManager.notify(NOTIFICATION_ID_SERVICE_PROGRESS, notification);
        startForeground(NOTIFICATION_ID_SERVICE_PROGRESS, notification);

        CronetEngine.Builder engineBuilder = new JavaCronetProvider(this).createBuilder();
        engine = engineBuilder.build();

        // Start bootstrap process in separate thread
        bootstrapTask = new BootstrapTask();
        bootstrapTask.execute();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if (action != null)
                switch (action) {
                    case ACTION_STOP:
                        bootstrapTask.cancel(false);
                        return START_NOT_STICKY;
                }
        }
        return super.onStartCommand(intent, flags, startId);
    }


    private class BootstrapTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... voids) {
            NotificationManager notificationManager = getSystemService(NotificationManager.class);

            //
            // Prepare Paths
            //
            Path destinationDir = Paths.get(getFilesDir().toPath().toString(), ".aliaswallet");
            Path bootstrapPath = Paths.get(destinationDir.toString(), "BootstrapChain.zip");
            try {
                //
                // Prepare Download URL
                //
                URL bootstrapURL;
                try {
                    bootstrapURL = new URL("https://download.alias.cash/files/bootstrap/BootstrapChain.zip");
                } catch (MalformedURLException e) {
                    throw new RuntimeException(e);
                }
                //
                // PHASE 1: Download File
                //
                downloadBootstrap(notificationManager, bootstrapURL, bootstrapPath);
                if (isCancelled()) {
                    updateProgress(notificationManager, STATE_CANCEL, 0, false, "Canceled...");
                    return null;
                }

                //
                // PHASE 2: Extract files
                //
                updateProgress(notificationManager, STATE_EXTRACTION, 0,true, "Extracting...");
                prepareBootstrap(bootstrapPath, destinationDir);

                //
                // PHASE 3: Finished
                //
                updateProgress(notificationManager, STATE_FINISHED, 0,false, "Successfully finished!");
            } catch (Exception e) {
                //
                // PHASE -1: Error
                //
                Log.d(TAG, "BootstrapTask: Failed with exception: " + e.getMessage(), e);
                updateProgress(notificationManager, STATE_ERROR, 0,false, "Failed...");
            }
            finally {
                // clean up
                try {
                    Files.deleteIfExists(bootstrapPath);
                }
                catch (Exception ex) {
                    Log.d(TAG, "BootstrapTask: Failed to delete bootstrap file", ex);
                }
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            stopSelf();
        }

        @Override
        protected void onCancelled() {
            super.onCancelled();
            stopSelf();
        }

        private void downloadBootstrap(NotificationManager notificationManager, URL bootstrapURL, Path bootstrapFile) {
            InputStream input = null;
            OutputStream output = null;
            try {
                // Make sure there is no bootstrap file on disc before download
                Files.deleteIfExists(bootstrapFile);
                // Make sure target directory for bootstrap exists
                Files.createDirectories(bootstrapFile.getParent());

                HttpURLConnection connection = (HttpURLConnection)engine.openConnection(bootstrapURL);
                connection.setConnectTimeout(10000);
                connection.connect();
                // this will be useful so that you can show a typical 0-100% progress bar
                int fileLength = connection.getContentLength();
                if (fileLength == -1) {
                    updateProgress(notificationManager, STATE_DOWNLOAD, 0, true, "Downloading...");
                }

                input = new BufferedInputStream(bootstrapURL.openStream());
                output = new FileOutputStream(bootstrapFile.toFile());

                // download the file and write to disk
                byte data[] = new byte[1024];
                long total = 0;
                int count, lastProgress = -1;
                while ((count = input.read(data)) != -1 && !isCancelled()) {
                    total += count;
                    // publishing the progress....
                    if (fileLength != -1) {
                        int progress = (int) (total * 100 / fileLength);
                        if (lastProgress != progress) {
                            updateProgress(notificationManager, STATE_DOWNLOAD, progress, false, "Downloading...");
                            lastProgress = progress;
                        }
                    }
                    output.write(data, 0, count);
                }
                output.flush();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            finally {
                // clean up
                close(input);
                close(output);
            }
        }

        private void prepareBootstrap(Path bootstrapPath, Path destinationDir) throws Exception {
            // delete existing blockchain data
            deleteBlockchainFiles(destinationDir);

            // extract bootstrap zip file
            try (ZipFile zf = new ZipFile(bootstrapPath.toFile())) {
                Enumeration<? extends ZipEntry> entries = zf.entries();
                while(entries.hasMoreElements()) {
                    ZipEntry entry = entries.nextElement();
                    Path targetPath = newPath(destinationDir, entry);
                    if (entry.isDirectory()) {
                        Files.createDirectories(targetPath);
                    } else {
                        Files.copy(zf.getInputStream(entry), targetPath);
                    }
                }
            }
            catch (Exception ex) {
                // cleanup blockchain files after exception
                try {
                    deleteBlockchainFiles(destinationDir);
                }
                catch (Exception ex2) {
                    Log.d(TAG, "prepareBootstrap: Failed with delete extracted bootstrap files after exception: " + ex2.getMessage(), ex2);
                }
                throw ex;
            }
        }
    }

    protected void updateProgress(NotificationManager notificationManager, int state, int progress, boolean indeterminate, String text) {
        int max = state == STATE_DOWNLOAD ? 100 : 0;
        if (state != STATE_CANCEL) {
            notificationBuilder.setProgress(max, progress, indeterminate);
            notificationBuilder.setContentText(text);
            // Final notification is shown with separate Id that its not closed when service is stopped.
            int notificationId = (state == STATE_ERROR || state == STATE_FINISHED) ? NOTIFICATION_ID_SERVICE_RESULT : NOTIFICATION_ID_SERVICE_PROGRESS;
            notificationManager.notify(notificationId, notificationBuilder.build());
        }
        sendBootstrapProgressBroadcast(state, progress, indeterminate);
    }

    protected void sendBootstrapProgressBroadcast(int state, int progress, boolean indeterminate) {
        Intent broadCastIntent = new Intent();
        broadCastIntent.setAction(BOOTSTRAP_BROADCAST_ACTION);
        broadCastIntent.putExtra("state", state);
        broadCastIntent.putExtra("progress", progress);
        broadCastIntent.putExtra("indeterminate", indeterminate);
        sendBroadcast(broadCastIntent);
    }

    private static void deleteBlockchainFiles(Path coreDataDir) throws IOException {
        // delete existing blockchain data
        Path pathTxleveldb = Paths.get(coreDataDir.toString(), "txleveldb");
        if (Files.exists(pathTxleveldb)) {
            Files.walk(pathTxleveldb)
                    .sorted(Comparator.reverseOrder())
                    .map(Path::toFile)
                    .forEach(File::delete);
            if (Files.exists(pathTxleveldb)) {
                throw new RuntimeException("prepareBootstrap: failed to delete: " + pathTxleveldb);
            }
        }
        Path pathBlkDat = Paths.get(coreDataDir.toString(), "blk0001.dat");
        Files.deleteIfExists(pathBlkDat);
        if (Files.exists(pathBlkDat)) {
            throw new RuntimeException("prepareBootstrap: failed to delete: " + pathBlkDat);
        }
    }

    public static Path newPath(Path destinationDir, ZipEntry zipEntry) throws IOException {
        Path destPath = Paths.get(destinationDir.toString(), zipEntry.getName());
        if (!destPath.startsWith(destinationDir)) {
            throw new IOException("Entry is outside of the target dir: " + zipEntry.getName());
        }
        return destPath;
    }

    public static void close(Closeable c) {
        if (c == null) return;
        try {
            c.close();
        } catch (IOException e) {
            Log.d(TAG, "startDownload: Failed to close Closeable", e);
        }
    }
}
