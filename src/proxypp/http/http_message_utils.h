#pragma once

#include <boost/beast.hpp>

namespace beast = boost::beast;

namespace proxypp::http::details
{
  std::optional<std::size_t> FindCrlf(const beast::flat_buffer& buffer);

  std::string
  BufferPrefixToString(const beast::flat_buffer& buffer, std::size_t length);

  /// Parses a chunk-size line without trailing CRLF.
  ///
  /// Chunk extensions are allowed and ignored.
  /// Returns std::nullopt if the line is malformed or overflows size_t.
  std::optional<std::size_t> ParseChunkSizeLine(std::string_view line);

  bool StartsWithCrlf(const beast::flat_buffer& buffer);
}