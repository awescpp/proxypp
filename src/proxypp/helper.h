/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iosfwd>
#include <magic_enum/magic_enum.hpp>
#include <type_traits>

namespace proxypp::helper
{
  template <typename T>
    requires std::is_enum_v<T>
  std::ostream& WriteEnumName(std::ostream& os, T value)
  {
    return os << magic_enum::enum_name(value);
  }
}