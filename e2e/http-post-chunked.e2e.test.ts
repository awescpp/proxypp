import {
  afterAll,
  beforeAll,
  beforeEach,
  describe,
  expect,
  onTestFailed,
  test,
} from 'vitest'
import { HttpFixtureServer } from './fixtures/fixture-server.types'
import { ProxyProcess, startProxyProcess } from './helpers/proxy-process'
import { createHttpFixtureServer } from './fixtures/express-http-server'
import { findAvailablePort } from './helpers/ports'
import * as http from 'node:http'
import express from 'express'

describe('HttpProxySession', () => {
  let server: HttpFixtureServer
  let proxy: ProxyProcess

  beforeAll(async () => {
    const serverBuilder = createHttpFixtureServer()

    serverBuilder.app.use(express.json())

    serverBuilder.app.post('/chunked', async (request, response) => {
      response.status(200).json(request.body)
    })

    server = await serverBuilder.start()
    const proxyPort = await findAvailablePort()
    proxy = await startProxyProcess(proxyPort)
  })

  beforeEach(() => {
    onTestFailed(() => {
      proxy.printLog()
    })
  })

  afterAll(async () => {
    await proxy?.stop()
    await server?.close()
  })

  test('forward chunked **request** through proxy++ should return correct response', async () => {
    const responseBody = await new Promise<string>((resolve, reject) => {
      const request = http.request(
        {
          host: proxy.host,
          port: proxy.port,
          method: 'POST',

          // Forward proxy requires absolute-form.
          path: `${server.origin}/chunked`,

          headers: {
            'Content-Type': 'application/json',
          },
        },
        async (response) => {
          try {
            expect(response.statusCode).toBe(200)
            const chunks: Buffer[] = []
            for await (const chunk of response) {
              chunks.push(Buffer.from(chunk))
            }
            resolve(Buffer.concat(chunks).toString('utf8'))
          } catch (error) {
            reject(error)
          }
        },
      )

      request.on('error', reject)

      // Because Content-Length is not set, Node.js will use
      // Transfer-Encoding: chunked for these streamed writes.
      request.write('{"message":')
      request.write('"proxy++"')
      request.write('}')

      request.end()
    })

    expect(JSON.parse(responseBody)).toEqual({
      message: 'proxy++',
    })
  })
})
