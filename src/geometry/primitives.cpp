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

#include "primitives.hpp"

/*
    Check if a point p is inside the circumcircle made up of (p1, p2, p3).
    Returns the center and squared radius.
    Note: A point on the edge is inside the circumcircle.
*/
int Primitives::circumCircle(const Point& p, const Point& p1, const Point& p2, const Point& p3, Point& center, double& r2)
{
    static constexpr double EPSILON = 1e-10;

    double fabsy1y2 = std::fabs(p1.y - p2.y);
    double fabsy2y3 = std::fabs(p2.y - p3.y);

    // Check for coincident points
    if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
        return false;

    // Compute the center
    if (fabsy1y2 < EPSILON)
    {
        double m2 = - (p3 - p2).xslope();
        Point mb = (p2 + p3) / 2.0;
        center.x = (p1.x + p2.x) / 2.0;
        center.y = m2 * (center.x - mb.x) + mb.y;
    }
    else if (fabsy2y3 < EPSILON)
    {
        double m1 = - (p2 - p1).xslope();
        Point ma = (p1 + p2) / 2.0;
        center.x = (p2.x + p3.x) / 2.0;
        center.y = m1 * (center.x - ma.x) + ma.y;
    }
    else
    {
        double m1 = - (p2 - p1).xslope();
        double m2 = - (p3 - p2).xslope();
        Point ma = (p1 + p2) / 2.0;
        Point mb = (p2 + p3) / 2.0;
        center.x = (m1 * ma.x - m2 * mb.x + mb.y - ma.y) / (m1 - m2);
        if (fabsy1y2 > fabsy2y3)
            center.y = m1 * (center.x - ma.x) + ma.y;
        else
            center.y = m2 * (center.x - mb.x) + mb.y;
    }

    // Compute the radius
    r2 = (p2 - center).dist2();
    double dr2 = (p - center).dist2();

    return (dr2 - r2) <= EPSILON;
}

/*
    Check if the segments ]p1, p2] and [q1, q2] intersect.
    Note: we consider that parallel segments do not intersect.
*/
bool Primitives::interSegments(const Point& p1, const Point& p2, const Point& q1, const Point& q2)
{
    // I = a.p1 + (1-a).p2 = b.q1 + (1-b).q2 with 0 <= a, b <= 1
    // a.(p1-p2) + p2 = b.(q1-q2) + q2
    //
    // -(p2-p1) * a + (q2-q1) * b = q2-p2
    Point dp = p2 - p1;
    Point dq = q2 - q1;
    Point d2 = q2 - p2;

    // -dp.x * a + dq.x * b = d2.x
    // -dp.y * a + dq.y * b = d2.y
    //
    // det * M^-1 =  dq.y  -dq.x
    //               dp.y  -dp.x
    double det = dq.det(dp);
    // Note: we consider that parallel segments do not intersect.
    if (det == 0)
        return false;

    double a = d2.det(dq) / det;
    double b = d2.det(dp) / det;

    return a >= 0 && b >= 0 && a < 1 && b <= 1;
}

/*
    Interpolate p.z according to the triangle {p1, p2, p3}.
*/
double Primitives::interpolate(const Point& p, const Point& p1, const Point& p2, const Point& p3)
{
    Point c = interpolateCoeffs(p, p1, p2, p3);
    return c.x*p1.z + c.y*p2.z + c.z*p3.z;
}

Point Primitives::interpolateCoeffs(const Point& p, const Point& p1, const Point& p2, const Point& p3)
{
    // p1.x*a + p2.x*b + p3.x*c = p.x
    // p1.y*a + p2.y*b + p3.y*c = p.y
    //      a +      b +      c = 1
    double det = p1.det(p2) + p2.det(p3) + p3.det(p1);

    // det * M^-1 = TRANSPOSE_OF
    //      (p2-p3).y  (p3-p1).y  (p1-p2).y
    //      (p3-p2).x  (p1-p3).x  (p2-p1).x
    //      p2.det(p3) p3.det(p1) p1.det(p2)
    Point p12 = p2 - p1;
    Point p23 = p3 - p2;
    Point p31 = p1 - p3;
    double a = (-p23.y * p.x + p23.x * p.y + p2.det(p3)) / det;
    double b = (-p31.y * p.x + p31.x * p.y + p3.det(p1)) / det;
    double c = (-p12.y * p.x + p12.x * p.y + p1.det(p2)) / det;

    return Point(a, b, c); //a*p1.z + b*p2.z + c*p3.z;
}

