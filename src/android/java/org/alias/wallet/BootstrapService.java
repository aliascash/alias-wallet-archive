package org.alias.wallet;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.AsyncTask;
import android.os.IBinder;
import android.util.Log;

import com.google.android.gms.net.CronetProviderInstaller;
import com.google.android.gms.tasks.Task;

import net.lingala.zip4j.ZipFile;
import net.lingala.zip4j.exception.ZipException;

import org.chromium.net.CronetEngine;
import org.chromium.net.impl.JavaCronetProvider;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.time.Clock;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import static org.alias.wallet.BootstrapService.BootstrapError.*;

public class BootstrapService extends Service {

    private static final String TAG = "BootstrapService";

    public static final String BOOTSTRAP_BROADCAST_ACTION = "org.alias.wallet.bootstrap";

    public static final String ACTION_STOP = "ACTION_STOP";

    public static final String BOOTSTRAP_BASE_URL = "https://download.alias.cash/files/bootstrap/";

    public static final int STATE_DOWNLOAD = 1;
    public static final int STATE_EXTRACTION = 2;
    public static final int STATE_FINISHED = 3;
    public static final int STATE_CANCEL = -1;
    public static final int STATE_ERROR = -2;

    private static final String ERROR_MSG_ENOSPC = "ENOSPC";
    private static final String ERROR_MSG_BOOTSTRAP_INDEX_404 = "ERROR_BOOTSTRAP_INDEX_NOT_FOUND";
    private static final String ERROR_MSG_BOOTSTRAP_FILE_404 = "ERROR_BOOTSTRAP_FILE_NOT_FOUND";
    private static final String ERROR_MSG_BOOTSTRAP_HASH = "ERROR_BOOTSTRAP_HASH";

    private static int NOTIFICATION_ID_SERVICE_PROGRESS = 100;
    private static int NOTIFICATION_ID_SERVICE_RESULT = 101;

    private Notification.Builder notificationBuilder;

    private BootstrapTask bootstrapTask;
    private Notification.Action stopAction;

    private CronetEngine engine;

    public enum BootstrapError {
        UNDEFINED,
        ERROR_NOSPACE_DOWNLOAD,
        ERROR_NOSPACE_EXTRACTION,
        ERROR_EXTRACTION,
        ERROR_HASH,
        ERROR_BOOTSTRAP_INDEX_404,
        ERROR_BOOTSTRAP_FILE_404
    }

    public static class BootstrapPartDefinition {
        public String hash;
        public Path path;
        public URL url;

        public BootstrapPartDefinition(String hash, Path path, URL url) {
            this.hash = hash;
            this.path = path;
            this.url = url;
        }
    }

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
        stopAction = new Notification.Action.Builder(Icon.createWithResource(this, R.drawable.baseline_stop_black_24), "Abort", stopPendingIntent).build();

        notificationBuilder = new Notification.Builder(this, AliasService.CHANNEL_ID_SERVICE)
                .setContentTitle("Blockchain Bootstrap")//getText(R.string.notification_title))
                .setContentText("Downloading...")//getText(R.string.notification_message))
                .setOnlyAlertOnce(true)
                .setSmallIcon(R.drawable.ic_alias_app_white)
                .setColor(getColor(R.color.primary))
                .setContentIntent(pendingIntent)
                .setProgress(100, 0, false)
                .addAction(stopAction);
        //.setTicker(getText(R.string.ticker_text));
        Notification notification = notificationBuilder.build();

        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.cancel(NOTIFICATION_ID_SERVICE_RESULT);
        notificationManager.notify(NOTIFICATION_ID_SERVICE_PROGRESS, notification);
        startForeground(NOTIFICATION_ID_SERVICE_PROGRESS, notification);

        Task<Void> installTask = CronetProviderInstaller.installProvider(this);
        installTask.addOnCompleteListener(
                task -> {
                    CronetEngine.Builder engineBuilder;
                    if (task.isSuccessful()) {
                        Log.d(TAG, "BootstrapTask: CronetProviderInstaller successfull!");
                        engineBuilder = new CronetEngine.Builder(this);
                    }
                    else {
                        if (task.getException() != null) {
                            Log.d(TAG, "BootstrapTask: CronetProvider not available. Fallback to JavaCronetProvider", task.getException());
                        } else {
                            Log.d(TAG, "BootstrapTask: CronetProvider not available. Fallback to JavaCronetProvider");
                        }
                        engineBuilder = new JavaCronetProvider(this).createBuilder();
                    }
                    engine = engineBuilder
                            .enableHttp2(true)
                            .enableQuic(true)
                            .build();

                    // Start bootstrap process in separate thread
                    bootstrapTask = new BootstrapTask();
                    bootstrapTask.execute();
                });
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
            Path bootstrapTmpPath = Paths.get(destinationDir.toString(), "tmp_bootstrap");
            List<BootstrapPartDefinition> bootstrapPartDefinitionList = null;
            try {
                //
                // PHASE 0: Download Bootstrap index file
                //
                bootstrapPartDefinitionList = downloadBootstrapIndexURLs(bootstrapTmpPath);

                //
                // PHASE 1: Download Bootstrap files
                //
                for (int partIndex = 0; partIndex < bootstrapPartDefinitionList.size(); partIndex++) {
                    BootstrapPartDefinition bootstrapPartDefinition = bootstrapPartDefinitionList.get(partIndex);
                    if (!Files.exists(bootstrapPartDefinition.path)) {
                        downloadBootstrap(notificationManager, bootstrapPartDefinition, partIndex, bootstrapPartDefinitionList.size());
                        if (isCancelled()) {
                            updateProgress(notificationManager, STATE_CANCEL, 0, 0, partIndex, bootstrapPartDefinitionList.size(), false, "Canceled...");
                            return null;
                        }
                    }
                }

                //
                // PHASE 2: Extract files
                //
                updateProgress(notificationManager, STATE_EXTRACTION, 0, 0 ,0 ,0,true, "Extracting...");
                prepareBootstrap(bootstrapPartDefinitionList, destinationDir);
                cleanupDirectory(bootstrapTmpPath);

                //
                // PHASE 3: Finished
                //
                updateProgress(notificationManager, STATE_FINISHED, 0,0, 0, 0, false, "Successfully finished!");
            } catch (Exception e) {
                //
                // PHASE -1: Error
                //
                BootstrapError errorCode = BootstrapError.UNDEFINED;
                Log.e(TAG, "BootstrapTask: Failed with exception: " + e.getMessage(), e);
                String errorText = "Failed!";
                if (e.getMessage() != null && e.getMessage().contains(ERROR_MSG_ENOSPC)) {
                    errorText += " No space left on device.";
                    if (e instanceof ZipException) {
                        errorCode = ERROR_NOSPACE_EXTRACTION;
                    }
                    else {
                        errorCode = ERROR_NOSPACE_DOWNLOAD;
                    }
                }
                else if (e instanceof ZipException) {
                    errorText += " Bootstrap archive extraction error.";
                    errorCode = ERROR_EXTRACTION;
                    cleanupDirectory(bootstrapTmpPath);
                }
                else if (e.getMessage() != null && e.getMessage().contains(ERROR_MSG_BOOTSTRAP_HASH)) {
                    errorText += " Bootstrap hash mismatch.";
                    errorCode = ERROR_HASH;
                }
                else if (e.getMessage() != null && e.getMessage().contains(ERROR_MSG_BOOTSTRAP_INDEX_404)) {
                    errorText += " Bootstrap index file missing on server.";
                    errorCode = ERROR_BOOTSTRAP_INDEX_404;
                }
                else if (e.getMessage() != null && e.getMessage().contains(ERROR_MSG_BOOTSTRAP_FILE_404)) {
                    errorText += " Bootstrap file missing on server.";
                    errorCode = ERROR_BOOTSTRAP_FILE_404;
                    cleanupDirectory(bootstrapTmpPath);
                }
                updateProgress(notificationManager, STATE_ERROR, errorCode.ordinal(), 0,0,0, false, errorText);
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

        private List<BootstrapPartDefinition> downloadBootstrapIndexURLs(Path destinationDir) throws Exception {
            Path bootstrapIndexPath = Paths.get(destinationDir.toString(), "BootstrapChainParts.txt");
            if (!Files.exists(bootstrapIndexPath) ||
                    (Duration.between(Files.getLastModifiedTime(bootstrapIndexPath).toInstant(), Clock.systemUTC().instant()).toDays() > 0)) {

                // Make sure target directory for bootstrap index exists and is empty
                cleanupDirectory(destinationDir);
                Files.createDirectories(bootstrapIndexPath.getParent());
                Path bootstrapFileTemp = bootstrapIndexPath.resolveSibling("downloading");

                InputStream input = null;
                OutputStream output = null;
                try {
                    URL bootstrapIndexUrl = new URL(BOOTSTRAP_BASE_URL + "BootstrapChainParts.txt");
                    HttpURLConnection connection = (HttpURLConnection) engine.openConnection(bootstrapIndexUrl);
                    connection.setConnectTimeout(10000);
                    connection.setReadTimeout(30000);
                    connection.connect();
                    if (connection.getResponseCode() == 404) {
                        throw new RuntimeException(ERROR_MSG_BOOTSTRAP_INDEX_404 + " > Server responded with HTTP RC 404 for file: " + bootstrapIndexUrl);
                    }
                    input = new BufferedInputStream(connection.getInputStream());
                    output = new FileOutputStream(bootstrapFileTemp.toFile());

                    // download the file and write to disk
                    byte buffer[] = new byte[1024];
                    int bytesRead = -1;
                    while ((bytesRead = input.read(buffer)) != -1) {
                        output.write(buffer, 0, bytesRead);
                    }
                    output.flush();
                    Files.move(bootstrapFileTemp, bootstrapIndexPath);
                } finally {
                    // clean up
                    close(input);
                    close(output);
                    deleteIfExists(bootstrapFileTemp);
                }
            }

            final List<BootstrapPartDefinition> bootstrapPartDefinitions = new ArrayList();
            try (BufferedReader reader = new BufferedReader(new FileReader(bootstrapIndexPath.toFile()))) {
                String nextLine = "";
                while ((nextLine = reader.readLine()) != null) {
                    String[] defArrray = nextLine.split("  ");
                    BootstrapPartDefinition def = new BootstrapPartDefinition(defArrray[0], Paths.get(destinationDir.toString(), defArrray[1]), new URL(BOOTSTRAP_BASE_URL + defArrray[1]));
                    bootstrapPartDefinitions.add(def);
                }
            }
            return bootstrapPartDefinitions;
        }

        private void downloadBootstrap(NotificationManager notificationManager, BootstrapPartDefinition bootstrapPartDefinition, int fileIndex, int totalFiles) throws Exception {
            Path bootstrapFileTemp = bootstrapPartDefinition.path.resolveSibling("downloading");
            InputStream input = null;
            OutputStream output = null;
            try {
                // Make sure there is no bootstrap file on disc before download
                Files.deleteIfExists(bootstrapPartDefinition.path);
                Files.deleteIfExists(bootstrapFileTemp);

                // Make sure target directory for bootstrap exists
                Files.createDirectories(bootstrapPartDefinition.path.getParent());

                HttpURLConnection connection = (HttpURLConnection)engine.openConnection(bootstrapPartDefinition.url);
                connection.setConnectTimeout(10000);
                connection.setReadTimeout(30000);
                connection.connect();
                if (connection.getResponseCode() == 404) {
                    throw new RuntimeException(ERROR_MSG_BOOTSTRAP_FILE_404 + " > Server responded with HTTP RC 404 for file: " + bootstrapPartDefinition.url);
                }
                // this will be useful so that you can show a typical 0-100% progress bar
                int fileLength = connection.getContentLength();
                if (fileLength == -1) {
                    updateProgress(notificationManager, STATE_DOWNLOAD, 0, 0, fileIndex, totalFiles, true, "Downloading... ("+ (fileIndex+1) + "/" + totalFiles +")");
                }

                MessageDigest md = MessageDigest.getInstance("SHA-256");
                DigestInputStream dis = new DigestInputStream(connection.getInputStream(), md);
                input = new BufferedInputStream(dis);
                output = new FileOutputStream(bootstrapFileTemp.toFile());

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
                            updateProgress(notificationManager, STATE_DOWNLOAD, 0, progress, fileIndex, totalFiles,false, "Downloading... ("+ (fileIndex+1) + "/" + totalFiles +")");
                            lastProgress = progress;
                        }
                    }
                    output.write(data, 0, count);
                }
                output.flush();
                if (!isCancelled()) {
                    String fileHash = toHex(md.digest());
                    if (!fileHash.equalsIgnoreCase(bootstrapPartDefinition.hash)) {
                        throw new RuntimeException(ERROR_MSG_BOOTSTRAP_HASH +
                                " > Hash of downloaded file " + bootstrapPartDefinition.url + " is " + fileHash + " but should be " + bootstrapPartDefinition.hash);
                    }
                    else {
                        Files.move(bootstrapFileTemp, bootstrapPartDefinition.path);
                    }
                }
            }
            finally {
                // clean up
                close(input);
                close(output);
                deleteIfExists(bootstrapFileTemp);
            }
        }

        private void prepareBootstrap(List<BootstrapPartDefinition> bootstrapPartDefinitions, Path destinationDir) throws Exception {
            // delete existing blockchain data
            deleteBlockchainFiles(destinationDir);

            // Extract ZIP file(s). zip4j can handle splitted zip files.
            try {
                new ZipFile(bootstrapPartDefinitions.get(bootstrapPartDefinitions.size()-1).path.toFile()).extractAll(destinationDir.toString());
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

    protected void updateProgress(NotificationManager notificationManager, int state, int errorCode, int progress, int indexOfItem, int numOfItems, boolean indeterminate, String text) {
        int max = state == STATE_DOWNLOAD ? 100 : 0;
        if (state != STATE_CANCEL) {
            notificationBuilder.setProgress(max, progress, indeterminate);
            notificationBuilder.setContentText(text);
            int notificationId;
            if (state == STATE_ERROR || state == STATE_FINISHED) {
                // Final notification is shown with separate Id that its not closed when service is stopped.
                notificationId = NOTIFICATION_ID_SERVICE_RESULT;
                notificationBuilder.setActions();
                notificationBuilder.setAutoCancel(true);
            }
            else {
                notificationId = NOTIFICATION_ID_SERVICE_PROGRESS;
                notificationBuilder.setActions(stopAction);
            }
            notificationManager.notify(notificationId, notificationBuilder.build());
        }
        sendBootstrapProgressBroadcast(state, errorCode, progress, indexOfItem, numOfItems, indeterminate);
    }

    protected void sendBootstrapProgressBroadcast(int state, int errorCode, int progress, int indexOfItem, int numOfItems, boolean indeterminate) {
        Intent broadCastIntent = new Intent();
        broadCastIntent.setAction(BOOTSTRAP_BROADCAST_ACTION);
        broadCastIntent.putExtra("state", state);
        broadCastIntent.putExtra("errorCode", errorCode);
        broadCastIntent.putExtra("progress", progress);
        broadCastIntent.putExtra("indexOfItem", indexOfItem);
        broadCastIntent.putExtra("numOfItems", numOfItems);
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

    public static void cleanupDirectory(Path directory) {
        // clean up temporary files
        try {
            Files.walk(directory).sorted(Comparator.reverseOrder()).map(Path::toFile).forEach(File::delete);
        } catch (Exception ex) {
            Log.d(TAG, "BootstrapTask: Failed to delete files in " + directory, ex);
        }
    }

    public static void deleteIfExists(Path path) {
        try {
            Files.deleteIfExists(path);
        } catch (Exception ex) {
            Log.d(TAG, "BootstrapTask: Failed to delete bootstrap temp file: " + path, ex);
        }
    }

    public static void close(Closeable c) {
        if (c == null) return;
        try {
            c.close();
        } catch (IOException e) {
            Log.d(TAG, "startDownload: Failed to close Closeable", e);
        }
    }

    public static String toHex(byte[] bytes) {
        // Create Hex String
        StringBuilder hexString = new StringBuilder();
        for (byte aMessageDigest : bytes) {
            String h = Integer.toHexString(0xFF & aMessageDigest);
            while (h.length() < 2)
                h = "0" + h;
            hexString.append(h);
        }
        return hexString.toString();
    }
}
