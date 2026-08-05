/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { access } from 'node:fs/promises'
import { constants } from 'node:fs'
import { join } from 'node:path'
import { execa } from 'execa'
import pc from 'picocolors'

const vcToolsComponent = 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'

async function ensureFileExists(filePath, message) {
  try {
    await access(filePath, constants.F_OK)
  } catch {
    throw new Error(`${message}\n ${filePath}`)
  }
}

function parseEnvironment(output) {
  const environment = {}
  for (const line of output.split(/\r?\n/)) {
    const separator = line.indexOf('=')
    if (separator <= 0) {
      continue
    }
    const name = line.substring(0, separator)
    environment[name] = line.substring(separator + 1)
  }
  return environment
}

function hasMsvcEnvironment() {
  return (
    process.env.VSCMD_VER !== undefined &&
    process.env.VCToolsInstallDir !== undefined
  )
}

// The VS build environment is initialized by calling vswhere.exe and VsDevCmd.bat.
// Specifically, vswhere.exe locates the Visual Studio installation path,
// while VsDevCmd.bat sets up the VC development environment, which includes configuring the relevant environment variables.
export async function getMsvcEnvironment({
  arch = 'x64',
  hostArch = 'x64',
} = {}) {
  if (process.platform !== 'win32') {
    throw new Error('MSVC environment is available only on Windows')
  }
  if (hasMsvcEnvironment()) {
    return {
      env: { ...process.env },
      info: {
        reused: true,
        developerEnvironmentVersion: process.env.VSCMD_VER,
        toolsetVersion: process.env.VCToolsVersion,
        hostArch: process.env.VSCMD_ARG_HOST_ARCH,
        targetArch: process.env.VSCMD_ARG_TGT_ARCH,
      },
    }
  }

  const programFilesX86 =
    process.env['ProgramFiles(x86)'] ?? process.env.ProgramFiles

  if (programFilesX86 === undefined) {
    throw new Error('Cannot locate the Program Files directory')
  }

  const vswhere = join(
    programFilesX86,
    'Microsoft Visual Studio',
    'Installer',
    'vswhere.exe',
  )

  await ensureFileExists(
    vswhere,
    'Cannot find vswhere.exe. Please install Visual Studio Installer.',
  )

  const { stdout } = await execa(vswhere, [
    '-latest',
    '-products',
    '*',
    '-requires',
    vcToolsComponent,
    '-format',
    'json',
    '-utf8',
  ])

  let installations

  try {
    installations = JSON.parse(stdout.replace('/^\uFEFF/', ''))
  } catch {
    throw new Error('Failed to parse the output from vswhere.exe')
  }

  if (!Array.isArray(installations) || installations.length === 0) {
    throw new Error(
      `Cannot find a Visual Studio installation with MSVC C++ tools`,
    )
  }

  const installation = installations[0]

  console.assert(`installationPath` in installation)

  const devCmd = join(
    installation.installationPath,
    'Common7',
    'Tools',
    'VsDevCmd.bat',
  )

  await ensureFileExists(
    devCmd,
    'Cannot find the Visual Studio developer command script',
  )

  const commandInterpreter = process.env.ComSpec ?? 'cmd.exe'

  const initializeCommand = [
    'chcp 936 >nul',
    `call "${devCmd}" -arch=${arch} -host_arch=${hostArch}>nul`,
    'set',
  ].join(' && ')

  const { stdout: environmentOutput } = await execa(
    commandInterpreter,
    ['/d', '/s', '/u', '/c', `"${initializeCommand}"`],
    {
      encoding: 'utf16le',
      windowsVerbatimArguments: true,
      windowsHide: true,
    },
  )

  const env = parseEnvironment(environmentOutput)
  if (env.VSCMD_VER === undefined) {
    throw new Error(`VsDevCmd.bat did not initialize the MSVC environment`)
  }
  if (env.VCToolsInstallDir === undefined) {
    throw new Error('The MSVC toolset environment was not initialized')
  }

  return {
    env,
    info: {
      reused: false,
      displayName: installation.displayName,
      version: installation.installationVersion,
      installPath: installation.installationPath,
      developerEnvironmentVersion: env.VSCMD_VER,
      toolsetVersion: env.VCToolsVersion,
      hostArch: env.VSCMD_ARG_HOST_ARCH,
      targetArch: env.VSCMD_ARG_TGT_ARCH,
    },
  }
}

export function printMsvcInfo(info) {
  const fields = info.reused
    ? [
        ['Source', 'Current developer environment'],
        ['VSCMD version', info.developerEnvironmentVersion],
        ['MSVC toolset', info.toolsetVersion],
        ['Host arch', info.hostArch],
        ['Target arch', info.targetArch],
      ]
    : [
        ['Visual Studio', info.displayName],
        ['Version', info.version],
        ['Install path', info.installPath],
        ['VSCMD version', info.developerEnvironmentVersion],
        ['MSVC toolset', info.toolsetVersion],
        ['Host arch', info.hostArch],
        ['Target arch', info.targetArch],
      ]

  const labelWidth = Math.max(...fields.map(([label]) => label.length))

  console.log()
  console.log(pc.bold(pc.cyan('MSVC Development Environment')))
  console.log(pc.dim('─'.repeat(48)))

  for (const [label, value] of fields) {
    const formattedValue =
      value === undefined ? pc.yellow('unknown') : pc.green(value)

    console.log(`  ${pc.dim(label.padStart(labelWidth))}  ${formattedValue}`)
  }

  console.log()
}
