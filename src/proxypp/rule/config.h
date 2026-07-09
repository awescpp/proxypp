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
}

JSONCONS_N_MEMBER_NAME_TRAITS(proxypp::rule::Config, 1, (version, "version"),
                              (schema, "$schema"), (http, "http"))