# wrocpp-public-function-contracts

Status: blocked (needs a Clang that parses P2900 contracts and AST matchers for them)

## Mistake

A codebase adopting C++26 contracts wants every public function in its API headers to state at
least one precondition or postcondition, and nothing checks that.

## Evidence

The wro.cpp toolset page on contracts notes that "verify all public functions have contracts"
needs a clang-tidy rule and none exists.

## Must flag

A non-deleted, non-defaulted function declaration in a file matching the public header pattern,
with external linkage, that has no `pre` or `post` contract specifier.

## Must not flag

Private and protected members, functions in `detail` namespaces, special members, and functions
with no parameters and a `void` return.

## Form

Query check. Requires a Clang that parses P2900 contracts; until then the check is marked
experimental and its tests are skipped on toolchains without `__cpp_contracts`.
