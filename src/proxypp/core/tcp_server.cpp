#include "tcp_server.h"

#include "proxypp/http/http_proxy_session.h"
#include "proxypp/log/log.h"

proxypp::core ::TcpServer::TcpServer(asio::any_io_executor ex,
                                     std::string_view address,
                                     std::size_t port)
    : acceptor_(ex), address_(address), port_(port)
{}

void proxypp::core::TcpServer::Run()
{
  const tcp::endpoint ep{asio::ip::make_address(address_),
                         static_cast<asio::ip::port_type>(port_)};

  boost::system::error_code ec;

  acceptor_.open(ep.protocol(), ec);
  if(ec)
    {
      std::cerr << std::format("TcpServer open {}:{} failed", address_, port_);
      return;
    }

  acceptor_.set_option(asio::socket_base::reuse_address(true), ec);

  acceptor_.bind(ep, ec);
  if(ec)
    {
      std::cerr << std::format("TcpServer bind {}:{} failed", address_, port_);
      return;
    }

  acceptor_.listen(asio::socket_base::max_listen_connections, ec);
  if(ec)
    {
      std::cerr << std::format("TcpServer start listening on {}:{} failed",
                               address_, port_);
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

          co_await std::make_shared<http::HttpProxySession>(
            std::move(client_sock))
            ->Run();
        }
    },
    asio::detached);

  LOG_CORE_INFO("proxy++ running on {}:{}", address_, port_);
}
