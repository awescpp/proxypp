/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tcp_server.h"
#include "proxypp/http/http_proxy_session.h"
#include "proxypp/log/log.h"

namespace
{
  const auto logger = proxypp::log::Get(proxypp::log::Module::core);
}

proxypp::core::TcpServer::TcpServer(asio::any_io_executor ex,
                                    TcpServerOptions options)
    : acceptor_(ex), options_(std::move(options))
{
  logger->debug(R"(TCP server created: bind_address="{}", bind_port={})",
                options.address, options.port);
}

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
    logger->error(R"({}: code={}, category={}, message="{}")", context,
                  ec.value(), ec.category().name(), ec.message());

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

  logger->info("TCP server started: address={}, port={}", address, port);

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
          auto [ec, client_sock] = co_await self->acceptor_.async_accept(
            asio::as_tuple(asio::use_awaitable));
          if(ec)
            {
              if(ec == asio::error::operation_aborted)
                {
                  logger->debug("client accept loop stopped");
                  co_return;
                }

              logger->error(
                R"(failed to accept connection: code={}, message="{}")",
                ec.value(), ec.message());
              continue;
            }

          const auto session_id
            = self->next_session_id_.fetch_add(1, std::memory_order::relaxed);

          auto http_proxy_session = std::make_shared<http::HttpProxySession>(
            session_id, std::move(client_sock), self->rule_engine(),
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
              const auto http_logger = log::Get(log::Module::http);
              try
                {
                  std::rethrow_exception(e);
                }
              catch(const boost::system::system_error& ex)
                {
                  http_logger->error(
                    R"(HTTP proxy session terminated with an unhandled system error: code={}, message="{}")",
                    ex.code().value(), ex.code().message());
                }
              catch(const std::exception& ex)
                {
                  http_logger->error(
                    R"(HTTP proxy session terminated with an unhandled exception: message="{}")",
                    ex.what());
                }
              catch(...)
                {
                  http_logger->error(
                    "HTTP proxy session terminated with an unknown exception");
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
      catch(const boost::system::system_error& ex)
        {
          logger->critical(
            R"(client accept loop terminated with an unhandled system error: code={}, message="{}")",
            ex.code().value(), ex.code().message());
        }
      catch(const std::exception& ex)
        {
          logger->critical(
            R"(client accept loop terminated with an unhandled exception: message="{}")",
            ex.what());
        }
      catch(...)
        {
          logger->critical(
            "client accept loop terminated with an unknown exception");
        }
    });
}
