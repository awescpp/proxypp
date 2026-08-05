/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { Command } from 'commander'
import { getMsvcEnvironment, printMsvcInfo } from './msvc-environment.mjs'
import { execa } from 'execa'

const presetAliases = {
  win32: {
    debug: 'windows-msvc-debug-local',
    release: 'windows-msvc-release-local',
    sanitizers: 'windows-msvc-asan-local',
  },
  linux: {
    debug: 'linux-gcc-debug-local',
    release: 'linux-gcc-release-local',
    sanitizers: 'linux-gcc-sanitizers-local',
    coverage: 'linux-gcc-coverage-local',
  },
}

// debug / release .etc
const knownPresetAliases = new Set(
  Object.values(presetAliases).flatMap((aliases) => Object.keys(aliases)),
)

function getPlatformPresetAliases() {
  const aliases = presetAliases[process.platform]
  if (aliases === undefined) {
    throw new Error(`Unsupported platform: ${process.platform}`)
  }
  return aliases
}

function getDefaultPreset() {
  return process.env.PROXYPP_CMAKE_PRESET ?? getPlatformPresetAliases().debug
}

function resolvePreset(name) {
  const aliases = getPlatformPresetAliases()
  if (name in aliases) {
    return aliases[name]
  }
  if (knownPresetAliases.has(name)) {
    throw new Error(
      `Preset alias ${name} is not supported on ${process.platform}`,
    )
  }
  return undefined
}

function requireMsvcEnvironment(preset) {
  return process.platform === 'win32' && preset.startsWith('windows-msvc-')
}

async function createExecaOptions(preset) {
  if (!requireMsvcEnvironment(preset)) {
    return { stdio: 'inherit' }
  }

  const { env, info } = await getMsvcEnvironment()
  printMsvcInfo(info)

  return {
    stdio: 'inherit',
    env,
    extendEnv: false,
  }
}

async function run(command, args, options) {
  console.log(`> ${command} ${args.join(' ')}`)
  await execa(command, args, options)
}

async function runCMakeAction(action, preset, extraArguments, execaOptions) {
  switch (action) {
    case 'configure':
      await run('cmake', ['--preset', preset, ...extraArguments], execaOptions)
      break
    case 'build':
      await run(
        'cmake',
        ['--build', '--preset', preset, ...extraArguments],
        execaOptions,
      )
      break
    case 'configure-build':
      await run('cmake', ['--preset', preset, ...extraArguments], execaOptions)
      await run(
        'cmake',
        ['--build', '--preset', preset, ...extraArguments],
        execaOptions,
      )
      break
    case 'test':
      await run('ctest', ['--preset', preset, ...extraArguments], execaOptions)
      break
    case 'workflow':
      await run(
        'cmake',
        ['--workflow', '--preset', preset, ...extraArguments],
        execaOptions,
      )
      break
    default:
      throw new Error(`Unsupported action: ${action}`)
  }
}

function formatPresetAliases(platform) {
  return Object.entries(presetAliases[platform])
    .map(([alias, preset]) => `  ${alias.padEnd(12)} ${preset}`)
    .join('\n')
}

function addActionCommand(program, name, description) {
  program
    .command(name)
    .description(description)
    .option('-p, --preset <name>', 'preset alias or exact CMake preset name')
    .allowUnknownOption()
    .addHelpText(
      'after',
      `
Examples:
  node scripts/cmake-presets.mjs ${name}
  node scripts/cmake-presets.mjs ${name} --preset release
  node scripts/cmake-presets.mjs ${name} --preset release --parallel

The preset defaults to PROXYPP_CMAKE_PRESET, or the platform debug preset.
Unknown options are passed directly to CMake/CTest.
Use "--" only when passing an option handled by this wrapper, such as --help.
`,
    )
    .action(async (options, command) => {
      const extraArguments = command.args
      if (
        extraArguments[0] !== undefined &&
        !extraArguments[0].startsWith('-')
      ) {
        throw new Error(
          `Unexpected positional argument: ${extraArguments[0]}. Use --preset <name> to select a preset.`,
        )
      }
      const preset = resolvePreset(options.preset) ?? getDefaultPreset()
      const execaOptions = await createExecaOptions(preset)
      await runCMakeAction(name, preset, extraArguments, execaOptions)
    })
}

function createProgram() {
  const program = new Command()

  program
    .name('cmake-presets')
    .description('Configure, build, test, and run CMake presets for proxy++.')
    .showHelpAfterError()
    .showSuggestionAfterError()
    .addHelpText(
      'after',
      `
Preset selection precedence:
  1. --preset <name>
  2. PROXYPP_CMAKE_PRESET environment variable
  3. Platform debug preset

Windows preset aliases:
${formatPresetAliases('win32')}

Linux preset aliases:
${formatPresetAliases('linux')}

Examples:
  node scripts/cmake-presets.mjs configure
  node scripts/cmake-presets.mjs build --preset release --parallel
  node scripts/cmake-presets.mjs test --preset debug --output-on-failure
  node scripts/cmake-presets.mjs workflow --preset sanitizers
`,
    )
  addActionCommand(program, 'configure', 'Configure a CMake preset.')
  addActionCommand(program, 'build', 'Build a CMake build preset.')
  addActionCommand(
    program,
    'configure-build',
    'Configure and then build the same preset.',
  )
  addActionCommand(program, 'test', 'Run a CTest preset.')
  addActionCommand(program, 'workflow', 'Run a CMake workflow preset.')

  return program
}

async function main() {
  const program = createProgram()
  if (process.argv.length === 2) {
    program.outputHelp()
    return
  }
  await program.parseAsync(process.argv)
}

try {
  await main()
} catch (error) {
  console.error('\nCMake preset command failed.')
  console.error(
    error instanceof Error ? (error.shortMessage ?? error.message) : error,
  )
  process.exitCode = 1
}
