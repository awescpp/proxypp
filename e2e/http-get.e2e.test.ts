import { afterAll, beforeAll, describe, expect, test } from 'vitest'
import { HttpFixtureServer } from './fixtures/fixture-server.types'
import { ProxyProcess, startProxyProcess } from './helpers/proxy-process'
import { AxiosInstance } from 'axios'
import { createHttpFixtureServer } from './fixtures/express-http-server'
import { findAvailablePort } from './helpers/ports'
import { createHttpProxyClient } from './helpers/http-client'

describe('HttpProxySession', () => {
  let server: HttpFixtureServer
  let proxy: ProxyProcess
  let client: AxiosInstance

  beforeAll(async () => {
    const serverBuilder = createHttpFixtureServer()

    serverBuilder.app.get('/hello', (request, response) => {
      response.status(200).json({
        message: 'hello from proxy++ e2e',
        method: request.method,
        path: request.path,
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

  test('forward get request should return response', async () => {
    const url = `${server.origin}/hello`
    const response = await client.get(url)
    expect(response.status).toBe(200)
    expect(response.data).toStrictEqual({
      message: 'hello from proxy++ e2e',
      method: 'GET',
      path: '/hello',
    })
  })

  test('request with unavailable proxy should fail', async () => {
    const unavailableClient = createHttpProxyClient({
      proxyHost: proxy.host,
      proxyPort: 8848,
    })

    await expect(
      unavailableClient.get(`${server.origin}/hello`),
    ).rejects.toThrow()
  })
})
