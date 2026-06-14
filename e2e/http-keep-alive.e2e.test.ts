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
import { createHttpFixtureServer } from './fixtures/express-http-server'
import { Agent, IncomingHttpHeaders, request } from 'node:http'
import { findAvailablePort } from './helpers/ports'

describe('HttpProxySession - keepalive', () => {
  let server: HttpFixtureServer
  let proxy: ProxyProcess
  let remoteConnectionCount = 0
  let requestCount = 0
  let agent: Agent

  beforeAll(async () => {
    const serverBuilder = createHttpFixtureServer()

    serverBuilder.app.get(['/first', '/second'], (request, response) => {
      requestCount += 1

      const body = JSON.stringify({
        url: request.url,
        requestCount,
      })

      response
        .status(200)
        .set('Connection', 'keep-alive')
        .set('Content-Type', 'application/json')
        .set('Content-Length', Buffer.byteLength(body).toString())
        .send(body)
    })

    if (typeof serverBuilder.onConnection !== 'function') {
      throw new Error(
        'This fixture server does not support connection tracking',
      )
    }

    serverBuilder.onConnection(() => {
      remoteConnectionCount += 1
    })

    server = await serverBuilder.start()

    agent = new Agent({ keepAlive: true, maxSockets: 1 })

    const proxyPort = await findAvailablePort()
    proxy = await startProxyProcess(proxyPort)
  })

  afterAll(async () => {
    await server?.close()
    await proxy?.stop()
    agent.destroy()
  })

  type ProxyResponse = {
    statusCode: number | undefined
    headers: IncomingHttpHeaders
    body: string
    reuseSocket: boolean
  }

  const requestViaProxy = (path: string): Promise<ProxyResponse> => {
    const chunks: Buffer[] = []
    return new Promise((resolve, reject) => {
      const req = request(
        {
          agent: agent,
          hostname: proxy.host,
          port: proxy.port,
          method: 'GET',
          path: `http://${server.host}:${server.port}${path}`,
          headers: {
            Host: `${server.host}:${server.port}`,
          },
        },
        (res) => {
          res.on('data', (chunk: Buffer) => {
            chunks.push(chunk)
          })

          res.on('end', () => {
            resolve({
              statusCode: res.statusCode,
              headers: res.headers,
              body: Buffer.concat(chunks).toString('utf-8'),
              reuseSocket: req.reusedSocket,
            })
          })

          res.on('error', reject)
        },
      )

      req.on('error', reject)
      req.end()
    })
  }

  test('keep_alive_requests_should_reuse_remote_connection', async () => {
    onTestFailed(() => proxy.printLog())

    const firstProxyResponse = await requestViaProxy('/first')
    expect(firstProxyResponse.statusCode).toBe(200)
    expect(firstProxyResponse.reuseSocket).toBe(false)
    expect(JSON.parse(firstProxyResponse.body)).toEqual({
      url: '/first',
      requestCount: 1,
    })

    const secondProxyResponse = await requestViaProxy('/second')
    expect(secondProxyResponse.statusCode).toBe(200)
    expect(secondProxyResponse.reuseSocket).toBe(true)
    expect(JSON.parse(secondProxyResponse.body)).toEqual({
      url: '/second',
      requestCount: 2,
    })

    expect(requestCount).toBe(2)
    expect(remoteConnectionCount).toBe(1)
  })
})
