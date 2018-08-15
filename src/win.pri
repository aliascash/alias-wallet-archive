#Include libs the windows way, for visual studio 64 bit compiler
#static for VS
#Compile Qt from source
#Refrences for MSVC
# http://amin-ahmadi.com/2016/09/22/how-to-build-qt-5-7-statically-using-msvc14-microsoft-visual-studio-2015/
# https://stackoverflow.com/questions/32735678/how-to-build-qt-5-5-qtwebengine-on-windows-msvc-2015
# configure -prefix "C:\D\QtSDK\5.10.1\msvc2017_64_static" -platform win64-msvc2017 -developer-build -opensource -confirm-license -nomake examples -nomake tests -opengl desktop -debug-and-release -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -sql-sqlite -sql-odbc -make libs
# nmake
# nmake install
CONFIG += static
QMAKE_LFLAGS += -static
QMAKE_CXXFLAGS += /MT
QMAKE_CFLAGS += -O2 -MT
QMAKE_CXXFLAGS += -O2 -MT

DEFINES -= NDEBUG


CONFIG(release, debug|release) {
    message( "release" )
    LIBSPATH = $$PWD/../packages64bit/lib
}
CONFIG(debug, debug|release) {
    message( "debug" )
    LIBSPATH = $$PWD/../packages64bit/debug/lib
}


HEADERS += $$PWD/win/unistd.h
INCLUDEPATH += $$PWD/win

#Command to install dependencies (Static)
#vcpkg.exe install boost:x64-windows-static berkeleydb:x64-windows-static leveldb:x64-windows-static libevent:x64-windows-static lua:x64-windows-static openssl:x64-windows-static zlib:x64-windows-static
#Command to install dependencies (Dynamic)
#vcpkg.exe install boost:x64-windows berkeleydb:x64-windows leveldb:x64-windows libevent:x64-windows lua:x64-windows openssl:x64-windows zlib:x64-windows
DEFINES += BOOST_ASIO_ENABLE_OLD_SERVICES BOOST_SPIRIT_THREADSAFE BOOST_THREAD_USE_LIB
#DEFINES += BOOST_DISABLE_CURRENT_FUNCTION



INCLUDEPATH += $$PWD/../packages64bit/include
DEPENDPATH += $$PWD/../packages64bit/include

LIBS += -L$${LIBSPATH}/ -llua
PRE_TARGETDEPS += $${LIBSPATH}/lua.lib
LIBS += -L$${LIBSPATH} -lboost_chrono-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_chrono-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_filesystem-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_filesystem-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_program_options-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_program_options-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_system-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_system-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_thread-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_thread-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lboost_unit_test_framework-vc140-mt
PRE_TARGETDEPS += $${LIBSPATH}/boost_unit_test_framework-vc140-mt.lib
LIBS += -L$${LIBSPATH}/ -lzlib
PRE_TARGETDEPS += $${LIBSPATH}/zlib.lib
LIBS += -L$${LIBSPATH}/ -llibcryptoMD
PRE_TARGETDEPS += $${LIBSPATH}/libcryptoMD.lib
LIBS += -L$${LIBSPATH}/ -llibsslMD
PRE_TARGETDEPS += $${LIBSPATH}/libsslMD.lib
LIBS += -L$${LIBSPATH}/ -levent
PRE_TARGETDEPS += $${LIBSPATH}/event.lib
LIBS += -L$${LIBSPATH}/ -levent_core
PRE_TARGETDEPS += $${LIBSPATH}/event_core.lib
LIBS += -L$${LIBSPATH}/ -levent_extra
PRE_TARGETDEPS += $${LIBSPATH}/event_extra.lib
LIBS += -L$${LIBSPATH}/ -lWS2_32
PRE_TARGETDEPS += $${LIBSPATH}/WS2_32.lib
LIBS += -L$${LIBSPATH}/ -lAdvAPI32
PRE_TARGETDEPS += $${LIBSPATH}/AdvAPI32.lib
LIBS += -L$${LIBSPATH}/ -lshell32
PRE_TARGETDEPS += $${LIBSPATH}/shell32.lib
LIBS += -L$${LIBSPATH}/ -lUser32
PRE_TARGETDEPS += $${LIBSPATH}/User32.lib
LIBS += -L$${LIBSPATH}/ -lCrypt32
PRE_TARGETDEPS += $${LIBSPATH}/Crypt32.lib
#ShLwApi
LIBS += -L$${LIBSPATH}/ -lShLwApi
PRE_TARGETDEPS += $${LIBSPATH}/ShLwApi.lib

#LevelDB from VCPKG
LIBS += -L$${LIBSPATH}/ -llibleveldb
PRE_TARGETDEPS += $${LIBSPATH}/libleveldb.lib
#BerklyDB4.8 from our project repo (VCPKG version is not supported)
LIBS += -L$${LIBSPATH}/ -llibdb48
PRE_TARGETDEPS += $${LIBSPATH}/libdb48.lib
#libdb_stl48.lib (We need both libdb and libdbstl (Standered library)
LIBS += -L$${LIBSPATH}/ -llibdb_stl48
PRE_TARGETDEPS += $${LIBSPATH}/libdb_stl48.lib

LIBS += -L$$PWD/../tor/ -ltorlib
PRE_TARGETDEPS += $$PWD/../tor/torlib.lib
LIBS += -L$$PWD/../leveldb/ -lleveldblib
PRE_TARGETDEPS += $$PWD/../leveldb/leveldblib.lib
#LIBS += -L$$PWD/../db4.8/ -lberkeleydblib
#PRE_TARGETDEPS += $$PWD/../db4.8/berkeleydblib.lib

QMAKE_CFLAGS_WARN_ON -= -W3
QMAKE_CFLAGS_WARN_ON += -W2

QMAKE_CXXFLAGS_WARN_ON -= -W3
QMAKE_CXXFLAGS_WARN_ON += -W2

QMAKE_CXXFLAGS += -O2 -D_FORTIFY_SOURCE=1
