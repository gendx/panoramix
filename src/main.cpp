/*
    Panoramix - 3D view of your surroundings.
    Copyright (C) 2017  Guillaume Endignoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt
*/

#include <QApplication>
#include <QMetaType>
#include <google/protobuf/stubs/common.h>

#include "config.hpp"
#include "ui/mainwindow.hpp"

int main(int argc, char** argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    QApplication app(argc, argv);
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<asio::error_code>("asio::error_code");

    std::string mapboxToken = MAPBOX_TOKEN;
    std::string cacheFolder = CACHE_FOLDER;

    MainWindow mainWindow(mapboxToken, cacheFolder);
    mainWindow.showMaximized();

    double lat = 46.43139, lon = 8.09416, zoom = 14;
    mainWindow.loadLatLon(lat, lon, zoom);

    return app.exec();
}

