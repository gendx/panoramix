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

#ifndef POLYGON_HPP
#define POLYGON_HPP

#include <iostream>
#include <vector>
#include "point.hpp"

class Polygon : public std::vector<Point>
{
public:
    friend std::ostream& operator<<(std::ostream& out, const Polygon& p);

    Polygon() = default;

    void translate(const Point& p);
    void scaleXY(double factor);
    void scaleY(double factor);
};

#endif

