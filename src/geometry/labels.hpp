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

#ifndef LABELS_HPP
#define LABELS_HPP

#include <fstream>
#include "protobuf/mvt.hpp"

class TileInfo {
public:
    TileInfo() = default;
    inline TileInfo(int _zoom, int _x, int _y) :
        x(_x), y(_y), zoom(_zoom) {}

    int x;
    int y;
    int zoom;
};


class Labels
{
public:
    void load(std::ifstream& ifs);

    inline int count() const;
    void filter(const TileInfo& tile, std::vector<Label>& labels);

private:
    std::vector<Label> mLabels;
};

inline int Labels::count() const
    {return mLabels.size();}

#endif
