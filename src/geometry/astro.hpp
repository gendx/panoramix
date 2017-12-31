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

#ifndef ASTRO_HPP
#define ASTRO_HPP

#include "geometry/point.hpp"

// Astronomical computations
class Astro {
public:
    static Point mercatorFromLatLonRad(double lat, double lon, double z = 0.0);
    static Point mercatorFromLatLonDeg(double lat, double lon, double z = 0.0);
    static Point mercatorToModel(const Point& p, const Point& origin);
    static Point mercatorFromModel(const Point& p, const Point& origin);

    static double mercatorToLatDeg(const Point& p);
    static double mercatorToLonDeg(const Point& p);

private:
    static double mercatorToLatRad(const Point& p);
    static double mercatorToLonRad(const Point& p);
};

#endif
