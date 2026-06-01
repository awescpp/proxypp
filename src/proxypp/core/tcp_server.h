#pragma once

#include "proxypp/common.h"

#include <boost/beast.hpp>

using socket_t = boost::beast::tcp_stream;

namespace proxypp ::core
{
  class TcpServer : public std::enable_shared_from_this<TcpServer>
  {
  public:
    explicit TcpServer(asio::any_io_executor, std::string_view address,
                       std::size_t port);

    void Run();

  private:
    tcp::acceptor acceptor_;
    std::string address_;
    std::size_t port_;
  };
} // namespace proxypp::core
