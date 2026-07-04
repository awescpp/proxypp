/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/http/adapter/beast_request_adapter.h"
#include "proxypp/http/adapter/beast_response_adapter.h"
#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>

namespace proxypp::http::adapter::test
{
  namespace http_ = boost::beast::http;

  constexpr auto kHeaderName = "x-proxypp-test";
  constexpr auto kHeaderValue = "value";
  constexpr auto kHeaderAnotherValue = "another value";

  inline auto GetAdapter(http_::request_header<>& message)
  {
    return std::make_unique<adapter::BeastRequestAdapter>(message);
  }

  inline auto GetAdapter(http_::response_header<>& message)
  {
    return std::make_unique<adapter::BeastResponseAdapter>(message);
  }

  namespace detail
  {
    template <typename Message>
    void add_header_should_add_header_when_missing(Message& message)
    {
      BOOST_REQUIRE(!message.contains(kHeaderName));

      auto adapter = GetAdapter(message);
      adapter->AddHeader(kHeaderName, kHeaderValue);
      BOOST_TEST(message[kHeaderName] == kHeaderValue);
      BOOST_TEST(message.count(kHeaderName) == 1);
    }

    template <typename Message>
    void add_header_should_not_override_existing_header(Message& message)
    {
      message.set(kHeaderName, kHeaderValue);
      BOOST_REQUIRE(message.contains(kHeaderName));

      auto adapter = GetAdapter(message);
      adapter->AddHeader(kHeaderName, kHeaderAnotherValue);

      BOOST_TEST(message[kHeaderName] == kHeaderValue);
    }

    template <typename Message>
    void set_header_should_add_header_when_missing(Message& message)
    {
      BOOST_REQUIRE(!message.contains(kHeaderName));

      auto adapter = GetAdapter(message);
      adapter->SetHeader(kHeaderName, kHeaderValue);

      BOOST_TEST(message[kHeaderName] == kHeaderValue);
    }

    template <typename Message>
    void set_header_should_override_existing_header(Message& message)
    {
      message.set(kHeaderName, kHeaderValue);
      BOOST_REQUIRE(message.contains(kHeaderName));

      auto adapter = GetAdapter(message);
      adapter->SetHeader(kHeaderName, kHeaderAnotherValue);

      BOOST_TEST(message[kHeaderName] == kHeaderAnotherValue);
      BOOST_TEST(message.count(kHeaderName) == 1);
    }

    template <typename Message>
    void replace_header_should_replace_existing_header(Message& message)
    {
      BOOST_REQUIRE(!message.contains(kHeaderName));

      message.set(kHeaderName, kHeaderValue);
      BOOST_REQUIRE(message.contains(kHeaderName));
      BOOST_REQUIRE(message[kHeaderName] == kHeaderValue);

      auto adapter = GetAdapter(message);
      adapter->ReplaceHeader(kHeaderName, kHeaderAnotherValue);

      BOOST_TEST(message[kHeaderName] == kHeaderAnotherValue);
      BOOST_TEST(message.count(kHeaderName) == 1);
    }

    template <typename Message>
    void replace_header_should_do_nothing_when_header_missing(Message& message)
    {
      BOOST_REQUIRE(!message.contains(kHeaderName));

      auto adapter = GetAdapter(message);
      adapter->ReplaceHeader(kHeaderName, kHeaderValue);

      BOOST_TEST(!message.contains(kHeaderName));
    }

    template <typename Message>
    void remove_header_should_remove_existing_header(Message& message)
    {
      message.set(kHeaderName, kHeaderValue);
      BOOST_REQUIRE(message.contains(kHeaderName));

      auto adapter = GetAdapter(message);
      adapter->RemoveHeader(kHeaderName);

      BOOST_TEST(!message.contains(kHeaderName));
    }

  }

}