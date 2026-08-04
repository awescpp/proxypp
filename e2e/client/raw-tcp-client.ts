/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import { createConnection } from 'node:net'

interface CreateRawTcpClientOptions {
  host: string
  port: number
  request: string
}

export function sendRawHttpRequest(
  options: CreateRawTcpClientOptions,
): Promise<String> {
  return new Promise((resolve, reject) => {
    const socket = createConnection({
      host: options.host,
      port: options.port,
    })

    let response = ''

    socket.setEncoding('utf-8')

    socket.once('connect', () => socket.write(options.request))

    socket.on('data', (chunk) => {
      response += Buffer.from(chunk).toString('utf-8')
    })

    socket.once('end', () => {
      resolve(response)
    })

    socket.once('error', reject)
  })
}
