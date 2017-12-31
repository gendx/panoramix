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

#include "mainwindow.hpp"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QInputDialog>
#include "panorama.hpp"
#include "database/networkmanager.hpp"

#include "protobuf/vector_tile.pb.h"

MainWindow::MainWindow(const std::string& mapboxToken, const std::string& cacheFolder) :
    mMdi(new QMdiArea),
    mFileToolbar(this->addToolBar("&File")),
    mDatabase(std::make_shared<Database>(mapboxToken, cacheFolder))
{
    this->createActions();

    this->setWindowTitle("Panoramix");

    this->setCentralWidget(mMdi);
    mMdi->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mMdi->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->tabSubwin();

    QObject::connect(mOpenAction, SIGNAL(triggered()), this, SLOT(open()));
    QObject::connect(mCloseAction, SIGNAL(triggered()), this, SLOT(closeActive()));
    QObject::connect(mCloseallAction, SIGNAL(triggered()), this, SLOT(closeAll()));

    QObject::connect(mTileAction, SIGNAL(triggered()), this, SLOT(tileSubwin()));
    QObject::connect(mCascadeAction, SIGNAL(triggered()), this, SLOT(cascadeSubwin()));
    QObject::connect(mTabAction, SIGNAL(triggered()), this, SLOT(tabSubwin()));
}

void MainWindow::loadLatLon(double lat, double lon, int zoom)
{
    Panorama* panorama = new Panorama(mDatabase, this);
    mMdi->addSubWindow(panorama);
    panorama->show();

    panorama->loadLatLon(lat, lon, zoom);
}


void MainWindow::createActions()
{
    QString folder(":/icons/");

    // Menus
    mFileMenu = this->menuBar()->addMenu("&File");
    mOpenAction = mFileMenu->addAction(QIcon(folder + "open.png"), "&Open");
    mCloseAction = mFileMenu->addAction(QIcon(folder + "close.png"), "&Close");
    mCloseallAction = mFileMenu->addAction(QIcon(folder + "close_all.png"), "Close all");

    mWindowMenu = this->menuBar()->addMenu("&Window");
    mTileAction = mWindowMenu->addAction("Ti&le");
    mCascadeAction = mWindowMenu->addAction("&Cascade");
    mTabAction = mWindowMenu->addAction("&Tab");

    // Toolbar
    mFileToolbar->setIconSize(QSize(16, 16));

    mFileToolbar->addAction(mCloseAction);
    mFileToolbar->addAction(mCloseallAction);
}


void MainWindow::closeEvent(QCloseEvent*)
{
    NetworkManager::manager.cancel();
    this->closeAll();
}

void MainWindow::open()
{
    static constexpr double MERCATOR_MAX = 85;
    bool ok;

    double lat = QInputDialog::getDouble(this, "Latitude", "Latitude", 0, -MERCATOR_MAX, MERCATOR_MAX, 8, &ok);
    if (!ok) return;

    double lon = QInputDialog::getDouble(this, "Longitude", "Longitude", 0, -180, 180, 8, &ok);
    if (!ok) return;

    int zoom = 11;

    Panorama* panorama = new Panorama(mDatabase, this);
    mMdi->addSubWindow(panorama);
    panorama->show();

    panorama->loadLatLon(lat, lon, zoom);
}

void MainWindow::closeActive()
{
    mMdi->closeActiveSubWindow();
}

void MainWindow::closeAll()
{
    mMdi->closeAllSubWindows();
}


void MainWindow::tileSubwin()
{
    mMdi->setViewMode(QMdiArea::SubWindowView);
    mMdi->tileSubWindows();
}

void MainWindow::cascadeSubwin()
{
    mMdi->setViewMode(QMdiArea::SubWindowView);
    mMdi->cascadeSubWindows();
}

void MainWindow::tabSubwin()
{
    mMdi->setViewMode(QMdiArea::TabbedView);
}
