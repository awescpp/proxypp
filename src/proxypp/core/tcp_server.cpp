/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tcp_server.h"
#include "proxypp/http/http_proxy_session.h"
#include "proxypp/log/log.h"

proxypp::core::TcpServer::TcpServer(asio::any_io_executor ex,
                                    TcpServerOptions options)
    : acceptor_(ex), options_(std::move(options))
{}

proxypp::Result<proxypp::core::ListenEndpoint>
proxypp::core::TcpServer::Start()
{
  const tcp::endpoint endpoint {
    asio::ip::make_address(std::string { options_.address }),
    static_cast<asio::ip::port_type>(options_.port)
  };

  boost::system::error_code ec;

  const auto endpoint_text
    = fmt::format("{}:{}", endpoint.address().to_string(), endpoint.port());

  auto start_failed = [&](std::string_view context) {
    return Unexpected(Error {
      Errc::StartTcpServerFailed,
      fmt::format("{}: {} (code={}, category={})", context, ec.message(),
                  ec.value(), ec.category().name()),
    });
  };

  acceptor_.open(endpoint.protocol(), ec);
  if(ec)
    {
      return start_failed(
        fmt::format("failed to open TCP acceptor for {}", endpoint_text));
    }

  acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
  if(ec)
    {
      return start_failed(
        fmt::format("failed to enable SO_REUSEADDR for TCP acceptor on {}",
                    endpoint_text));
    }

  acceptor_.bind(endpoint, ec);
  if(ec)
    {
      return start_failed(
        fmt::format("failed to bind TCP acceptor to {}", endpoint_text));
    }

  acceptor_.listen(asio::socket_base::max_listen_connections, ec);
  if(ec)
    {
      return start_failed(
        fmt::format("failed to listen on TCP endpoint {}", endpoint_text));
    }

  StartAccept();

  const auto address = acceptor_.local_endpoint().address().to_string();
  const auto port = acceptor_.local_endpoint().port();

  LOG_CORE_INFO("proxy++ running on {}:{}", address, port);

  return ListenEndpoint { .address = address, .port = port };
}

void proxypp::core::TcpServer::StartAccept()
{
  auto self = shared_from_this();

  asio::co_spawn(
    acceptor_.get_executor(),
    [self]() mutable -> asio::awaitable<void> {
      for(;;)
        {
          auto [accept_ec, client_sock]
            = co_await self->acceptor_.async_accept(
              asio::as_tuple(asio::use_awaitable));
          if(accept_ec)
            {
              LOG_CORE_ERROR("accept connection error, {}",
                             accept_ec.message());
              continue;
            }

          LOG_CORE_DEBUG("accept a connection from {}:{}");

          auto http_proxy_session = std::make_shared<http::HttpProxySession>(
            std::move(client_sock), self->rule_engine(),
            self->http_rule_config());

          asio::co_spawn(
            self->acceptor_.get_executor(),
            [http_proxy_session]() -> asio::awaitable<void> {
              co_await http_proxy_session->Run();
            },
            [http_proxy_session](std::exception_ptr e) {
              if(!e)
                {
                  return;
                }
              try
                {
                  std::rethrow_exception(e);
                }
              catch(const boost::system::system_error& ex)
                {
                  LOG_HTTP_ERROR("unhandled system error in HttpProxySession, "
                                 "code={}, message={}",
                                 ex.code().value(), ex.code().message());
                }
              catch(const std::exception& ex)
                {
                  LOG_HTTP_ERROR("unhandled exception in HttpProxySession, {}",
                                 ex.what());
                }
              catch(...)
                {
                  LOG_HTTP_ERROR("unknown exception in HttpProxySession");
                }
            });
        }
    },
    [](std::exception_ptr e) {
      if(!e)
        {
          return;
        }

      try
        {
          std::rethrow_exception(e);
        }
      catch(const std::exception& ex)
        {
          LOG_CORE_ERROR("accept coroutine terminated, {}", ex.what());
        }
      catch(...)
        {
          LOG_CORE_ERROR("accept coroutine terminated by unknown exception");
        }
    });
}
