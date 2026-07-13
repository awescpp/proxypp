/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { setTimeout } from 'node:timers/promises'
import { execa, Options } from 'execa'
import { connect } from 'node:net'
import { resolve as resolvePath } from 'node:path'
import { existsSync } from 'node:fs'
import { mkdtemp } from 'node:fs/promises'
import path from 'node:path'
import os from 'node:os'
import * as process from 'node:process'
import { writeRulesFile } from './rule-files'

export interface ProxyProcess {
  readonly executable: string
  readonly host: string
  readonly port: number
  stop(): Promise<void>
  printLog(): void
}

export async function startProxyProcess(port: number): Promise<ProxyProcess> {
  const runtimeDir = await mkdtemp(path.join(os.tmpdir(), 'proxypp-e2e-'))

  // create a sample rule file to test
  const rulesFile = await writeRulesFile(runtimeDir)

  const host = '127.0.0.1'
  const executable = resolveProxyExecutable()
  const args = [
    'http',
    '--bind',
    host,
    '--port',
    String(port),
    '--rule-file',
    rulesFile,
  ]
  const subprocess = execa(executable, args, {
    cwd: process.cwd(),
    shell: false,
    stdout: 'pipe',
    stderr: 'pipe',
    cleanup: true,
    windowsHide: true,
    reject: false,
  } as Options)

  const stdoutChunks: string[] = []
  const stderrChunks: string[] = []

  subprocess.stdout?.setEncoding('utf-8')
  subprocess.stderr?.setEncoding('utf-8')

  subprocess.stdout?.on('data', (chunk: string) => {
    stdoutChunks.push(chunk)
  })

  subprocess.stderr?.on('data', (chunk: string) => {
    stderrChunks.push(chunk)
  })

  subprocess.on('error', (error) => {
    stderrChunks.push(`[proxy++ error]: ${error.message}`)
  })

  subprocess.on('exit', (code, signal) => {
    stdoutChunks.push(`[proxy++ exit] code ${code} signal ${signal}`)
  })

  try {
    await waitUntilListening(subprocess, host, port)
  } catch (error) {
    await stopSubprocess(subprocess)
    throw new Error(
      [
        `Failed to start proxy++ on ${host}:${port}`,
        `Executable: ${executable}`,
        `Arguments: ${args.join(' ')}`,
        'stdout:',
        stdoutChunks.join('') || '<empty>',
        '',
        'stderr:',
        stderrChunks.join('') || '<empty>',
        '',
        `Reason: ${formatError(error)}`,
      ].join('\n'),
    )
  }

  console.log(`proxy++ is listening on ${host}:${port}`)

  return {
    host,
    port,
    executable,
    stop() {
      return stopSubprocess(subprocess)
    },
    printLog() {
      const stdout = stdoutChunks.join('') || '<empyt>'
      const stderr = stderrChunks.join('') || '<empty>'
      console.error('--------proxy++ stdout--------')
      console.error(stdout)
      console.error('--------proxy++ stderr--------')
      console.error(stderr)
    },
  }
}

function resolveProxyExecutable(): string {
  const configuredPath = process.env.PROXYPP_BIN
  if (configuredPath !== undefined && configuredPath.length > 0) {
    const executable = resolvePath(configuredPath)
    if (!existsSync(executable)) {
      throw new Error(`PROXYPP_BIN does not exist: ${executable}`)
    }
    return executable
  }

  const candidates =
    process.platform === 'win32'
      ? ['cmake-build-debug/proxy++.exe', 'cmake-build-release/proxy++.exe']
      : ['build/proxy++']
  for (const candidate of candidates) {
    const executable = resolvePath(candidate)
    if (existsSync(executable)) {
      return executable
    }
  }

  throw new Error(
    [
      'Can not find the proxy++ executable.',
      'Build proxy++ first or set PROXYPP_BIN.',
      '',
      'PowerShell Example:',
      "$env:PROXYPP_BIN = 'cmake-build-debug/proxy++.exe'",
    ].join('\n'),
  )
}

async function waitUntilListening(
  subprocess: ReturnType<typeof execa>,
  host: string,
  port: number,
): Promise<void> {
  const deadline = Date.now() + 5_000
  while (Date.now() < deadline) {
    if (subprocess.exitCode !== null) {
      throw new Error(
        `proxy++ exited before opening its listening port, exit code: ${subprocess.exitCode}`,
      )
    }
    if (await canConnect(host, port)) {
      return
    }
    await delay(100)
  }
  throw new Error(`Timed out waiting for proxy++ on ${host}:${port}`)
}

function canConnect(host: string, port: number): Promise<boolean> {
  return new Promise((resolve, reject) => {
    const socket = connect({ host, port })
    socket.setTimeout(200)

    socket.once('connect', () => {
      socket.destroy()
      resolve(true)
    })

    socket.once('timeout', () => {
      socket.destroy()
      resolve(false)
    })

    socket.once('error', () => {
      socket.destroy()
      resolve(false)
    })
  })
}

async function stopSubprocess(subprocess: ReturnType<typeof execa>) {
  if (subprocess.exitCode !== null) {
    await subprocess
    return
  }

  subprocess.kill('SIGTERM')

  const exited = await Promise.race([
    subprocess.then(() => true),
    delay(2_000).then(() => false),
  ])

  if (!exited && subprocess.exitCode === null) {
    subprocess.kill('SIGKILL')
    await subprocess
  }
}

async function delay(milliseconds: number) {
  return setTimeout(milliseconds)
}
function formatError(error: unknown): string {
  return error instanceof Error ? error.message : String(error)
}
