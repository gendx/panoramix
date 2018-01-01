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

#include "worldmodel.hpp"

#include "geometry/astro.hpp"
#include "protobuf/xyz.pb.h"
#include "config.hpp"

#include <algorithm>

WorldModel::WorldModel(std::shared_ptr<Database> database) :
    mDatabase(database),
    mOrigin(0, 0, 1),
    mSelection(0, 0)
{
}

void WorldModel::loadLatLon(double lat, double lon, int zoom)
{
    auto self(shared_from_this());
    TaskManager::manager.launch([this, self, lat, lon, zoom](){
        Point origin = Astro::mercatorFromLatLonDeg(lat, lon);
        mOrigin.set(origin);
        mSelection.set(origin);

        double z = 1 << zoom;
        int x = origin.x * z;
        int y = origin.y * z;

        // Request tiles.
        auto tilelist = WorldModel::genTileList(x, y, zoom);
        for (auto& tile : tilelist)
        {
            auto self(shared_from_this());
            TaskManager::manager.launch([this, self, zoom, tile] {
                this->load(tile.zoom, tile.x, tile.y, true);
            });
        }

        // Request labels.
        auto self(shared_from_this());
        TaskManager::manager.launch([this, self] {
            this->loadGlobalLabels();
        });

        unsigned int countMessages = 0;
        while (countMessages < tilelist.size() + 1)
        {
            // Wait for messages.
            mMsgQueue.wait([] (const std::vector<Message>& queue) {return !queue.empty();} );

            std::vector<Message> messages;
            mMsgQueue.swap(messages);
            countMessages += messages.size();

            // Early abort
            if (!mReload.get())
            {
                std::cerr << "########## Nothing to reload (" << countMessages << "/" << (tilelist.size() + 1) << ") ##########" << std::endl;
                continue;
            }

            // Transfer all available messages.
            unsigned int failedMessages = 0;
            for (auto&& msg : messages)
            {
                if (msg.valid)
                {
                    switch (msg.type)
                    {
                    case Message::MSG_TILE:
                        mTiles.emplace_back(std::move(msg.tile));
                        break;
                    case Message::MSG_LABELS:
                        mLabels = std::move(msg.labels);
                        break;
                    }
                }
                else
                    ++failedMessages;
            }
            if (failedMessages)
                std::cerr << "Received " << failedMessages << " failures." << std::endl;

            // Update 3D model.
            Polygon points;
            for (auto& tile : mTiles)
                for (auto& p : tile.points)
                    points.push_back(p);

            std::cerr << "########## Updating (" << countMessages << "/" << (tilelist.size() + 1) << ") ########## with " << mTiles.size() << " tiles and " << points.size() << " points." << std::endl;

            if (points.size() < 3)
            {
                std::cerr << "Less than 3 points, skipping..." << std::endl;
                continue;
            }

            auto delaunay = std::make_shared<Delaunay>(std::move(points), true);

            // Update labels.
            unsigned int labelCount = 0;
            if (mLabels)
            {
                std::vector<Label> labels;
                for (auto& tile : mTiles)
                    mLabels->filter(tile.tileInfo, labels);
                std::cerr << "Filtered " << labels.size() << " labels for " << mTiles.size() << " tiles." << std::endl;

                // Adjust to model view
                auto visibleLabels = std::make_shared<std::vector<Label>>();
                for (const Label& l : labels)
                {
                    auto found = delaunay->findTrianglePoint(l.point);
                    if (found)
                    {
                        visibleLabels->push_back(l);
                        Point& p = visibleLabels->back().point;
                        p.z = found->z;
                        p = Astro::mercatorToModel(p, origin);
                    }
                }

                // Sort labels by priority (elevation)
                std::sort(visibleLabels->begin(), visibleLabels->end(),
                          [](const Label& lhs, const Label& rhs) {return lhs.elevationEstimate() > rhs.elevationEstimate();});

                labelCount = visibleLabels->size();
                mVisibleLabels.swap(visibleLabels);
            }

            // Prepare mesh in this thread to avoid blocking the main thread
            std::shared_ptr<Mesh> mesh = WorldModel::makeMesh(*delaunay, origin);
            mesh->tileCount = mTiles.size();
            mesh->labelCount = labelCount;

            mDelaunay.swap(delaunay);
            mMesh.swap(mesh);

            std::cerr << "New mesh available" << std::endl;

            // Send the reload signal
            auto call = [] (std::function<void()>& f) {if (f) f();};
            mReload.apply(call);
        }

        std::cerr << "########## Finished updating ##########" << std::endl;
    });
}

std::shared_ptr<WorldModel::Mesh> WorldModel::makeMesh(const Delaunay& delaunay, const Point& origin)
{
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
    std::vector<Point> points;
    mesh->pointCount = delaunay.iterPoints([&](const Point& p){
        Point q = Astro::mercatorToModel(p, origin);
        points.push_back(q);
        mesh->vertices.push_back(q.x);
        mesh->vertices.push_back(q.y);
        mesh->vertices.push_back(q.z);
#ifdef USE_EARTH_CURVATURE
        mesh->vertices.push_back(p.z);
#endif
    });
    delaunay.iterNormals(points, [&](const Point& n){
        // Changed Mercator orientation to direct orientation.
        mesh->normals.push_back(-n.x);
        mesh->normals.push_back(-n.y);
        mesh->normals.push_back(-n.z);
    });
    mesh->triangleCount = delaunay.iterTrianglesIndices([&](unsigned int a, unsigned int b, unsigned int c){
        // Changed Mercator orientation to direct orientation.
        mesh->indices.push_back(a);
        mesh->indices.push_back(c);
        mesh->indices.push_back(b);
    });
    return mesh;
}


std::vector<TileInfo> WorldModel::genTileList(int x, int y, int zoom)
{
    std::vector<TileInfo> result;

    int zz = 1 << zoom;
    int xx = x;
    int yy = y;
    int old_xcorner = 0, old_ycorner = 0;

    constexpr int MINZOOM = 11;
    constexpr int MINZOOMDELTA = 4;
    for (int i = 0 ; zoom-i >= MINZOOM ; ++i)
    {
        int xcorner = (xx-1) >> 1;
        int ycorner = (yy-1) >> 1;

        int mina = xcorner*2;
        int minb = ycorner*2;
        int maxa = mina + 4;
        int maxb = minb + 4;

        if (zoom-i == MINZOOM)
        {
            mina -= MINZOOMDELTA; minb -= MINZOOMDELTA;
            maxa += MINZOOMDELTA; maxb += MINZOOMDELTA;
        }

        for (int a = mina ; a < maxa ; ++a)
        {
            for (int b = minb ; b < maxb ; ++b)
            {
                if (a < 0 || a >= zz || b < 0 || b >= zz)
                    continue;
                if (i > 0 && (a >= old_xcorner && a < old_xcorner+2 && b >= old_ycorner && b < old_ycorner+2))
                    continue;

                result.emplace_back(zoom-i, a, b);
            }
        }

        xx >>= 1;
        yy >>= 1;
        old_xcorner = xcorner;
        old_ycorner = ycorner;
    }

    return result;
}

void WorldModel::loadGlobalLabels()
{
    std::cerr << "########## Loading labels... ##########" << std::endl;
    std::unique_ptr<Labels> labels;

    std::unique_ptr<std::ifstream> ifs = mDatabase->loadLabels();
    if (ifs)
    {
        labels = std::make_unique<Labels>();
        labels->load(*ifs);
        std::cerr << "########## Loaded labels! ##########" << std::endl;
    }
    else
        std::cerr << "########## Could not load labels ##########" << std::endl;

    auto f = [l = std::move(labels)] (std::vector<Message>& queue) mutable {
        bool valid = (bool)l;
        queue.emplace_back(Message::make_labels(std::move(l), valid));
    };
    mMsgQueue.apply(f);
    mMsgQueue.notify_one();
}

void WorldModel::load(int zoom, int x, int y, bool retry)
{
    int zz = 1 << zoom;
    int xx = (x + zz) % zz; // clip to range [0, 2^zoom-1]
    int yy = y;
    double scale = 1.0 / (4096.0 * zz);

    panoramix::XYZ xyz;
    bool valid = true;

    std::unique_ptr<std::ifstream> ifs = mDatabase->loadSimple(zoom, xx, yy, "xyz");
    if (ifs)
    {
        valid = xyz.ParseFromIstream(ifs.get());
        if (!valid)
            std::cerr << "Error parsing xyz: " << zoom << ", " << xx << ", " << yy << std::endl;
    }
    else
    {
        if (retry)
        {
            auto self(shared_from_this());
            mDatabase->loadMvt(zoom, x, y,
            // onSuccess
            [this, self, zoom, x, y, xx, yy] (const std::string& content) {
                this->tile2xyz(zoom, xx, yy, content);
                this->load(zoom, x, y, false);
            },
            // onError
            [this, self, zoom, x, y, xx, yy] {
                this->load(zoom, x, y, false);
            });
            return;
        }
        else
        {
            std::cerr << "Could not find/simplify xyz: " << zoom << ", " << xx << ", " << yy << std::endl;
            valid = false;
        }
    }

    Tile tile;
    if (valid)
    {
        // TODO: assert that tile is indeed 4096x4096
        Point translate(x*4096, y*4096);

        tile.tileInfo = TileInfo(zoom, x, y);
        // TODO: check multiple of 3
        for (int i = 0 ; i+2 < xyz.points_size() ; i+=3)
        {
            tile.points.emplace_back(xyz.points(i), xyz.points(i+1), xyz.points(i+2));
            auto& pt = tile.points.back();
            pt.add2(translate);
            pt.scaleXY(scale);
        }
    }

    auto f = [t = std::move(tile), valid] (std::vector<Message>& queue) mutable {
        queue.emplace_back(Message::make_tile(std::move(t), valid));
    };
    mMsgQueue.apply(f);
    mMsgQueue.notify_one();
}

void WorldModel::tile2xyz(int zoom, int x, int y, const std::string& content)
{
    vector_tile::Tile tile;
    if (!tile.ParseFromString(content))
    {
        std::cerr << "Error parsing tile: " << zoom << ", " << x << ", " << y << std::endl;
        return;
    }

    Mvt mvt(std::move(tile));
    std::vector<Polygon> path = mvt.getContour();

    if (!path.empty())
    {
        Polygon points;
        for (auto& polygon : path)
            for (auto& point : polygon)
                if (Mvt::isValid(point))
                    points.push_back(point);

        panoramix::XYZ xyz;
        for (auto& p : points)
        {
            xyz.add_points(p.x);
            xyz.add_points(p.y);
            xyz.add_points(p.z);
        }

        auto ofs = mDatabase->storeSimple(zoom, x, y, "xyz");
        if (ofs)
            xyz.SerializeToOstream(ofs.get());
        else
            std::cerr << "Cannot write cache entry: " << zoom << ", " << x << ", " << y << std::endl;
    }
}

