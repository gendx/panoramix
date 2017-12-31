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

#ifndef PRIMITIVES_HPP
#define PRIMITIVES_HPP

#include "point.hpp"

class Primitives
{
public:
    /*
        Check if a point p is inside the circumcircle made up of (p1, p2, p3).
        Returns the center and squared radius.
        Note: A point on the edge is inside the circumcircle.
    */
    static int circumCircle(const Point& p, const Point& p1, const Point& p2, const Point& p3, Point& center, double& r2);

    /*
        Check if the segments ]p1, p2] and [q1, q2] intersect.
        Note: we consider that parallel segments do not intersect.
    */
    static bool interSegments(const Point& p1, const Point& p2, const Point& q1, const Point& q2);

    /*
        Interpolate p.z according to the triangle {p1, p2, p3}.
    */
    static double interpolate(const Point& p, const Point& p1, const Point& p2, const Point& p3);

    static Point interpolateCoeffs(const Point& p, const Point& p1, const Point& p2, const Point& p3);
};

#endif
