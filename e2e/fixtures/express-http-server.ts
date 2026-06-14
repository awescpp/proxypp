import { createServer as createHttpServer, type Server } from 'node:http'
import { createServer as createHttpsServer } from 'node:https'
import express, { type Express } from 'express'

import type {
  HttpFixtureServer,
  FixtureServerBuilder,
} from './fixture-server.types'
import { listenOnAvailablePort } from './server-utils'
import { fileURLToPath } from 'node:url'
import path from 'node:path'
import { readFileSync } from 'node:fs'

export interface ExpressHttpFixtureServerBuilder extends FixtureServerBuilder<HttpFixtureServer> {
  readonly app: Express
}

export function createHttpFixtureServer(
  useHttps = false,
): ExpressHttpFixtureServerBuilder {
  const host = '127.0.0.1'
  const app = createFixtureApplication()
  const connectionListeners: Array<() => void> = []

  let started = false

  return {
    app,
    onConnection(listener) {
      connectionListeners.push(listener)
    },
    async start(): Promise<HttpFixtureServer> {
      if (started) {
        throw new Error('HTTP fixture server has already been started')
      }

      started = true

      let httpServer

      if (useHttps) {
        const __filename = fileURLToPath(import.meta.url)
        const __dirname = path.dirname(__filename)
        const certDir = path.resolve(__dirname, './certs')
        httpServer = createHttpsServer(
          {
            key: readFileSync(path.join(certDir, 'localhost.key')),
            cert: readFileSync(path.join(certDir, 'localhost.crt')),
          },
          app,
        )
      } else {
        httpServer = createHttpServer(app)
      }

      httpServer.on('connection', () => {
        for (const listener of connectionListeners) {
          listener()
        }
      })

      let port

      try {
        port = await listenOnAvailablePort(httpServer, host)
      } catch (error) {
        started = false
        throw error
      }
      return {
        transport: 'tcp',
        protocol: 'http',
        host,
        port,
        origin: `http://${host}:${port}`,
        close() {
          return closeServer(httpServer)
        },
      }
    },
  }
}

function createFixtureApplication(): Express {
  const app = express()
  app.disable('x-powered-by')
  return app
}

function closeServer(server: Server): Promise<void> {
  return new Promise<void>((resolve, reject) => {
    if (!server.listening) {
      resolve()
      return
    }

    server.close((error) => {
      if (error !== undefined) {
        reject(error)
        return
      }

      resolve()
    })
  })
}
