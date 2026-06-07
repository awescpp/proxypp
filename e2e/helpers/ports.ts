import { createServer } from 'node:net'

/**
 * Find an available port
 * @param host hostname
 */
export async function findAvailablePort(host = '127.0.0.1'): Promise<number> {
  const server = createServer()
  return new Promise<number>((resolve, reject) => {
    server.once('error', reject)
    server.listen(0, host, () => {
      const address = server.address()
      if (address === null || typeof address === 'string') {
        server.close()
        reject(new Error('Failed to allocate an available TCP port'))
        return
      }
      const port = address.port
      server.close((error) => {
        if (error !== undefined) {
          reject(error)
          return
        }
        resolve(port)
      })
    })
  })
}
