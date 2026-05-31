#pragma once

#include <boost/beast.hpp>
#include "proxypp/common.h"

namespace beast = boost::beast;
namespace http_ = beast::http;
using socket_t = beast::tcp_stream;

namespace proxypp::http {

    class HttpProxySession : public std::enable_shared_from_this<HttpProxySession> {
    public:
        // rvalue reference here means the caller must transfer ownership of the socket.
        explicit HttpProxySession(asio::ip::tcp::socket &&client_sock);

        void Run();

    private:
        asio::awaitable<std::optional<http_::request_header<>>> ParseClientRequestHeader();

        asio::awaitable<std::optional<socket_t>> ConnectRemote();

        asio::awaitable<void> DoSession(std::optional<socket_t>);

        socket_t client_sock_;
    };

} // namespace proxypp::http
