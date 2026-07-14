/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/json.h"

namespace proxypp::helper::json
{
  Result<proxypp::json> ParseJson(std::string_view content);

  template <typename T> Result<T> Convert(const proxypp::json& jv)
  {
    try
      {
        return jv.as<T>();
      }
    catch(jsonlib::conv_error& e)
      {
        return Unexpected(Error { Errc::JsonConversionFailed });
      }
    catch(const std::exception& e)
      {
        return Unexpected(
          Error { Errc::InternalError, "unexpected json conversion error" });
      }
  }
}