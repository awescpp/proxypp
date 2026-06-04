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
    explicit HttpProxySession(asio::ip::tcp::socket&& client_sock);

    asio::awaitable<void> Run();

  private:
    using RequestParser = http_::request_parser<http_::buffer_body>;
    using ResponseParser = http_::response_parser<http_::buffer_body>;

    using RequestHeader = http_::request_header<>;
    using ResponseHeader = http_::response_header<>;

    struct RemoteInfo
    {
      std::string scheme;
      std::string host;
      std::string port;
      std::string forward_target;
    };

    struct RemoteConnectionState
    {
      std::string host;
      std::string port;
      bool connected = false;
    };

    enum class ExchangeResult
    {
      Continue,
      Close
    };

  private:
    asio::awaitable<ExchangeResult> HandleOneExchange();

    asio::awaitable<std::optional<RequestHeader>>
    ReadClientRequestHeader(RequestParser& request_parser);

    std::optional<RemoteInfo>
    ParseRemoteInfo(const RequestHeader& request_header);

    RequestHeader BuildRemoteRequestHeader(const RequestHeader& request_header,
                                           const RemoteInfo& remote_info);

    asio::awaitable<bool> EnsureRemoteConnected(const RemoteInfo& remote_info);

    asio::awaitable<bool>
    WriteRequestHeader(const RequestHeader& request_header);

    asio::awaitable<bool> ForwardRequestBody(RequestParser& request_parser);

    asio::awaitable<std::optional<ResponseHeader>>
    ReadResponseHeader(ResponseParser& response_parser);

    asio::awaitable<bool>
    WriteResponseHeader(const ResponseHeader& remote_response_header);

    asio::awaitable<bool> ForwardResponseBody(ResponseParser& response_parser);

    bool ShouldKeepAlive(const RequestHeader& client_request_header,
                         const ResponseHeader& remote_response_header) const;

    void CloseRemote();

    void Close();

  private:
    socket_t client_sock_;
    socket_t remote_sock_;

    beast::flat_buffer client_read_buffer_;
    beast::flat_buffer remote_read_buffer_;

    std::vector<std::byte> forward_buffer_
      = std::vector<std::byte>(64 * 1024); // client->proxy->remote
    std::vector<std::byte> backward_buffer_
      = std::vector<std::byte>(64 * 1024); // remote->proxy->client

    RemoteConnectionState remote_connection_state_;
  };

} // namespace proxypp::http
