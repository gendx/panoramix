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

#include "mvt.hpp"

Mvt::Mvt(vector_tile::Tile&& tile) :
    mTile(std::move(tile))
{
}


std::vector<Polygon> Mvt::getContour() const
{
    std::vector<Polygon> contour;

    for (auto& layer : mTile.layers())
    {
        auto& layer_name = layer.name();
        if (layer_name == "contour")
        {
            for (auto& feature : layer.features())
            {
                std::unique_ptr<int> ele = Mvt::getElevation(layer, feature);
                if (!ele)
                    continue;

                Mvt::getGeometry(feature, *ele, contour);
            }
        }
    }

    return contour;
}


// TODO: use std::optional
std::unique_ptr<int> Mvt::getElevation(const vector_tile::Tile_Layer& layer, const vector_tile::Tile_Feature& feature)
{
    for (int i = 0 ; i < feature.tags_size() ; i += 2)
    {
        if (i+1 < feature.tags_size())
        {
            int k = feature.tags(i);
            int v = feature.tags(i+1);

            if (k >= 0 && v >= 0 && k < layer.keys_size() && layer.keys(k) == "ele" && v < layer.values_size())
            {
                auto& value = layer.values(v);
                if (value.has_int_value())
                    return std::make_unique<int>(value.int_value());
            }
        }
    }

    return std::unique_ptr<int>();
}

void Mvt::getGeometry(const vector_tile::Tile_Feature& feature, int elevation, std::vector<Polygon>& path)
{
    size_t size = path.size();
    Point cursor(0, 0, elevation);
    Point start = cursor;
    Polygon polygon;

    for (int k = 0 ; k < feature.geometry_size() ; ++k)
    {
        unsigned int command = feature.geometry(k);
        unsigned int id = command & 0x7;
        unsigned int count = command >> 3;

        if (id == 1 || id == 2)
        {
            for (unsigned int l = 0 ; l < 2 * count ; ++l)
            {
                ++k;
                if (k >= feature.geometry_size())
                {
                    std::cerr << "Path error" << std::endl;
                    path.resize(size);
                    return;
                }

                unsigned int v = feature.geometry(k);
                int value = (v >> 1) ^ (-(v & 1));

                if (l % 2 == 0)
                    cursor.x += value;
                else
                {
                    cursor.y += value;

                    if (id == 1) // moveto
                    {
                        if (!polygon.empty())
                        {
                            path.push_back(std::move(polygon));
                            polygon.clear();
                        }
                        start = cursor;
                    }
                    // id == 2 : lineto
                    polygon.push_back(cursor);
                }
            }
        }
        else if (id == 7) // closepath
        {
            polygon.push_back(start);
            path.push_back(std::move(polygon));
            polygon.clear();
        }
    }

    if (!polygon.empty())
        path.push_back(std::move(polygon));
}
