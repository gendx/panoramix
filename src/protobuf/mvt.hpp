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

#ifndef MVT_HPP
#define MVT_HPP

#include <memory>
#include "protobuf/vector_tile.pb.h"
#include "geometry/polygon.hpp"

class Label
{
public:
    enum Type {
        PEAK, SADDLE, VOLCANO, OTHER
    };

    Label(const Label& label) = default;
    Label(Label&& label) = default;
    Label& operator=(const Label&) = default;

    inline Label(std::string&& _name, Point&& _p, Type _t, bool _hasElevation) :
        name(std::move(_name)), point(std::move(_p)), elevation(point.z), type(_t), hasElevation(_hasElevation) {}

    inline bool operator==(const Label& l) const
        {return name == l.name && point == l.point;}
    inline bool operator<(const Label& l) const
        {return point == l.point ? name < l.name : Point::lexcomp2(point, l.point);}

    inline double elevationEstimate() const
        {return hasElevation ? elevation : point.z;}

    std::string name;
    Point point;
    double elevation;
    Type type;
    bool hasElevation;
};

class Mvt
{
public:
    Mvt(vector_tile::Tile&& tile);

    std::vector<Polygon> getContour() const;

    inline static bool isValid(const Point& p);

private:
    static std::unique_ptr<int> getElevation(const vector_tile::Tile_Layer& layer, const vector_tile::Tile_Feature& feature);
    static void getGeometry(const vector_tile::Tile_Feature& feature, int elevation, std::vector<Polygon>& path);

    vector_tile::Tile mTile;
};

inline bool Mvt::isValid(const Point& p)
    {return p.x >= 0 && p.x <= 4096 && p.y >= 0 && p.y <= 4096;}

#endif // MVT_HPP

