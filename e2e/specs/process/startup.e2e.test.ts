/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { createConnection } from 'node:net'
import { setupProxyppProcess } from '@/e2e/process/proxypp-fixture'

describe('proxy++ startup', () => {
  const { getProxypp } = setupProxyppProcess()

  test('should listen on the configured endpoint after startup', async () => {
    const proxypp = getProxypp()
    assert(proxypp !== undefined)
    expect(proxypp.host).toBe('127.0.0.1')
    await expectTcpConnection(proxypp.host, proxypp.port)
  })

  function expectTcpConnection(host: string, port: number) {
    return new Promise<void>((resolve, reject) => {
      const socket = createConnection({ host, port })
      socket.once('connect', () => {
        socket.destroy()
        resolve()
      })
      socket.once('error', reject)
    })
  }
})
