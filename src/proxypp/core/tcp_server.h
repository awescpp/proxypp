/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/common.h"
#include "proxypp/rule/http/model.h"
#include "proxypp/rule/rule_engine.h"
#include <boost/beast.hpp>
#include <optional>

using socket_t = boost::beast::tcp_stream;

namespace proxypp ::core
{
  struct TcpServerOptions
  {
    std::string address;
    std::size_t port;
    std::optional<rule::http::Config> http_rule_config;
    std::shared_ptr<rule::RuleEngine> rule_engine;
  };

  class TcpServer : public std::enable_shared_from_this<TcpServer>
  {
  public:
    explicit TcpServer(asio::any_io_executor ex, TcpServerOptions options);

    void Run();

  private:
    std::string_view address() const { return options_.address; }

    std::size_t port() const { return options_.port; }

    std::shared_ptr<rule::RuleEngine> rule_engine() const
    {
      return options_.rule_engine;
    }

    std::optional<rule::http::Config> http_rule_config() const
    {
      return options_.http_rule_config;
    }

    tcp::acceptor acceptor_;
    TcpServerOptions options_;
  };
} // namespace proxypp::core
