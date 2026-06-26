/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/json.hpp>

namespace proxypp::rule
{
  struct Match
  {
    std::string expr;
  };

  Match tag_invoke(boost::json::value_to_tag<Match>,
                   const boost::json::value& value);
}