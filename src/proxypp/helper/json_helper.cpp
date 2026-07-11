/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/helper/json_helper.h"

namespace proxypp::helper::json
{
  Result<proxypp::json> ParseJson(std::string_view content)
  {
    try
      {
        return proxypp::json::parse(content);
      }
    catch(const std::exception& e)
      {
        return Unexpected(Error { Errc::JsonParseFailed, e.what() });
      }
    catch(...)
      {
        return Unexpected(
          Error { Errc::JsonParseFailed, "unknown json parse error" });
      }
  }
}