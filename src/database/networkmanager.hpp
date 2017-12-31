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

#ifndef NETWORKMANAGER_HPP
#define NETWORKMANAGER_HPP

#include <thread>
#include <asio/io_service.hpp>
#include <asio/ssl/context.hpp>
#include <list>
#include "https.hpp"
#include "util/concurrency.hpp"

class NetworkManager
{
public:
    static NetworkManager manager;

    void getHTTPS(const std::string& server, const std::string& path, const std::function<void(const std::string&)>& onSuccess, const std::function<void(asio::error_code ec)>& onError);
    void cancel();

private:
    NetworkManager();
    ~NetworkManager();

    void loop();
    LoaderContext makeContext(const std::function<void(const std::string&)>& onSuccess, const std::function<void(asio::error_code ec)>& onError);

    asio::io_service mIOService;
    std::unique_ptr<asio::io_service::work> mWork;
    std::thread mThread;
    asio::ssl::context mSSLContext;

    LockGuarded<std::list<std::shared_ptr<HTTPS>>> mRequestQueue;
    LockGuarded<unsigned int> mPendingCount;
    std::thread mThreadQueue;
};

#endif // NETWORKMANAGER_HPP
