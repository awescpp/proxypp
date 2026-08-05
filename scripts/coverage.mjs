/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { execa } from 'execa'
import { fileURLToPath } from 'node:url'
import { dirname, resolve } from 'node:path'
import { mkdir, rm } from 'node:fs/promises'
import { glob } from 'glob'
import open from 'open'
import { Command } from 'commander'

const scriptFile = fileURLToPath(import.meta.url)
const scriptDir = dirname(scriptFile)
const projectRoot = resolve(scriptDir, '..')

const defaultPreset = 'linux-gcc-coverage-local'
const defaultBrowser = 'firefox'

function createProgram() {
  return new Command()
    .name('coverage')
    .description('Generate a GCC/lcov coverage report for proxy++.')
    .option(
      '-p, --preset <name>',
      'CMake configure/build/test preset',
      defaultPreset,
    )
    .option('--no-clean', 'reuse the existing coverage build directory')
    .option(
      '-o, --open [browser]',
      `open the generated HTML report (default browser: ${defaultBrowser})`,
    )
    .showHelpAfterError()
    .addHelpText(
      'after',
      `
Examples:
  node scripts/coverage.mjs
  node scripts/coverage.mjs --no-clean
  node scripts/coverage.mjs --open
  node scripts/coverage.mjs --open chromium
  node scripts/coverage.mjs --preset linux-gcc-coverage-local
`,
    )
}

function printStep(message) {
  console.log(`\n==>${message}`)
}

async function run(command, args) {
  await execa(command, args, {
    cwd: projectRoot,
    stdio: 'inherit',
    preferLocal: 'true',
  })
}

async function requireCommand(command) {
  try {
    await execa(command, ['--version'], {
      cwd: projectRoot,
      stdout: 'ignore',
      stderr: 'ignore',
      preferLocal: true,
    })
  } catch {
    throw new Error(
      `Required command "${command}" was not found or could not be executed.`,
    )
  }
}

async function ensureCoverageData(buildDir) {
  const files = await glob('**/*.gcda', { cwd: buildDir, absolute: true })
  if (files.length === 0) {
    throw new Error(
      `No .gcda files were generated after running tests: ${buildDir}`,
    )
  }
  console.log(`Found ${files.length} runtime coverage data files`)
}

async function openCoverageReport(reportFile, browserOption) {
  // browserOption is a boolean or string
  const browser = browserOption === true ? defaultBrowser : browserOption
  await open(reportFile, { app: { name: browser } })
}

async function generateCoverageReport(options) {
  if (process.platform !== 'linux') {
    throw new Error('Coverage testing is supported only on Linux.')
  }

  const preset = options.preset
  const shouldClean = options.clean
  const browserOption = options.open

  const buildDir = resolve(projectRoot, 'out', 'build', preset)
  const coverageDir = resolve(projectRoot, 'out', 'coverage', preset)
  const htmlDir = resolve(coverageDir, 'html')

  const baseInfo = resolve(coverageDir, 'coverage-base.info')
  const testInfo = resolve(coverageDir, 'coverage-test.info')
  const allInfo = resolve(coverageDir, 'coverage-all.info')
  const finalInfo = resolve(coverageDir, 'coverage.info')
  const reportFile = resolve(htmlDir, 'index.html')

  printStep('Checking required tools')

  await Promise.all([
    requireCommand('cmake'),
    requireCommand('ctest'),
    requireCommand('lcov'),
    requireCommand('genhtml'),
  ])

  if (shouldClean) {
    printStep('Cleaning previous coverage build')
    await Promise.all([
      rm(buildDir, { recursive: true, force: true }),
      rm(coverageDir, { recursive: true, force: true }),
    ])
  } else {
    printStep('Reusing existing coverage build')
    await rm(coverageDir, { recursive: true, force: true })
  }

  await mkdir(coverageDir, { recursive: true })

  printStep(`Configuration preset: ${preset}`)
  await run('cmake', ['--preset', preset])

  printStep(`Building preset: ${preset}`)
  await run('cmake', ['--build', '--preset', preset, '--parallel'])

  printStep('Capturing zero-coverage baseline')
  await run('lcov', [
    '--capture',
    '--initial',
    '--directory',
    buildDir,
    '--output-file',
    baseInfo,
    '--branch-coverage',
    '--rc',
    'geninfo_unexecuted_blocks=1',
    '--ignore-errors',
    'mismatch,mismatch',
  ])

  printStep('Resetting runtime coverage counters')
  await run('lcov', ['--zerocounters', '--directory', buildDir])

  printStep('Running unit tests')
  await run('ctest', ['--preset', preset])

  await ensureCoverageData(buildDir)

  printStep('Capturing test coverage')
  await run('lcov', [
    '--capture',
    '--directory',
    buildDir,
    '--output-file',
    testInfo,
    '--branch-coverage',
    '--rc',
    'geninfo_unexecuted_blocks=1',
    '--ignore-errors',
    'mismatch,mismatch',
  ])

  printStep('Combining baseline and test coverage')
  await run('lcov', [
    '--add-tracefile',
    baseInfo,
    '--add-tracefile',
    testInfo,
    '--output-file',
    allInfo,
    '--branch-coverage',
  ])

  printStep('Filtering proxy++ production sources')
  await run('lcov', [
    '--extract',
    allInfo,
    `${resolve(projectRoot, 'src', 'proxypp')}/*`,
    '--output-file',
    finalInfo,
    '--branch-coverage',
  ])

  printStep('Generating HTML coverage report')
  await run('genhtml', [
    finalInfo,
    '--output-directory',
    htmlDir,
    '--branch-coverage',
    '--demangle-cpp',
    '--legend',
    '--flat',
    '--prefix',
    projectRoot,
    '--title',
    'proxy++ Coverage',
  ])

  printStep('Coverage summary')
  await run('lcov', ['--summary', finalInfo, '--branch-coverage'])

  console.log('\nCoverage report generated successfully')
  console.log(`    ${reportFile}`)

  if (browserOption !== undefined) {
    printStep('Opening coverage report')
    await openCoverageReport(reportFile, browserOption)
  }
}

async function main() {
  const program = createProgram()
  await program.parseAsync(process.argv)
  await generateCoverageReport(program.opts())
}

try {
  await main()
} catch (error) {
  console.error('\nCoverage generation failed.')
  if (error instanceof Error) {
    console.error(error.shortMessage ?? error.message)
  } else {
    console.error(error)
  }
  process.exitCode = 1
}
