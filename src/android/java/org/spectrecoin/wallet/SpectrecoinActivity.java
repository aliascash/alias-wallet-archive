package org.spectrecoin.wallet;

import android.os.Bundle;

public class SpectrecoinActivity extends org.qtproject.qt5.android.bindings.QtActivity {

    public static String nativeLibraryDir;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        nativeLibraryDir = getApplicationInfo().nativeLibraryDir;
        super.onCreate(savedInstanceState);
    }
}