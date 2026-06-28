/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define PROXYPP_DISABLE_COPY(Class)                                           \
  Class(const Class& other) = delete;                                         \
  Class& operator=(const Class& other) = delete

#define PROXYPP_DISABLE_MOVE(Class)                                           \
  Class(Class&& other) = delete;                                              \
  Class& operator=(Class&& other) = delete

#define PROXYPP_DISABLE_COPY_AND_MOVE(Class)                                  \
  PROXYPP_DISABLE_COPY(Class);                                                \
  PROXYPP_DISABLE_MOVE(Class)

#define PROXYPP_DEFAULT_MOVE(Class)                                           \
  Class(Class&& other) noexcept = default;                                    \
  Class& operator=(Class&& other) noexcept = default