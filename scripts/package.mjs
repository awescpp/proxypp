/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { Command } from 'commander'
import { execa } from 'execa'

const defaultPresets = {
  win32: 'windows-msvc-debug-local',
  linux: 'linux-gcc-debug-local',
}

function getDefaultPreset() {
  if (process.env.PROXYPP_CMAKE_PRESET !== undefined) {
    return process.env.PROXYPP_CMAKE_PRESET
  }

  const preset = defaultPresets[process.platform]
  if (preset === undefined) {
    throw new Error(`Unsupported platform: ${process.platform}`)
  }

  return preset
}

async function run(command, args) {
  console.log(`> ${command} ${args.join(' ')}`)
  await execa(command, args, { stdio: 'inherit', preferLocal: true })
}

async function packageSource(options) {
  const preset = options.preset ?? getDefaultPreset()
  await run('cmake', [
    '--build',
    '--preset',
    preset,
    '--target',
    'package_source',
  ])
}

async function packageBinary() {
  throw new Error('Binary packaging is not implemented yet.')
}

function createProgram() {
  const program = new Command()
  program
    .name('package')
    .description('Package proxy++ source code and binaries.')
    .showHelpAfterError()
    .showSuggestionAfterError()

  program
    .command('source')
    .description('Create the source package.')
    .option('-p, --preset <name>', 'CMake build preset')
    .action(packageSource)

  program
    .command('binary')
    .command('binary')
    .description('Create the binary package.')
    .action(packageBinary)

  program.addHelpText(
    'after',
    `
Examples:
  node scripts/package.mjs source
  node scripts/package.mjs source --preset windows-msvc-debug-local
  node scripts/package.mjs binary
`,
  )

  return program
}

async function main() {
  const program = createProgram()

  if (process.argv.length === 2) {
    program.outputHelp()
  }

  await program.parseAsync(process.argv)
}

try {
  await main()
} catch (error) {
  console.error('\nPackage generation failed.')
  console.error(
    error instanceof Error ? (error.shortMessage ?? error.message) : error,
  )
  process.exitCode = 1
}
