import type { Server } from 'node:net'

export async function listenOnAvailablePort(
  server: Server,
  host: string,
): Promise<number> {
  await new Promise<void>((resolve, reject) => {
    const onError = (error: Error): void => {
      server.off('listening', onListening)
      reject(error)
    }

    const onListening = (): void => {
      server.off('error', onError)
      resolve()
    }

    server.once('error', onError)
    server.once('listening', onListening)

    server.listen(0, host)
  })

  const address = server.address()

  if (address === null || typeof address === 'string') {
    throw new Error('Fixture server did not obtain a TCP address')
  }

  return address.port
}
