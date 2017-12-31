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

#include "https.hpp"

#include <asio/ssl.hpp>
#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <asio/read_until.hpp>
#include <asio/read.hpp>
#include "util/gzip.hpp"

#include <iostream>

SSLContext::SSLContext(const std::string& protocol, const std::string& cipher) :
    mProtocol(protocol),
    mCipher(cipher)
{
}


Loader::Loader(const LoaderContext& callbacks) :
    mCallbacks(callbacks)
{
}


std::ostream& operator<<(std::ostream& out, const HTTPS& https)
{
    return out << "{https://" << https.mServer << " ; " << https.mPath << "} ";
}

HTTPS::HTTPS(asio::ssl::context& sslContext, asio::io_service& ioService, const std::string& server, const std::string& path, const LoaderContext& callbacks) :
    Loader(callbacks),
    mResolver(ioService),
    mSocket(ioService, sslContext),
    mServer(server),
    mPath(path)
{
    // SSL mode.
    mSocket.set_verify_mode(asio::ssl::verify_peer);
    mSocket.set_verify_callback(
#ifdef PRINT_CERTIFICATE
        [this](bool preverified, asio::ssl::verify_context& ctx) {
            char subject_name[256];
            X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
            X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
            bool verified = asio::ssl::rfc2818_verification(mServer)(preverified, ctx);
            std::cout << *this << "Verifying: " << subject_name << std::endl;
            std::cout << *this << "Verified: " << verified << std::endl;
            return verified;
        }
#else
        asio::ssl::rfc2818_verification(mServer)
#endif
    );

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&mRequest);
    request_stream << "GET ";
    request_stream << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";
}


void HTTPS::launch()
{
    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    asio::ip::tcp::resolver::query query(mServer, "https");
    auto self(shared_from_this());

    mResolver.async_resolve(query,
        [this, self](const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpointIterator) {this->handleResolve(ec, endpointIterator);}
    );
}

void HTTPS::cancel()
{
    std::cerr << *this << "Cancelling HTTPS..." << std::endl;
    mCallbacks.onError(asio::error_code(asio::error::connection_aborted));
}


void HTTPS::handleResolve(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpointIterator)
{
    if (ec)
    {
        std::cerr << *this << "Resolve error: " << ec.message() << std::endl;
        mCallbacks.onError(ec);
        return;
    }

    // Attempt a connection to each endpoint in the list until we
    // successfully establish a connection.
    auto self(shared_from_this());

    asio::async_connect(mSocket.lowest_layer(), endpointIterator,
        [this](const asio::error_code& ec, asio::ip::tcp::resolver::iterator next)
        {
            if (ec)
                std::cerr << *this << "Error: " << ec.message() << std::endl;
            return next;
        },
        [this, self](const asio::error_code& ec, asio::ip::tcp::resolver::iterator it) {this->handleConnect(ec, it);}
    );
}

void HTTPS::handleConnect(const asio::error_code& ec, asio::ip::tcp::resolver::iterator /* it */)
{
    if (ec)
    {
        std::cerr << *this << "Connect error: " << ec.message() << std::endl;
        mCallbacks.onError(ec);
        return;
    }

    // The connection was successful. Do the handshake.
    auto self(shared_from_this());

    mSocket.async_handshake(asio::ssl::stream_base::client,
        [this, self](const asio::error_code& ec) {this->handleHandshake(ec);}
    );
}

void HTTPS::handleHandshake(const asio::error_code& ec)
{
    if (ec)
    {
        std::cerr << *this << "Handshake error: " << ec.message() << std::endl;
        mCallbacks.onError(ec);
        return;
    }

    const SSL* ssl = mSocket.native_handle();
    std::string protocol = SSL_get_version(ssl);
    const SSL_CIPHER* _cipher = SSL_get_current_cipher(ssl);
    std::string cipher = SSL_CIPHER_get_name(_cipher);

    auto self(shared_from_this());
    mCallbacks.onSSLHandshake(self, SSLContext(protocol, cipher));
}

void HTTPS::acceptHandshake()
{
    // The handshake was successful. Send the request.
    auto self(shared_from_this());

    asio::async_write(mSocket, mRequest,
        [this, self](const asio::error_code& ec, std::size_t bytesTransferred) {this->handleWriteRequest(ec, bytesTransferred);}
    );
}

void HTTPS::handleWriteRequest(const asio::error_code& ec, std::size_t /* bytesTransferred */)
{
    if (ec)
    {
        std::cerr << *this << "Write request error: " << ec.message() << std::endl;
        mCallbacks.onError(ec);
        return;
    }

    // Read the response status line. The response_ streambuf will
    // automatically grow to accommodate the entire line. The growth may be
    // limited by passing a maximum size to the streambuf constructor.
    auto self(shared_from_this());

    asio::async_read_until(mSocket, mResponse, "\r\n",
        [this, self](const asio::error_code& ec, std::size_t bytesTransferred) {this->handleReadStatusLine(ec, bytesTransferred);}
    );
}

void HTTPS::handleReadStatusLine(const asio::error_code& ec, std::size_t /* bytesTransferred */)
{
    if (ec)
    {
        std::cerr << *this << "Read status error: " << ec.message() << std::endl;
        mCallbacks.onError(ec);
        return;
    }

    // Check that response is OK.
    std::istream response_stream(&mResponse);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
        std::cerr << *this << "Invalid response\n";
        return;
    }

    // Read the response headers, which are terminated by a blank line.
    auto self(shared_from_this());

    asio::async_read_until(mSocket, mResponse, "\r\n\r\n",
        [this, self](const asio::error_code& ec, std::size_t bytesTransferred) {this->handleReadHeaders(ec, bytesTransferred);}
    );
}

void HTTPS::handleReadHeaders(const asio::error_code& ec, std::size_t /* bytesTransferred */)
{
    if (ec)
    {
        std::cerr << *this << "Read headers error: " << ec.message() << std::endl;
        mCallbacks.onError(ec);
        return;
    }

    // Process the response headers.
    std::istream response_stream(&mResponse);
    std::string header;
    mGzip = false;
    while (std::getline(response_stream, header) && header != "\r")
    {
        // TODO: improve this check
        if (header == "Content-Encoding: gzip\r")
            mGzip = true;
    }

    // Start reading remaining data until EOF.
    auto self(shared_from_this());

    asio::async_read(mSocket, mResponse,
        asio::transfer_at_least(1),
        [this, self](const asio::error_code& ec, std::size_t bytesTransferred) {this->handleReadContent(ec, bytesTransferred);}
    );
}

void HTTPS::handleReadContent(const asio::error_code& ec, std::size_t /* bytesTransferred */)
{
    if (ec)
    {
        if (ec.value() == asio::error::eof || ec.value() == 0x140000db)
        {
            // Get all of the data.
            std::ostringstream ss;
            ss << &mResponse;
            std::string content = ss.str();

            if (mGzip)
            {
                content = Gzip::decompress(content);
                if (content.empty())
                {
                    mCallbacks.onError(asio::error_code(asio::error::invalid_argument));
                    return;
                }
            }

            mCallbacks.onFinish(content);
        }
        else
        {
            std::cerr << *this << "Read content error: " << ec.message() << std::endl;
            mCallbacks.onError(ec);
        }
        return;
    }

    // TODO: send a progress update?
    mCallbacks.onUpdate();

    // Continue reading remaining data until EOF.
    auto self(shared_from_this());

    asio::async_read(mSocket, mResponse,
        asio::transfer_at_least(1),
        [this, self](const asio::error_code& ec, std::size_t bytesTransferred) {this->handleReadContent(ec, bytesTransferred);}
    );
}
