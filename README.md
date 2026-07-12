# proxy++

[![License](https://img.shields.io/github/license/awescpp/proxypp)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/build-CMake-blue)
![vcpkg](https://img.shields.io/badge/deps-vcpkg-blue)

proxy++ is an open-source C++ proxy infrastructure project, currently focused on HTTP/HTTPS network proxying, extensibility, observability, and practical enterprise use cases.

> proxy++ is currently in early development. APIs, configuration, command-line options, and runtime behavior may change before the first stable release.

## Goals

proxy++ aims to provide a simple, extensible, and developer-friendly foundation for building proxy, traffic processing, and protocol mediation tools:

* HTTP/HTTPS proxy forwarding
* traffic debugging and inspection
* enterprise network middleware customization
* rule-based request/response processing
* future programmable filtering and observability features

The project is not designed to be just another high-performance proxy. Instead, proxy++ focuses on ease of use, extensibility, and practical integration scenarios.

## Current status

The project is under active development.

Currently implemented or in progress:

* HTTP proxy forwarding
* request and response header forwarding
* request and response body forwarding
* Content-Length body forwarding
* chunked body forwarding
* close-delimited response body forwarding
* keep-alive behavior for HTTP forwarding
* HTTPS tunnel support
* unit tests based on Boost.Test
* end-to-end tests based on Vitest

More features will be added gradually.

## Roadmap

Planned areas include:

* more complete HTTP/HTTPS proxy support
* rule-based traffic processing
* configuration file support
* JavaScript-based programmable filters
* traffic logging and observability
* enterprise-oriented customization features
* packaging and release automation

## Build

proxy++ uses CMake and C++20.

Required tools:

* CMake
* A C++20 compiler
* vcpkg
* Node.js and pnpm, for scripts and end-to-end tests

Install dependencies with vcpkg according to `vcpkg.json`, then configure and build the project with CMake.

On Windows, the project also provides npm scripts to simplify common development tasks.

```bash
pnpm build
```

## Test

Run unit tests:

```bash
pnpm test:unit
```

Run end-to-end tests:

```bash
pnpm test:e2e
```

Run all tests:

```bash
pnpm test:all
```

## Contributing

proxy++ is currently in early development and is not accepting external pull requests yet.

The project direction, architecture, and public APIs are still evolving. Contributions may be opened later when the project becomes more stable.

## License

proxy++ is licensed under the Apache License, Version 2.0.

See [LICENSE](./LICENSE) for details.

Third-party dependency notices are listed in [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md).