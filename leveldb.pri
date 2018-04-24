#INCLUDEPATH += $$PWD/include $$PWD/helpers $$PWD/port $$PWD/util $$PWD $$PWD/../

#LIBS += $$PWD/out-static/libmemenv.a \
#                     $$PWD/out-static/libleveldb.a

macx {
    LIBS += -L$$PWD/out-static -lleveldb \
    -L$$PWD/out-static -lmemenv

    INCLUDEPATH += $$PWD/out-static
    DEPENDPATH += $$PWD/out-static
    INCLUDEPATH += $$PWD/include
    DEPENDPATH += $$PWD/include
    INCLUDEPATH += $$PWD/helpers
    DEPENDPATH += $$PWD/helpers

    PRE_TARGETDEPS += $$PWD/out-static/libleveldb.a \
    $$PWD/out-static/libmemenv.a
}
