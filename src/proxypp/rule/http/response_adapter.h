/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adapter_share.h"
#include <optional>
#include <string_view>
#include <vector>

namespace proxypp::rule::http
{
  class ResponseAdapter
  {
  public:
    virtual ~ResponseAdapter() = default;

    virtual unsigned Status() const = 0;

    virtual std::string Reason() const = 0;

    virtual unsigned Version() const = 0;

    virtual std::optional<std::string> GetHeader(std::string_view name) = 0;

    virtual std::vector<rule::http::Header> GetAllHeaders() const = 0;

    virtual void SetHeader(std::string_view name, std::string_view value) = 0;

    virtual void AddHeader(std::string_view name, std::string_view value) = 0;

    virtual void
    ReplaceHeader(std::string_view name, std::string_view value) = 0;

    virtual void RemoveHeader(std::string_view name) = 0;
  };
}