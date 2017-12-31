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

#include "database.hpp"

#include <fstream>
#include "networkmanager.hpp"
#include "config.hpp"

#include <iostream>
#include <chrono>
#include <ctime>

Database::Database(const std::string& token, const std::string& cacheFolder) :
    mToken(token),
    mCache(cacheFolder)
{
}

std::unique_ptr<std::ifstream> Database::loadLabels()
{
    auto ifs = mCache.readLabels();
    if (ifs)
        return ifs;

    return std::unique_ptr<std::ifstream>();
}

std::unique_ptr<std::ifstream> Database::loadSimple(int z, int x, int y, const std::string& ext)
{
    std::string key = std::to_string(z) + "-" + std::to_string(x) + "-" + std::to_string(y) + "." + ext;
    return mCache.read(key);
}

std::unique_ptr<std::ofstream> Database::storeSimple(int z, int x, int y, const std::string& ext)
{
    std::string key = std::to_string(z) + "-" + std::to_string(x) + "-" + std::to_string(y) + "." + ext;
    return mCache.write(key);
}

void Database::loadMvt(int z, int x, int y, const std::function<void(const std::string&)>& onSuccess, const std::function<void()>& onError)
{
    std::string key = std::to_string(z) + "-" + std::to_string(x) + "-" + std::to_string(y) + ".mvt";

    static const std::string domain = MAPBOX_DOMAIN;
    static const std::string source = MAPBOX_SOURCE;
    std::string path = "/v4/" + source + "/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".mvt?access_token=" + mToken;

    bool exists = false;
    {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mRequests.find(key) != mRequests.end())
            exists = true;
        else
            mRequests.emplace(std::piecewise_construct,
                              std::forward_as_tuple(key),
                              std::forward_as_tuple(onSuccess, onError));
    }

    if (exists)
        std::cerr << "Request is already pending for key: " << key << std::endl;
    else
    {
        // NetworkManager emits finished(key, content) later.
        NetworkManager::manager.getHTTPS(domain, path,
                                         [this, key] (const std::string& content) {finished(key, content);},
                                         [this, key] (asio::error_code ec) {error(key, ec);});

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cerr << "Network get key: " << key << " @ " << std::ctime(&now);
    }
}

void Database::finished(const std::string& key, const std::string& content)
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cerr << "Network finished key: " << key << " @ " << std::ctime(&now);

    std::unique_lock<std::mutex> lock(mMutex);
    auto found = mRequests.find(key);

    if (found == mRequests.end())
    {
        lock.unlock();
        std::cerr << "key was not requested: " << key << std::endl;
    }
    else
    {
        Request request = found->second;
        mRequests.erase(key);
        lock.unlock();

        request.onSuccess(content);
    }
}

void Database::error(const std::string& key, asio::error_code ec)
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cerr << "Network error for key: " << key << " @ " << std::ctime(&now) << std::endl;

    std::unique_lock<std::mutex> lock(mMutex);
    auto found = mRequests.find(key);

    if (found == mRequests.end())
    {
        lock.unlock();
        std::cerr << "key was not requested: " << key << std::endl;
    }
    else
    {
        Request request = found->second;
        mRequests.erase(key);
        lock.unlock();

        std::cerr << "Network error: [" << std::hex << ec.value() << std::dec << "] " << ec.message() << std::endl;
        request.onError();
    }
}
