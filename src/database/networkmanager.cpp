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

#include "networkmanager.hpp"

#include "config.hpp"
#include <iostream>

NetworkManager NetworkManager::manager;


NetworkManager::NetworkManager() :
    mWork(std::make_unique<asio::io_service::work>(mIOService)),
    mThread([this] {mIOService.run();}),
    mSSLContext(asio::ssl::context::sslv23),
    mPendingCount(0),
    mThreadQueue([this] {this->loop();})
{
    mSSLContext.set_default_verify_paths();
}

NetworkManager::~NetworkManager()
{
    this->cancel();
    mWork.reset();
    mIOService.stop();

    std::cerr << "[*]Waiting for pending count..." << std::endl;
    mPendingCount.wait([](const unsigned int& count) {return count == 0;});
    std::cerr << "[*]Pending count OK" << std::endl;
    mThread.join();
    mThreadQueue.join();
}


void NetworkManager::cancel()
{
    // Abort signal in front of the list.
    std::cerr << "[*]Pushing cancel signal." << std::endl;
    std::list<std::shared_ptr<HTTPS>> queue;
    queue.push_back(nullptr);
    mRequestQueue.swap(queue);
    mRequestQueue.notify_one();

    // Call error function on cancelled requests.
    auto incr_count = [](unsigned int& count) {
        ++count;
        std::cerr << "[+]Pending count = " << count << std::endl;
    };

    for (auto& request : queue)
    {
        if (request)
        {
            mPendingCount.apply(incr_count);
            request->cancel();
        }
    }
}

void NetworkManager::loop()
{
    for (;;)
    {
        mRequestQueue.wait([](const std::list<std::shared_ptr<HTTPS>>& queue) {return !queue.empty();});

        // Take request.
        std::shared_ptr<HTTPS> request;
        auto pop_request = [&request](std::list<std::shared_ptr<HTTPS>>& queue) {
            request = queue.front();
            queue.pop_front();
        };
        mRequestQueue.apply(pop_request);

        // Abort signal.
        if (request == nullptr)
        {
            std::cerr << "[*]Aborting new requests." << std::endl;
            break;
        }

        // Wait for some requests to finish
        mPendingCount.wait([](const unsigned int& count) {return count < MAX_REQUESTS;});
        auto incr_count = [](unsigned int& count) {
            ++count;
            std::cerr << "[+]Pending count = " << count << std::endl;
        };
        mPendingCount.apply(incr_count);
        request->launch();
    }
}

void NetworkManager::getHTTPS(const std::string& server, const std::string& path, const std::function<void(const std::string&)>& onSuccess, const std::function<void(asio::error_code ec)>& onError)
{
    auto push_request = [this, server, path, onSuccess, onError](std::list<std::shared_ptr<HTTPS>>& queue) {
        queue.push_back(std::make_shared<HTTPS>(mSSLContext, mIOService, server, path, this->makeContext(onSuccess, onError)));
    };
    mRequestQueue.apply(push_request);
    mRequestQueue.notify_one();
}


LoaderContext NetworkManager::makeContext(const std::function<void(const std::string&)>& onSuccess, const std::function<void(asio::error_code ec)>& onError)
{
    auto decr_count = [](unsigned int& count) {
        --count;
        std::cerr << "[-]Pending count = " << count << std::endl;
    };
    LoaderContext context;
    context.onFinish = [this, decr_count, onSuccess](const std::string& data) {
        mPendingCount.apply(decr_count);
        mPendingCount.notify_one();
        onSuccess(data);
    };
    context.onError = [this, decr_count, onError](asio::error_code ec)
    {
        if (ec.value() == asio::error::eof)
            std::cerr << "EOF" << std::endl;
        else
            std::cerr << "Fatal error [" << std::hex << ec.value() << std::dec << "]: " << ec.message() << std::endl;
        mPendingCount.apply(decr_count);
        mPendingCount.notify_one();
        onError(ec);
    };
    context.onUpdate = []()
    {
    };
    context.onSSLHandshake = [](std::shared_ptr<HTTPS> https, SSLContext /* sslContext */)
    {
        https->acceptHandshake();
    };

    return context;
}

