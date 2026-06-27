/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/result.h"

namespace proxypp::rule
{
  Result<void> ValidateJsonBySchema(std::string_view schema_text,
                                    std::string_view document_text);
}