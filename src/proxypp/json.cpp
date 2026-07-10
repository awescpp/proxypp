/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/json.h"
#include "proxypp/error.h"

namespace proxypp
{
  Result<json> ParseJson(std::string_view content)
  {
    try
      {
        return json::parse(content);
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
