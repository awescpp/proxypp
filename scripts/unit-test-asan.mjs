/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import process from 'node:process'
import { runCommand } from './run-command.mjs'

// ASan cmake options:
// -DCMAKE_TOOLCHAIN_FILE="<path-to-vcpkg.cmake>" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPROXYPP_ENABLE_ASAN=ON -DPROXYPP_BUILD_APP=OFF -DBUILD_TESTING=ON

const buildDir = 'cmake-build-relwithdebinfo-asan'

if (process.platform === 'win32') {
  await runCommand(
    'powershell',
    ['-ExecutionPolicy', 'Bypass', '-File', 'scripts/unit-test.ps1'],
    {
      env: {
        CMAKE_BUILD_DIR: buildDir,
      },
    },
  )
} else if (process.platform === 'linux') {
  await runCommand('cmake', ['--build', buildDir])
  await runCommand('ctest', ['--test-dir', buildDir, '--output-on-failure'])
} else {
  console.error(`unsupported platform ${process.platform}`)
  process.exit(-1)
}
