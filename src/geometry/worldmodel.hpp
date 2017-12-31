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

#ifndef WORLD_MODEL_HPP
#define WORLD_MODEL_HPP

#include <memory>
#include "database/database.hpp"
#include "util/concurrency.hpp"
#include "geometry/delaunay.hpp"
#include "geometry/labels.hpp"

class WorldModel : public std::enable_shared_from_this<WorldModel>
{
public:
    WorldModel(std::shared_ptr<Database> database);

    void loadLatLon(double lat, double lon, int zoom);

    struct Mesh {
        Mesh() :
            pointCount(0), triangleCount(0), tileCount(0), labelCount(0) {}

        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<unsigned int> indices;
        unsigned int pointCount;
        unsigned int triangleCount;
        unsigned int tileCount;
        unsigned int labelCount;
    };

    inline LockGuardedShared<std::vector<Label>>& visibleLabels();
    inline LockGuarded<std::shared_ptr<Delaunay>>& delaunay();
    inline LockGuardedShared<Mesh>& mesh();
    inline LockGuarded<Point>& origin();
    inline LockGuarded<Point>& selection();

    inline LockGuarded<std::function<void()>>& reload();

private:
    struct Tile {
        TileInfo tileInfo;
        Polygon points;
    };

    // TODO: use proper variant type
    struct Message {
        enum Type {MSG_TILE, MSG_LABELS};

        inline static Message make_tile(Tile&& t, bool valid) {
            Message m;
            m.type = MSG_TILE;
            m.valid = valid;
            m.tile = std::move(t);
            return m;
        }

        inline static Message make_labels(std::unique_ptr<Labels>&& l, bool valid) {
            Message m;
            m.type = MSG_LABELS;
            m.valid = valid;
            m.labels = std::move(l);
            return m;
        }

        Type type;
        bool valid;
        Tile tile;
        std::unique_ptr<Labels> labels;
    };

    static std::vector<TileInfo> genTileList(int x, int y, int zoom);
    void loadGlobalLabels();
    void load(int zoom, int x, int y, bool retry);
    void tile2xyz(int zoom, int x, int y, const std::string& content);

    static std::shared_ptr<Mesh> makeMesh(const Delaunay& delaunay, const Point& origin);

    std::shared_ptr<Database> mDatabase;
    // TODO: use std::optional
    std::unique_ptr<Labels> mLabels;
    std::vector<Tile> mTiles;
    LockGuarded<std::vector<Message>> mMsgQueue;
    LockGuardedShared<std::vector<Label>> mVisibleLabels;
    LockGuarded<std::shared_ptr<Delaunay>> mDelaunay;
    LockGuardedShared<Mesh> mMesh;
    LockGuarded<Point> mOrigin;
    LockGuarded<Point> mSelection;

    LockGuarded<std::function<void()>> mReload;
};

inline LockGuardedShared<std::vector<Label>>& WorldModel::visibleLabels()
    {return mVisibleLabels;}
inline LockGuarded<std::shared_ptr<Delaunay>>& WorldModel::delaunay()
    {return mDelaunay;}
inline LockGuardedShared<WorldModel::Mesh>& WorldModel::mesh()
    {return mMesh;}
inline LockGuarded<Point>& WorldModel::origin()
    {return mOrigin;}
inline LockGuarded<Point>& WorldModel::selection()
    {return mSelection;}

inline LockGuarded<std::function<void()>>& WorldModel::reload()
    {return mReload;}


#endif // WORLD_MODEL_HPP
