#pragma once
#include <boost/beast.hpp>

namespace proxypp::http
{

  enum class MessageDirection
  {
    Request,
    Response,
  };

  namespace http_ = boost::beast::http;

  template <MessageDirection> struct BodyFraming;

  // specification of BodyFraming
  template <> struct BodyFraming<MessageDirection::Request>
  {
    enum class Type
    {
      None,
      ContentLength,
      Chunked,
    };
  };

  template <> struct BodyFraming<MessageDirection::Response>
  {
    enum class Type
    {
      None,
      ContentLength,
      Chunked,
      CloseDelimited,
      Tunnel,
    };
  };

  template <MessageDirection direction> struct BodyInfo
  {
    BodyFraming<direction>::Type framing;
    std::size_t content_length = 0;
  };

  using RequestParser = http_::request_parser<http_::buffer_body>;
  using ResponseParser = http_::response_parser<http_::buffer_body>;

  using RequestBodyFraming = BodyFraming<MessageDirection::Request>::Type;
  using ResponseBodyFraming = BodyFraming<MessageDirection::Response>::Type;

  BodyInfo<MessageDirection::Request>
  DetermineBodyInfo(const RequestParser& request_parser);

  BodyInfo<MessageDirection::Response>
  DetermineBodyInfo(const RequestParser& request_parser,
                    const ResponseParser& response_parser);

  std::ostream& operator<<(std::ostream& os, RequestBodyFraming framing);

  std::ostream& operator<<(std::ostream& os, ResponseBodyFraming framing);

}
