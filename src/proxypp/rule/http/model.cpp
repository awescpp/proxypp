/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model.h"
#include "proxypp/helper.h"
#include <stdexcept>

proxypp::rule::http::Phase
proxypp::rule::http::PhaseFromString(std::string_view value)
{
  if(value == "request")
    {
      return Phase::Request;
    }
  if(value == "response")
    {
      return Phase::Response;
    }

  throw std::invalid_argument(std::format("invalid phase {}", value));
}

std::ostream& proxypp::rule::http::operator<<(std::ostream& os, Phase value)
{
  return helper::WriteEnumName(os, value);
}

proxypp::rule::http::Target
proxypp::rule::http::TargetFromString(std::string_view value)
{
  if(value == "header")
    {
      return Target::Header;
    }
  throw std::invalid_argument(std::format("invalid target {}", value));
}

std::ostream& proxypp::rule::http::operator<<(std::ostream& os, Target value)
{
  return helper::WriteEnumName(os, value);
}
