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

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QMdiArea>
#include "database/database.hpp"

// The application's main window.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const std::string& mapboxToken, const std::string& cacheFolder);

    void loadLatLon(double lat, double lon, int zoom);

private slots:
    void open();
    void closeActive();
    void closeAll();

    void tileSubwin();
    void cascadeSubwin();
    void tabSubwin();

private:
    void closeEvent(QCloseEvent*) override;

    void createActions();


    QMdiArea* mMdi;

    QMenu* mFileMenu;
    QAction* mOpenAction;
    QAction* mCloseAction;
    QAction* mCloseallAction;

    QMenu* mWindowMenu;
    QAction* mTileAction;
    QAction* mCascadeAction;
    QAction* mTabAction;

    QToolBar* mFileToolbar;

    std::shared_ptr<Database> mDatabase;
};

#endif // MAIN_WINDOW_HPP

