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

#include "delaunay.hpp"
#include "primitives.hpp"

#include <algorithm>
#include <unordered_set>

#ifdef ENABLE_GEOMETRIC_ASSERT
#include <cassert>
#endif


Delaunay::Delaunay(std::vector<Point>&& points, bool quadtree) :
    mPoints(std::move(points))
{
#ifdef ENABLE_GEOMETRIC_ASSERT
    assert(mPoints.size() >= 3);
#endif
    init(quadtree);
}

void Delaunay::init(bool quadtree)
{
    // Remove duplicates
    std::sort(mPoints.begin(), mPoints.end(), Point::lexcomp2);
    auto it = std::unique(mPoints.begin(), mPoints.end(),
                          [](const Point& p, const Point& q){ return p.x == q.x && p.y == q.y; });
    size_t newsize = it - mPoints.begin();
    std::cerr << "Creating Delaunay... " << mPoints.size() << " -> " << newsize << " point(s)" << std::endl;
    mPoints.resize(newsize);

    triangulate();

    if (quadtree)
        mQuadTree.setPoints(mPoints);
}

std::unique_ptr<Point> Delaunay::findTrianglePoint(const Point& query) const
{
    OTriangle otri = findTriangle(query);
    int p1 = mPool.getOrg(otri);
    int p2 = mPool.getDest(otri);
    int p3 = mPool.getApex(otri);

    if (p1 >= 0 && p2 >= 0 && p3 >= 0)
    {
        double z = Primitives::interpolate(query, mPoints[p1], mPoints[p2], mPoints[p3]);
        return std::make_unique<Point>(query.x, query.y, z);
    }

    return std::unique_ptr<Point>();
}

OTriangle Delaunay::findTriangle(const Point& query) const
{
    int start = mQuadTree.find(query);
    OTriangle otri = OTriangle::decode(mPoint2triangle[start]);

    Point center = this->triangleCenter(otri);

    for (bool started = false; ; started = true)
    {
        if (!this->validTriangle(otri))
            break;

        // Once started, do not go backwards!
        if (!started)
        {
            if (this->intersectEdge(query, center, otri))
            {
                otri = mPool.sym(otri);
                continue;
            }
        }

        otri.next();
        if (this->intersectEdge(query, center, otri))
        {
            otri = mPool.sym(otri);
            continue;
        }

        otri.next();
        if (this->intersectEdge(query, center, otri))
        {
            otri = mPool.sym(otri);
            continue;
        }

        break;
    }

    return otri;
}

bool Delaunay::validTriangle(const OTriangle& otri) const
{
    int p1 = mPool.getOrg(otri);
    int p2 = mPool.getDest(otri);
    int p3 = mPool.getApex(otri);
    return p1 >= 0 && p2 >= 0 && p3 >= 0;
}

Point Delaunay::triangleCenter(const OTriangle& otri) const
{
    int p1 = mPool.getOrg(otri);
    int p2 = mPool.getDest(otri);
    int p3 = mPool.getApex(otri);
    return (mPoints[p1] + mPoints[p2] + mPoints[p3]) / 3.0;
}

bool Delaunay::intersectEdge(const Point& query, const Point& cursor, const OTriangle& otri) const
{
    int e1 = mPool.getOrg(otri);
    int e2 = mPool.getDest(otri);
    return Primitives::interSegments(query, cursor, mPoints[e1], mPoints[e2]);
}


unsigned int Delaunay::iterPoints(const std::function<void(const Point&)>& f) const
{
    for (auto&& p : mPoints)
        f(p);
    return mPoints.size();
}

void Delaunay::iterNormals(const std::vector<Point>& points, const std::function<void(const Point&)>& f) const
{
    std::vector<Point> faceNormals;
    mPool.getFaceNormals(points, faceNormals);

    for (auto i : mPoint2triangle)
    {
        OTriangle otri = OTriangle::decode(i);
#ifdef ENABLE_GEOMETRIC_ASSERT
        assert(mPool.getOrg(otri) >= 0);
#endif

        Point normal(0, 0, 0);
        int normalCount = 0;

        OTriangle iter = otri;
        for (;;)
        {
            int dest = mPool.getDest(iter);
            int apex = mPool.getApex(iter);
            // Don't count points on the hull
            if (dest >= 0 && apex >= 0)
            {
                normal += faceNormals[iter.tri];
                ++normalCount;
            }

            iter = mPool.sym(iter);
            iter.next();
            if (iter == otri)
                break;
        }

#ifdef ENABLE_GEOMETRIC_ASSERT
        assert(normalCount > 0);
#endif
        normal.normalize3();

        f(normal);
    }
}

unsigned int Delaunay::iterTrianglesIndices(const std::function<void(unsigned int, unsigned int, unsigned int)>& f) const
{
    for (int i : mValidTriangles)
    {
        OTriangle otri = OTriangle::decode(i);
        int e1 = mPool.getOrg(otri);
        int e2 = mPool.getDest(otri);
        int e3 = mPool.getApex(otri);
        f(e1, e2, e3);
    }
    return mValidTriangles.size();
}


void Delaunay::triangulate()
{
    // Alternate order with subsets of <= 3 vertices always sorted by x
    QuadTree quadTree;
    quadTree.setPoints(mPoints, true);

    std::vector<Point> points;
    for (int r : quadTree.rank())
        points.push_back(mPoints[r]);
    mPoints = std::move(points);

    OTriangle hullleft, hullright;
    mPool.clear();
    mPool.divconq(mPoints, 0, mPoints.size(), true, hullleft, hullright);

    finalize(hullright);
}

void Delaunay::finalize(const OTriangle& start)
{
    mValidTriangles.clear();
    mPoint2triangle.clear();
    mPoint2triangle.resize(mPoints.size(), -1);

    std::unordered_set<int> discovered;
    std::vector<OTriangle> queue;

    queue.push_back(start);
    while (!queue.empty())
    {
        // Traverse triangle
        OTriangle otri = queue.back();
        queue.pop_back();
        if (discovered.find(otri.tri) != discovered.end())
            continue;
        discovered.insert(otri.tri);

        // Add half-edges if not ghost
        int p1 = mPool.getOrg(otri);
        int p2 = mPool.getDest(otri);
        int p3 = mPool.getApex(otri);

        if (p1 >= 0 && p2 >= 0 && p3 >= 0)
        {
#ifdef ENABLE_GEOMETRIC_ASSERT
            assert(Point::det(mPoints[p1], mPoints[p2], mPoints[p3]) > 0.0);
#endif

            mValidTriangles.push_back(otri.encode());

            mPoint2triangle[p1] = otri.encode();
            otri.next();
            mPoint2triangle[p2] = otri.encode();
            otri.next();
            mPoint2triangle[p3] = otri.encode();
            otri.next();
        }

        // Add neighbors
        queue.push_back(mPool.sym(otri));
        otri.next();
        queue.push_back(mPool.sym(otri));
        otri.next();
        queue.push_back(mPool.sym(otri));
    }
}
