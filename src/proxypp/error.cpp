/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/error.h"

#include <utility>

proxypp::Error::Error(boost::system::error_code error_code, std::string msg)
    : code(error_code), message(std::move(msg))
{
  if(message.empty() && code)
    {
      message = code.message();
    }
}

proxypp::Error::operator bool() const noexcept
{
  return static_cast<bool>(code);
}