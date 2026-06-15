/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "body_info.h"

#include <magic_enum/magic_enum.hpp>

proxypp::http::BodyInfo<proxypp::http::MessageDirection::Request>
proxypp::http::DetermineBodyInfo(const RequestParser& request_parser)
{
  if(request_parser.chunked())
    {
      return {RequestBodyFraming::Chunked, 0};
    }
  const auto content_length = request_parser.content_length();
  if(content_length.has_value())
    {
      return {RequestBodyFraming::ContentLength, *content_length};
    }
  return {RequestBodyFraming::None};
}

proxypp::http::BodyInfo<proxypp::http::MessageDirection::Response>
proxypp::http::DetermineBodyInfo(const RequestParser& request_parser,
                                 const ResponseParser& response_parser)
{
  const auto status_code = response_parser.get().result_int();
  if(request_parser.get().method() == http_::verb::head)
    {
      return {ResponseBodyFraming::None};
    }
  if(status_code >= 100 && status_code < 200)
    {
      return {ResponseBodyFraming::None};
    }
  if(status_code == 204 || status_code == 304)
    {
      return {ResponseBodyFraming::None};
    }
  if(request_parser.get().method() == http_::verb::connect
     && status_code >= 200 && status_code < 300)
    {
      return {ResponseBodyFraming::Tunnel};
    }
  if(response_parser.chunked())
    {
      return {ResponseBodyFraming::Chunked};
    }
  if(response_parser.content_length().has_value())
    {
      return {ResponseBodyFraming::ContentLength,
              *response_parser.content_length()};
    }
  return {ResponseBodyFraming::CloseDelimited};
}

std::ostream&
proxypp::http::operator<<(std::ostream& os, ResponseBodyFraming framing)
{
  return os << magic_enum::enum_name(framing);
}

std::ostream&
proxypp::http::operator<<(std::ostream& os, RequestBodyFraming framing)
{
  return os << magic_enum::enum_name(framing);
}
