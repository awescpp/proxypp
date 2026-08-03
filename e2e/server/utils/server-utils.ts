/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import type { Server, Socket } from 'node:net'
import { errorMonitor } from 'node:events'

export async function listenOnEphemeralPort(
  server: Server,
  host: string,
): Promise<number> {
  await new Promise<void>((resolve, reject) => {
    const onError = (error: Error) => {
      server.off('listening', onListening)
      reject(error)
    }

    const onListening = () => {
      server.off('error', onError)
      resolve()
    }

    server.once('error', onError)
    server.once('listening', onListening)

    server.listen(0, host)
  })

  const address = server.address()

  if (address === null || typeof address === 'string') {
    throw new Error('Server did not obtain a TCP address')
  }

  return address.port
}

export async function closeServer(
  server: Server,
  sockets: ReadonlySet<Socket>,
) {
  const closeServer_ = server.listening
    ? new Promise<void>((resolve, reject) => {
        server.close((error) => {
          if (error !== undefined) {
            reject(error)
            return
          }
          resolve()
        })
      })
    : Promise.resolve()

  for (const socket of sockets) {
    socket.destroy()
  }

  await closeServer_
}
