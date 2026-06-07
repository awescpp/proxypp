import { createServer, type Server } from 'node:http'

import express, { type Express } from 'express'

import type {
  HttpFixtureServer,
  FixtureServerBuilder,
} from './fixture-server.types'
import { listenOnAvailablePort } from './server-utils'

export interface ExpressHttpFixtureServerBuilder extends FixtureServerBuilder<HttpFixtureServer> {
  readonly app: Express
}

export function createHttpFixtureServer(): ExpressHttpFixtureServerBuilder {
  const host = '127.0.0.1'
  const app = createFixtureApplication()

  let started = false

  return {
    app,

    async start(): Promise<HttpFixtureServer> {
      if (started) {
        throw new Error('HTTP fixture server has already been started')
      }

      started = true

      const server = createServer(app)

      let port

      try {
        port = await listenOnAvailablePort(server, host)
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
          return closeServer(server)
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
