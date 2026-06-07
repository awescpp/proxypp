import axios, { AxiosInstance } from 'axios'

export interface CreateHttpProxyClientOptions {
  readonly proxyHost: string
  readonly proxyPort: number
}

export function createHttpProxyClient(options: CreateHttpProxyClientOptions): AxiosInstance {
  return axios.create({
    proxy: {
      protocol: 'http',
      host: options.proxyHost,
      port: options.proxyPort,
    },
    timeout: 5_000,
    // treat all status code as success
    validateStatus: () => true,
    maxRedirects: 0,
  })
}
