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

#ifndef TRIANGULATE_HPP
#define TRIANGULATE_HPP

#include <vector>
#include <functional>
#include "point.hpp"
#include "config.hpp"

#ifdef ENABLE_GEOMETRIC_ASSERT
#include <cassert>
#endif

class OTriangle {
public:
    inline OTriangle() :
        tri(0), orient(0) {}
    inline OTriangle(int _tri, int _orient) :
        tri(_tri), orient(_orient) {}

    inline bool operator==(const OTriangle& otri) const
        {return tri == otri.tri && orient == otri.orient;}
    inline bool operator!=(const OTriangle& otri) const
        {return tri != otri.tri || orient != otri.orient;}

    inline void next()
        {orient = (orient+1) % 3;}
    inline void prev()
        {orient = (orient+2) % 3;}

    inline int encode() const
        {return (tri << 2) | orient;}
    inline static OTriangle decode(int _val)
        {return OTriangle(_val >> 2, _val & 3);}

    int tri;
    int orient;
};

class Triangle {
public:
    Triangle() = default;

    inline void setn(int i, const OTriangle& otri)
        {n[i] = otri.encode();}
    inline void clearn(int i)
        {n[i] = -1;}
    inline OTriangle getn(int i) const
        {return OTriangle::decode(n[i]);}

private:
    // Neighbors
    int n[3] = {-1,-1,-1};

public:
    // Vertices
    int v[3] = {-1,-1,-1};
};


class TrianglePool {
public:
    TrianglePool() = default;

    inline void clear();

    inline int getOrg(const OTriangle& otri) const;
    inline int getDest(const OTriangle& otri) const;
    inline int getApex(const OTriangle& otri) const;

    inline OTriangle sym(const OTriangle& otri) const;

    // Triangulate a set of points by divide-and-conquer
    void divconq(const std::vector<Point>& points, int begin, int end, bool usex, OTriangle& farleft, OTriangle& farright);

    void getFaceNormals(const std::vector<Point>& points, std::vector<Point>& normals) const;

private:
    OTriangle makeTriangle();
    OTriangle makeTriangle(int org, int dest, int apex = -1);

    void edge(int begin, OTriangle& farleft, OTriangle& farright);
    void triangle(const std::vector<Point>& points, int begin, OTriangle& farleft, OTriangle& farright);

    void walkCounterClockwise(const std::vector<Point>& points, OTriangle& otri, const std::function<bool(const Point&, const Point&)>& pred) const;
    void walkClockwise(const std::vector<Point>& points, OTriangle& otri, const std::function<bool(const Point&, const Point&)>& pred) const;

    void prepareHoriz(const std::vector<Point>& points, OTriangle& farleft, OTriangle& innerleft, OTriangle& innerright, OTriangle& farright) const;
    void restoreHoriz(const std::vector<Point>& points, OTriangle& farleft, OTriangle& farright) const;

    void knitLeft(const std::vector<Point>& points, OTriangle& leftcand, int lowerleft, int lowerright, int& upperleft);
    void knitRight(const std::vector<Point>& points, OTriangle& rightcand, int lowerleft, int lowerright, int& upperright);
    void mergeHulls(const std::vector<Point>& points, bool usex, OTriangle& farleft, OTriangle& innerleft, OTriangle& innerright, OTriangle& farright);

    inline void setVertices(OTriangle& otri, int org, int dest, int apex) {
        Triangle& tri = pool[otri.tri];
        tri.v[ otri.orient       ] = apex;
        tri.v[(otri.orient+1) % 3] = org;
        tri.v[(otri.orient+2) % 3] = dest;
    }

    inline void setApex(OTriangle& otri, int vertex) {
        Triangle& tri = pool[otri.tri];
        tri.v[ otri.orient       ] = vertex;
    }
    inline void setOrg(OTriangle& otri, int vertex) {
        Triangle& tri = pool[otri.tri];
        tri.v[(otri.orient+1) % 3] = vertex;
    }
    inline void setDest(OTriangle& otri, int vertex) {
        Triangle& tri = pool[otri.tri];
        tri.v[(otri.orient+2) % 3] = vertex;
    }

    inline void bind(OTriangle& otri1, OTriangle& otri2) {
#ifdef ENABLE_GEOMETRIC_ASSERT
        assert(getOrg(otri1) == getDest(otri2) && getDest(otri1) == getOrg(otri2));
#endif
        Triangle& tri1 = pool[otri1.tri];
        Triangle& tri2 = pool[otri2.tri];
        tri1.setn(otri1.orient, otri2);
        tri2.setn(otri2.orient, otri1);
    }

    std::vector<Triangle> pool;
};

inline void TrianglePool::clear()
    {pool.clear();}

inline int TrianglePool::getApex(const OTriangle& otri) const {
    const Triangle& tri = pool[otri.tri];
    return tri.v[ otri.orient       ];
}
inline int TrianglePool::getOrg(const OTriangle& otri) const {
    const Triangle& tri = pool[otri.tri];
    return tri.v[(otri.orient+1) % 3];
}
inline int TrianglePool::getDest(const OTriangle& otri) const {
    const Triangle& tri = pool[otri.tri];
    return tri.v[(otri.orient+2) % 3];
}

inline OTriangle TrianglePool::sym(const OTriangle& otri) const {
    const Triangle& tri = pool[otri.tri];
    return tri.getn(otri.orient);
}

#endif

