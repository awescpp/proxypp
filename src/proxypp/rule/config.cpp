/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"

proxypp::rule::Config
proxypp::rule::tag_invoke(boost::json::value_to_tag<Config>,
                          const boost::json::value& value)
{
  Config config;
  const auto& object = value.as_object();
  if(auto* schema = object.if_contains("$schema"))
    {
      config.schema = boost::json::value_to<std::string>(*schema);
    }
  config.version = boost::json::value_to<int>(object.at("version"));
  if(auto* http = object.if_contains("http"))
    {
      config.http = boost::json::value_to<http::Config>(*http);
    }
  return config;
}