/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "match.h"

proxypp::rule::Match
proxypp::rule::tag_invoke(boost::json::value_to_tag<Match>,
                          const boost::json::value& value)
{
  return { .expr = boost::json::value_to<std::string>(value.at("expr")) };
}