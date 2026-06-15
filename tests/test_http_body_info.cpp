/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE HttpBodyInfoTest

#include "proxypp/http/body_info.h"

#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>

using namespace proxypp::http;
namespace asio = boost::asio;

template <typename Parser,
          typename = std::enable_if_t<
            std::is_same_v<std::remove_cvref_t<Parser>, RequestParser>
            || std::is_same_v<std::remove_cvref_t<Parser>, ResponseParser>>>
void ParserHttpHeader(Parser& parser, std::string_view header_text)
{
  parser.skip(true);
  boost::system::error_code ec;
  parser.put(asio::buffer(header_text), ec);
  BOOST_REQUIRE_MESSAGE(!ec, ec.message());
  BOOST_REQUIRE(parser.is_done());
}

BOOST_AUTO_TEST_SUITE(RequestBodyInfoDetectionTests)

BOOST_AUTO_TEST_CASE(
  determine_request_body_info_should_return_chunked_for_chunked_request)
{
  RequestParser parser;
  std::string_view header_text = "POST http://example.com/upload HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "Transfer-Encoding: chunked\r\n"
                                 "\r\n";
  ParserHttpHeader(parser, header_text);
  const auto body_info = DetermineBodyInfo(parser);
  BOOST_TEST(body_info.framing == RequestBodyFraming::Chunked);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_request_body_info_should_return_content_length_for_request_with_content_length)
{
  RequestParser parser;
  std::string_view header_text = "POST http://example.com/upload HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "Content-Length: 123\r\n"
                                 "\r\n";
  ParserHttpHeader(parser, header_text);
  const auto body_info = DetermineBodyInfo(parser);
  BOOST_TEST(body_info.framing == RequestBodyFraming::ContentLength);
  BOOST_TEST(body_info.content_length == 123);
}

BOOST_AUTO_TEST_CASE(
  determine_request_body_info_should_return_content_length_for_zero_content_length)
{
  RequestParser parser;
  std::string_view header_text = "POST http://example.com/upload HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "Content-Length: 0\r\n"
                                 "\r\n";
  ParserHttpHeader(parser, header_text);
  const auto body_info = DetermineBodyInfo(parser);
  BOOST_TEST(body_info.framing == RequestBodyFraming::ContentLength);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_request_body_info_should_return_none_for_request_without_body_framing)
{
  RequestParser parser;
  std::string_view header_text = "POST http://example.com/upload HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "\r\n";
  ParserHttpHeader(parser, header_text);
  const auto body_info = DetermineBodyInfo(parser);
  BOOST_TEST(body_info.framing == RequestBodyFraming::None);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  parse_http_header_should_fail_when_content_length_and_chunked_both_present)
{
  // According to RFC 9112, the current HTTP/1.1 specification, when both
  // Content-Length and Transfer-Encoding: chunked are present,
  // Transfer-Encoding takes precedence, and Content-Length must be ignored

  // If both Content-Length and Transfer-Encoding: chunked are present in the
  // header, Boost.Beast will throw an exception during parsing, so we no
  // longer need to test DetermineBodyInfo for this case
  RequestParser parser;
  std::string_view header_text = "POST http://example.com/upload HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "Content-Length: 123\r\n"
                                 "Transfer-Encoding: chunked\r\n"
                                 "\r\n";
  // ParserHttpHeader(parser, header_text);
  parser.skip(true);
  boost::system::error_code ec;
  parser.put(asio::buffer(header_text), ec);
  BOOST_CHECK(ec == boost::beast::http::error::bad_transfer_encoding);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ResponseBodyInfoDetectionTests)

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_none_for_head_response)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "HEAD http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text = "HTTP/1.1 200 OK\r\n"
                                                "Content-Length: 123\r\n"
                                                "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::None);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_none_for_1xx_response)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "GET http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text = "HTTP/1.1 100 Continue\r\n"
                                                "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::None);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_none_for_204_response)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "GET http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text = "HTTP/1.1 204 No Content\r\n"
                                                "Content-Length: 123\r\n"
                                                "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::None);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_none_for_304_response)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "GET http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text = "HTTP/1.1 304 Not Modified\r\n"
                                                "Content-Length: 123\r\n"
                                                "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::None);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_none_for_successful_connect_response)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "CONNECT example.com:443 HTTP/1.1\r\n"
      "Host: example.com:443\r\n"
      "\r\n";

  const std::string_view response_header_text
    = "HTTP/1.1 200 Connection Established\r\n"
      "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::Tunnel);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_chunked_for_chunked_response)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "GET http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text
    = "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::Chunked);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_content_length_for_response_with_content_length)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "GET http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text = "HTTP/1.1 200 OK\r\n"
                                                "Content-Length: 456\r\n"
                                                "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::ContentLength);
  BOOST_TEST(body_info.content_length == 456);
}

BOOST_AUTO_TEST_CASE(
  determine_response_body_info_should_return_close_delimited_for_response_without_body_framing)
{
  RequestParser request_parser;
  ResponseParser response_parser;

  const std::string_view request_header_text
    = "GET http://example.com/ HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";

  const std::string_view response_header_text = "HTTP/1.1 200 OK\r\n"
                                                "\r\n";

  ParserHttpHeader(request_parser, request_header_text);
  ParserHttpHeader(response_parser, response_header_text);

  const auto body_info = DetermineBodyInfo(request_parser, response_parser);
  BOOST_TEST(body_info.framing == ResponseBodyFraming::CloseDelimited);
  BOOST_TEST(body_info.content_length == 0);
}

BOOST_AUTO_TEST_SUITE_END()