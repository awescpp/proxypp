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

    using EmptyBodyRequest = http_::request<http_::empty_body>;
    using EmptyBodyResponse = http_::response<http_::empty_body>;

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

    struct ForwardPeer
    {
      std::string name;
      socket_t& sock;
    };

    enum class ExchangeResult
    {
      Continue,
      Close
    };

    enum class ChunkedState
    {
      ReadChunkSizeLine,
      ParseChunkSizeLine,
      ForwardChunkSizeLine,

      ForwardChunkDataWithCrlf,

      ForwardTrailers,

      Completed
    };

  private:
    asio::awaitable<ExchangeResult> HandleOneExchange();

    asio::awaitable<std::optional<RequestHeader>>
    ReadClientRequestHeader(RequestParser& client_request_parser);

    std::optional<RemoteInfo>
    ParseRemoteInfo(const RequestHeader& client_request_header);

    RequestHeader
    BuildRemoteRequestHeader(const RequestHeader& client_request_header,
                             const RemoteInfo& remote_info);

    asio::awaitable<std::optional<tcp::resolver::results_type>>
    ResolveRemote(const RemoteInfo& remote_info);

    asio::awaitable<bool>
    ConnectRemote(const tcp::resolver::results_type& endpoints);

    asio::awaitable<bool> EnsureRemoteConnected(const RemoteInfo& remote_info);

    asio::awaitable<bool>
    WriteRemoteRequestHeader(const RequestHeader& remote_request_header);

    asio::awaitable<bool>
    ForwardRequestBody(RequestParser& client_request_parser);

    asio::awaitable<bool>
    ForwardRequestBodyByContentLength(std::size_t content_length);

    asio::awaitable<bool> ForwardRequestBodyByChunked();

    asio::awaitable<std::optional<ResponseHeader>>
    ReadRemoteResponseHeader(ResponseParser& remote_response_parser);

    asio::awaitable<bool> WriteRemoteResponseHeaderToClient(
      const ResponseHeader& remote_response_header);

    asio::awaitable<bool>
    ForwardResponseBody(ResponseParser& remote_response_parser);

    asio::awaitable<bool>
    ForwardResponseBodyByContentLength(std::size_t content_length);

    asio::awaitable<bool>
    ForwardResponseBodyByChunked(ResponseParser& response_parser);

    asio::awaitable<bool>
    ForwardExactly(beast::flat_buffer& read_buffer, ForwardPeer from_peer,
                   ForwardPeer target_peer, std::size_t content_length);

    bool ShouldKeepAlive(const RequestParser& client_request_parser,
                         const ResponseParser& remote_response_parser) const;

    asio::awaitable<std::optional<std::size_t>>
    ReadSomeFromPeer(beast::flat_buffer& buffer, ForwardPeer from_peer);

    asio::awaitable<std::optional<std::size_t>>
    WriteToPeer(beast::flat_buffer& buffer, ForwardPeer target_peer,
                std::size_t bytes_to_write);

    asio::awaitable<bool>
    ForwardChunked(beast::flat_buffer& read_buffer, ForwardPeer from_peer,
                   ForwardPeer target_peer);

    void CloseRemote();

    void CloseClient();

    void ResetRemoteState();

    void Close();

  private:
    socket_t client_sock_;
    socket_t remote_sock_;

    beast::flat_buffer client_read_buffer_;
    beast::flat_buffer remote_read_buffer_;

    RemoteConnectionState remote_state_;
  };

} // namespace proxypp::http
