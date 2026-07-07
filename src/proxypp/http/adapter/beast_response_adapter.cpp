/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/http/adapter/beast_response_adapter.h"
proxypp::http::adapter::BeastResponseAdapter::BeastResponseAdapter(
  http_::response_header<>& response)
    : response_(response)
{}

void proxypp::http::adapter::BeastResponseAdapter::SetHeader(
  std::string_view name, std::string_view value)
{
  response_.set(name, value);
}

void proxypp::http::adapter::BeastResponseAdapter::AddHeader(
  std::string_view name, std::string_view value)
{
  if(!response_.contains(name))
    {
      response_.set(name, value);
    }
}

void proxypp::http::adapter::BeastResponseAdapter::ReplaceHeader(
  std::string_view name, std::string_view value)
{
  if(response_.contains(name))
    {
      response_.set(name, value);
    }
}

void proxypp::http::adapter::BeastResponseAdapter::RemoveHeader(
  std::string_view name)
{
  response_.erase(name);
}

unsigned proxypp::http::adapter::BeastResponseAdapter::Status() const
{
  return response_.result_int();
}

std::string proxypp::http::adapter::BeastResponseAdapter::Reason() const
{
  return response_.reason();
}

unsigned proxypp::http::adapter::BeastResponseAdapter::Version() const
{
  return response_.version();
}

std::optional<std::string>
proxypp::http::adapter::BeastResponseAdapter::GetHeader(std::string_view name)
{
  const auto it = response_.find(name);
  if(it == response_.end())
    {
      return std::nullopt;
    }
  return it->value();
}

std::vector<proxypp::rule::http::Header>
proxypp::http::adapter::BeastResponseAdapter::GetAllHeaders() const
{
  std::vector<rule::http::Header> headers;
  headers.reserve(std::distance(response_.begin(), response_.end()));

  for(auto it = response_.begin(); it != response_.end(); ++it)
    {
      headers.emplace_back(std::string { it->name_string() },
                           std::string { it->value() });
    }

  return headers;
}
