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

#include "quadtree.hpp"

#include <algorithm>
#include <cassert>

void QuadTree::setPoints(const std::vector<Point>& points, bool usex3)
{
    n = points.size();
    assert(n > 0);
    std::vector<int> xindices;
    std::vector<int> yindices;
    std::vector<int> rank(n, 0);

    int log2n = 0;
    int powlog2n = 1;
    while (powlog2n < n)
    {
        powlog2n <<= 1;
        ++log2n;
    }

    // Sort points by x and by y.
    xindices.reserve(n);
    yindices.reserve(n);
    for (int i = 0 ; i < n ; ++i)
    {
        xindices.push_back(i);
        yindices.push_back(i);
    }

    if (usex3)
    {
        std::sort(xindices.begin(), xindices.end(),
                  [&](int a, int b){return points[a].x == points[b].x ? points[a].y < points[b].y : points[a].x < points[b].x;});
        std::sort(yindices.begin(), yindices.end(),
                  [&](int a, int b){return points[a].y == points[b].y ? points[a].x < points[b].x : points[a].y < points[b].y;});
    }
    else
    {
        std::sort(xindices.begin(), xindices.end(),
                  [&](int a, int b){return points[a].x < points[b].x;});
        std::sort(yindices.begin(), yindices.end(),
                  [&](int a, int b){return points[a].y < points[b].y;});
    }

    mSplit.resize(powlog2n);
    build(points, xindices, yindices, rank, 0, n, log2n, 0, true, usex3);

    mRankInternal.clear();
    mRankInternal.resize(powlog2n, -1);
    for (int i = 0 ; i < n ; ++i)
        mRankInternal[rank[i]] = i;

    mRank.clear();
    for (int j : mRankInternal)
        if (j >= 0)
            mRank.push_back(j);
}

int QuadTree::find(const Point& p) const
{
    assert(n > 0);
    int begin = 0;
    int end = n;
    int i = 0;
    int r = 0;
    int powlog2n = 1;

    for (bool side = false ; powlog2n < n ; powlog2n <<= 1, side = !side)
    {
        int diff = (end - begin) / 2;

        double value = side ? p.y : p.x;

        if (diff && value < mSplit[i])
        {
            i = 2*i + 1;
            r = 2*r;
            end = begin+diff;
        }
        else
        {
            i = 2*i + 2;
            r = 2*r + 1;
            begin = begin+diff;
        }
    }

    int result = mRankInternal[r];
    assert(result >= 0);
    return result;
}

void QuadTree::build(const std::vector<Point>& points, std::vector<int>& xindices, std::vector<int>& yindices, std::vector<int>& rank, int begin, int end, int log2n, int spliti, bool usex, bool usex3)
{
    // If 'usex3', subsets of <= 3 vertices are sorted by x coordinate.
    if (usex3 && end - begin <= 3 && !usex)
    {
        build(points, yindices, xindices, rank, begin, end, log2n, spliti, !usex, usex3);
        return;
    }

    int diff = (end - begin) / 2;
    if (!log2n)
        return;

    // Update rank according to x coordinate
    for (int i = begin ; i < begin+diff ; ++i)
    {
        int& r = rank[xindices[i]];
        r = 2*r;
    }
    for (int i = begin+diff ; i < end ; ++i)
    {
        int& r = rank[xindices[i]];
        r = 2*r + 1;
    }

    if (diff)
    {
        auto& p1 = points[xindices[begin+diff-1]];
        auto& p2 = points[xindices[begin+diff]];
        double split = (usex ? p1.x + p2.x : p1.y + p2.y) / 2.0;
        mSplit[spliti] = split;
    }

    // Stable partition y coordinate according to rank
    std::stable_partition(yindices.begin() + begin, yindices.begin() + end,
                          [&](int a){return rank[a] % 2 == 0;});

    // Recurse, inverting both sides
    build(points, yindices, xindices, rank, begin, begin+diff, log2n - 1, 2*spliti + 1, !usex, usex3);
    build(points, yindices, xindices, rank, begin+diff, end, log2n - 1, 2*spliti + 2, !usex, usex3);
}
