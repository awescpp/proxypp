/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { setupProxyppProcess } from '@/e2e/process/proxypp-fixture'
import { startExpressHttpServer } from '@/e2e/server/express-http-server'
import http from 'node:http'
import { expect, onTestFinished } from 'vitest'

describe('connection lifecycle tests', () => {
  const { getProxypp } = setupProxyppProcess()

  test('should reuse the same connection for multiple requests', async () => {
    let requestCount = 0
    let connectionCount = 0
    const server = await startExpressHttpServer({
      configure: (app) => {
        app.get(['/first', '/second'], (request, response) => {
          requestCount += 1

          const body = JSON.stringify({
            url: request.url,
            requestCount,
          })

          response
            .status(200)
            .setHeader('Connection', 'keep-alive')
            .setHeader('Content-Type', 'application/json')
            .setHeader('Content-Length', Buffer.byteLength(body).toString())
            .end(body)
        })
      },
      onConnection: () => {
        connectionCount += 1
      },
    })

    const proxypp = getProxypp()
    assert(proxypp !== undefined)

    type ProxyResponse = {
      statusCode: number | undefined
      headers: http.IncomingHttpHeaders
      body: string
      reuseSocket: boolean
    }
    const agent = new http.Agent({ keepAlive: true, maxSockets: 1 })
    onTestFinished(async () => {
      agent.destroy()
      await server?.close()
    })

    const requestViaProxy = (path: string): Promise<ProxyResponse> => {
      assert(proxypp !== undefined)
      const chunks: Buffer[] = []
      return new Promise((resolve, reject) => {
        const request = http.request(
          {
            agent,
            hostname: proxypp.host, // request host
            port: proxypp.port, // request port
            method: 'GET',
            path: `http://${server.host}:${server.port}${path}`, // request path (to the proxy)
            headers: {
              host: `${server.host}:${server.port}`,
            },
          },
          (response) => {
            response.on('error', reject)
            response.on('end', () => {
              const body = Buffer.concat(chunks).toString('utf-8')
              resolve({
                statusCode: response.statusCode,
                headers: response.headers,
                body,
                // Check if it's a reusable socket
                reuseSocket: request.reusedSocket,
              })
            })
            response.on('data', (chunk: Buffer) => {
              chunks.push(chunk)
            })
          },
        )
        request.on('error', reject)
        request.end()
      })
    }

    const firstResponse = await requestViaProxy('/first')
    expect(firstResponse.statusCode).toBe(200)
    expect(firstResponse.reuseSocket).toBe(false)
    expect(JSON.parse(firstResponse.body)).toStrictEqual({
      url: '/first',
      requestCount: 1,
    })

    const secondResponse = await requestViaProxy('/second')
    expect(secondResponse.statusCode).toBe(200)
    expect(secondResponse.reuseSocket).toBe(true)
    expect(JSON.parse(secondResponse.body)).toStrictEqual({
      url: '/second',
      requestCount: 2,
    })

    expect(requestCount).toBe(2)
    expect(connectionCount).toBe(1)
  })
})
