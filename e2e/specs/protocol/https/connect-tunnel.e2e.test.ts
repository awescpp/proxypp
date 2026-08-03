/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { setupProxyppProcess } from '@/e2e/process/proxypp-fixture'
import { startExpressHttpServer } from '@/e2e/server/express-http-server'
import https from 'node:https'
import { createHttpProxyClient } from '@/e2e/client/axios-client'
import { expect } from 'vitest'

describe('https tunnel tests', () => {
  const { getProxypp } = setupProxyppProcess()

  test('forward https request via CONNECT tunnel should return response', async () => {
    const server = await startExpressHttpServer({
      protocol: 'https',
      configure: (app) => {
        app.get('/echo', (request, response) => {
          response.status(200).json({
            method: request.method,
            path: request.path,
            host: request.headers.host,
            message: 'test message',
          })
        })
      },
    })

    onTestFinished(async () => {
      await server?.close()
    })

    const proxypp = getProxypp()
    assert(proxypp !== undefined)

    const client = createHttpProxyClient({
      proxyHost: proxypp.host,
      proxyPort: proxypp.port,
    })

    const response = await client.get(`${server.origin}/echo`, {
      httpsAgent: new https.Agent({ rejectUnauthorized: false }),
    })

    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      method: 'GET',
      path: '/echo',
      host: `${server.host}:${server.port}`,
      message: 'test message',
    })
  })
})
