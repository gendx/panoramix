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

#ifndef HTTPS_HPP
#define HTTPS_HPP

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/streambuf.hpp>

class HTTPS;

class SSLContext
{
public:
    SSLContext(const std::string& protocol, const std::string& cipher);

private:
    std::string mProtocol;
    std::string mCipher;
};

struct LoaderContext
{
    std::function<void(const std::string&)> onFinish;
    std::function<void(asio::error_code ec)> onError;
    std::function<void()> onUpdate;
    std::function<void(std::shared_ptr<HTTPS>, SSLContext)> onSSLHandshake;
};

class Loader
{
public:
    Loader(const LoaderContext& callbacks);
    virtual ~Loader() = default;

    virtual void launch() = 0;
    virtual void cancel() = 0;

protected:
    LoaderContext mCallbacks;
};

class HTTPS : public Loader, public std::enable_shared_from_this<HTTPS>
{
    friend std::ostream& operator<<(std::ostream& out, const HTTPS& https);

public:
    HTTPS(asio::ssl::context& sslContext, asio::io_service& ioService, const std::string& server, const std::string& path, const LoaderContext& callbacks);

    void launch();
    void cancel();
    void acceptHandshake();

private:
    void handleResolve(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpointIterator);
    void handleConnect(const asio::error_code& ec, asio::ip::tcp::resolver::iterator it);
    void handleHandshake(const asio::error_code& ec);
    void handleWriteRequest(const asio::error_code& ec, std::size_t bytesTransferred);
    void handleReadStatusLine(const asio::error_code& ec, std::size_t bytesTransferred);
    void handleReadHeaders(const asio::error_code& ec, std::size_t bytesTransferred);
    void handleReadContent(const asio::error_code& ec, std::size_t bytesTransferred);

    asio::ip::tcp::resolver mResolver;
    asio::ssl::stream<asio::ip::tcp::socket> mSocket;
    std::string mServer;
    std::string mPath;
    asio::streambuf mRequest;
    asio::streambuf mResponse;
    bool mGzip;
};

#endif // HTTPS_HPP
