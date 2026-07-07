/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace proxypp::rule::http
{
  struct Header
  {
    std::string name;
    std::string value;
  };
}