/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

import type { Socket as TcpSocket } from 'node:net'
import type { RemoteInfo, Socket as UdpSocket } from 'node:dgram'

// region base interfaces

export interface FixtureServer {
  readonly host: string
  readonly port: number
  close(): Promise<void>
}

export interface FixtureServerBuilder<TServer extends FixtureServer> {
  start(): Promise<TServer>
  onConnection?: (listener: () => void) => void
}

// endregion

// region TCPFixtureServer related types

export interface TcpFixtureServer extends FixtureServer {
  readonly transport: 'tcp'
}

export type TcpConnectionHandler = (socket: TcpSocket) => void | Promise<void>

export interface TcpFixtureServerBuilder extends FixtureServerBuilder<TcpFixtureServer> {
  onConnection(handler: TcpConnectionHandler): this
}

// endregion

// region HttpFixtureServer related types

export interface HttpFixtureServer extends TcpFixtureServer {
  readonly protocol: 'http'
  readonly origin: string
}

// endregion

// region UDPFixtureServer related types

export interface UdpFixtureServer extends FixtureServer {
  readonly transport: 'udp'
}

export type UdpMessageHandler = (
  message: Buffer,
  remoteInfo: RemoteInfo,
  socket: UdpSocket,
) => void | Promise<void>

export interface UdpFixtureServerBuilder extends FixtureServerBuilder<UdpFixtureServer> {
  onMessage(handler: UdpMessageHandler): this
}

// endregion