/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { execa } from 'execa'
import { setTimeout as delay } from 'node:timers/promises'
import { resolve as resolvePath, join } from 'node:path'
import { existsSync } from 'node:fs'
import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises'
import { tmpdir } from 'node:os'

const readyTimeoutMilliseconds = 5_000

interface StartProxyppProcessOptions {
  readonly args: readonly string[]
  readonly readyFile?: string
  readonly ruleFile?: string
}

interface ProcessFiles {
  readonly ruleFile: string
  readonly readyFile: string
  cleanup(): Promise<void>
}

type ProxyppSubprocess = ReturnType<typeof execa>

async function waitForReadyFile(
  subprocess: ProxyppSubprocess,
  readyFile: string,
) {
  const deadline = Date.now() + readyTimeoutMilliseconds
  while (Date.now() < deadline) {
    if (subprocess.exitCode !== null || subprocess.signalCode !== null) {
      throw new Error(
        `proxy++ exited before becoming ready, exit code: ${subprocess.exitCode}`,
      )
    }

    try {
      const content = await readFile(readyFile, 'utf-8')
      return parseReadyFile(content)
    } catch (error) {
      if (!isFileNotFound(error)) {
        throw error
      }
    }

    await delay(50)
  }

  throw new Error(`Timed out waiting for proxy++ ready file: ${readyFile}`)
}

async function prepareProcessFiles(
  options: StartProxyppProcessOptions,
): Promise<ProcessFiles> {
  let temporaryDirectory: string | undefined

  const getTemporaryDirectory = async () => {
    if (temporaryDirectory === undefined) {
      temporaryDirectory = await mkdtemp(join(tmpdir(), 'proxypp-e2e-'))
    }
    return temporaryDirectory
  }

  try {
    const ruleFile =
      options.ruleFile === undefined
        ? join(await getTemporaryDirectory(), 'rules.json')
        : resolvePath(options.ruleFile)

    const readyFile =
      options.readyFile === undefined
        ? join(await getTemporaryDirectory(), 'ready.json')
        : resolvePath(options.readyFile)

    if (options.ruleFile === undefined) {
      await writeFile(
        ruleFile,
        JSON.stringify({
          $schema: 'https://proxypp.net/schemas/proxypp_rules_schema_v1.json',
          version: 1,
          http: { rules: [] },
        }),
        'utf-8',
      )
    }

    await rm(readyFile, { force: true })

    return {
      ruleFile,
      readyFile,
      async cleanup() {
        if (temporaryDirectory === undefined) return
        await rm(temporaryDirectory, { force: true, recursive: true })
      },
    }
  } catch (error) {
    if (temporaryDirectory !== undefined) {
      await rm(temporaryDirectory, { force: true, recursive: true }).catch(
        () => {},
      )
    }
    throw error
  }
}

function parseReadyFile(content: string) {
  let value: unknown
  try {
    value = JSON.parse(content)
  } catch (error) {
    throw new Error(`Failed to parse proxy++ ready file: ${formatError(error)}`)
  }

  if (typeof value !== 'object' || value === null) {
    throw new Error('The proxy++ ready file must contain a JSON object')
  }

  const record = value as Record<string, unknown>
  const host = record.host
  const port = record.port

  if (typeof host !== 'string' || host.length === 0) {
    throw new Error('The proxy++ ready file contains an invalid host')
  }

  if (
    typeof port !== 'number' ||
    !Number.isInteger(port) ||
    port < 1 ||
    port > 65535
  ) {
    throw new Error('The proxy++ ready file contains an invalid port')
  }

  return { host, port }
}

function resolveProxyppExecutable(): string {
  const givenPath = process.env.PROXYPP_BIN
  if (givenPath !== undefined && givenPath.length > 0) {
    const executable = resolvePath(givenPath)
    if (!existsSync(executable)) {
      throw new Error(`PROXYPP_BIN does not exist: ${executable}`)
    }
    return executable
  }

  const candidates =
    process.platform === 'win32'
      ? [
          'out/build/windows-msvc-debug-local/proxy++.exe',
          'out/build/windows-msvc-release-local/proxy++.exe',
        ]
      : [
          'out/build/linux-gcc-debug-local/proxy++',
          'out/build/linux-gcc-release-local/proxy++',
        ]

  for (const candidate of candidates) {
    const executable = resolvePath(candidate)
    if (existsSync(executable)) {
      return executable
    }
  }

  throw new Error(
    [
      'Cannot find the proxy++ executable.',
      'Build proxy++ first or set PROXYPP_BIN.',
      '',
      'PowerShell example:',
      "$env:PROXYPP_BIN = 'out/build/windows-msvc-debug-local/proxy++.exe'",
      '',
      'Bash example:',
      "export PROXYPP_BIN='out/build/linux-gcc-debug-local/proxy++'",
    ].join('\n'),
  )
}

async function stopSubprocess(subprocess: ProxyppSubprocess): Promise<void> {
  if (subprocess.exitCode !== null || subprocess.signalCode !== null) {
    // wait process exit
    await subprocess
    return
  }
  subprocess.kill()
}

function isFileNotFound(error: unknown) {
  return (
    typeof error === 'object' &&
    error !== null &&
    'code' in error &&
    error.code === 'ENOENT'
  )
}

function formatError(error: unknown) {
  return error instanceof Error ? error.message : String(error)
}

function assertManagedOptionsNotPresent(args: readonly string[]) {
  for (const arg of args) {
    if (arg === '--rule-file' || arg.startsWith('--rule-file=')) {
      throw new Error(
        'Pass ruleFile through StartProxyppProcessOptions.ruleFile',
      )
    }

    if (arg === '--ready-file' || arg.startsWith('--ready-file=')) {
      throw new Error(
        'Pass readyFile through StartProxyppProcessOptions.readyFile',
      )
    }
  }
}

export async function startProxyppProcess(options: StartProxyppProcessOptions) {
  assertManagedOptionsNotPresent(options.args)
  const executable = resolveProxyppExecutable()
  const files = await prepareProcessFiles(options)
  const args = [
    ...options.args,
    '--ready-file',
    files.readyFile,
    '--rule-file',
    files.ruleFile,
  ]

  // check execa options: https://github.com/sindresorhus/execa/blob/main/docs/api.md#options-1
  const subprocess = execa(executable, args, {
    stdout: 'pipe',
    stderr: 'pipe',
    reject: false,
  })

  const stdoutChunks: string[] = []
  const stderrChunks: string[] = []

  let processError: Error | undefined
  let exitCode: number | null = null
  let exitSignal: NodeJS.Signals | null = null

  subprocess.stdout?.setEncoding('utf-8')
  subprocess.stderr?.setEncoding('utf-8')
  subprocess.stdout?.on('data', (chunk: string) => {
    stdoutChunks.push(chunk)
  })

  subprocess.stderr?.on('data', (chunk: string) => {
    stderrChunks.push(chunk)
  })

  subprocess.once('error', (error) => (processError = error))
  subprocess.once('exit', (code, signal) => {
    exitCode = code
    exitSignal = signal
  })

  let endpoint

  try {
    endpoint = await waitForReadyFile(subprocess, files.readyFile)
  } catch (error) {
    try {
      await stopSubprocess(subprocess)
    } finally {
      await files.cleanup()
    }
    throw new Error(
      [
        'Failed to start proxy++.',
        `Executable: ${executable}`,
        `Arguments: ${args.join(' ')}`,
        `Ready file: ${files.readyFile}`,
        `Exit code: ${exitCode ?? '<not exited>'}`,
        `Exit signal: ${exitSignal ?? '<none>'}`,
        processError === undefined
          ? undefined
          : `Process error: ${processError.message}`,
        '',
        'stdout:',
        stdoutChunks.join('') || '<empty>',
        '',
        'stderr:',
        stderrChunks.join('') || '<empty>',
        '',
        `Reason: ${formatError(error)}`,
      ]
        .filter((line) => line !== undefined)
        .join('\n'),
    )
  }

  let stopped = false

  return {
    host: endpoint.host,
    port: endpoint.port,

    async stop() {
      if (stopped) return
      stopped = true
      try {
        await stopSubprocess(subprocess)
      } finally {
        await files.cleanup()
      }
    },

    printLogs() {
      console.error('-------- proxy++ stdout --------')
      console.error(stdoutChunks.join('') || '<empty>')
      console.error('-------- proxy++ stderr --------')
      console.error(stderrChunks.join('') || '<empty>')
      console.error('--------------------------------')
    },
  }
}
