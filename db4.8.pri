#INCLUDEPATH += build_unix

macx {
    LIBS += -L$$PWD/build_unix -ldb_cxx \
    -L$$PWD/build_unix -ldb

    INCLUDEPATH += $$PWD/build_unix
    DEPENDPATH += $$PWD/build_unix

    PRE_TARGETDEPS += $$PWD/build_unix/libdb_cxx.a \
    $$PWD/build_unix/libdb.a
}
