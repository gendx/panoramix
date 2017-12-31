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

#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <vector>
#include "point.hpp"

class QuadTree
{
public:
    QuadTree() = default;

    // If 'usex3', subsets of <= 3 vertices are sorted by x coordinate.
    void setPoints(const std::vector<Point>& points, bool usex3 = false);
    int find(const Point& p) const;

    inline const std::vector<int>& rank() const;

private:
    void build(const std::vector<Point>& points, std::vector<int>& xindices, std::vector<int>& yindices, std::vector<int>& rank, int begin, int end, int log2n, int spliti, bool usex, bool usex3);

    int n;
    std::vector<double> mSplit;
    std::vector<int> mRankInternal;
    std::vector<int> mRank;
};

inline const std::vector<int>& QuadTree::rank() const
    {return mRank;}

#endif

