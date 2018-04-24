TEMPLATE = app
TARGET = spectre
#find . -type d
QT += testlib webenginewidgets webchannel
CONFIG += c++14

TEMPLATE = subdirs

SUBDIRS = \
    src \   # sub-project names
