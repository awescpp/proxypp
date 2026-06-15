/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import {
  afterAll,
  beforeAll,
  describe,
  expect,
  onTestFailed,
  test,
} from 'vitest'
import { HttpFixtureServer } from './fixtures/fixture-server.types'
import { ProxyProcess, startProxyProcess } from './helpers/proxy-process'
import { AxiosInstance } from 'axios'
import { createHttpFixtureServer } from './fixtures/express-http-server'
import { findAvailablePort } from './helpers/ports'
import { createHttpProxyClient } from './helpers/http-client'
import https from 'node:https'

describe('HttpProxySession - HTTPS Tunnel proxy', () => {
  let server: HttpFixtureServer
  let proxy: ProxyProcess
  let client: AxiosInstance

  beforeAll(async () => {
    const serverBuilder = createHttpFixtureServer(true)

    serverBuilder.app.get('/service', (request, response) => {
      response.status(200).json({
        message: 'a secured text from a https server',
        method: request.method,
        path: request.path,
        host: request.headers.host,
      })
    })

    server = await serverBuilder.start()

    const proxyPort = await findAvailablePort()
    proxy = await startProxyProcess(proxyPort)

    client = createHttpProxyClient({
      proxyHost: proxy.host,
      proxyPort: proxy.port,
    })
  })

  afterAll(async () => {
    await proxy?.stop()
    await server?.close()
  })

  test('forward https request via CONNECT tunnel should return response', async () => {
    onTestFailed(() => proxy.printLog())

    const response = await client.get(
      `https://${server.host}:${server.port}/service`,
      {
        httpsAgent: new https.Agent({
          rejectUnauthorized: false,
        }),
      },
    )
    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      message: 'a secured text from a https server',
      method: 'GET',
      path: '/service',
      host: `${server.host}:${server.port}`,
    })
  })
})
