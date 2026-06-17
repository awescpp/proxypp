# Third-Party Notices

This file provides a summary of third-party open source software used by proxy++.

The proxy++ project itself is licensed under the Apache License, Version 2.0. Third-party software remains licensed
under its own license terms. This file is provided for attribution and license-compliance purposes and does not replace
the original license texts distributed by
the upstream projects.

## C++ dependencies

The following third-party C++ libraries are used by proxy++.

| Component           | Usage                                                                                                                                                                        | License                    |
|---------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------|
| Boost C++ Libraries | Networking, HTTP parsing, URL handling, testing, and utility components, including Boost.Asio, Boost.Beast, Boost.DateTime, Boost.Url, Boost.Test, and Boost.Multiprecision. | Boost Software License 1.0 |
| CLI11               | Command-line option parsing.                                                                                                                                                 | BSD 3-Clause License       |
| spdlog              | Logging.                                                                                                                                                                     | MIT License                |
| magic_enum          | Enum reflection utilities.                                                                                                                                                   | MIT License                |
| fmt                 | Formatting library, possibly used transitively by spdlog depending on the build configuration.                                                                               | MIT License                |
| quickjs-ng          | A small and embeddable JavaScript engine supporting the latest ECMAScript specification.                                                                                     | MIT License                |

## Node.js development dependencies

The following Node.js packages are used for development, formatting, commit workflow, TypeScript type checking, and
end-to-end testing. They are development dependencies and are not intended to be distributed with proxy++ binary
releases.

| Component                       | Usage                                           | License            |
|---------------------------------|-------------------------------------------------|--------------------|
| @commitlint/cli                 | Commit message linting.                         | MIT License        |
| @commitlint/config-conventional | Conventional commit rules for commitlint.       | MIT License        |
| @types/express                  | TypeScript type definitions for Express.        | MIT License        |
| @types/node                     | TypeScript type definitions for Node.js.        | MIT License        |
| axios                           | HTTP client used by end-to-end tests.           | MIT License        |
| commitizen                      | Interactive commit message helper.              | MIT License        |
| cz-git                          | Commitizen adapter.                             | MIT License        |
| execa                           | Process execution helper used by scripts/tests. | MIT License        |
| express                         | HTTP fixture server used by end-to-end tests.   | MIT License        |
| prettier                        | Code formatter.                                 | MIT License        |
| simple-git-hooks                | Git hook management.                            | MIT License        |
| typescript                      | TypeScript compiler and type checker.           | Apache License 2.0 |
| vitest                          | End-to-end test runner.                         | MIT License        |

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
