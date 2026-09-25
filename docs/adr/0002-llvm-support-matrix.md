# ADR-0002: LLVM 22, 23 and trunk, all required, on Linux and macOS

Status: accepted (2026-09-25)

## Context

Query checks need LLVM 22.1 or later. The plugin API has no stable ABI, and LLVM 22 deprecated
`clang-tidy/ClangTidyModuleRegistry.h` with removal planned for LLVM 24, so plugin sources will
need a version switch around that release.

## Decision

CI builds and tests every check against LLVM 22, LLVM 23 and LLVM trunk. All three are required
to pass; trunk is not allowed to fail. Linux (apt.llvm.org packages) and macOS (Homebrew `llvm`)
both run. Windows is documented as query-checks-only.

## Consequences

- API churn on trunk breaks CI early, before a release reaches readers. The cost is occasional red
  builds caused by LLVM rather than by this repository; those are fixed here, not waived.
- Release artifacts for the plugin are published per LLVM major and per OS.
