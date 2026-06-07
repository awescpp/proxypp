import { createServer, type Server, type Socket } from 'node:net'

import type {
  TcpFixtureServer,
  TcpFixtureServerBuilder,
  TcpConnectionHandler,
} from './fixture-server.types'
import { listenOnAvailablePort } from './server-utils'

export function createTcpFixtureServer(
  host = '127.0.0.1',
): TcpFixtureServerBuilder {
  const connectionHandlers: TcpConnectionHandler[] = []

  let started = false

  return {
    onConnection(handler: TcpConnectionHandler): TcpFixtureServerBuilder {
      if (started) {
        throw new Error(
          'Cannot add a connection handler after the TCP fixture server has started',
        )
      }
      connectionHandlers.push(handler)
      return this
    },

    async start(): Promise<TcpFixtureServer> {
      if (started) {
        throw new Error('TCP fixture server has already been started')
      }

      started = true

      const sockets = new Set<Socket>()

      const server = createServer((socket) => {
        sockets.add(socket)

        socket.once('close', () => {
          sockets.delete(socket)
        })

        socket.once('error', () => {
          sockets.delete(socket)
        })

        for (const handler of connectionHandlers) {
          Promise.resolve(handler(socket)).catch((error: unknown) => {
            socket.destroy(toError(error))
          })
        }
      })

      let port
      try {
        port = await listenOnAvailablePort(server, host)
      } catch (error) {
        started = false
        throw error
      }

      let closed = false

      return {
        transport: 'tcp',
        host,
        port,

        async close(): Promise<void> {
          if (closed) {
            return
          }
          closed = true
          await closeServer(server, sockets)
        },
      }
    },
  }
}

async function closeServer(
  server: Server,
  sockets: ReadonlySet<Socket>,
): Promise<void> {
  for (const socket of sockets) {
    socket.destroy()
  }

  if (!server.listening) {
    return
  }

  await new Promise<void>((resolve, reject) => {
    server.close((error) => {
      if (error !== undefined) {
        reject(error)
        return
      }
      resolve()
    })
  })
}

function toError(error: unknown): Error {
  return error instanceof Error ? error : new Error(String(error))
}
