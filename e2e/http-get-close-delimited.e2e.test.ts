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
import { TcpFixtureServer } from './fixtures/fixture-server.types'
import { ProxyProcess, startProxyProcess } from './helpers/proxy-process'
import { AxiosInstance } from 'axios'
import { createTcpFixtureServer } from './fixtures/tcp-server'
import { findAvailablePort } from './helpers/ports'
import { createHttpProxyClient } from './helpers/http-client'

describe('HttpProxySession - close delimited body', () => {
  let server: TcpFixtureServer
  let proxy: ProxyProcess
  let client: AxiosInstance
  let receivedRequest = ''
  beforeAll(async () => {
    const serverBuilder = createTcpFixtureServer()
    serverBuilder.onConnection((socket) => {
      socket.once('data', (data) => {
        receivedRequest += data.toString()
        // Intentionally no Content-Length and no Transfer-Encoding.
        // The response body is delimited by closing the TCP connection.
        // send http header
        socket.write('HTTP/1.1 200 OK\r\n')
        socket.write('Content-Type: application/json\r\n')
        socket.write('\r\n')
        // send body
        socket.write(
          JSON.stringify({
            message: 'close-delimited body',
          }),
        )
        // close connection
        socket.end()
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

  test('close-delimited response body should forward to client', async () => {
    const url = `http://${server.host}:${server.port}/`

    onTestFailed(() => {
      proxy.printLog()
    })

    const response = await client.get(url)

    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      message: 'close-delimited body',
    })
    expect(receivedRequest).toContain('GET / HTTP/1.1')
  })
})
