#   Panoramix - 3D view of your surroundings.
#   Copyright (C) 2017  Guillaume Endignoux
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt


QT += widgets
QT += gui

CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QMAKE_CXXFLAGS_RELEASE = -Ofast

# To run ASAN and UBSAN
#QMAKE_CXXFLAGS += -fsanitize=address,undefined
#QMAKE_LFLAGS += -fsanitize=address,undefined
# To run TSAN
#QMAKE_CXXFLAGS += -fsanitize=thread
#QMAKE_LFLAGS += -fsanitize=thread

TEMPLATE = app

# TODO: you must adapt this to your config
INCLUDEPATH += . \
    /path/to/asio-1.10.8/include/

DEFINES += ASIO_STANDALONE

# TODO: you must adapt this to your config
LIBS += -L/usr/local/lib/ -lssl -lcrypto -lprotobuf -lz

HEADERS += \
    config.hpp \
    database/cache.hpp \
    database/database.hpp \
    database/https.hpp \
    database/networkmanager.hpp \
    geometry/astro.hpp \
    geometry/delaunay.hpp \
    geometry/labels.hpp \
    geometry/point.hpp \
    geometry/polygon.hpp \
    geometry/primitives.hpp \
    geometry/quadtree.hpp \
    geometry/triangulate.hpp \
    geometry/worldmodel.hpp \
    protobuf/mvt.hpp \
    protobuf/cache_index.pb.h \
    protobuf/labels.pb.h \
    protobuf/vector_tile.pb.h \
    protobuf/xyz.pb.h \
    ui/mainwindow.hpp \
    ui/openglwidget.hpp \
    ui/panorama.hpp \
    util/concurrency.hpp \
    util/gzip.hpp \

SOURCES += \
    main.cpp \
    database/cache.cpp \
    database/database.cpp \
    database/https.cpp \
    database/networkmanager.cpp \
    geometry/astro.cpp \
    geometry/delaunay.cpp \
    geometry/labels.cpp \
    geometry/point.cpp \
    geometry/polygon.cpp \
    geometry/primitives.cpp \
    geometry/quadtree.cpp \
    geometry/triangulate.cpp \
    geometry/worldmodel.cpp \
    protobuf/mvt.cpp \
    protobuf/cache_index.pb.cc \
    protobuf/labels.pb.cc \
    protobuf/vector_tile.pb.cc \
    protobuf/xyz.pb.cc \
    ui/mainwindow.cpp \
    ui/openglwidget.cpp \
    ui/panorama.cpp \
    util/gzip.cpp \
    util/concurrency.cpp

RESOURCES += \
    panoramix.qrc

