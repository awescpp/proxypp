# Third-Party Notices

This file provides a summary of third-party open source software used by proxy++.

The proxy++ project itself is licensed under the Apache License, Version 2.0. Third-party software remains licensed
under its own license terms. This file is provided for attribution and license-compliance purposes and does not replace
the original license texts distributed by
the upstream projects.

## C++ dependencies

The following third-party C++ libraries are used by proxy++.

| Component           | Usage                                                                                                                                                                        | License                    | Website / Repository                      |
|---------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------|-------------------------------------------|
| Boost C++ Libraries | Networking, HTTP parsing, URL handling, testing, and utility components, including Boost.Asio, Boost.Beast, Boost.DateTime, Boost.Url, Boost.Test, and Boost.Multiprecision. | Boost Software License 1.0 | https://www.boost.org/                    |
| CLI11               | Command-line option parsing.                                                                                                                                                 | BSD 3-Clause License       | https://github.com/CLIUtils/CLI11         |
| spdlog              | Logging.                                                                                                                                                                     | MIT License                | https://github.com/gabime/spdlog          |
| magic_enum          | Enum reflection utilities.                                                                                                                                                   | MIT License                | https://github.com/Neargye/magic_enum     |
| quickjs-ng          | A small and embeddable JavaScript engine supporting the latest ECMAScript specification.                                                                                     | MIT License                | https://github.com/quickjs-ng/quickjs     |
| tl::expected        | C++20-compatible implementation of `std::expected`, used for explicit error handling without exceptions.                                                                     | CC0-1.0                    | https://github.com/TartanLlama/expected   |
| jsoncons            | JSON schema validation.                                                                                                                                                      | Boost Software License 1.0 | https://github.com/danielaparker/jsoncons | 

## Node.js development dependencies

The following Node.js packages are used for development, formatting, commit workflow, TypeScript type checking, and
end-to-end testing. They are development dependencies and are not intended to be distributed with proxy++ binary
releases.

| Component                       | Usage                                           | License            | Website / Repository                                                                             |
|---------------------------------|-------------------------------------------------|--------------------|--------------------------------------------------------------------------------------------------|
| @commitlint/cli                 | Commit message linting.                         | MIT License        | https://github.com/conventional-changelog/commitlint                                             |
| @commitlint/config-conventional | Conventional commit rules for commitlint.       | MIT License        | https://github.com/conventional-changelog/commitlint/tree/master/@commitlint/config-conventional |
| @types/express                  | TypeScript type definitions for Express.        | MIT License        | https://github.com/DefinitelyTyped/DefinitelyTyped/tree/master/types/express                     |
| @types/node                     | TypeScript type definitions for Node.js.        | MIT License        | https://github.com/DefinitelyTyped/DefinitelyTyped/tree/master/types/node                        |
| axios                           | HTTP client used by end-to-end tests.           | MIT License        | https://github.com/axios/axios                                                                   |
| commitizen                      | Interactive commit message helper.              | MIT License        | https://github.com/commitizen/cz-cli                                                             |
| cz-git                          | Commitizen adapter.                             | MIT License        | https://github.com/Zhengqbbb/cz-git                                                              |
| execa                           | Process execution helper used by scripts/tests. | MIT License        | https://github.com/sindresorhus/execa                                                            |
| express                         | HTTP fixture server used by end-to-end tests.   | MIT License        | https://github.com/expressjs/express                                                             |
| prettier                        | Code formatter.                                 | MIT License        | https://github.com/prettier/prettier                                                             |
| simple-git-hooks                | Git hook management.                            | MIT License        | https://github.com/toplenboren/simple-git-hooks                                                  |
| typescript                      | TypeScript compiler and type checker.           | Apache License 2.0 | https://github.com/microsoft/TypeScript                                                          |
| vitest                          | End-to-end test runner.                         | MIT License        | https://github.com/vitest-dev/vitest                                                             |

## Release packaging note

When creating a source or binary release of proxy++, include this file together with the proxy++ `LICENSE` and `NOTICE`
files.

For binary releases, also include the exact upstream license texts for any third-party libraries that are actually
distributed with the release package, such as static or dynamic libraries linked into or shipped with proxy++.

For vcpkg-based builds, the upstream license text for each installed C++ package can usually be found under:

```text
vcpkg_installed/<triplet>/share/<package>/copyright
```

For Node.js development dependencies, the upstream license text can usually be found in the corresponding package
directory under `node_modules`, if the dependency is redistributed.
