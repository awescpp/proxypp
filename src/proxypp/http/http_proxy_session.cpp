/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_proxy_session.h"
#include "http_message_utils.h"
#include "proxypp/log/log.h"
#include "proxypp/rule/http/apply.h"
#include <boost/url.hpp>
#include <magic_enum/magic_enum.hpp>

namespace
{
  const auto logger = proxypp::log::Get(proxypp::log::Module::http);
}

namespace proxypp::http
{
  HttpProxySession::HttpProxySession(
    SessionId session_id, asio::ip::tcp::socket&& client_sock,
    std::shared_ptr<rule::RuleEngine> rule_engine,
    std::optional<rule::http::Config> rule_http_config)
      : client_sock_ { std::move(client_sock) },
        remote_sock_ { client_sock_.get_executor() },
        rule_engine_ { std::move(rule_engine) },
        rule_http_config_ { std::move(rule_http_config) },
        session_id_(session_id)
  {}

  asio::awaitable<void> HttpProxySession::Run()
  {
    logger->debug("HTTP proxy session started: client=\"{}\", session_id={}",
                  GetRemoteEndpointStr(client_sock_.socket()), session_id_);

    while(co_await HandleOneExchange() == ExchangeResult::Continue)
      {
      }

    Close();

    logger->debug("HTTP proxy session stopped: session_id={}", session_id_);
  }

  asio::awaitable<HttpProxySession::ExchangeResult>
  HttpProxySession::HandleOneExchange()
  {
    logger->debug("HTTP exchange started: session_id={}", session_id_);

    auto LogExchangeCompleted = [this](ExchangeResult result) {
      logger->debug("HTTP exchange completed: result={}, session_id={}",
                    magic_enum::enum_name(result), session_id_);
    };

    RequestParser client_request_parser;

    auto client_request_header
      = co_await ReadClientRequestHeader(client_request_parser);
    if(!client_request_header.has_value())
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    // handle HTTPS tunnel proxy
    if(client_request_header->method() == http_::verb::connect)
      {
        co_return co_await HandleConnectExchange(*client_request_header);
      }

    auto remote_info = ParseRemoteInfo(*client_request_header);
    if(!remote_info.has_value())
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    if(!co_await EnsureRemoteConnected(*remote_info))
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    // rewrite remote_request_header
    auto remote_request_header
      = BuildRemoteRequestHeader(*client_request_header, *remote_info);

    auto request_adapter = adapter::BeastRequestAdapter(remote_request_header);

    if(rule_http_config_.has_value())
      {
        auto apply_request_result = rule::http::ApplyRequest(
          *rule_engine_, *rule_http_config_, request_adapter);
        if(!apply_request_result)
          {
            logger->error(
              R"(failed to apply rules to HTTP request: error="{}", session_id={})",
              apply_request_result.error().message(), session_id_);
          }
        else
          {
            logger->debug(
              "HTTP request rule processing completed: session_id={}",
              session_id_);
          }
      }
    else
      {
        logger->trace(
          R"(HTTP request rule processing skipped: reason="no rules configured", session_id={})",
          session_id_);
      }

    if(!co_await WriteRemoteRequestHeader(remote_request_header))
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    if(!co_await ForwardRequestBody(client_request_parser))
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    ResponseParser remote_response_parser;

    auto remote_response_header
      = co_await ReadRemoteResponseHeader(remote_response_parser);

    if(!remote_response_header.has_value())
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    auto response_adapter
      = adapter::BeastResponseAdapter(*remote_response_header);
    if(rule_http_config_.has_value())
      {
        auto apply_response_result
          = rule::http::ApplyResponse(*rule_engine_, *rule_http_config_,
                                      request_adapter, response_adapter);
        if(!apply_response_result)
          {
            logger->error(
              R"(failed to apply rules to HTTP response: error="{}", session_id={})",
              apply_response_result.error().message(), session_id_);
          }
        else
          {
            logger->debug(
              "HTTP response rule processing completed: session_id={}",
              session_id_);
          }
      }

    if(!co_await WriteRemoteResponseHeaderToClient(*remote_response_header))
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    if(!co_await ForwardResponseBody(client_request_parser,
                                     remote_response_parser))
      {
        LogExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    if(ShouldKeepAlive(client_request_parser, remote_response_parser))
      {
        LogExchangeCompleted(ExchangeResult::Continue);
        co_return ExchangeResult::Continue;
      }

    LogExchangeCompleted(ExchangeResult::Close);
    co_return ExchangeResult::Close;
  }

  // region HTTPS tunnel proxy related

  asio::awaitable<HttpProxySession::ExchangeResult>
  HttpProxySession::HandleConnectExchange(const RequestHeader& request_header)
  {
    logger->debug("HTTP CONNECT exchange started: session_id={}", session_id_);

    auto LogConnectExchangeCompleted = [this](ExchangeResult result) {
      logger->debug(
        "HTTP CONNECT exchange completed: result={}, session_id={}",
        magic_enum::enum_name(result), session_id_);
    };

    const auto remote_info = ParseRemoteInfo(request_header);
    if(!remote_info.has_value())
      {
        LogConnectExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }
    if(!co_await EnsureRemoteConnected(*remote_info))
      {
        LogConnectExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }
    if(!co_await WriteConnectEstablishedResponseToClient())
      {
        LogConnectExchangeCompleted(ExchangeResult::Close);
        co_return ExchangeResult::Close;
      }

    co_await RelayTunnelBidirectional();
    LogConnectExchangeCompleted(ExchangeResult::Close);
    co_return ExchangeResult::Close;
  }

  asio::awaitable<bool>
  HttpProxySession::WriteConnectEstablishedResponseToClient()
  {
    // According to the HTTP specification, when forwarding HTTPS traffic,
    // if the proxy successfully connects to the remote server, it should
    // directly send an HTTP header back to the client
    // indicating that the remote connection has been established and
    // that encrypted data can now be sent.
    // The proxy does not need to wait for a response from the remote server.
    EmptyBodyResponse response;
    response.result(http_::status::ok);
    response.version(11);
    response.reason("Connection Established");
    response.set(http_::field::proxy_connection, "keep-alive");
    http_::response_serializer<http_::empty_body> serializer { response };
    const auto [ec, bytes_written] = co_await http_::async_write(
      client_sock_, serializer, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        logger->error(
          R"(failed to write CONNECT established response to client: code={}, error="{}", session_id={})",
          ec.value(), ec.message(), session_id_);

        co_return false;
      }

    logger->debug(
      "CONNECT established response written to client: session_id={}",
      session_id_);

    co_return true;
  }

  asio::awaitable<void> HttpProxySession::ClientToRemoteTunnel()
  {
    co_await ForwardViaTunnel(client_read_buffer_,
                              { .name = "client", .sock = client_sock_ },
                              { .name = "remote", .sock = remote_sock_ });
  }

  asio::awaitable<void> HttpProxySession::RemoteToClientTunnel()
  {
    co_await ForwardViaTunnel(remote_read_buffer_,
                              { .name = "remote", .sock = remote_sock_ },
                              { .name = "client", .sock = client_sock_ });
  }

  asio::awaitable<void>
  HttpProxySession::ForwardViaTunnel(beast::flat_buffer& read_buffer,
                                     ForwardPeer from_peer,
                                     ForwardPeer target_peer)
  {
    for(;;)
      {
        if(read_buffer.size() == 0)
          {
            const auto read_result
              = co_await ReadSomeFromPeer(read_buffer, from_peer);
            if(!read_result.has_value())
              {
                break;
              }
            if(read_result->eof)
              {
                break;
              }
          }
        const auto bytes_to_write = read_buffer.size();
        const auto bytes_written
          = co_await WriteToPeer(read_buffer, target_peer, bytes_to_write);
        if(!bytes_written.has_value())
          {
            break;
          }
      }

    logger->trace("tunnel forwarding stopped: from={}, to={}, session_id={}",
                  from_peer.name, target_peer.name, session_id_);

    Close();
  }

  asio::awaitable<void> HttpProxySession::RelayTunnelBidirectional()
  {
    auto self = shared_from_this();
    auto ex = co_await asio::this_coro::executor;
    asio::co_spawn(
      ex,
      [self]() -> asio::awaitable<void> {
        co_await self->ClientToRemoteTunnel();
      },
      asio::detached);
    co_await self->RemoteToClientTunnel();
  }

  // endregion

  asio::awaitable<std::optional<HttpProxySession::RequestHeader>>
  HttpProxySession::ReadClientRequestHeader(
    RequestParser& client_request_parser)
  {
    const auto [ec, bytes_read] = co_await http_::async_read_header(
      client_sock_, client_read_buffer_, client_request_parser,
      asio::as_tuple(asio::use_awaitable));

    if(ec)
      {
        if(ec == asio::error::eof || ec == beast::http::error::end_of_stream)
          {
            logger->debug("client connection closed while waiting for request "
                          "header: session_id={}",
                          session_id_);
          }
        else
          {
            logger->error(
              R"(failed to read client request header: code={}, error="{}", session_id={})",
              ec.value(), ec.message(), session_id_);
          }
        co_return std::nullopt;
      }

    logger->debug("client request header read: bytes_read={}, session_id={}",
                  bytes_read, session_id_);

    co_return client_request_parser.get().base();
  }

  std::optional<HttpProxySession::RemoteInfo>
  HttpProxySession::ParseRemoteInfo(const RequestHeader& client_request_header)
  {
    if(client_request_header.method() == http_::verb::connect)
      {
        return ParseConnectRemoteInfo(client_request_header);
      }
    return ParseHttpRemoteInfo(client_request_header);
  }

  std::optional<HttpProxySession::RemoteInfo>
  HttpProxySession::ParseConnectRemoteInfo(
    const RequestHeader& client_request_header)
  {
    const auto target = client_request_header.target();

    auto authority_result = boost::urls::parse_authority(target);
    if(!authority_result)
      {
        return std::nullopt;
      }

    if(authority_result->host().empty())
      {
        return std::nullopt;
      }

    if(authority_result->port().empty())
      {
        return std::nullopt;
      }

    RemoteInfo remote_info;
    remote_info.scheme = "tunnel";
    remote_info.host = authority_result->host();
    remote_info.port = authority_result->port();
    remote_info.forward_target.clear();

    return remote_info;
  }

  std::optional<HttpProxySession::RemoteInfo>
  HttpProxySession::ParseHttpRemoteInfo(
    const RequestHeader& client_request_header)
  {
    const auto target = client_request_header.target();

    logger->trace(
      R"(resolving remote target from client request header: target="{}", session_id={})",
      target, session_id_);

    auto LogFailed = [this, &target](std::string_view reason) {
      logger->error(
        R"(failed to resolve remote target from client request header: target="{}", error="{}", session_id={})",
        target, reason, session_id_);
    };

    auto LogSuccess = [this](const RemoteInfo& remote_info) {
      logger->debug(
        R"(resolved remote target from client request header: scheme={}, host="{}", port={}, forward_target="{}", session_id={})",
        remote_info.scheme, remote_info.host, remote_info.port,
        remote_info.forward_target, session_id_);
    };

    if(auto absolute_form = boost::urls::parse_absolute_uri(target))
      {
        logger->trace("absolute-form request target detected: session_id={}",
                      session_id_);
        RemoteInfo remote_info;
        if(absolute_form->scheme() != "http")
          {
            LogFailed("unsupported scheme");
            return std::nullopt;
          }
        // FIXME: remove this later
        if(absolute_form->encoded_target().empty())
          {
            LogFailed("target is empty");
            return std::nullopt;
          }
        if(absolute_form->host().empty())
          {
            LogFailed("host is empty");
            return std::nullopt;
          }

        remote_info.scheme = absolute_form->scheme();
        remote_info.host = absolute_form->host();
        remote_info.port = absolute_form->port();
        if(remote_info.port.empty())
          {
            remote_info.port = "80";
          }

        remote_info.forward_target = absolute_form->encoded_path();
        if(remote_info.forward_target.empty())
          {
            remote_info.forward_target = "/";
          }
        if(absolute_form->has_query())
          {
            remote_info.forward_target += '?';
            remote_info.forward_target += absolute_form->encoded_query();
          }

        LogSuccess(remote_info);
        return remote_info;
      }
    if(auto origin_form = boost::urls::parse_origin_form(target))
      {
        logger->trace("origin-form request target detected: session_id={}",
                      session_id_);
        LogFailed("origin-form is not supported");
        return std::nullopt;
      }
    if(auto authority_form = boost::urls::parse_authority(target))
      {
        logger->trace("asterisk-form request target detected: session_id={}",
                      session_id_);
        LogFailed("authority-form is not supported");
        return std::nullopt;
      }
    if(target == "*")
      {
        logger->trace("asterisk-form request target detected");
        LogFailed("asterisk-form is not supported");
        return std::nullopt;
      }

    LogFailed("unrecognised request-target form");

    return std::nullopt;
  }

  HttpProxySession::RequestHeader HttpProxySession::BuildRemoteRequestHeader(
    const RequestHeader& client_request_header, const RemoteInfo& remote_info)
  {
    RequestHeader remote_request_header;
    remote_request_header.method(client_request_header.method());
    remote_request_header.version(client_request_header.version());
    remote_request_header.target(remote_info.forward_target);

    for(const auto& field : client_request_header)
      {
        remote_request_header.insert(field.name_string(), field.value());
      }

    constexpr std::array fields_to_erase
      = { http_::field::proxy_connection, http_::field::proxy_authorization,
          http_::field::proxy_authenticate };

    for(const auto& field : fields_to_erase)
      {
        if(remote_request_header.erase(field) > 0)
          {
            logger->trace(
              R"(remote request header field removed: field="{}", session_id={})",
              http_::to_string(field), session_id_);
          }
      }

    logger->trace(
      R"(remote request header built: method={}, version={}, target="{}", field_count={}, session_id={})",
      remote_request_header.method_string(), remote_request_header.version(),
      remote_request_header.target(),
      std::distance(remote_request_header.begin(),
                    remote_request_header.end()),
      session_id_);

    // TODO: later
    // remote_request_header.set(http_::field::keep_alive,
    //                           remote_request_header["Connection"]);
    return remote_request_header;
  }

  asio::awaitable<std::optional<tcp::resolver::results_type>>
  HttpProxySession::ResolveRemote(const RemoteInfo& remote_info)
  {
    tcp::resolver resolver { client_sock_.get_executor() };

    const auto [ec, endpoints] = co_await resolver.async_resolve(
      remote_info.host, remote_info.port, asio::as_tuple(asio::use_awaitable));

    if(ec)
      {
        logger->error(
          R"(failed to resolve remote host: scheme="{}", host="{}", port={}, forward_target="{}", code={}, error="{}", session_id={})",
          remote_info.scheme, remote_info.host, remote_info.port,
          remote_info.forward_target, ec.value(), ec.message(), session_id_);
        co_return std::nullopt;
      }
    if(endpoints.empty())
      {
        logger->error(
          R"(remote host resolves to 0 endpoints, scheme="{}", host="{}", port={}, forward_target="{}", session_id={})",
          remote_info.scheme, remote_info.host, remote_info.port,
          remote_info.forward_target, session_id_);
        co_return std::nullopt;
      }

    logger->trace("host resolved: endpoint_count={}, session_id={}",
                  endpoints.size(), session_id_);

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
        logger->error(
          R"(failed to connect remote endpoints: code={}, error="{}", session_id={})",
          ec.value(), ec.message(), session_id_);
        co_return false;
      }
    logger->debug(R"(remote endpoint connected: remote="{}", session_id={})",
                  GetRemoteEndpointStr(remote_sock_.socket()), session_id_);
    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::EnsureRemoteConnected(const RemoteInfo& remote_info)
  {
    if(remote_state_.connected && remote_sock_.socket().is_open()
       && remote_state_.host == remote_info.host
       && remote_state_.port == remote_info.port)
      {
        logger->trace("ensure remote connected: reuse remote connection");
        co_return true;
      }

    CloseRemote();

    const auto endpoints = co_await ResolveRemote(remote_info);
    if(!endpoints.has_value())
      {
        co_return false;
      }

    if(const auto connected = co_await ConnectRemote(*endpoints); !connected)
      {
        co_return false;
      }

    remote_state_.host = remote_info.host;
    remote_state_.port = remote_info.port;
    remote_state_.connected = true;
    logger->trace(
      R"(set remote state: host="{}", port="{}", connected={}, session_id={})",
      remote_state_.host, remote_state_.port, remote_state_.connected,
      session_id_);

    co_return true;
  }

  asio::awaitable<bool> HttpProxySession::WriteRemoteRequestHeader(
    const RequestHeader& remote_request_header)
  {
    http_::request<http_::empty_body> request;
    request.method(remote_request_header.method());
    request.version(remote_request_header.version());
    request.target(remote_request_header.target());
    // cannot use field.name here because field.name is an enum. If
    // encounter a non-existent enum value, it will throw a runtime
    // exception.
    for(const auto& field : remote_request_header)
      {
        request.insert(field.name_string(), field.value());
      }

    http_::request_serializer<http_::empty_body> serializer { request };
    const auto [ec, bytes_written] = co_await http_::async_write_header(
      remote_sock_, serializer, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        logger->error(
          R"(failed to write request header to remote server: code={}, error="{}", session_id={})",
          ec.value(), ec.message(), session_id_);
        co_return false;
      }

    logger->debug("request header written to remote server: bytes_written={}, "
                  "session_id={}",
                  bytes_written, session_id_);

    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardRequestBody(RequestParser& client_request_parser)
  {
    const auto request_body_info = DetermineBodyInfo(client_request_parser);
    switch(request_body_info.framing)
      {
      case RequestBodyFraming::ContentLength:
        co_return co_await ForwardRequestBodyByContentLength(
          request_body_info.content_length);
      case RequestBodyFraming::Chunked:
        co_return co_await ForwardRequestBodyByChunked();
      case RequestBodyFraming::None: co_return true;
      }
  }

  asio::awaitable<bool> HttpProxySession::ForwardRequestBodyByContentLength(
    std::size_t content_length)
  {
    logger->debug("forwarding request body using Content-Length: "
                  "content_length={}, session_id={}",
                  content_length, session_id_);

    const auto forward_result = co_await ForwardExactly(
      client_read_buffer_, { .name = "client", .sock = client_sock_ },
      { .name = "remote", .sock = remote_sock_ }, content_length);

    co_return forward_result;
  }

  asio::awaitable<bool> HttpProxySession::ForwardRequestBodyByChunked()
  {
    logger->debug("forwarding request body using Transfer-Encoding: chunked: "
                  "session_id={}",
                  session_id_);

    const auto forward_result = co_await ForwardChunked(
      client_read_buffer_, { .name = "client", .sock = client_sock_ },
      { .name = "remote", .sock = remote_sock_ });

    co_return forward_result;
  }

  asio::awaitable<std::optional<HttpProxySession::ResponseHeader>>
  HttpProxySession::ReadRemoteResponseHeader(
    ResponseParser& remote_response_parser)
  {
    // Note: `remote_read_buffer_` may contain not only the
    // `remote_response_header`, but also some remaining bytes of the response
    // body.
    const auto [ec, bytes_read] = co_await http_::async_read_header(
      remote_sock_, remote_read_buffer_, remote_response_parser,
      asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        logger->error(
          R"(failed to ready response header from remote server: code={}, error="{}", session_id={})",
          ec.value(), ec.message(), session_id_);

        co_return std::nullopt;
      }

    logger->debug("remote response header read: bytes_read={}, session_id={}",
                  bytes_read, session_id_);

    co_return remote_response_parser.get().base();
  }

  asio::awaitable<bool> HttpProxySession::WriteRemoteResponseHeaderToClient(
    const ResponseHeader& remote_response_header)
  {
    http_::response<http_::buffer_body> response;
    response.version(remote_response_header.version());
    response.result(remote_response_header.result());
    response.reason(remote_response_header.reason());

    for(const auto& field : remote_response_header)
      {
        response.insert(field.name_string(), field.value());
      }

    logger->trace(
      R"(response header built for client: version={}, status={}, reason="{}", field_count={}, session_id={})",
      response.version(), static_cast<unsigned>(response.result()),
      response.reason(), std::distance(response.begin(), response.end()),
      session_id_);

    http_::response_serializer<http_::buffer_body> serializer { response };
    const auto [ec, bytes_written] = co_await http_::async_write_header(
      client_sock_, serializer, asio::as_tuple(asio::use_awaitable));
    boost::ignore_unused(bytes_written);
    if(ec)
      {
        logger->error(
          R"(failed to write response header to client: code={}, error="{}", session_id={})",
          ec.value(), ec.message(), session_id_);
        co_return false;
      }

    logger->debug(
      "response header written to client: bytes_written={}, session_id={}",
      bytes_written, session_id_);

    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardResponseBody(RequestParser& client_request_parser,
                                        ResponseParser& remote_response_parser)
  {
    const auto response_body_info
      = DetermineBodyInfo(client_request_parser, remote_response_parser);
    switch(response_body_info.framing)
      {
      case ResponseBodyFraming::ContentLength:
        co_return co_await ForwardResponseBodyByContentLength(
          response_body_info.content_length);

      case ResponseBodyFraming::Chunked:
        co_return co_await ForwardResponseBodyByChunked(
          remote_response_parser);

      case ResponseBodyFraming::CloseDelimited:
        co_return co_await ForwardResponseBodyByCloseDelimited();

      case ResponseBodyFraming::Tunnel: break;

      case ResponseBodyFraming::None: co_return true;
      }

    co_return false;
  }

  asio::awaitable<bool> HttpProxySession::ForwardResponseBodyByContentLength(
    std::size_t content_length)
  {
    logger->debug("forwarding response body using Content-Length: "
                  "content_length={}, session_id={}",
                  content_length, session_id_);

    const auto forward_result = co_await ForwardExactly(
      remote_read_buffer_, { .name = "remote", .sock = remote_sock_ },
      { .name = "client", .sock = client_sock_ }, content_length);

    co_return forward_result;
  }

  asio::awaitable<bool> HttpProxySession::ForwardResponseBodyByChunked(
    ResponseParser& response_parser)
  {
    logger->debug("forwarding response body using Transfer-Encoding: chunked: "
                  "session_id={}",
                  session_id_);

    const auto forward_result = co_await ForwardChunked(
      remote_read_buffer_, { .name = "remote", .sock = remote_sock_ },
      { .name = "client", .sock = client_sock_ });

    co_return forward_result;
  }

  asio::awaitable<bool> HttpProxySession::ForwardResponseBodyByCloseDelimited()
  {
    logger->debug("forwarding response body until EOF: session_id={}",
                  session_id_);

    const auto forward_result = co_await ForwardUntilEof(
      remote_read_buffer_, { .name = "remote", .sock = remote_sock_ },
      { .name = "client", .sock = client_sock_ });

    co_return forward_result;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardExactly(beast::flat_buffer& read_buffer,
                                   ForwardPeer from_peer,
                                   ForwardPeer target_peer,
                                   std::size_t content_length)
  {
    std::size_t bytes_sent = 0;
    while(bytes_sent < content_length)
      {
        const auto bytes_remaining = content_length - bytes_sent;

        // write until read_buffer is empty, then read bytes from source to
        // fill read_buffer
        if(read_buffer.size() == 0)
          {
            const auto bytes_read
              = co_await ReadSomeFromPeer(read_buffer, from_peer);
            // TODO: what if bytes_read == 0 ??
            if(!bytes_read.has_value())
              {
                co_return false;
              }
          }

        const auto bytes_to_write
          = std::min<std::size_t>(bytes_remaining, read_buffer.size());

        const auto bytes_written
          = co_await WriteToPeer(read_buffer, target_peer, bytes_to_write);

        if(!bytes_written.has_value())
          {
            co_return false;
          }

        bytes_sent += *bytes_written;
      }

    logger->debug("byte forwarding completed: from={}, to={}, "
                  "bytes_to_forward={}, bytes_forwarded={}, session_id={}",
                  from_peer.name, target_peer.name, content_length, bytes_sent,
                  session_id_);

    co_return true;
  }

  bool HttpProxySession::ShouldKeepAlive(
    const RequestParser& client_request_parser,
    const ResponseParser& remote_response_parser) const
  {
    const auto should_keepalive = client_request_parser.keep_alive()
                                  && remote_response_parser.keep_alive()
                                  && client_sock_.socket().is_open()
                                  && remote_sock_.socket().is_open();

    logger->trace(
      "HTTP keep-alive conditions: client_keep_alive={}, "
      "remote_keep_alive={}, client_open={}, remote_open={}, session_id={}",
      client_request_parser.keep_alive(), remote_response_parser.keep_alive(),
      client_sock_.socket().is_open(), remote_sock_.socket().is_open(),
      session_id_);

    logger->debug("HTTP keep-alive decision: keep_alive={}, session_id={}",
                  should_keepalive, session_id_);

    return should_keepalive;
  }

  asio::awaitable<std::optional<HttpProxySession::ReadSomeResult>>
  HttpProxySession::ReadSomeFromPeer(beast::flat_buffer& buffer,
                                     ForwardPeer from_peer)
  {
    auto mutable_buffer = buffer.prepare(64 * 1024);
    const auto [ec, bytes_read] = co_await from_peer.sock.async_read_some(
      mutable_buffer, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        if(ec == asio::error::eof)
          {
            buffer.commit(bytes_read);

            logger->trace(
              "EOF receive from {}, eof found: bytes_read={}, session_id={}",
              from_peer.name, bytes_read, session_id_);

            co_return ReadSomeResult { .bytes_read = bytes_read, .eof = true };
          }

        if(ec == asio::error::operation_aborted)
          {
            logger->trace("read from {} aborted: bytes_read={}, session_id={}",
                          from_peer.name, bytes_read, session_id_);
            buffer.commit(bytes_read);
            co_return std::nullopt;
          }

        logger->error(
          R"(failed to read bytes from {}: code={}, error="{}", session_id={})",
          from_peer.name, ec.value(), ec.message(), session_id_);

        co_return std::nullopt;
      }
    if(bytes_read == 0)
      {
        logger->trace("read 0 bytes from {}: session_id={}", from_peer.name,
                      session_id_);
        co_return std::nullopt;
      }

    buffer.commit(bytes_read);

    logger->trace("bytes read from {}:  bytes_read={} ,session_id={}",
                  from_peer.name, bytes_read, session_id_);

    co_return ReadSomeResult { .bytes_read = bytes_read, .eof = false };
  }

  asio::awaitable<std::optional<std::size_t>>
  HttpProxySession::WriteToPeer(beast::flat_buffer& buffer,
                                ForwardPeer target_peer,
                                std::size_t bytes_to_write)
  {
    const auto [ec, bytes_written] = co_await asio::async_write(
      target_peer.sock, beast::buffers_prefix(bytes_to_write, buffer.cdata()),
      asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        logger->error(
          R"(failed to write to {}: code={}, error="{}", session_id={})",
          target_peer.name, ec.value(), ec.message(), session_id_);
        co_return std::nullopt;
      }

    if(bytes_written != bytes_to_write)
      {
        logger->error("unexpected number of bytes written to {}: "
                      "bytes_to_write={}, bytes_written={}, session_id={}",
                      target_peer.name, bytes_to_write, bytes_written,
                      session_id_);
        co_return std::nullopt;
      }

    buffer.consume(bytes_written);

    logger->trace("bytes written to {}: bytes_written={}, session_id={}",
                  target_peer.name, bytes_written, session_id_);

    co_return bytes_written;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardChunked(beast::flat_buffer& read_buffer,
                                   ForwardPeer from_peer,
                                   ForwardPeer target_peer)
  {
    auto state = ChunkedState::ReadChunkSizeLine;
    std::size_t chunk_size_line_length = 0;
    std::size_t chunk_size = 0;
    std::size_t chunk_bytes_remaining = 0;

    while(state != ChunkedState::Completed)
      {
        switch(state)
          {
            case ChunkedState::ReadChunkSizeLine: {
              const auto crlf_pos = details::FindCrlf(read_buffer);
              if(crlf_pos.has_value())
                {
                  chunk_size_line_length = *crlf_pos;
                  state = ChunkedState::ParseChunkSizeLine;
                  break;
                }
              const auto read_result
                = co_await ReadSomeFromPeer(read_buffer, from_peer);
              if(!read_result.has_value() || read_result->bytes_read == 0)
                {
                  co_return false;
                }
              break;
            }
            case ChunkedState::ParseChunkSizeLine: {
              const auto line = details::BufferPrefixToString(
                read_buffer, chunk_size_line_length);
              const auto chunk_body_size = details::ParseChunkSizeLine(line);
              if(chunk_body_size.has_value())
                {
                  chunk_size = *chunk_body_size;
                  state = ChunkedState::ForwardChunkSizeLine;
                  break;
                }

              logger->error(
                R"(failed to parse chunk size: line="{}", session_id={})",
                line, session_id_);

              co_return false;
            }
            case ChunkedState::ForwardChunkSizeLine: {
              const auto bytes_to_write = chunk_size_line_length + 2;
              const auto bytes_written = co_await WriteToPeer(
                read_buffer, target_peer, bytes_to_write);
              if(!bytes_written.has_value())
                {
                  co_return false;
                }
              if(chunk_size == 0)
                {
                  state = ChunkedState::ForwardTrailers;
                  break;
                }
              if(chunk_size > std::numeric_limits<std::size_t>::max() - 2)
                {
                  // TODO: how to build a num that larger than std::size_t ?

                  logger->error(
                    R"(overflowed chunk size: chunk_size={}, session_id={})",
                    chunk_size, session_id_);

                  co_return false;
                }
              chunk_bytes_remaining = chunk_size + 2;
              state = ChunkedState::ForwardChunkDataWithCrlf;
              break;
            }
            case ChunkedState::ForwardChunkDataWithCrlf: {
              // read some bytes if no more bytes to write
              if(read_buffer.size() == 0)
                {
                  const auto read_bytes
                    = co_await ReadSomeFromPeer(read_buffer, from_peer);
                  if(!read_bytes.has_value())
                    {
                      co_return false;
                    }
                  break;
                }
              const auto bytes_to_forward = std::min<std::size_t>(
                chunk_bytes_remaining, read_buffer.size());
              const auto bytes_written = co_await WriteToPeer(
                read_buffer, target_peer, bytes_to_forward);
              if(!bytes_written.has_value())
                {
                  co_return false;
                }
              chunk_bytes_remaining -= *bytes_written;
              if(chunk_bytes_remaining == 0)
                {
                  state = ChunkedState::ReadChunkSizeLine;
                }
              break;
            }
            case ChunkedState::ForwardTrailers: {
              // 0\r\n
              // X-Checksum: a1b2c3d4e5f6\r\n  <-- Trailers start here
              // X-Rows-Processed: 42\r\n
              // \r\n
              const auto crlf_pos = details::FindCrlf(read_buffer);
              if(!crlf_pos.has_value())
                {
                  const auto bytes_read
                    = co_await ReadSomeFromPeer(read_buffer, from_peer);
                  if(!bytes_read.has_value())
                    {
                      co_return false;
                    }
                  break;
                }

              const auto trailer_line_length = *crlf_pos;

              const auto bytes_written = co_await WriteToPeer(
                read_buffer, target_peer, trailer_line_length + 2);

              if(!bytes_written.has_value())
                {
                  co_return false;
                }

              if(trailer_line_length == 0)
                {
                  state = ChunkedState::Completed;
                }
              break;
            }
            case ChunkedState::Completed: {
              break;
            }
          }
      }

    logger->debug("chunked transfer completed: from={}, to={},  session_id={}",
                  from_peer.name, target_peer.name, session_id_);

    co_return true;
  }

  asio::awaitable<bool>
  HttpProxySession::ForwardUntilEof(beast::flat_buffer& read_buffer,
                                    ForwardPeer from_peer,
                                    ForwardPeer target_peer)
  {
    // drain the read_buffer first
    if(read_buffer.size() > 0)
      {
        const auto bytes_written
          = co_await WriteToPeer(read_buffer, target_peer, read_buffer.size());
        if(!bytes_written.has_value())
          {
            co_return false;
          }
      }

    for(;;)
      {
        const auto read_result
          = co_await ReadSomeFromPeer(read_buffer, from_peer);
        if(!read_result.has_value())
          {
            co_return false;
          }

        if(read_result->bytes_read > 0)
          {
            const auto bytes_written = co_await WriteToPeer(
              read_buffer, target_peer, read_result->bytes_read);

            if(!bytes_written.has_value())
              {
                co_return false;
              }
          }

        if(read_result->eof)
          {
            break;
          }
      }
    co_return true;
  }

  std::string HttpProxySession::GetRemoteEndpointStr(const tcp::socket& socket)
  {
    boost::system::error_code ec;
    const auto endpoint = socket.remote_endpoint(ec);
    if(ec)
      {
        logger->warn(
          R"(failed to get remote endpoint: code={}, error="{}", session_id={})",
          ec.value(), ec.message(), session_id_);
        return "unknown";
      }
    return std::format("{}:{}", endpoint.address().to_string(),
                       endpoint.port());
  }

  void
  HttpProxySession::CloseSocket(tcp::socket& socket, std::string_view peer)
  {
    if(!socket.is_open())
      {
        logger->debug(
          R"({} socket close skipped: reason="already closed", session_id={})",
          peer, session_id_);
        return;
      }

    boost::system::error_code ignored;
    auto ec = socket.shutdown(asio::socket_base::shutdown_both, ignored);
    if(ec)
      {
        if(ec == asio::error::not_connected
           || ec == asio::error::operation_aborted
           || ec == asio::error::connection_reset)
          {
            logger->trace(
              R"({} socket shutdown completed with an expected error: code={}, error="{}", session_id={})",
              peer, ec.value(), ec.message(), session_id_);
          }
        else
          {
            logger->warn(
              R"(failed to shutdown {} socket: code={}, error="{}", session_id={})",
              peer, ec.value(), ec.message(), session_id_);
          }
      }

    ec = socket.close(ignored);

    if(ec)
      {
        logger->warn(
          R"(failed to close {} socket: code={}, error="{}", session_id={})",
          peer, ec.value(), ec.message(), session_id_);
      }
    else
      {
        logger->debug("{} socket closed: session_id={}", peer, session_id_);
      }
  }

  void HttpProxySession::CloseRemote()
  {
    CloseSocket(remote_sock_.socket(), "remote");
    ResetRemoteState();
  }

  void HttpProxySession::CloseClient()
  {
    CloseSocket(client_sock_.socket(), "client");
  }

  void HttpProxySession::ResetRemoteState()
  {
    remote_state_.host = "";
    remote_state_.port = "";
    remote_state_.connected = false;
    logger->trace(
      R"(remote state reset: host="{}", port="{}", connected={}, session_id={})",
      remote_state_.host, remote_state_.port, remote_state_.connected,
      session_id_);
  }

  void HttpProxySession::Close()
  {
    if(closed_)
      {
        return;
      }

    closed_ = true;
    CloseRemote();
    CloseClient();
  }

} // namespace proxypp::http
