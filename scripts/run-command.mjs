/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import process from 'node:process'
import { execa } from 'execa'

export const buildDir = process.env.CMAKE_BUILD_DIR ?? 'cmake-build-debug'

export async function runCommand(command, args) {
  await execa(command, args, {
    stdio: 'inherit',
  })
}
