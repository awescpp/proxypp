/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { describe, expect, onTestFinished } from 'vitest'
import { fileURLToPath } from 'node:url'
import { setupProxyppProcess } from '@/e2e/process/proxypp-fixture'
import { startExpressHttpServer } from '@/e2e/server/express-http-server'
import { createHttpProxyClient } from '@/e2e/client/axios-client'

const ruleFile = fileURLToPath(
  new URL('./header-modification.rules.json', import.meta.url),
)

describe('http header modification rule tests', () => {
  // Use the specified rule file to launch proxy++
  const { getProxypp } = setupProxyppProcess(ruleFile)

  function getHttpProxyClient() {
    const proxypp = getProxypp()
    assert(proxypp !== undefined)

    return createHttpProxyClient({
      proxyHost: proxypp.host,
      proxyPort: proxypp.port,
    })
  }

  test('request phase should set request header', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.get('/echo', (request, response) => {
          response.status(200).json({
            requestHeader: request.headers['x-proxypp-e2e-request'],
          })
        })
      },
    })

    onTestFinished(async () => {
      await server?.close()
    })

    const client = getHttpProxyClient()

    const response = await client.get(`${server.origin}/echo`)
    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      requestHeader: 'request-rule-applied',
    })
  })

  test('response phase rule should set response header', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.get('/echo', (request, response) => {
          response.status(200).json({
            message: 'test message',
          })
        })
      },
    })

    onTestFinished(async () => {
      await server?.close()
    })

    const client = getHttpProxyClient()

    const response = await client.get(`${server.origin}/echo`)
    expect(response.status).toBe(200)
    expect(response.headers['x-proxypp-e2e-response']).toBe(
      'response-rule-applied',
    )
    expect(response.data).toStrictEqual({
      message: 'test message',
    })
  })
})
