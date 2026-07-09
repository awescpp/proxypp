/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/json.h"
#include <iosfwd>

namespace proxypp::rule
{
  enum class Op
  {
    NotSet = 0,
    Set,
    Add,
    Remove,
    Replace,
  };

  std::ostream& operator<<(std::ostream& os, Op value);

}

JSONCONS_ENUM_NAME_TRAITS(proxypp::rule::Op, (Set, "set"), (Add, "add"),
                          (Remove, "remove"), (Replace, "replace"))