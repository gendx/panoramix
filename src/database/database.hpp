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

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <unordered_map>
#include <mutex>
#include <asio/error_code.hpp>

#include "cache.hpp"

class Database
{
public:
    Database(const std::string& token, const std::string& cacheFolder);

    std::unique_ptr<std::ifstream> loadLabels();
    std::unique_ptr<std::ifstream> loadSimple(int z, int x, int y, const std::string& ext);
    std::unique_ptr<std::ofstream> storeSimple(int z, int x, int y, const std::string& ext);
    void loadMvt(int z, int x, int y, const std::function<void(const std::string&)>& onSuccess, const std::function<void()>& onError);

private:
    struct Request {
        Request(const std::function<void(const std::string&)>& _onSuccess, const std::function<void()>& _onError) :
            onSuccess(_onSuccess), onError(_onError) {}

        std::function<void(const std::string&)> onSuccess;
        std::function<void()> onError;
    };

    void finished(const std::string& key, const std::string& content);
    void error(const std::string& key, asio::error_code ec);

    std::mutex mMutex;
    std::string mToken;
    Cache mCache;
    std::unordered_map<std::string, Request> mRequests;
};

#endif // DATABASE_HPP
