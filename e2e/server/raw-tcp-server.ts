/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { createServer, type Socket } from 'node:net'
import { closeServer, listenOnEphemeralPort } from './utils/server-utils'

const host = '127.0.0.1'

type ConnectionHandler = (socket: Socket) => void | Promise<void>

function toError(error: unknown): Error {
  return error instanceof Error ? error : new Error(String(error))
}

async function handleConnection(
  socket: Socket,
  handler: ConnectionHandler,
): Promise<void> {
  try {
    await handler(socket)
  } catch (error) {
    socket.destroy(toError(error))
  }
}

export async function startRawTcpServer(onConnection: ConnectionHandler) {
  const sockets = new Set<Socket>()
  const server = createServer((socket) => {
    sockets.add(socket)

    socket.once('close', () => {
      sockets.delete(socket)
    })

    socket.once('error', () => {
      sockets.delete(socket)
    })

    handleConnection(socket, onConnection)
  })

  try {
    const port = await listenOnEphemeralPort(server, host)
    let closed = false
    return {
      host,
      port,
      async close() {
        if (closed) return
        closed = true
        await closeServer(server, sockets)
      },
    }
  } catch (error) {}
}
