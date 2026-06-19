/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/error.h"

#include <tl/expected.hpp>

namespace proxypp
{
  template <typename T> using Result = tl::expected<T, Error>;

  [[nodiscard]]
  inline tl::unexpected<Error> Unexpected(Error error)
  {
    return tl::unexpected<Error>{std::move(error)};
  }
}