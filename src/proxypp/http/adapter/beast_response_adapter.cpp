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

std::string_view proxypp::http::adapter::BeastResponseAdapter::Reason()
{
  return response_.reason();
}

unsigned proxypp::http::adapter::BeastResponseAdapter::Version() const
{
  return response_.version();
}

std::optional<std::string_view>
proxypp::http::adapter::BeastResponseAdapter::Header(std::string_view name)
{
  const auto it = response_.find(name);
  if(it == response_.end())
    {
      return std::nullopt;
    }
  return it->value();
}
