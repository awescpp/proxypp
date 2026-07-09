/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/json.h"

namespace proxypp::rule
{
  struct Match
  {
    std::string expr;
  };
}

JSONCONS_ALL_MEMBER_TRAITS(proxypp::rule::Match, expr)