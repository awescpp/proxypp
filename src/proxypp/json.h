/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "result.h"
#include <jsoncons/json.hpp>
#include <string_view>

namespace proxypp
{
  using json = jsoncons::ojson;
  namespace jsonlib = jsoncons;
}
