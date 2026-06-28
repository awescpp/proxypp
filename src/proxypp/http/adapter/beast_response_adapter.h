/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/class_utils.h"
#include "proxypp/rule/http/response_adapter.h"
#include <boost/beast/http/message.hpp>

namespace proxypp::http::adapter
{
  namespace http_ = boost::beast::http;

  class BeastResponseAdapter final : public rule::http::ResponseAdapter
  {
  public:
    explicit BeastResponseAdapter(http_::response_header<>& response);

    PROXYPP_DISABLE_COPY_AND_MOVE(BeastResponseAdapter);

    void SetHeader(std::string_view name, std::string_view value) override;

    void AddHeader(std::string_view name, std::string_view value) override;

    void ReplaceHeader(std::string_view name, std::string_view value) override;

    void RemoveHeader(std::string_view name) override;

    unsigned Status() const override;

    std::string_view Reason() override;

    unsigned Version() const override;

    std::optional<std::string_view> Header(std::string_view name) override;

  private:
    http_::response_header<>& response_;
  };
}