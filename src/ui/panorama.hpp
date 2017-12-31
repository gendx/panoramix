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

#ifndef PANORAMA_HPP
#define PANORAMA_HPP

#include <QGridLayout>

#include "ui/openglwidget.hpp"
#include "geometry/worldmodel.hpp"

class Panorama : public QWidget
{
    Q_OBJECT

public:
    Panorama(std::shared_ptr<Database> database, QWidget* parent);
    ~Panorama();

    void loadLatLon(double lat, double lon, int zoom);

signals:
    void reload();

private slots:
    void doReload();

private:
    std::shared_ptr<WorldModel> mWorldModel;

    QHBoxLayout* mLayout;
    GLWidget* mGLWidget;
};

#endif // PANORAMA_HPP

