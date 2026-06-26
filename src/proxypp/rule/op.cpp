/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "op.h"
#include "proxypp/helper.h"
#include <stdexcept>

proxypp::rule::Op proxypp::rule::tag_invoke(boost::json::value_to_tag<Op>,
                                            const boost::json::value& value)
{
  if(value == "set")
    {
      return Op::Set;
    }
  if(value == "add")
    {
      return Op::Add;
    }
  if(value == "remove")
    {
      return Op::Remove;
    }
  if(value == "replace")
    {
      return Op::Replace;
    }
  throw std::invalid_argument("invalid argument");
}

std::ostream& proxypp::rule::operator<<(std::ostream& os, Op value)
{
  return helper::WriteEnumName(os, value);
}