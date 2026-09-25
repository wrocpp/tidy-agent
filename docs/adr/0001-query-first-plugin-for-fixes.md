# ADR-0001: Query checks first, a plugin only where a check must fix

Status: accepted (2026-09-25)

## Context

clang-tidy offers two ways to add a check without upstreaming it.

- **Query-based custom checks** (`CustomChecks` in `.clang-tidy`, LLVM 22.1+, enabled with
  `--experimental-custom-checks`). A check is an AST matcher in YAML. Nothing to compile, so a
  reader copies one file. The checks cannot emit fix-its, and the matcher syntax follows the AST
  matcher library, which the LLVM docs say may change at any time.
- **Plugins** (`-load=libX.so`, a `ClangTidyModule`, optionally `TransformerClangTidyCheck`). Full
  power: fix-its, preprocessor and comment callbacks, statement-sequence analysis. The shared
  library must be built against the same LLVM major version as the `clang-tidy` that loads it, and
  `clang-tidy` must be built with plugin support (not the case on Windows).

## Decision

Every check starts as a query check if a matcher can express its detection. A check moves to (or
gains a twin in) the plugin only when one of these holds:

1. it needs a fix-it, because the point of the check is that an agent or `run-clang-tidy -fix`
   applies it across a large codebase;
2. its detection needs more than one AST node in isolation (a sequence of statements, data flow,
   comments).

## Consequences

- Readers on any platform with LLVM 22+ get the detection-only checks by copying `checks/query/`.
- Checks that also exist in the plugin keep the same name suffix, so a codebase can switch from the
  query form to the plugin form without renaming suppressions beyond the prefix
  (`custom-wrocpp-x` to `wrocpp-x`).
- The plugin carries a per-LLVM-major build cost (see ADR-0002).
