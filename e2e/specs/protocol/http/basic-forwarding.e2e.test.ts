/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { startExpressHttpServer } from '@/e2e/server/express-http-server'
import { createHttpProxyClient } from '@/e2e/client/axios-client'
import { startRawTcpServer } from '@/e2e/server/raw-tcp-server'
import { setupProxyppProcess } from '@/e2e/process/proxypp-fixture'

describe('http basic forwarding tests', () => {
  const { getProxypp } = setupProxyppProcess()

  function getHttpProxyClient() {
    const proxypp = getProxypp()
    assert(proxypp !== undefined)
    return createHttpProxyClient({
      proxyHost: proxypp.host,
      proxyPort: proxypp.port,
    })
  }

  test('GET request should be forwarded', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.get('/echo', (request, response) => {
          response.status(200).json({
            message: 'test message',
            method: request.method,
            path: request.path,
          })
        })
      },
    })

    onTestFinished(async () => {
      await server.close()
    })

    const client = getHttpProxyClient()
    const response = await client.get(`${server.origin}/echo`)
    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      message: 'test message',
      method: 'GET',
      path: '/echo',
    })
  })

  test('POST request with Content-Length should be forwarded', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.post('/echo', (request, response) => {
          return response.status(200).json({
            method: request.method,
            path: request.path,
            body: request.body,
            contentLength: request.headers['content-length'],
            transferEncoding: request.headers['transfer-encoding'],
          })
        })
      },
    })

    onTestFinished(async () => {
      await server?.close()
    })

    const requestBody = {
      message: 'test message',
      value: 42,
    }
    const client = getHttpProxyClient()
    const response = await client.post(`${server.origin}/echo`, requestBody)
    expect(response.status).toBe(200)
    expect(response.data.method).toBe('POST')
    expect(response.data.path).toBe('/echo')
    expect(Number(response.data.contentLength)).toBeGreaterThan(0)
    expect(response.data.transferEncoding).toBeUndefined()
    expect(response.data.body).toStrictEqual(requestBody)
  })

  test('should convert absolute-form request target to origin-form', async () => {
    onTestFinished(async () => {
      await server?.close()
    })

    let receivedRequest = ''

    const server = await startRawTcpServer((socket) => {
      let buffer = ''
      socket.on('data', (data) => {
        buffer += data.toString('utf-8')

        // \r\n\r\n represents the end of the HTTP header
        const headerEnd = buffer.indexOf('\r\n\r\n')
        if (headerEnd === -1) {
          return
        }
        // Here we only fetch the HTTP header.
        receivedRequest = buffer.substring(0, headerEnd)
        socket.end(
          ['HTTP/1.1 200 OK', 'Content-Length: 0', '', ''].join('\r\n'),
        )
      })
    })

    const client = getHttpProxyClient()

    assert(server !== undefined)
    const response = await client.get(
      `http://${server.host}:${server.port}/echo?name=proxypp`,
    )
    assert(receivedRequest.includes('\r\n'))
    const requestLine = receivedRequest.split('\r\n')[0]

    expect(response.status).toBe(200)
    expect(requestLine).toBe('GET /echo?name=proxypp HTTP/1.1')
  })

  test('response status and body should be preserved', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.post('/echo', (request, response) => {
          response.status(201).json({
            code: 0,
            message: 'resource created',
            items: [1, 2, 3],
          })
        })
      },
    })

    onTestFinished(async () => {
      await server?.close()
    })

    const client = getHttpProxyClient()
    const response = await client.post(`${server.origin}/echo`)
    expect(response.status).toBe(201)
    expect(response.data).toStrictEqual({
      code: 0,
      message: 'resource created',
      items: [1, 2, 3],
    })
  })
})
