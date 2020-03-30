# Spectrecoin building from source on Windows

The goal is to build the whole project using CMake. At the moment the Windows
build could use the current directory structure but is still not CMake based.
To reach this goal, some more refactorings and updated need to be finished
first.

## Install required tools and libs

- Visual studio: https://www.visualstudio.com/downloads/
- Qt SDK: https://www.qt.io/download-qt-installer
- VCPKG: https://github.com/microsoft/vcpkg/
- Boost ...



### Visual Studio 2019
Visual Studio 2019 Community Edition is enough.

Install at least these components:

![Required VS 2019 components](VS_InstallerComponents.png)

If you're not on an English Windows, you need to install the Englisch language package too:

![VS 2019 language pack](VS_InstallerComponents_Language.png)



### Qt
Here are the components from Qt SDK that we really need to compile Spectrecoin
application. Please keep Qt creator selected as well! If MinGW is ticked you
may untick that, unless you need it for other projects.

![Required Qt components](Qt_Windows.png)



### VCPKG

Clone VCPKG Git repository

**Important 1:** It must be reset to the state before the update of Boost to
1.70, as we're based on Boost 1.69 at the moment.

**Important 2:** You need to use `Start menu` > `Visual Studio 2019` > 
`Developer Command Prompt for VS 2019` for all the next steps!

```
D:\coding> git clone https://github.com/Microsoft/vcpkg.git
D:\coding> cd vcpkg
D:\coding\vcpkg> git checkout 208bb8ee -- ports/boost
D:\coding\vcpkg> .\bootstrap-vcpkg.bat
```

After this, make vcpkg global available by executing the following cmd once 
as Administrator:

```
D:\coding\vcpkg> .\vcpkg.exe integrate install
```

Additionally you need to setup the environment variable `asdf` with the path
to the tools directory on your Visual Studio installation:

```
VS150COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools
```

Subsequent executions could be done with normal user permissions.

Now install the following packages:

```
D:\coding\vcpkg> .\vcpkg.exe install berkeleydb leveldb openssl
```

The installation of Boost needs special handling, as the used Visual Studio is
too new for Boost 1.69. To tweak the boost build, the required files needs to
be created at first. So start the build of boost, even if it will fail:

```
D:\coding\vcpkg> .\vcpkg.exe install boost-atomic boost-chrono boost-date-time boost-filesystem boost-iostreams boost-program-options boost-regex boost-system boost-thread 
```

Now modify the file `D:\coding\vcpkg\installed\x86-windows\tools\boost-build\src\tools\msvc.jam`
by replacing all occurrences of `VS150COMNTOOLS` with `VS160COMNTOOLS`. After
this, restart the Boost build:

```
D:\coding\vcpkg> .\vcpkg.exe install boost-atomic boost-chrono boost-date-time boost-filesystem boost-iostreams boost-program-options boost-regex boost-system boost-thread 
```

ToDocument: 
- Fix for boost-math 
- Fix for boost-thread


# Work in progress!!!
## Setup environment vars

![CMake environment variables](CMakeEnvVars.png)

Please use the default installation path `C:\Program Files (x86)\Microsoft Visual Studio`.


# OLD stuff:

## Easy (Prebuilt libs)

Since quite many of our users found it hard to compile, especially on
Windows, we are adding an easy way and provided prebuilt package for all
the libraries required to compile Spectrecoin wallet. Go ahead and download
all the libraries from the following links:

https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Prebuild.libraries.win64.zip
https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.QT.libraries.win64.zip
https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Tor.libraries.win64.zip

Clone Spectrecoin repository. You can simply download the “Zip”s from Github.

Now unzip Prebuild, QT and Tor libraries.zip that you just downloaded into the
root folder of the cloned repository. Once done properly you should end up with
`src` and `packages64bit` all on one folder.

Now go ahead and open a the file src/src.pro (It should open up with Qt Creator).
Make sure MSVC 64 bit compiler is selected. Click configure and build and run as
usual with Qt.


## Library Notes

- The Tor libraries zip contains simply the official Tor binaries and are
  only required at runtime.
- The libraries contained in Spectrecoin.QT.libraries.win64.zip have been
  automatically gathered by QT with the windeployqt tool.
  See http://doc.qt.io/qt-5/windows-deployment.html

## Library Notes

You can use the scripts `scripts/win-genbuild.bat` and `scripts/win-build.bat`
to build from cmdline. The first script creates the header file `build.h` with
some required variable definitions. The second script performs the build itself
and to use it, you need to define `%QTDIR%`. This env var must point to the root
folder or your local Qt installation.

At the moment it is assumed, that MSVC is installed on the default path
`C:\Program Files (x86)\Microsoft Visual Studio`.
