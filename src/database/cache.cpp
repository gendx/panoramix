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

#include "cache.hpp"

#include "config.hpp"
#include "protobuf/cache_index.pb.h"
#include <fstream>
#include <QDir>

#include <iostream>

Cache::Cache(const std::string& folder) :
    mFolder(folder)
{
    panoramix::CacheIndex index;

    std::ifstream ifs(mFolder + "/" + INDEX_FILE, std::ifstream::binary);
    index.ParseFromIstream(&ifs);

    for (auto& f : index.files())
        mFiles.push_back(f.name());

    std::cerr << "Index loaded with " << mFiles.size() << " files." << std::endl;
}


void Cache::flushIndex() const
{
    if (!QDir().mkpath(QString::fromStdString(mFolder)))
    {
        std::cerr << "Cannot create cache folder: " << mFolder << std::endl;
        return;
    }

    panoramix::CacheIndex index;
    for (auto& filename : mFiles)
    {
        auto file = index.add_files();
        file->set_name(filename);
    }

    std::ofstream ofs(mFolder + "/" + INDEX_FILE, std::ofstream::binary);
    index.SerializePartialToOstream(&ofs);
}

bool Cache::has(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto found = std::find(mFiles.begin(), mFiles.end(), key);
    return found != mFiles.end();
}

std::unique_ptr<std::ifstream> Cache::readLabels() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return std::make_unique<std::ifstream>(mFolder + "/" + LABELS_FILE, std::ifstream::binary);
}

std::unique_ptr<std::ifstream> Cache::read(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mFiles.begin(), mFiles.end(), key);
    if (it == mFiles.end())
        return std::unique_ptr<std::ifstream>();

    // Put at front of LRU list
    if (it != mFiles.begin())
    {
        auto end = it;
        ++end;
        std::copy_backward(mFiles.begin(), it, end);
        mFiles.front() = key;

        flushIndex();
    }

    return std::make_unique<std::ifstream>(mFolder + "/" + key, std::ifstream::binary);
}

std::unique_ptr<std::ofstream> Cache::write(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (!QDir().mkpath(QString::fromStdString(mFolder)))
    {
        std::cerr << "Cannot create cache folder: " << mFolder << std::endl;
        return std::unique_ptr<std::ofstream>();
    }

    mFiles.insert(mFiles.begin(), key);
    if (mFiles.size() > CACHE_LIMIT)
    {
        for (unsigned int i = CACHE_LIMIT ; i < mFiles.size() ; ++i)
        {
            std::cerr << "Full cache, removing " << mFiles[i] << std::endl;
            QDir().remove(QString::fromStdString(mFolder + "/" + mFiles[i]));
        }
        mFiles.resize(CACHE_LIMIT);
    }

    flushIndex();

    return std::make_unique<std::ofstream>(mFolder + "/" + key, std::ofstream::binary);
}
