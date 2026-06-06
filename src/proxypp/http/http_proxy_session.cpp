#include "http_proxy_session.h"

#include "proxypp/log/log.h"

#include <boost/url.hpp>

namespace proxypp::http
{
  HttpProxySession::HttpProxySession(asio::ip::tcp::socket&& client_sock)
      : client_sock_{std::move(client_sock)},
        remote_sock_{client_sock_.get_executor()}
  {}

  asio::awaitable<void> HttpProxySession::Run()
  {
    while(co_await HandleOneExchange() == ExchangeResult::Continue)
      {
        LOG_HTTP_DEBUG("keep-alive=true, handle next exchange");
      }
    Close();
  }

  asio::awaitable<HttpProxySession::ExchangeResult>
  HttpProxySession::HandleOneExchange()
  {
    RequestParser client_request_parser;

    auto client_request_header
      = co_await ReadClientRequestHeader(client_request_parser);
    if(!client_request_header.has_value())
      {
        co_return ExchangeResult::Close;
      }

    auto remote_info = ParseRemoteInfo(*client_request_header);
    if(!remote_info.has_value())
      {
        co_return ExchangeResult::Close;
      }

    LOG_HTTP_DEBUG("remote_info constructed");

    if(!co_await EnsureRemoteConnected(*remote_info))
      {
        co_return ExchangeResult::Close;
      }

    // rewrite remote_request_header
    auto remote_request_header
      = BuildRemoteRequestHeader(*client_request_header, *remote_info);

    if(!co_await WriteRemoteRequestHeader(remote_request_header))
      {
        co_return ExchangeResult::Close;
      }

    if(!co_await ForwardRequestBody(client_request_parser))
      {
        co_return ExchangeResult::Close;
      }

    ResponseParser remote_response_parser;

    auto remote_response_header
      = co_await ReadRemoteResponseHeader(remote_response_parser);

    if(!remote_response_header.has_value())
      {
        co_return ExchangeResult::Close;
      }

    if(!co_await WriteRemoteResponseHeaderToClient(*remote_response_header))
      {
        co_return ExchangeResult::Close;
      }

    if(!co_await ForwardResponseBody(remote_response_parser))
      {
        co_return ExchangeResult::Close;
      }

    if(ShouldKeepAlive(client_request_parser, remote_response_parser))
      {
        co_return ExchangeResult::Continue;
      }

    co_return ExchangeResult::Close;
  }

  asio::awaitable<std::optional<HttpProxySession::RequestHeader>>
  HttpProxySession::ReadClientRequestHeader(
    RequestParser& client_request_parser)
  {
    const auto [ec, bytes_read] = co_await http_::async_read_header(
      client_sock_, client_read_buffer_, client_request_parser,
      asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        LOG_HTTP_ERROR("read client request header error, {}", ec.message());
        co_return std::nullopt;
      }

    LOG_HTTP_DEBUG("read client request header, {} bytes", bytes_read);

    co_return client_request_parser.get().base();
  }

  std::optional<HttpProxySession::RemoteInfo>
  HttpProxySession::ParseRemoteInfo(const RequestHeader& client_request_header)
  {
    if(auto absolute_form_result
       = boost::urls::parse_absolute_uri(client_request_header.target()))
      {
        RemoteInfo remote_info;
        if(absolute_form_result->scheme() != "http")
          {
            LOG_HTTP_ERROR("unsupported scheme: {}",
                           absolute_form_result->scheme());
            return std::nullopt;
          }
        if(absolute_form_result->encoded_target().empty())
          {
            LOG_HTTP_ERROR("empty target");
            return std::nullopt;
          }
        if(absolute_form_result->host().empty())
          {
            LOG_HTTP_ERROR("host is empty");
            return std::nullopt;
          }

        remote_info.scheme = absolute_form_result->scheme();
        remote_info.host = absolute_form_result->host();
        remote_info.port = absolute_form_result->port();
        if(remote_info.port.empty())
          {
            remote_info.port = "80";
          }

        remote_info.forward_target = absolute_form_result->encoded_path();
        if(remote_info.forward_target.empty())
          {
            remote_info.forward_target = "/";
          }
        if(absolute_form_result->has_query())
          {
            remote_info.forward_target += "?";
            remote_info.forward_target
              += absolute_form_result->encoded_query();
          }
        LOG_HTTP_DEBUG("parse remote_info");
        return remote_info;
      }
    if(auto origin_form_result
       = boost::urls::parse_origin_form(client_request_header.target()))
      {
        LOG_HTTP_ERROR("origin form not implement");
        return std::nullopt;
      }
    if(auto authority_form_result
       = boost::urls::parse_authority(client_request_header.target()))
      {
        LOG_HTTP_ERROR("authority form not implement");
        return std::nullopt;
      }
    if(client_request_header.target() == "*")
      {
        LOG_HTTP_ERROR("* form not implement");
        return std::nullopt;
      }
    return std::nullopt;
  }

  HttpProxySession::RequestHeader HttpProxySession::BuildRemoteRequestHeader(
    const RequestHeader& client_request_header, const RemoteInfo& remote_info)
  {
    RequestHeader remote_request_header;
    remote_request_header.method(client_request_header.method());
    remote_request_header.version(client_request_header.version());
    remote_request_header.target(remote_info.forward_target);

    for(auto& field : client_request_header)
      {
        remote_request_header.set(field.name(), field.value());
      }
    // erase proxy related headers
    remote_request_header.erase(http_::field::proxy_connection);
    remote_request_header.erase(http_::field::proxy_authorization);
    remote_request_header.erase(http_::field::proxy_authenticate);

    // TODO: later
    // remote_request_header.set(http_::field::keep_alive,
    //                           remote_request_header["Connection"]);
    return remote_request_header;
  }

  asio::awaitable<std::optional<tcp::resolver::results_type>>
  HttpProxySession::ResolveRemote(const RemoteInfo& remote_info)
  {
    tcp::resolver resolver{client_sock_.get_executor()};

    LOG_HTTP_DEBUG("before resolve");

    const auto [ec, endpoints] = co_await resolver.async_resolve(
      remote_info.host, remote_info.port, asio::as_tuple(asio::use_awaitable));

    LOG_HTTP_DEBUG("after resolve");

    if(ec)
      {
        LOG_HTTP_ERROR("resolve {}:{} failed, {}", remote_info.host,
                       remote_info.port, ec.message());
        co_return std::nullopt;
      }
    if(endpoints.empty())
      {
        LOG_HTTP_ERROR("no endpoints resolved for {}:{}", remote_info.host,
                       remote_info.port);
        co_return std::nullopt;
      }

    LOG_HTTP_DEBUG("resolve {}:{}, {} endpoints found", remote_info.host,
                   remote_info.port, endpoints.size());

    co_return endpoints;
  }

  asio::awaitable<bool>
  HttpProxySession::ConnectRemote(const tcp::resolver::results_type& endpoints)
  {
    const auto [ec, sock_ignored] = co_await remote_sock_.async_connect(
      endpoints, asio::as_tuple(asio::use_awaitable));
    boost::ignore_unused(sock_ignored);
    if(ec)
      {
        LOG_HTTP_ERROR("connect remote failed, {}", ec.message());
        co_return false;
      }
    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::EnsureRemoteConnected(const RemoteInfo& remote_info)
  {
    if(remote_state_.connected && remote_sock_.socket().is_open()
       && remote_state_.host == remote_info.host
       && remote_state_.port == remote_info.port)
      {
        LOG_HTTP_DEBUG("reuse remote_info");
        co_return true;
      }

    LOG_HTTP_DEBUG("before close remote_sock");

    CloseRemote();

    LOG_HTTP_DEBUG("...");

    auto endpoints = co_await ResolveRemote(remote_info);
    if(!endpoints.has_value())
      {
        co_return false;
      }

    const auto connected = co_await ConnectRemote(*endpoints);
    if(!connected)
      {
        co_return false;
      }

    remote_state_.host = remote_info.host;
    remote_state_.port = remote_info.port;
    remote_state_.connected = true;

    co_return true;
  }

  asio::awaitable<bool> HttpProxySession::WriteRemoteRequestHeader(
    const RequestHeader& remote_request_header)
  {
    http_::request<http_::empty_body> request;
    request.method(remote_request_header.method());
    request.version(remote_request_header.version());
    request.target(remote_request_header.target());
    for(const auto& field : remote_request_header)
      {
        request.set(field.name(), field.value());
      }

    http_::request_serializer<http_::empty_body> serializer{request};
    const auto [ec, bytes_write] = co_await http_::async_write_header(
      remote_sock_, serializer, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        LOG_HTTP_ERROR("write request header to remote failed, {}",
                       ec.message());
        co_return false;
      }
    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardRequestBody(RequestParser& client_request_parser)
  {
    const auto& client_request = client_request_parser.get();

    const auto has_transfer_encoding
      = client_request.find(http_::field::transfer_encoding)
        != client_request.end();

    if(client_request.chunked())
      {
        LOG_HTTP_ERROR(
          "chunked http request forwarding has not been implemented yet");
        co_return false;
      }

    if(has_transfer_encoding)
      {
        LOG_HTTP_ERROR("unsupported transfer-encoding: {}",
                       client_request[http_::field::transfer_encoding]);
        co_return false;
      }

    if(client_request.has_content_length())
      {
        const auto content_length = client_request_parser.content_length();
        if(!content_length.has_value())
          {
            co_return true;
          }
        co_return co_await ForwardRequestBodyByContentLength(*content_length);
      }

    co_return true;
  }

  asio::awaitable<bool> HttpProxySession::ForwardRequestBodyByContentLength(
    std::size_t content_length)
  {
    co_return co_await ForwardExactly(
      client_read_buffer_, forward_buffer_, {"client", client_sock_},
      {"remote", remote_sock_}, content_length);
  }

  asio::awaitable<bool> HttpProxySession::ForwardRequestBodyByChunked()
  {
    // TODO: implement later
    co_return false;
  }

  asio::awaitable<std::optional<HttpProxySession::ResponseHeader>>
  HttpProxySession::ReadRemoteResponseHeader(
    ResponseParser& remote_response_parser)
  {
    // Note: `remote_read_buffer_` may contain not only the
    // `remote_response_header`, but also some remaining bytes of the response
    // body.
    const auto [ec_read, bytes_read] = co_await http_::async_read_header(
      remote_sock_, remote_read_buffer_, remote_response_parser,
      asio::as_tuple(asio::use_awaitable));
    if(ec_read)
      {
        LOG_HTTP_ERROR("read remote response header failed, {}",
                       ec_read.message());
        co_return std::nullopt;
      }

    LOG_HTTP_DEBUG("read remote response header, {} bytes", bytes_read);

    co_return remote_response_parser.get().base();
  }

  asio::awaitable<bool> HttpProxySession::WriteRemoteResponseHeaderToClient(
    const ResponseHeader& remote_response_header)
  {
    http_::response<http_::buffer_body> response;
    response.version(remote_response_header.version());
    response.result(remote_response_header.result());
    response.reason(remote_response_header.reason());

    for(auto& field : remote_response_header)
      {
        if(field.name() != http_::field::unknown)
          {
            response.set(field.name(), field.value());
          }
        else
          {
            response.set(field.name_string(), field.value());
          }
      }

    http_::response_serializer<http_::buffer_body> serializer{response};
    const auto [ec_write, bytes_written] = co_await http_::async_write_header(
      client_sock_, serializer, asio::as_tuple(asio::use_awaitable));
    boost::ignore_unused(bytes_written);
    if(ec_write)
      {
        LOG_HTTP_ERROR("write response header to client failed, {}",
                       ec_write.message());
        co_return false;
      }
    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardResponseBody(ResponseParser& remote_response_parser)
  {
    const auto& remote_response = remote_response_parser.get();
    if(remote_response.chunked())
      {
        LOG_HTTP_ERROR(
          "chunked http response forwarding has not been implemented yet");
        co_return false;
      }

    const auto has_transfer_encoding
      = remote_response.find(http_::field::transfer_encoding)
        != remote_response.end();

    if(has_transfer_encoding)
      {
        LOG_HTTP_ERROR("unsupported transfer-encoding: {}",
                       remote_response[http_::field::transfer_encoding]);
        co_return false;
      }

    const auto content_length = remote_response_parser.content_length();
    if(!content_length.has_value())
      {
        LOG_HTTP_ERROR("response body length is unknown, treat as no body in "
                       "current version");
        co_return true;
      }

    co_return co_await ForwardResponseBodyByContentLength(*content_length);
  }

  asio::awaitable<bool> HttpProxySession::ForwardResponseBodyByContentLength(
    std::size_t content_length)
  {
    co_return co_await ForwardExactly(
      remote_read_buffer_, backward_buffer_, {"remote", remote_sock_},
      {"client", client_sock_}, content_length);
  }

  asio::awaitable<bool> HttpProxySession::ForwardResponseBodyByChunked(
    ResponseParser& response_parser)
  {
    co_return false;
  }

  asio::awaitable<bool> HttpProxySession::ForwardExactly(
    beast::flat_buffer& header_buffer, std::vector<std::byte>& body_buffer,
    ForwardPeer from_peer, ForwardPeer target_peer, std::size_t content_length)
  {
    std::size_t bytes_sent = 0;
    while(bytes_sent < content_length)
      {
        const auto bytes_remaining = content_length - bytes_sent;
        // drain the buffered body first
        if(header_buffer.size() > 0)
          {
            const auto bytes_to_write
              = std::min<std::size_t>(bytes_remaining, header_buffer.size());
            const auto [ec_write, bytes_written] = co_await asio::async_write(
              target_peer.sock,
              // `read_buffer` is not a contiguous buffer, so
              // `buffers_prefix` is used here to extract the first
              // `bytes_to_write` bytes.
              beast::buffers_prefix(bytes_to_write, header_buffer.data()),
              asio::as_tuple(asio::use_awaitable));
            if(ec_write)
              {
                LOG_HTTP_ERROR("write buffered body to {} failed, {}",
                               target_peer.name, ec_write.message());
                co_return false;
              }
            if(bytes_to_write != bytes_written)
              {
                LOG_HTTP_ERROR("write body to {} incomplete",
                               target_peer.name);
                co_return false;
              }
            header_buffer.consume(bytes_written);
            bytes_sent += bytes_written;
          }
        else
          {
            // read request body from client
            const auto bytes_to_read
              = std::min<std::size_t>(bytes_remaining, body_buffer.size());
            const auto [ec_read, bytes_read]
              = co_await from_peer.sock.async_read_some(
                asio::buffer(body_buffer, bytes_to_read),
                asio::as_tuple(asio::use_awaitable));
            if(ec_read)
              {
                LOG_HTTP_ERROR("read request body from {} failed, {}",
                               from_peer.name, ec_read.message());
                co_return false;
              }
            if(bytes_read == 0)
              {
                LOG_HTTP_ERROR("read 0 bytes from {}, {} connection "
                               "may closed prematurely",
                               from_peer.name, from_peer.name);
                co_return false;
              }

            const auto [ec_write, bytes_written] = co_await asio::async_write(
              target_peer.sock, asio::buffer(body_buffer, bytes_read),
              asio::as_tuple(asio::use_awaitable));
            if(ec_write)
              {
                LOG_HTTP_ERROR("write body to {} failed, {}", target_peer.name,
                               ec_write.message());
                co_return false;
              }
            bytes_sent += bytes_written;
          }
      }

    co_return true;
  }

  bool HttpProxySession::ShouldKeepAlive(
    const RequestParser& client_request_parser,
    const ResponseParser& remote_response_parser) const
  {
    return client_request_parser.keep_alive()
           && remote_response_parser.keep_alive()
           && client_sock_.socket().is_open()
           && remote_sock_.socket().is_open();
  }

  void HttpProxySession::CloseRemote()
  {
    if(remote_sock_.socket().is_open())
      {
        boost::system::error_code ec;
        remote_sock_.socket().shutdown(asio::socket_base::shutdown_both, ec);
        if(ec)
          {
            LOG_HTTP_ERROR("shutdown remote socket failed, {}", ec.message());
            ec.clear();
          }

        remote_sock_.socket().close(ec);

        if(ec)
          {
            LOG_HTTP_ERROR("close remote socket failed, {}", ec.message());
          }
        else
          {
            LOG_HTTP_DEBUG("close remote socket");
          }
        ResetRemoteState();
      }
  }

  void HttpProxySession::CloseClient()
  {
    if(client_sock_.socket().is_open())
      {
        boost::system::error_code ec;
        client_sock_.socket().shutdown(asio::socket_base::shutdown_both, ec);
        if(ec)
          {
            LOG_HTTP_ERROR("shutdown client socket failed, {}", ec.message());
            ec.clear();
          }

        client_sock_.socket().close(ec);
        if(ec)
          {
            LOG_HTTP_ERROR("close client socket failed, {}", ec.message());
          }
        else
          {
            LOG_HTTP_DEBUG("close client socket");
          }
      }
  }

  void HttpProxySession::ResetRemoteState()
  {
    remote_state_.host = "";
    remote_state_.port = "";
    remote_state_.connected = false;
    LOG_HTTP_DEBUG("reset remote state");
  }
  void HttpProxySession::Close()
  {
    CloseRemote();
    CloseClient();
  }

} // namespace proxypp::http
