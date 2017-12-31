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

#include "triangulate.hpp"

inline static double counterClockwise(const std::vector<Point>& points, int a, int b, int c)
{
    return Point::det(points[a], points[b], points[c]);
}

inline static double incircle(const std::vector<Point>& points, int a, int b, int c, int d)
{
    return Point::incircle(points[a], points[b], points[c], points[d]);
}


OTriangle TrianglePool::makeTriangle()
{
    int tri = pool.size();
    pool.emplace_back();
    return OTriangle(tri, 0);
}

OTriangle TrianglePool::makeTriangle(int org, int dest, int apex)
{
    OTriangle otri = makeTriangle();
    setVertices(otri, org, dest, apex);
    return otri;
}

void TrianglePool::edge(int begin, OTriangle& farleft, OTriangle& farright)
{
    // Vertices
    farleft = makeTriangle(begin, begin+1);
    farright = makeTriangle(begin+1, begin);

    // Neighbors
    bind(farleft, farright);
    farleft.prev();
    farright.next();

    bind(farleft, farright);
    farleft.prev();
    farright.next();

    bind(farleft, farright);
    farleft = farright;
    farleft.prev();
}

void TrianglePool::triangle(const std::vector<Point>& points, int begin, OTriangle& farleft, OTriangle& farright)
{
    double area = counterClockwise(points, begin, begin+1, begin+2);

    // Colinear
    if (area == 0.0)
    {
        // Vertices
        OTriangle tri0 = makeTriangle(begin, begin+1);
        OTriangle tri1 = makeTriangle(begin+1, begin);
        OTriangle tri2 = makeTriangle(begin+2, begin+1);
        OTriangle tri3 = makeTriangle(begin+1, begin+2);

        // Neighbors
        bind(tri0, tri1);
        bind(tri2, tri3);

        tri0.next();
        tri1.prev();
        tri2.next();
        tri3.prev();
        bind(tri0, tri3);
        bind(tri1, tri2);

        tri0.next();
        tri1.prev();
        tri2.next();
        tri3.prev();
        bind(tri0, tri1);
        bind(tri2, tri3);

        farleft = tri1;
        farright = tri2;
    }
    // Not colinear
    else
    {
        OTriangle midtri, tri1, tri2, tri3;

        // Vertices
        if (area > 0.0)
        {
            midtri = makeTriangle(begin, begin+1, begin+2);
            tri1 = makeTriangle(begin+1, begin);
            tri2 = makeTriangle(begin+2, begin+1);
            tri3 = makeTriangle(begin, begin+2);
        }
        else
        {
            midtri = makeTriangle(begin, begin+2, begin+1);
            tri1 = makeTriangle(begin+2, begin);
            tri2 = makeTriangle(begin+1, begin+2);
            tri3 = makeTriangle(begin, begin+1);
        }

        // Neighbors
        bind(midtri, tri1);
        midtri.next();
        bind(midtri, tri2);
        midtri.next();
        bind(midtri, tri3);

        tri1.prev();
        tri2.next();
        bind(tri1, tri2);

        tri1.prev();
        tri3.prev();
        bind(tri1, tri3);

        tri2.next();
        tri3.prev();
        bind(tri2, tri3);

        farleft = tri1;

        if (area > 0.0)
            farright = tri2;
        else
        {
            farright = farleft;
            farright.next();
        }
    }
}

// Walk counter-clockwise around the hull
void TrianglePool::walkCounterClockwise(const std::vector<Point>& points, OTriangle& otri, const std::function<bool(const Point&, const Point&)>& pred) const
{
    int org = getOrg(otri);
    int apex = getApex(otri);
    while (pred(points[apex], points[org]))
    {
        otri.next();
        otri = sym(otri);
        org = apex;
        apex = getApex(otri);
    }
}

// Walk clockwise around the hull
void TrianglePool::walkClockwise(const std::vector<Point>& points, OTriangle& otri, const std::function<bool(const Point&, const Point&)>& pred) const
{
    int dest = getDest(otri);
    int apex = getApex(otri);
    while (pred(points[apex], points[dest]))
    {
        otri.prev();
        otri = sym(otri);
        dest = apex;
        apex = getApex(otri);
    }
}

void TrianglePool::prepareHoriz(const std::vector<Point>& points, OTriangle& farleft, OTriangle& innerleft, OTriangle& innerright, OTriangle& farright) const
{
    // Shift extremal vertices to topmost and bottommost, instead of leftmost and rightmost.
    walkCounterClockwise(points, farleft,
                         [](const Point& apex, const Point& org){return apex.y < org.y;});
    farright = sym(farright);
    walkCounterClockwise(points, farright,
                         [](const Point& apex, const Point& org){return apex.y > org.y;});
    farright = sym(farright);

    innerleft = sym(innerleft);
    walkCounterClockwise(points, innerleft,
                         [](const Point& apex, const Point& org){return apex.y > org.y;});
    innerleft = sym(innerleft);
    walkCounterClockwise(points, innerright,
                         [](const Point& apex, const Point& org){return apex.y < org.y;});
}

void TrianglePool::restoreHoriz(const std::vector<Point>& points, OTriangle& farleft, OTriangle& farright) const
{
    // Restore extremal vertices to leftmost and rightmost
    farleft = sym(farleft);
    walkClockwise(points, farleft,
                  [](const Point& apex, const Point& dest){return apex.x < dest.x;});
    farleft = sym(farleft);
    walkClockwise(points, farright,
                  [](const Point& apex, const Point& dest){return apex.x > dest.x;});
}

void TrianglePool::knitLeft(const std::vector<Point>& points, OTriangle& leftcand, int lowerleft, int lowerright, int& upperleft)
{
    OTriangle nextedge = leftcand;
    nextedge.prev();
    nextedge = sym(nextedge);
    int nextapex = getApex(nextedge);

    if (nextapex == -1)
        return;
    bool badedge = incircle(points, lowerleft, lowerright, upperleft, nextapex) > 0.0;

    while (badedge)
    {
        // Edge flip.
        nextedge.next();
        OTriangle topcasing = sym(nextedge);
        nextedge.next();
        OTriangle sidecasing = sym(nextedge);
        leftcand.next();
        OTriangle outercasing = sym(leftcand);

        // Update the vertices.
        setVertices(leftcand, -1, nextapex, lowerleft);
        setVertices(nextedge, nextapex, -1, upperleft);

        // Bind the triangles.
        bind(leftcand, nextedge); // TODO: need to bind?
        leftcand.next();
        nextedge.prev();
        bind(leftcand, sidecasing);
        leftcand.next();
        bind(nextedge, topcasing);
        nextedge.prev();
        bind(nextedge, outercasing);

        // Update exposed vertice.
        upperleft = nextapex;
        nextedge = sidecasing;
        nextapex = getApex(nextedge);

        if (nextapex == -1)
            return;
        badedge = incircle(points, lowerleft, lowerright, upperleft, nextapex) > 0.0;
    }
}

void TrianglePool::knitRight(const std::vector<Point>& points, OTriangle& rightcand, int lowerleft, int lowerright, int& upperright)
{
    OTriangle nextedge = rightcand;
    nextedge.next();
    nextedge = sym(nextedge);
    int nextapex = getApex(nextedge);

    if (nextapex == -1)
        return;
    bool badedge = incircle(points, lowerleft, lowerright, upperright, nextapex) > 0.0;

    while (badedge)
    {
        // Edge flip.
        nextedge.prev();
        OTriangle topcasing = sym(nextedge);
        nextedge.prev();
        OTriangle sidecasing = sym(nextedge);
        rightcand.prev();
        OTriangle outercasing = sym(rightcand);

        // Update the vertices.
        setVertices(rightcand, nextapex, -1, lowerright);
        setVertices(nextedge, -1, nextapex, upperright);

        // Bind the triangles.
        bind(rightcand, nextedge); // TODO: need to bind?
        rightcand.prev();
        nextedge.next();
        bind(rightcand, sidecasing);
        rightcand.prev();
        bind(nextedge, topcasing);
        nextedge.next();
        bind(nextedge, outercasing);

        // Update exposed vertice.
        upperright = nextapex;
        nextedge = sidecasing;
        nextapex = getApex(nextedge);

        if (nextapex == -1)
            return;
        badedge = incircle(points, lowerleft, lowerright, upperright, nextapex) > 0.0;
    }
}

void TrianglePool::mergeHulls(const std::vector<Point>& points, bool usex, OTriangle& farleft, OTriangle& innerleft, OTriangle& innerright, OTriangle& farright)
{
    // Horizontal cut.
    if (!usex)
        prepareHoriz(points, farleft, innerleft, innerright, farright);

    int innerleftdest = getDest(innerleft);
    int innerleftapex = getApex(innerleft);
    int innerrightorg = getOrg(innerright);
    int innerrightapex = getApex(innerright);

    // Find the tangent below both hulls.
    bool changemade = true;
    while (changemade)
    {
        changemade = false;

        // Move to the bottom of the left hull.
        if (counterClockwise(points, innerleftdest, innerleftapex, innerrightorg) > 0.0)
        {
            innerleft.prev();
            innerleft = sym(innerleft);
            innerleftdest = innerleftapex;
            innerleftapex = getApex(innerleft);
            changemade = true;
        }

        // Move to the bottom of the right hull.
        if (counterClockwise(points, innerrightapex, innerrightorg, innerleftdest) > 0.0)
        {
            innerright.next();
            innerright = sym(innerright);
            innerrightorg = innerrightapex;
            innerrightapex = getApex(innerright);
            changemade = true;
        }
    }

    // Next "gear tooth".
    OTriangle leftcand = sym(innerleft);
    OTriangle rightcand = sym(innerright);

    // New triangle at the bottom.
    OTriangle baseedge = makeTriangle(innerrightorg, innerleftdest);
    baseedge.next();
    bind(baseedge, innerleft);
    baseedge.next();
    bind(baseedge, innerright);
    baseedge.next();

    // Update far triangles.
    int farleftpt = getOrg(farleft);
    if (innerleftdest == farleftpt)
    {
        farleft = baseedge;
        farleft.next();
    }
    int farrightpt = getDest(farright);
    if (innerrightorg == farrightpt)
    {
        farright = baseedge;
        farright.prev();
    }

    // Current vertices for "knitting".
    int lowerleft = innerleftdest;
    int lowerright = innerrightorg;
    int upperleft = getApex(leftcand);
    int upperright = getApex(rightcand);

    // Close the gap between the two hulls.
    while (true)
    {
        bool leftfinished = counterClockwise(points, upperleft, lowerleft, lowerright) <= 0.0;
        bool rightfinished = counterClockwise(points, upperright, lowerleft, lowerright) <= 0.0;

        if (leftfinished && rightfinished)
        {
            // New bounding triangle.
            OTriangle nextedge = makeTriangle(lowerleft, lowerright);

            bind(nextedge, baseedge);
            nextedge.next();
            bind(nextedge, rightcand);
            nextedge.next();
            bind(nextedge, leftcand);

            // Horizontal cut.
            if (!usex)
                restoreHoriz(points, farleft, farright);
            break;
        }

        if (!leftfinished)
            knitLeft(points, leftcand, lowerleft, lowerright, upperleft);
        if (!rightfinished)
            knitRight(points, rightcand, lowerleft, lowerright, upperright);

        if (leftfinished || (!rightfinished && incircle(points, upperleft, lowerleft, lowerright, upperright) > 0.0))
        {
            // Add edge from lower-left to upper-right.
            setOrg(rightcand, lowerleft);
            bind(baseedge, rightcand);
            baseedge = rightcand;
            baseedge.prev();
            lowerright = upperright;
            rightcand = sym(baseedge);
            upperright = getApex(rightcand);
        }
        else
        {
            // Add edge from upper-left to lower-right.
            setDest(leftcand, lowerright);
            bind(baseedge, leftcand);
            baseedge = leftcand;
            baseedge.next();
            lowerleft = upperleft;
            leftcand = sym(baseedge);
            upperleft = getApex(leftcand);
        }
    }
}

// Triangulate a set of points by divide-and-conquer
void TrianglePool::divconq(const std::vector<Point>& points, int begin, int end, bool usex, OTriangle& farleft, OTriangle& farright)
{
    if (end <= begin + 3)
        usex = true;

    // 2 vertices = 1 edge
    if (end == begin + 2)
        edge(begin, farleft, farright);
    // 3 vertices = 1 triangle
    else if (end == begin + 3)
        triangle(points, begin, farleft, farright);
    // Recursion
    else
    {
        OTriangle innerleft, innerright;
        int diff = (end - begin) / 2;
        divconq(points, begin, begin+diff, !usex, farleft, innerleft);
        divconq(points, begin+diff, end, !usex, innerright, farright);
        mergeHulls(points, usex, farleft, innerleft, innerright, farright);
    }

#ifdef ENABLE_GEOMETRIC_ASSERT
    assert(getDest(farleft) < 0);
    assert(getOrg(farright) < 0);
#endif
}

void TrianglePool::getFaceNormals(const std::vector<Point>& points, std::vector<Point>& normals) const
{
    normals.clear();
    normals.reserve(pool.size());
    for (auto&& tri : pool)
    {
        int p1 = tri.v[0];
        int p2 = tri.v[1];
        int p3 = tri.v[2];

        Point normal;
        if (p1 >= 0 && p2 >= 0 && p3 >= 0)
        {
            const Point& a = points[p1];
            const Point& b = points[p2];
            const Point& c = points[p3];
            normal = (b-a).cross(c-a);
            //normal.normalize3();
        }

        normals.emplace_back(normal);
    }
}
