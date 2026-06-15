/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_message_utils.h"

#include "proxypp/common.h"

namespace
{
  // According to HTTP chunked transfer coding grammar, chunk-size starts
  // directly with 1*HEXDIG. Prefix OWS before chunk-size is invalid.
  //
  // Only trailing OWS before chunk-extension is tolerated here, for example:
  //   "1a ;foo=bar"
  //   "1a\t;foo=bar"
  //
  // Do not trim leading OWS; inputs like " 1a" or "\t1a" should be rejected.
  std::string_view TrimTrailingOws(std::string_view text)
  {
    while(!text.empty() && (text.back() == ' ' || text.back() == '\t'))
      {
        text.remove_suffix(1);
      }

    return text;
  }
}

std::optional<std::size_t>
proxypp::http::details::FindCrlf(const beast::flat_buffer& buffer)
{
  const auto begin = asio::buffers_begin(buffer.data());
  const auto end = asio::buffers_end(buffer.data());
  constexpr std::array crlf{'\r', '\n'};
  const auto it = std::search(begin, end, crlf.begin(), crlf.end());
  if(it == end)
    {
      return std::nullopt;
    }
  return static_cast<std::size_t>(std::distance(begin, it));
}

std::string
proxypp::http::details::BufferPrefixToString(const beast::flat_buffer& buffer,
                                             std::size_t length)
{
  std::string result;
  result.resize(length);
  asio::buffer_copy(asio::buffer(result.data(), result.size()),
                    beast::buffers_prefix(length, buffer.data()));
  return result;
}

std::optional<std::size_t>
proxypp::http::details::ParseChunkSizeLine(std::string_view line)
{
  // ParseChunkSizeLine expects a line without CRLF.
  // If CR or LF is present, the caller did not split the buffer correctly.
  if(line.find('\r') != std::string_view::npos
     || line.find('\n') != std::string_view::npos)
    {
      return std::nullopt;
    }

  const auto semicolon_pos = line.find(';');
  auto size_text = line.substr(0, semicolon_pos);
  if(size_text.empty())
    {
      return std::nullopt;
    }

  size_text = TrimTrailingOws(size_text);

  std::size_t size = 0;
  for(const char ch : size_text)
    {
      unsigned digit = 0;
      if(ch >= '0' && ch <= '9')
        {
          digit = static_cast<unsigned>(ch - '0');
        }
      else if(ch >= 'a' && ch <= 'f')
        {
          digit = static_cast<unsigned>(ch - 'a' + 10);
        }
      else if(ch >= 'A' && ch <= 'F')
        {
          digit = static_cast<unsigned>(ch - 'A' + 10);
        }
      else
        {
          return std::nullopt;
        }
      constexpr auto max = std::numeric_limits<std::size_t>::max();
      if(size > (max - digit) / 16)
        {
          return std::nullopt;
        }

      size = size * 16 + digit;
    }

  return size;
}

bool proxypp::http::details::StartsWithCrlf(const beast::flat_buffer& buffer)
{
  if(buffer.size() < 2)
    {
      return false;
    }
  std::array<char, 2> bytes{};
  asio::buffer_copy(asio::buffer(bytes),
                    beast::buffers_prefix(2, buffer.data()));
  return bytes[0] == '\r' && bytes[1] == '\n';
}
