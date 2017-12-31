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

#include "astro.hpp"

#include "config.hpp"

Point Astro::mercatorFromLatLonRad(double lat, double lon, double z)
{
    /* constexpr */ double pi = std::atan(1)*4;
    double x = (lon / (2.0 * pi)) + 0.5;
    double s = std::asinh(std::tan(lat));
    double y = 0.5 - s / (2.0 * pi);

    return Point(x, y, z);
}

Point Astro::mercatorFromLatLonDeg(double lat, double lon, double z)
{
    /* constexpr */ double pi = std::atan(1)*4;
    return Astro::mercatorFromLatLonRad(lat * pi / 180.0, lon * pi / 180.0, z);
}

Point Astro::mercatorToModel(const Point& p, const Point& origin)
{
#ifdef USE_EARTH_CURVATURE
    double lat = Astro::mercatorToLatRad(p);
    double lon = Astro::mercatorToLonRad(p);
    double lat_orig = Astro::mercatorToLatRad(origin);
    double lon_orig = Astro::mercatorToLonRad(origin);

    double dlon = lon - lon_orig;
    double x =  std::cos(lat)*std::sin(dlon);
    double y = -std::cos(lat)*std::cos(dlon);
    double z =  std::sin(lat);

    Point q(x,
            std::sin(lat_orig)*y + std::cos(lat_orig)*z,
           -std::cos(lat_orig)*y + std::sin(lat_orig)*z);

    q *= 1.0 + p.z / EARTH_RADIUS;
    q.z -= 1.0;
    return q * EARTH_RADIUS;
#else
    /* constexpr */ double pi = std::atan(1)*4;
    double scaleFactor = 2*pi*EARTH_RADIUS * std::cos(Astro::mercatorToLatRad(origin));

    Point q = p - origin;
    q.x *= scaleFactor;
    q.y *= -scaleFactor;
    return q;
#endif
}

Point Astro::mercatorFromModel(const Point& p, const Point& origin)
{
#ifdef USE_EARTH_CURVATURE
    Point q = p / EARTH_RADIUS;
    q.z += 1.0;
    double r = std::sqrt(q.dist3());
    double zz = (r - 1.0) * EARTH_RADIUS;
    q /= r;

    double lat_orig = Astro::mercatorToLatRad(origin);
    double lon_orig = Astro::mercatorToLonRad(origin);

    double x = q.x;
    double y = std::sin(lat_orig)*q.y - std::cos(lat_orig)*q.z;
    double z = std::cos(lat_orig)*q.y + std::sin(lat_orig)*q.z;

    double lat = std::asin(z);
    double dlon = std::atan2(x, -y);
    double lon = lon_orig + dlon;

    return Astro::mercatorFromLatLonRad(lat, lon, zz);
#else
    /* constexpr */ double pi = std::atan(1)*4;
    double scaleFactor = 2*pi*EARTH_RADIUS * std::cos(Astro::mercatorToLatRad(origin));

    Point q = p;
    q.x /= scaleFactor;
    q.y /= -scaleFactor;
    return q + origin;
#endif
}


double Astro::mercatorToLatRad(const Point& p)
{
    /* constexpr */ double pi = std::atan(1)*4;
    double s = (0.5 - p.y) * 2.0 * pi;
    return std::atan(std::sinh(s));
}

double Astro::mercatorToLonRad(const Point& p)
{
    /* constexpr */ double pi = std::atan(1)*4;
    return (p.x - 0.5) * 2.0 * pi;
}

double Astro::mercatorToLatDeg(const Point& p)
{
    /* constexpr */ double pi = std::atan(1)*4;
    return Astro::mercatorToLatRad(p) * 180.0 / pi;
}

double Astro::mercatorToLonDeg(const Point& p)
{
    /* constexpr */ double pi = std::atan(1)*4;
    return Astro::mercatorToLonRad(p) * 180.0 / pi;
}
