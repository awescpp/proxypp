#include "http_proxy_session.h"

#include "proxypp/log/log.h"

namespace proxypp::http {

    HttpProxySession::HttpProxySession(asio::ip::tcp::socket &&client_sock) :
        client_sock_(std::move(client_sock)), remote_sock_(client_sock_.get_executor()),
        resolver_(client_sock_.get_executor()) {}

    asio::awaitable<void> HttpProxySession::Run() {

    }

    asio::awaitable<std::optional<HttpProxySession::RequestHeader>> HttpProxySession::ParseClientRequestHeader() {

    }

    std::optional<HttpProxySession::RemoteInfo> HttpProxySession::ParseRemoteInfo(const RequestHeader &) {

    }

    asio::awaitable<std::optional<tcp::resolver::results_type>> HttpProxySession::ResolveRemote(const RemoteInfo &) {

    }

    asio::awaitable<bool> HttpProxySession::ConnectRemote(const tcp::resolver::results_type &) {

    }

    HttpProxySession::RequestHeader HttpProxySession::BuildRemoteRequestHeader(const RequestHeader &,
                                                                               const RemoteInfo &) {

    }

    asio::awaitable<bool> HttpProxySession::WriteRemoteRequestHeader(const RequestHeader &) {

    }

    asio::awaitable<void> HttpProxySession::RelayClientToRemote() {

    }

    asio::awaitable<void> HttpProxySession::RelayRemoteToClient() {

    }

    asio::awaitable<void> HttpProxySession::StartRelay() {

    }

    void HttpProxySession::Close() {
        
    }


} // namespace proxypp::http
