# Spectrecoin building from source with CMake on Windows

# Work in progress!!!

## Install required tools and libs

- Visual studio: https://www.visualstudio.com/downloads/
- Qt SDK: https://www.qt.io/download-qt-installer
- BerkeleyDB 5.3.28 http://download.oracle.com/otn/berkeley-db/db-5.3.28.msi
- Boost ...

Here is the components from Qt SDK that we really need to compile our application (Keep Qt creator selected as well). If MingW is ticked you may untick that (Unless you need it for other projects)

![alt text](https://github.com/spectrecoin/spectre/raw/master/doc/Qt%20windows.png)

Once you install Visual studio. Go to Windows start menu and find "Visual studio installer"

Modify visual studio and make sure all those components are picked.

![alt text](https://github.com/spectrecoin/spectre/raw/master/doc/Visual%20studio%20installer%20components.png)

## Setup environment vars

![CMake environment variables](CMakeEnvVars.png)



# OLD stuff:

## Easy (Prebuilt libs)

Since quit many of our users found it hard to compile. Especially on Windows. We are adding an easy way and provided prebuilt package for all the libraries required to compile Spectre wallet. Go ahead and download all the libraries from the following links:

https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Prebuild.libraries.win64.zip
https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.QT.libraries.win64.zip
https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Tor.libraries.win64.zip

Clone Spectre repository. You can simply download the “Zip”s from github.

Now unzip Prebuild, QT and Tor libraries.zip that you just downloaded into the source root folder. Once done properly you should end up with “src” and “packages64bit” all into one folder.

Now go ahead and open a the file src/src.pro (It should open up with Qt Creator). Make sure our MSVC 64 bit compiler is selected. Click configure and build and run as usual with Qt.


## Library Notes

- The Tor libraries zip contains simply the official Tor binaries and are only required at runtime.
- The libraries contained in Spectrecoin.QT.libraries.win64.zip have been automatically gathered by QT with the windeployqt tool. See http://doc.qt.io/qt-5/windows-deployment.html

