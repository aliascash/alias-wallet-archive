# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
#
# SPDX-License-Identifier: MIT

TEMPLATE = app
TARGET = spectre
#find . -type d
QT += testlib webenginewidgets webchannel

TEMPLATE = subdirs

macx {
SUBDIRS = \
    src \   # sub-project names
}

win32 {
SUBDIRS = \
    tor \   # sub-project names
    leveldb \   # sub-project names
    db4.8 \   # sub-project names
    src \   # sub-project names

src.depends = tor leveldb db4.8
}
