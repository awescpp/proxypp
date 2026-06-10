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
import { AxiosInstance } from 'axios'
import { createHttpFixtureServer } from './fixtures/express-http-server'
import { findAvailablePort } from './helpers/ports'
import { createHttpProxyClient } from './helpers/http-client'
import { setTimeout as delay } from 'node:timers/promises'

describe('HttpProxySession', () => {
  let server: HttpFixtureServer
  let proxy: ProxyProcess
  let client: AxiosInstance

  beforeAll(async () => {
    const serverBuilder = createHttpFixtureServer()

    serverBuilder.app.get('/chunked', async (request, response) => {
      response.status(200)
      response.setHeader('Content-Type', 'text/plain; charset=utf-8')
      response.setHeader('Trailer', 'X-Checksum')
      response.write('hello ') // <-- beware that here is a space followed by 'hello'
      response.write('w')
      response.write('orl')
      await delay(Math.random() * 1000)
      response.write('d!')
      response.addTrailers({
        'X-Checksum': 'foobar',
      })
      response.end()
    })

    server = await serverBuilder.start()
    const proxyPort = await findAvailablePort()
    proxy = await startProxyProcess(proxyPort)

    client = createHttpProxyClient({
      proxyHost: proxy.host,
      proxyPort: proxy.port,
      responseType: 'stream',
    })
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

  test('forward chunked **response** through proxy++ should return complete body', async () => {
    const url = `${server.origin}/chunked`
    const response = await client.get(url)
    expect(response.status).toBe(200)
    expect(response.headers['transfer-encoding']).toBe('chunked')
    const chunks: Buffer[] = []
    // response.data is a Readable stream
    // so use for await ...of to consume its chunks asynchronously
    for await (const chunk of response.data) {
      chunks.push(chunk)
    }
    const message = Buffer.concat(chunks).toString('utf-8')
    expect(message).toBe('hello world!')
  })
})
