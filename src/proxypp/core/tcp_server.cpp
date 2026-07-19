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

void proxypp::core::TcpServer::Run()
{
  const tcp::endpoint ep { asio::ip::make_address(std::string { address() }),
                           static_cast<asio::ip::port_type>(port()) };

  boost::system::error_code ec;

  acceptor_.open(ep.protocol(), ec);
  if(ec)
    {
      std::cerr << std::format("TcpServer open {}:{} failed", address(),
                               port());
      return;
    }

  acceptor_.set_option(asio::socket_base::reuse_address(true), ec);

  acceptor_.bind(ep, ec);
  if(ec)
    {
      std::cerr << std::format("TcpServer bind {}:{} failed", address(),
                               port());
      return;
    }

  acceptor_.listen(asio::socket_base::max_listen_connections, ec);
  if(ec)
    {
      std::cerr << std::format("TcpServer start listening on {}:{} failed",
                               address(), port());
      return;
    }

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

  LOG_CORE_INFO("proxy++ running on {}:{}", address(), port());
}
