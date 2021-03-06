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

#include "panorama.hpp"

Panorama::Panorama(std::shared_ptr<Database> database, QWidget* parent) :
    QWidget(parent),
    mWorldModel(std::make_shared<WorldModel>(database)),
    mLayout(new QHBoxLayout(this)),
    mGLWidget(new GLWidget(mWorldModel, this))
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle("panorama");

    mLayout->addWidget(mGLWidget);

    QObject::connect(this, SIGNAL(reload()), this, SLOT(doReload()));
    mWorldModel->reload().set([this] {emit reload();});
}

Panorama::~Panorama()
{
    mWorldModel->reload().set(std::function<void()>());
}


void Panorama::loadLatLon(double lat, double lon, int zoom)
{
    mWorldModel->loadLatLon(lat, lon, zoom);
}

void Panorama::doReload()
{
    mGLWidget->reload();
    mGLWidget->reloadEye();
    mGLWidget->update();
}
