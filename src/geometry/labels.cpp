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

#include "labels.hpp"

#include "protobuf/labels.pb.h"
#include "geometry/astro.hpp"

void Labels::load(std::ifstream& ifs)
{
    panoramix::Labels labels;
    if (!labels.ParseFromIstream(&ifs))
    {
        std::cerr << "Error parsing global label file" << std::endl;
        return;
    }

    for (auto& l : labels.labels())
    {
        bool hasElevation = l.has_ele();
        Point pt = Astro::mercatorFromLatLonDeg(l.lat(), l.lon(), hasElevation ? l.ele() : -1000);

        Label::Type type;
        switch (l.type())
        {
        case panoramix::Labels::PEAK:
            type = Label::PEAK;
            break;
        case panoramix::Labels::SADDLE:
            type = Label::SADDLE;
            break;
        case panoramix::Labels::VOLCANO:
            type = Label::VOLCANO;
            break;
        default:
            type = Label::OTHER;
        }

        mLabels.emplace_back(std::string(l.name()), std::move(pt), type, hasElevation);
    }

    std::cerr << "Labels size: " << mLabels.size()*sizeof(Label) << "+ bytes for " << mLabels.size() << " labels." << std::endl;
}

void Labels::filter(const TileInfo& tile, std::vector<Label>& labels)
{
    double zz = 1 << tile.zoom;
    double xmin = tile.x / zz;
    double xmax = (tile.x + 1) / zz;
    double ymin = tile.y / zz;
    double ymax = (tile.y + 1) / zz;

    for (auto& l : mLabels)
    {
        double x = l.point.x;
        double y = l.point.y;
        if (x >= xmin && x < xmax && y >= ymin && y < ymax)
            labels.push_back(l);
    }
}
