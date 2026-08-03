/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { startProxyppProcess } from '@/e2e/process/proxypp-process'

export function setupProxyppProcess() {
  let proxypp: Awaited<ReturnType<typeof startProxyppProcess>> | undefined

  beforeEach(async () => {
    proxypp = await startProxyppProcess({
      args: ['http', '--bind', '127.0.0.1', '--port', '0'],
    })

    onTestFailed(async () => {
      proxypp?.printLogs()
    })
  })

  afterEach(async () => {
    await proxypp?.stop()
    proxypp = undefined
  })

  return {
    getProxypp: () => proxypp,
  }
}
