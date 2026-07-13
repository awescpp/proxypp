/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { mkdir, writeFile } from 'node:fs/promises'
import path from 'node:path'

export async function writeRulesFile(runtimeDir: string): Promise<string> {
  await mkdir(runtimeDir, { recursive: true })

  const rulesFile = path.join(runtimeDir, 'rules.json')

  // basic rule file
  const rules = {
    version: 1,
  }

  await writeFile(rulesFile, `${JSON.stringify(rules, null, 2)}\n`, 'utf8')

  return rulesFile
}
