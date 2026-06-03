#pragma once

#include "proxypp/common.h"

#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http_ = beast::http;
using socket_t = beast::tcp_stream;

namespace proxypp::http
{

  class HttpProxySession
      : public std::enable_shared_from_this<HttpProxySession>
  {
  public:
    // rvalue reference here means the caller must transfer ownership of the
    // socket.
    explicit HttpProxySession(asio::ip::tcp::socket &&client_sock);

    asio::awaitable<void> Run();

  private:
    struct RemoteInfo
    {
      std::string scheme;
      std::string host;
      std::string port;
      std::string forward_target;
      bool is_connect = false;
    };

    using RequestHeader = http_::request_header<>;
    using EmptyBodyRequest = http_::request<http_::empty_body>;

    asio::awaitable<void> DoHttpForward(const RequestHeader &);

    asio::awaitable<std::optional<RequestHeader>> ReadClientRequestHeader();

    std::optional<RemoteInfo> ParseRemoteInfo(const RequestHeader &);

    asio::awaitable<std::optional<tcp::resolver::results_type>>
    ResolveRemote(const RemoteInfo &);

    asio::awaitable<bool> ConnectRemote(const tcp::resolver::results_type &);

    EmptyBodyRequest
    BuildRemoteRequest(const RequestHeader &, const RemoteInfo &);

    asio::awaitable<bool> WriteRemoteRequest(const EmptyBodyRequest &);

    asio::awaitable<void> RelayClientToRemote();

    asio::awaitable<void> RelayRemoteToClient();

    asio::awaitable<void> StartRelay();

    void Close();

    socket_t client_sock_;
    socket_t remote_sock_;
    beast::flat_buffer client_read_buffer_;
    std::vector<std::byte> forward_buffer_
      = std::vector<std::byte>(64 * 1024); // client->proxy->remote
    std::vector<std::byte> backward_buffer_
      = std::vector<std::byte>(64 * 1024); // remote->proxy->client
  };

} // namespace proxypp::http
