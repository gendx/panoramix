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

#include "polygon.hpp"

std::ostream& operator<<(std::ostream& out, const Polygon& p)
{
    out << "[";
    for (unsigned int i = 0 ; i < p.size() ; ++i)
    {
        if (i)
            out << ", ";
        out << p[i];
    }
    return out << "]";
}


void Polygon::translate(const Point& p)
{
    for (auto& point : *this)
        point.add2(p);
}

void Polygon::scaleXY(double factor)
{
    for (auto& point : *this)
        point.scaleXY(factor);
}

void Polygon::scaleY(double factor)
{
    for (auto& point : *this)
        point.scaleY(factor);
}
