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

#ifndef POINT_HPP
#define POINT_HPP

#include <iostream>
#include <cmath>

class Point
{
public:
    friend std::ostream& operator<<(std::ostream& out, const Point& p);

    Point() = default;
    inline Point(double _x, double _y, double _z = 0);

    inline bool operator==(const Point& p) const;
    inline static bool lexcomp2(const Point& p, const Point& q);

    void rot2(double theta);
    inline Point& operator+=(const Point& p);
    inline Point& operator-=(const Point& p);
    inline Point& operator*=(double x);
    inline Point& operator/=(double x);
    inline void add2(const Point& p);
    inline void sub2(const Point& p);
    inline void scaleXY(double k);
    inline void scaleY(double k);

    inline Point operator+(const Point& p) const;
    inline Point operator-(const Point& p) const;
    inline Point operator*(double x) const;
    inline Point operator/(double x) const;

    inline Point cross(const Point& p) const;
    inline double scalar(const Point& p) const;

    inline void min2(const Point& p);
    inline void max2(const Point& p);
    inline double xslope() const;
    inline double dist2() const;
    inline double dist3() const;
    inline void normalize3();

    // Note: these primitives assume integer coordinates of reasonable
    // magnitude, otherwise the result accuracy is not guaranteed.
    static double det(const Point& p1, const Point& p2, const Point& p3);
    static double incircle(const Point& p1, const Point& p2, const Point& p3, const Point& p4);
    inline double det(const Point& p) const;

    double x;
    double y;
    double z;
};

inline Point::Point(double _x, double _y, double _z) :
    x(_x), y(_y), z(_z) {}

inline bool Point::operator==(const Point& p) const
    {return x == p.x && y == p.y && z == p.z;}
inline bool Point::lexcomp2(const Point& p, const Point& q)
    {return p.x == q.x ? p.y < q.y : p.x < q.x;}

inline Point& Point::operator+=(const Point& p)
    {x += p.x; y += p.y; z += p.z; return *this;}
inline Point& Point::operator-=(const Point& p)
    {x -= p.x; y -= p.y; z -= p.z; return *this;}
inline Point& Point::operator*=(double a)
    {x *= a; y *= a; z *= a; return *this;}
inline Point& Point::operator/=(double a)
    {x /= a; y /= a; z /= a; return *this;}
inline void Point::add2(const Point& p)
    {x += p.x; y += p.y;}
inline void Point::sub2(const Point& p)
    {x -= p.x; y -= p.y;}
inline void Point::scaleXY(double k)
    {x *= k; y *= k;}
inline void Point::scaleY(double k)
    {y *= k;}

inline Point Point::operator+(const Point& p) const
    {return Point(x + p.x, y + p.y, z + p.z);}
inline Point Point::operator-(const Point& p) const
    {return Point(x - p.x, y - p.y, z - p.z);}
inline Point Point::operator*(double a) const
    {return Point(x * a, y * a, z * a);}
inline Point Point::operator/(double a) const
    {return Point(x / a, y / a, z / a);}

inline Point Point::cross(const Point& p) const
    {return Point(y*p.z - z*p.y,
                  z*p.x - x*p.z,
                  x*p.y - y*p.x);}
inline double Point::scalar(const Point& p) const
    {return x*p.x + y*p.y + z*p.z;}

inline void Point::min2(const Point& p)
    {x = std::min(x, p.x); y = std::min(y, p.y);}
inline void Point::max2(const Point& p)
    {x = std::max(x, p.x); y = std::max(y, p.y);}
inline double Point::xslope() const
    {return x / y;}
inline double Point::dist2() const
    {return x*x + y*y;}
inline double Point::dist3() const
    {return x*x + y*y + z*z;}
inline void Point::normalize3()
    {(*this) /= std::sqrt(dist3());}
inline double Point::det(const Point& p) const
    {return x*p.y - y*p.x;}

#endif

