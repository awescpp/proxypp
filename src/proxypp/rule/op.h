/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/json.hpp>
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

  Op tag_invoke(boost::json::value_to_tag<Op>,
                const boost::json::value& value);

  std::ostream& operator<<(std::ostream& os, Op value);

}