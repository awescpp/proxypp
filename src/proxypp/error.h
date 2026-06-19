/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/error_code.hpp>
#include <string>

namespace proxypp
{
  struct Error
  {
    boost::system::error_code code;
    std::string message;

    explicit Error() = default;

    explicit Error(boost::system::error_code error_code, std::string msg = {});

    explicit operator bool() const noexcept;
  };
}