/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import express, { type Express } from 'express'
import http from 'node:http'
import https from 'node:https'
import { readFileSync } from 'node:fs'
import type { Socket } from 'node:net'
import { listenOnEphemeralPort, closeServer } from './utils/server-utils'

const host = '127.0.0.1'

interface StartExpressHttpServerOptions {
  readonly protocol?: 'http' | 'https'
  readonly json?: boolean
  readonly configure: (app: Express) => void
  readonly onConnection?: () => void
}

export async function startExpressHttpServer(
  options: StartExpressHttpServerOptions,
) {
  const protocol = options.protocol ?? 'http'
  const json = options.json ?? true

  const app = express()
  app.disable('x-powered-by')
  if (json) {
    app.use(express.json())
  }

  options.configure(app)

  const httpsOptions = {
    key: readFileSync(new URL('./certs/localhost.key', import.meta.url)),
    cert: readFileSync(new URL('./certs/localhost.crt', import.meta.url)),
  }

  const server =
    protocol === 'https'
      ? https.createServer(httpsOptions, app)
      : http.createServer(app)

  const sockets = new Set<Socket>()

  server.on('connection', (socket) => {
    sockets.add(socket)

    socket.on('close', () => {
      sockets.delete(socket)
    })

    if (options?.onConnection) {
      options.onConnection()
    }
  })

  try {
    // start server listening
    const port = await listenOnEphemeralPort(server, host)
    let closed = false

    return {
      host,
      port,
      origin: `${protocol}://${host}:${port}`,

      async close() {
        if (closed) return
        closed = true
        await closeServer(server, sockets)
      },
    }
  } catch (error) {
    await closeServer(server, sockets)
    throw error
  }
}
