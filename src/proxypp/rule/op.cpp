/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "op.h"
#include "proxypp/helper.h"

std::ostream& proxypp::rule::operator<<(std::ostream& os, Op value)
{
  return helper::WriteEnumName(os, value);
}