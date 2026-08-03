/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import http from 'node:http'
import { setupProxyppProcess } from '@/e2e/process/proxypp-fixture'
import { startExpressHttpServer } from '@/e2e/server/express-http-server'
import { createHttpProxyClient } from '@/e2e/client/axios-client'
import { startRawTcpServer } from '@/e2e/server/raw-tcp-server'
import { onTestFinished } from 'vitest'

describe('http forwarding message framing tests', () => {
  const { getProxypp } = setupProxyppProcess()

  test('chunked request body should be forwarded', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.post('/echo', (request, response) => {
          response.status(200).json(request.body)
        })
      },
    })

    onTestFinished(async () => {
      await server?.close()
    })

    const proxypp = getProxypp()

    const responseBody: string = await new Promise((resolve, reject) => {
      assert(proxypp !== undefined)
      const request = http.request(
        {
          host: proxypp.host,
          port: proxypp.port,
          method: 'post',
          path: `${server.origin}/echo`, // Forward proxy requests use the absolute-form request target.
          headers: {
            'Content-Type': 'application/json',
            'Transfer-Encoding': 'chunked',
          },
        },
        async (response) => {
          try {
            expect(response.statusCode).toBe(200)
            const chunks: Buffer[] = []
            for await (const chunk of response) {
              chunks.push(Buffer.from(chunk))
            }
            resolve(Buffer.concat(chunks).toString('utf-8'))
          } catch (error) {
            reject(error)
          }
        },
      )

      request.on('error', reject)

      request.write('{"message":"')
      request.write('test ')
      request.write('message')
      request.write('"}')

      request.end()
    })

    expect(JSON.parse(responseBody)).toStrictEqual({
      message: 'test message',
    })
  })

  test('chunked response body should be forwarded', async () => {
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.get('/echo', (request, response) => {
          response
            .status(200)
            .setHeader('Content-Type', 'text/plain; charset=utf-8')
            .setHeader('Trailer', 'X-Checksum')
            .write('test')
          response.write(' message')
          response.addTrailers({ 'X-Checksum': 'foobar' })
          response.end()
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
    client.defaults.responseType = 'stream'
    const response = await client.get(`${server.origin}/echo`)
    expect(response.status).toBe(200)
    // The HTTP headers in the response object returned by Axios are all lowercase.
    expect(response.headers['transfer-encoding']).toBe('chunked')

    let message = ''
    for await (const chunk of response.data) {
      message += Buffer.from(chunk).toString('utf-8')
    }

    expect(message).toBe('test message')
  })

  test('close-delimited response body should be forwarded', async () => {
    const server = await startRawTcpServer((socket) => {
      socket.on('data', (data) => {
        // Intentionally no Content-Length and no Transfer-Encoding.
        // The response body is delimited by closing the TCP connection.
        const header = [
          'HTTP/1.1 200 OK',
          'Content-Type: application/json',
          '',
          '',
        ].join('\r\n')
        socket.write(header)
        const body = JSON.stringify({
          message: 'response body sample message',
        })
        socket.write(body)
        socket.end()
      })
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

    assert(server !== undefined)
    const response = await client.get(`http://${server.host}:${server.port}/`)

    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      message: 'response body sample message',
    })
  })
})
