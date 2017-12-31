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

#include "point.hpp"

#include <cmath>

std::ostream& operator<<(std::ostream& out, const Point& p)
{
    return out << "[" << p.x << ", " << p.y << ", " << p.z << "]";
}

void Point::rot2(double theta)
{
    double c = std::cos(theta);
    double s = std::sin(theta);
    double xx = c*x - s*y;
    double yy = s*x + c*y;
    x = xx;
    y = yy;
}

double Point::det(const Point& p1, const Point& p2, const Point& p3)
{
    Point a = p2 - p1;
    Point b = p3 - p1;
    return a.x * b.y - a.y * b.x;
}

double Point::incircle(const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
    Point a = p1 - p4;
    Point b = p2 - p4;
    Point c = p3 - p4;
    // Determinant of:
    // a_x a_y |a|^2
    // b_x b_y |b|^2
    // c_x c_y |c|^2
    return a.dist2() * (b.x * c.y - b.y * c.x)
         + b.dist2() * (c.x * a.y - c.y * a.x)
         + c.dist2() * (a.x * b.y - a.y * b.x);
}

