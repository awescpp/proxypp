/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/class_utils.h"
#include "proxypp/rule/http/request_adapter.h"
#include <boost/beast/http/message.hpp>

namespace proxypp::http::adapter
{
  namespace http_ = boost::beast::http;

  class BeastRequestAdapter final : public rule::http::RequestAdapter
  {
  public:
    explicit BeastRequestAdapter(http_::request_header<>& request);

    PROXYPP_DISABLE_COPY_AND_MOVE(BeastRequestAdapter);

    std::string Method() const override;

    std::string Target() const override;

    unsigned Version() const override;

    std::optional<std::string_view> GetHeader(std::string_view name) override;

    void SetHeader(std::string_view name, std::string_view value) override;

    void AddHeader(std::string_view name, std::string_view value) override;

    void ReplaceHeader(std::string_view name, std::string_view value) override;

    void RemoveHeader(std::string_view name) override;

    std::vector<rule::http::Header> GetAllHeaders() const override;

  private:
    http_::request_header<>& request_;
  };
}