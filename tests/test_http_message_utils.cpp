#define BOOST_TEST_MODULE HttpMessageUtilsTest

#include "proxypp/http/http_message_utils.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/test/unit_test.hpp>

using namespace proxypp::http::details;

namespace
{
  beast::flat_buffer MakeFlatBuffer(std::string_view data)
  {
    beast::flat_buffer buffer;
    auto mutable_buffer = buffer.prepare(data.size());
    boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data));
    buffer.commit(data.size());
    return buffer;
  }
}

BOOST_AUTO_TEST_SUITE(FindCrlfTests)

BOOST_AUTO_TEST_CASE(find_crlf_should_return_position_when_buffer_contains_crlf)
{
  const auto buffer = MakeFlatBuffer("14;\r\n");
  const auto result = FindCrlf(buffer);
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 3);
}

BOOST_AUTO_TEST_CASE(
  find_crlf_should_return_nullopt_when_buffer_does_not_contain_crlf)
{
  const auto buffer = MakeFlatBuffer("4;\n\n");
  const auto result = FindCrlf(buffer);
  BOOST_TEST(!result.has_value());
}

BOOST_AUTO_TEST_CASE(
  find_crlf_should_return_position_of_first_crlf_when_buffer_contains_multiple_crlf)
{
  const auto buffer = MakeFlatBuffer("b;\r\nhello world\r\naa\r");
  const auto result = FindCrlf(buffer);
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 2);
}

BOOST_AUTO_TEST_CASE(find_crlf_should_work_when_crlf_at_beginning)
{
  const auto buffer = MakeFlatBuffer("\r\nhel");
  const auto result = FindCrlf(buffer);
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 0);
}

BOOST_AUTO_TEST_CASE(find_crlf_should_work_when_crlf_at_end)
{
  const auto buffer = MakeFlatBuffer("hello world\r\n");
  const auto result = FindCrlf(buffer);
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 11);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(BufferPrefixToStringTests)

BOOST_AUTO_TEST_CASE(buffer_prefix_to_string_should_return_prefix_content)
{
  const auto buffer = MakeFlatBuffer("hello world");
  const auto result = BufferPrefixToString(buffer, 5);
  BOOST_TEST(result == "hello");
}

BOOST_AUTO_TEST_CASE(
  buffer_prefix_to_string_should_return_empty_string_when_size_is_zero)
{
  const auto buffer = MakeFlatBuffer("hello world");
  const auto result = BufferPrefixToString(buffer, 0);
  BOOST_TEST(result == "");
}

BOOST_AUTO_TEST_CASE(buffer_prefix_to_string_should_not_consume_buffer)
{
  const auto buffer = MakeFlatBuffer("hello world");
  const auto buffer_size = buffer.size();
  const auto result = BufferPrefixToString(buffer, 5);
  BOOST_TEST(buffer.size() == buffer_size);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ParseChunkSizeLineTestSuite)

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_return_std_nullopt_when_line_contains_invalid_digit)
{
  BOOST_TEST(!ParseChunkSizeLine(";").has_value());
  BOOST_TEST(!ParseChunkSizeLine("0ax;"));
  BOOST_TEST(!ParseChunkSizeLine("0x1234;"));
}

struct TestCase
{
  std::string_view line;
  std::size_t expected;
};

BOOST_AUTO_TEST_CASE(parse_chunk_size_line_should_parse_decimal_digits_as_hex)
{
  const std::array cases{TestCase{"30;", 48}, TestCase{"16;", 22},
                         TestCase{"0;", 0}};

  for(const auto& test_case : cases)
    {
      const auto result = ParseChunkSizeLine(test_case.line);
      BOOST_TEST_CONTEXT("line=" << test_case.line
                                 << ", expected=" << test_case.expected)
      {
        BOOST_TEST(result.has_value());
        BOOST_TEST(*result == test_case.expected);
      }
    }
}

BOOST_AUTO_TEST_CASE(parse_chunk_size_line_should_parse_hex_letters)
{
  const std::array cases{TestCase{"a;", 10}, TestCase{"1A;", 26},
                         TestCase{"A1;", 161}, TestCase{"DF;", 223},
                         TestCase{"df;", 223}};
  for(const auto& test_case : cases)
    {
      const auto result = ParseChunkSizeLine(test_case.line);
      BOOST_TEST_CONTEXT("line=" << test_case.line
                                 << ", expected=" << test_case.expected)
      {
        BOOST_TEST(result.has_value());
        BOOST_TEST(*result == test_case.expected);
      }
    }
}

BOOST_AUTO_TEST_CASE(parse_chunk_size_line_should_ignore_chunk_extensions)
{
  BOOST_REQUIRE(ParseChunkSizeLine("3A;foo=bar").has_value());
  BOOST_TEST(*ParseChunkSizeLine("3A;foo=bar") == 58);
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_allow_whitespace_before_chunk_extension)
{
  const auto result = ParseChunkSizeLine("1a ;foo=bar");
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 26);
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_allow_whitespace_after_semicolon)
{
  const auto result = ParseChunkSizeLine("1a; foo=bar");
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 26);
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_return_nullopt_when_chunk_size_contains_whitespace)
{
  const auto result = ParseChunkSizeLine("1 a;foo=bar");
  BOOST_TEST(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_chunk_size_line_should_reject_tab_before_chunksize)
{
  // According to HTTP chunked transfer coding grammar, chunk-size starts
  // directly with 1*HEXDIG. Prefix OWS before chunk-size is invalid.
  const auto result = ParseChunkSizeLine("\t1a;foo=bar");
  BOOST_TEST(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_chunk_size_line_should_allow_tab_after_chunksize)
{
  const auto result = ParseChunkSizeLine("1a\t\t; foo=bar");
  BOOST_TEST(result.has_value());
  BOOST_TEST(*result == 26);
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_allow_optional_whitespace_around_chunk_size)
{
  const auto result = ParseChunkSizeLine("1a \t \t ; foo=bar");
  BOOST_TEST(result.has_value());
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_return_nullopt_when_line_format_invalid)
{
  BOOST_TEST(!ParseChunkSizeLine(";").has_value());
  BOOST_TEST(!ParseChunkSizeLine("").has_value());
  BOOST_TEST(!ParseChunkSizeLine(";").has_value());
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_return_nullopt_when_line_contains_crlf)
{
  const auto result = ParseChunkSizeLine("3a\r\n;");
  BOOST_TEST(!result.has_value());
}

BOOST_AUTO_TEST_CASE(
  parse_chunk_size_line_should_return_nullopt_when_value_overflows_size_t)
{
  const auto max = std::numeric_limits<std::size_t>::max();
  boost::multiprecision::cpp_int larger_value{max};
  larger_value += 1;

  const auto chunk_size_line = std::format("{};\r\n", larger_value.str(16));

  const auto result = ParseChunkSizeLine(chunk_size_line);
  BOOST_TEST(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(StartsWithCrlfTestSuite)

BOOST_AUTO_TEST_CASE(
  start_with_crlf_should_return_false_when_buffer_does_not_start_with_crlf)
{
  const auto buffer1 = MakeFlatBuffer("\n\r");
  BOOST_TEST(StartsWithCrlf(buffer1) == false);

  const auto buffer2 = MakeFlatBuffer("\r\r\nline");
  BOOST_TEST(StartsWithCrlf(buffer2) == false);
}

BOOST_AUTO_TEST_CASE(
  start_with_clrf_should_return_ture_when_buffer_starts_with_crlf)
{
  const auto buffer = MakeFlatBuffer("\r\nsome other stuff..");
  BOOST_TEST(StartsWithCrlf(buffer) == true);
}

BOOST_AUTO_TEST_CASE(starts_with_crlf_should_returns_false_when_buffer_is_empty)
{
  beast::flat_buffer buffer;
  BOOST_TEST(StartsWithCrlf(buffer) == false);
}

BOOST_AUTO_TEST_SUITE_END()
