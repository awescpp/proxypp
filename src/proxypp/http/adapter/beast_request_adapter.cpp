/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/http/adapter/beast_request_adapter.h"

proxypp::http::adapter::BeastRequestAdapter::BeastRequestAdapter(
  http_::request_header<>& request)
    : request_(request)
{}

std::string proxypp::http::adapter::BeastRequestAdapter::Method() const
{
  return request_.method_string();
}

std::string proxypp::http::adapter::BeastRequestAdapter::Target() const
{
  return request_.target();
}

unsigned proxypp::http::adapter::BeastRequestAdapter::Version() const
{
  return request_.version();
}

std::optional<std::string_view>
proxypp::http::adapter::BeastRequestAdapter::GetHeader(std::string_view name)
{
  const auto it = request_.find(name);
  if(it == request_.end())
    {
      return std::nullopt;
    }
  return it->value();
}

void proxypp::http::adapter::BeastRequestAdapter::SetHeader(
  std::string_view name, std::string_view value)
{
  request_.set(name, value);
}

void proxypp::http::adapter::BeastRequestAdapter::AddHeader(
  std::string_view name, std::string_view value)
{
  if(!request_.contains(name))
    {
      request_.set(name, value);
    }
}

void proxypp::http::adapter::BeastRequestAdapter::ReplaceHeader(
  std::string_view name, std::string_view value)
{
  if(request_.contains(name))
    {
      request_.set(name, value);
    }
}

void proxypp::http::adapter::BeastRequestAdapter::RemoveHeader(
  std::string_view name)
{
  request_.erase(name);
}

std::vector<proxypp::rule::http::Header>
proxypp::http::adapter::BeastRequestAdapter::GetAllHeaders() const
{
  std::vector<rule::http::Header> headers;

  headers.reserve(std::distance(request_.begin(), request_.end()));

  for(auto it = request_.begin(); it != request_.end(); ++it)
    {
      headers.emplace_back(std::string { it->name_string() },
                           std::string { it->value() });
    }
  return headers;
}
