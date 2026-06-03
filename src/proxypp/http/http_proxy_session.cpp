#include "http_proxy_session.h"

#include "proxypp/log/log.h"

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/url.hpp>

namespace proxypp::http
{

  HttpProxySession::HttpProxySession(asio::ip::tcp::socket &&client_sock)
      : client_sock_(std::move(client_sock)),
        remote_sock_(client_sock_.get_executor())
  {}

  asio::awaitable<void> HttpProxySession::Run()
  {
    const auto client_req_header = co_await ReadClientRequestHeader();
    if(!client_req_header)
      {
        LOG_HTTP_ERROR("proxy read client header failed");
        Close();
        co_return;
      }

    co_await DoHttpForward(*client_req_header);
  }

  asio::awaitable<void>
  HttpProxySession::DoHttpForward(const RequestHeader &client_req_header)
  {
    const auto remote_info = ParseRemoteInfo(client_req_header);
    if(!remote_info)
      {
        Close();
        co_return;
      }

    const auto resolved_endpoints = co_await ResolveRemote(*remote_info);
    if(!resolved_endpoints)
      {
        Close();
        co_return;
      }

    LOG_HTTP_DEBUG("resolve endpoints success, {} endpoints found",
                   resolved_endpoints->size());

    if(!co_await ConnectRemote(*resolved_endpoints))
      {
        Close();
        co_return;
      }

    LOG_HTTP_DEBUG("connect remote success");

    const auto remote_req
      = BuildRemoteRequest(client_req_header, *remote_info);

    if(!co_await WriteRemoteRequest(remote_req))
      {
        Close();
        co_return;
      }

    LOG_HTTP_DEBUG("proxy write request to remote success");

    co_await StartRelay();
  }

  asio::awaitable<std::optional<HttpProxySession::RequestHeader>>
  HttpProxySession::ReadClientRequestHeader()
  {
    http_::request_parser<http_::empty_body> parser;
    const auto [ec, bytes_read] = co_await http_::async_read_header(
      client_sock_, client_read_buffer_, parser,
      asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        LOG_HTTP_ERROR("proxy read client request header failed, {}",
                       ec.message());
        co_return std::nullopt;
      }
    RequestHeader header = parser.get();
    co_return header;
  }

  std::optional<HttpProxySession::RemoteInfo>
  HttpProxySession::ParseRemoteInfo(const RequestHeader &client_req_header)
  {
    RemoteInfo remote_info;
    if(auto absolute_form_result
       = boost::urls::parse_absolute_uri(client_req_header.target()))
      {
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
        if(remote_info.port.empty()) { remote_info.port = "80"; }

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
        return remote_info;
      }
    if(auto origin_form_result
       = boost::urls::parse_origin_form(client_req_header.target()))
      {
        LOG_HTTP_ERROR("origin form not implement");
        return std::nullopt;
      }
    if(auto authority_form_result
       = boost::urls::parse_authority(client_req_header.target()))
      {
        LOG_HTTP_ERROR("authority form not implement");
        return std::nullopt;
      }
    if(client_req_header.target() == "*")
      {
        LOG_HTTP_ERROR("* form not implement");
        return std::nullopt;
      }
    return std::nullopt;
  }

  asio::awaitable<std::optional<tcp::resolver::results_type>>
  HttpProxySession::ResolveRemote(const RemoteInfo &remote_info)
  {
    tcp::resolver resolver(client_sock_.get_executor());
    const auto [ec, resolved_endpoints] = co_await resolver.async_resolve(
      remote_info.host, remote_info.port, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        LOG_HTTP_ERROR("resolve {}:{} failed, {}", remote_info.host,
                       remote_info.port, ec.message());
        co_return std::nullopt;
      }
    co_return resolved_endpoints;
  }

  asio::awaitable<bool> HttpProxySession::ConnectRemote(
    const tcp::resolver::results_type &resolved_endpoints)
  {
    if(resolved_endpoints.empty())
      {
        LOG_HTTP_ERROR("no endpoints found to connect");
        co_return false;
      }
    auto [ec, endpoint] = co_await remote_sock_.async_connect(
      resolved_endpoints, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        LOG_HTTP_ERROR("connect to remote failed, {}", ec.message());
        co_return false;
      }
    co_return true;
  }

  HttpProxySession::EmptyBodyRequest
  HttpProxySession::BuildRemoteRequest(const RequestHeader &client_req_header,
                                       const RemoteInfo &remote_info)
  {
    EmptyBodyRequest remote_req;
    remote_req.method(client_req_header.method());
    remote_req.version(client_req_header.version());
    remote_req.target(remote_info.forward_target);
    for(auto &field : client_req_header)
      {
        remote_req.set(field.name(), field.value());
      }

    // remove proxy related headers
    remote_req.erase(http_::field::proxy_connection);
    remote_req.erase(http_::field::proxy_authorization);
    remote_req.erase(http_::field::proxy_authenticate);

    // other headers to remove, just for now
    remote_req.erase(http_::field::connection);
    remote_req.erase(http_::field::keep_alive);
    remote_req.erase(http_::field::te);
    remote_req.erase(http_::field::trailer);
    remote_req.erase(http_::field::transfer_encoding);
    remote_req.erase(http_::field::upgrade);

    remote_req.keep_alive(false);
    return remote_req;
  }

  asio::awaitable<bool>
  HttpProxySession::WriteRemoteRequest(const EmptyBodyRequest &remote_req)
  {
    http_::serializer<true, http_::empty_body> serializer{remote_req};
    auto [ec, bytes_write] = co_await http_::async_write_header(
      remote_sock_, serializer, asio::as_tuple(asio::use_awaitable));
    if(ec)
      {
        LOG_HTTP_ERROR("write request to remote failed, {}", ec.message());
        co_return false;
      }
    co_return true;
  }

  asio::awaitable<void> HttpProxySession::RelayClientToRemote()
  {
    // write remaining bytes in client_read_buffer to remote
    if(client_read_buffer_.size() > 0)
      {
        const auto [ec_write, bytes_write] = co_await asio::async_write(
          remote_sock_, client_read_buffer_.data(),
          asio::as_tuple(asio::use_awaitable));
        client_read_buffer_.consume(bytes_write);
        if(ec_write)
          {
            LOG_HTTP_ERROR("proxy write remaining bytes to remote error, {}",
                           ec_write.message());
            co_return;
          }
      }
    // relay client->proxy->remote
    for(;;)
      {
        const auto [ec_read, bytes_read]
          = co_await client_sock_.async_read_some(
            asio::buffer(forward_buffer_),
            asio::as_tuple(asio::use_awaitable));
        if(ec_read)
          {
            LOG_HTTP_ERROR("proxy read data from client error, {}",
                           ec_read.message());
            break;
          }

        const auto [ec_write, bytes_write] = co_await asio::async_write(
          remote_sock_, asio::buffer(forward_buffer_, bytes_read),
          asio::as_tuple(asio::use_awaitable));
        if(ec_write)
          {
            LOG_HTTP_ERROR("proxy write data to remote error, {}",
                           ec_write.message());
            break;
          }
      }
  }

  asio::awaitable<void> HttpProxySession::RelayRemoteToClient()
  {
    for(;;)
      {
        const auto [ec_read, bytes_read]
          = co_await remote_sock_.async_read_some(
            asio::buffer(backward_buffer_),
            asio::as_tuple(asio::use_awaitable));
        if(ec_read)
          {
            LOG_HTTP_ERROR("proxy read data from remote error, {}",
                           ec_read.message());
            break;
          }

        const auto [ec_write, bytes_write] = co_await asio::async_write(
          client_sock_, asio::buffer(backward_buffer_, bytes_read),
          asio::as_tuple(asio::use_awaitable));
        if(ec_write)
          {
            LOG_HTTP_ERROR("proxy write data to client error, {}",
                           ec_write.message());
            break;
          }
      }
  }

  asio::awaitable<void> HttpProxySession::StartRelay()
  {
    // auto self = shared_from_this();
    // asio::co_spawn(
    //   client_sock_.get_executor(),
    //   [self]() { return self->RelayClientToRemote(); }, asio::detached);
    //
    // co_await RelayRemoteToClient();

    using namespace asio::experimental::awaitable_operators;
    co_await (RelayClientToRemote() && RelayRemoteToClient());
    Close();
  }

  void HttpProxySession::Close()
  {
    boost::system::error_code ignored;
    client_sock_.socket().shutdown(tcp::socket::shutdown_both, ignored);
    client_sock_.socket().close();

    remote_sock_.socket().shutdown(tcp::socket::shutdown_both, ignored);
    remote_sock_.socket().close();

    LOG_HTTP_DEBUG("close http proxy session");
  }

} // namespace proxypp::http
