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

#ifndef DELAUNAY_HPP
#define DELAUNAY_HPP

#include "quadtree.hpp"
#include "triangulate.hpp"
#include <vector>
#include <memory>

class Delaunay
{
public:
    Delaunay(std::vector<Point>&& points, bool quadtree);

    std::unique_ptr<Point> findTrianglePoint(const Point& point) const;

    inline const std::vector<Point>& points() const;

    unsigned int iterPoints(const std::function<void(const Point&)>& f) const;
    void iterNormals(const std::vector<Point>& points, const std::function<void(const Point&)>& f) const;
    unsigned int iterTrianglesIndices(const std::function<void(unsigned int, unsigned int, unsigned int)>& f) const;

private:
    void init(bool quadtree);
    void triangulate();
    void finalize(const OTriangle& start);

    OTriangle findTriangle(const Point& point) const;

    bool validTriangle(const OTriangle& otri) const;
    Point triangleCenter(const OTriangle& otri) const;
    bool intersectEdge(const Point& p1, const Point& p2, const OTriangle& otri) const;

    std::vector<Point> mPoints;
    std::vector<Point> mNormals;
    QuadTree mQuadTree;

    TrianglePool mPool;
    std::vector<int> mValidTriangles;
    std::vector<int> mPoint2triangle;
};

inline const std::vector<Point>& Delaunay::points() const
    {return mPoints;}

#endif

