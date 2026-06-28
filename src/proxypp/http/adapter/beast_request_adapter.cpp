/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/http/adapter/beast_request_adapter.h"

proxypp::http::adapter::BeastRequestAdapter::BeastRequestAdapter(
  http_::request_header<>& request)
    : request_(request)
{}

std::string_view proxypp::http::adapter::BeastRequestAdapter::Method() const
{
  return request_.method_string();
}

std::string_view proxypp::http::adapter::BeastRequestAdapter::Target() const
{
  return request_.target();
}

unsigned proxypp::http::adapter::BeastRequestAdapter::Version() const
{
  return request_.version();
}

std::optional<std::string_view>
proxypp::http::adapter::BeastRequestAdapter::Header(std::string_view name)
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
{}

void proxypp::http::adapter::BeastRequestAdapter::AddHeader(
  std::string_view name, std::string_view value)
{}

void proxypp::http::adapter::BeastRequestAdapter::ReplaceHeader(
  std::string_view name, std::string_view value)
{}

void proxypp::http::adapter::BeastRequestAdapter::RemoveHeader(
  std::string_view name)
{}
