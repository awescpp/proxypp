#include "http_proxy_session.h"

#include "proxypp/log/log.h"

namespace proxypp::http {

    HttpProxySession::HttpProxySession(asio::ip::tcp::socket &&client_sock) : client_sock_(std::move(client_sock)) {}

    void HttpProxySession::Run() {
        auto self = shared_from_this();
        asio::co_spawn(
                client_sock_.get_executor(),
                [self]() mutable -> asio::awaitable<void> {
                    const auto header = co_await self->ParseClientRequestHeader();
                    if (!header.has_value()) {
                        LOG_HTTP_ERROR("parse client request header failed");
                        co_return;
                    }
                    LOG_HTTP_DEBUG("client request: method={} target={} http={}.{} host={} connection={}",
                                   header->method_string(), header->target(), header->version() / 10,
                                   header->version() % 10, (*header)[http_::field::host],
                                   (*header)[http_::field::connection]);
                    if (LOG_HTTP_TRACE_ENABLED()) {
                        std::ostringstream oss;
                        oss << header.value();
                        LOG_HTTP_TRACE("client request header:\n{}", oss.str());
                    }
                },
                asio::detached);
    }

    asio::awaitable<std::optional<http_::request_header<>>> HttpProxySession::ParseClientRequestHeader() {
        http_::request_parser<http_::empty_body> parser;
        beast::flat_buffer buffer;
        const auto [ec, bytes_read] =
                co_await http_::async_read_header(client_sock_, buffer, parser, asio::as_tuple(asio::use_awaitable));
        if (ec) {
            LOG_CORE_ERROR("parse client request header failed, {}", ec.message());
            co_return std::nullopt;
        }
        const http_::request_header<> header = parser.get();
        co_return header;
    }

} // namespace proxypp::http
