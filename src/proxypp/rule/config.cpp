/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/config.h"

proxypp::rule::Config
proxypp::rule::tag_invoke(boost::json::value_to_tag<Config>,
                          const boost::json::value& value)
{
  const auto& obj = value.as_object();
  Config config;
  if(const auto* schema = obj.if_contains("$schema"))
    {
      config.schema = boost::json::value_to<std::string>(*schema);
    }
  config.version = obj.at("version").as_int64();
  if(const auto* http = obj.if_contains("http"))
    {
      config.http = boost::json::value_to<http::Config>(*http);
    }
  return config;
}