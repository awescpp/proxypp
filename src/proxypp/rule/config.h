/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "http/model.h"

namespace proxypp::rule
{
  inline constexpr int kRulesSchemaVersion = 1;

  struct Config
  {
    std::optional<std::string> schema;
    int version = kRulesSchemaVersion;
    std::optional<http::Config> http;
  };

  Config tag_invoke(boost::json::value_to_tag<Config>,
                    const boost::json::value& value);

}