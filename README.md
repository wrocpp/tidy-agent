# tidy-agent

clang-tidy checks for the C++ mistakes AI coding agents keep making, and a Claude Code plugin that
runs them on every edit and turns new recurring mistakes into new checks.

A rule written in `CLAUDE.md` is applied when the model remembers it. A clang-tidy check is applied
every time, by the agent's hook, by CI, and by `run-clang-tidy -fix` across a whole codebase. This
repository is the second kind.

> Status: pre-release. v0.1 is planned for late October 2026, alongside the
> [tidy-agent series on wro.cpp](https://wrocpp.github.io/). Check names and options may change
> until then.

## The checks

Every check comes from a mistake found in the fix history of real agent-written code. The evidence
for each is in its spec under [`docs/checks/`](docs/checks/).

| Check | Catches | Form |
| --- | --- | --- |
| `array-counted-size` | `const std::array<T, N>` with a hand-counted `N` | query |
| `brace-init-list-hijack` | `Value{v}` / `return {v};` picking an `initializer_list` constructor | query (+ plugin fix-it) |
| `unit-in-integer-name` | `int timeoutMs`, `int64_t ttl_sec` | query |
| `view-of-temporary` | a view class constructed from a temporary owner | config for `bugprone-dangling-handle` |
| `magic-numbers-in-tests` | bare integer thresholds in test assertions | query |
| `public-function-contracts` | public API functions without `pre`/`post` | blocked on Clang contracts support |
| `default-then-assign` | `T x; x.a = ..; x.b = ..;` and `push_back` runs | plugin, fix-it |
| `escaping-ref-capture` | a by-reference lambda that is returned or stored | plugin |
| `needless-shared-ptr` | a `shared_ptr` that is never shared | plugin, fix-it |
| `header-comment-policy` | history and git pointers in header comments | plugin |

**Query** checks are YAML in [`checks/query/`](checks/query/). They need clang-tidy 22 or later and
nothing else: no build, any platform. They detect but cannot fix.

**Plugin** checks need a fix-it or more than one AST node. They are a shared library built against
your LLVM major version (Linux and macOS; see [ADR-0001](docs/adr/0001-query-first-plugin-for-fixes.md)).

## Use the query checks

```bash
python3 tools/build_config.py            # writes dist/tidy-agent.clang-tidy
clang-tidy --experimental-custom-checks --config-file=dist/tidy-agent.clang-tidy -p build src/foo.cpp
```

Or copy the entries you want from `checks/query/*.yaml` into the `CustomChecks:` list of your own
`.clang-tidy`, and add `custom-wrocpp-*` to `Checks:`. clang-tidy prefixes query checks with
`custom-`, so they report as `custom-wrocpp-<name>`.

## Build and use the plugin checks

```bash
cmake -S checks/plugin -B build -G Ninja \
  -DLLVM_DIR=$(llvm-config --cmakedir) -DClang_DIR=$(llvm-config --cmakedir)/../clang
cmake --build build
clang-tidy -load=build/libwrocpp-tidy.so --checks='-*,wrocpp-*' -p build-of-your-project src/foo.cpp
```

Build against the same LLVM major version as the `clang-tidy` that loads it. Add `--fix` to apply
the fix-its (`default-then-assign` for aggregates, `needless-shared-ptr` for `auto` variables), and
`--format-style=file` to let clang-format tidy the whitespace the removed lines leave behind.

## Use the Claude Code plugin

```text
/plugin marketplace add wrocpp/tidy-agent
/plugin install tidy-agent@wrocpp
```

It installs two things:

- **A `PostToolUse` hook** that runs clang-tidy, with your project's `.clang-tidy` and
  `compile_commands.json`, on every C or C++ file the agent edits. Findings from checks in your
  `WarningsAsErrors` stop the agent until it fixes them; other findings are handed to it as context.
  Every finding is logged to `.tidy-agent/findings.jsonl`
  ([ADR-0003](docs/adr/0003-hook-block-policy.md)).
- **The `mistake-to-check` skill.** Tell it "the agent keeps doing X", or let it read the review-fix
  commits and the findings log. It proposes the cheapest rule that catches the mistake: enabling an
  existing check, configuring one, a compiler warning, a query check, or a plugin check, in that
  order ([ADR-0005](docs/adr/0005-escalation-ladder.md)). It tests the rule against positive and
  negative cases, reports how often it fires on your code, and hands you a diff. It never enables
  anything itself. It reads session transcripts only when asked, and never copies from them
  ([ADR-0004](docs/adr/0004-transcript-privacy.md)).

## Development

```bash
python3 tools/run_tests.py --clang-tidy /path/to/clang-tidy
```

Each check has a spec in `docs/checks/<name>.md` and tests in `tests/<name>/`. A test file starts
with `// spec: <name>` and marks every line that must be flagged with `// expect: <diagnostic>`;
every other line must stay clean. CI runs all checks against LLVM 22, 23 and trunk on Linux and
macOS ([ADR-0002](docs/adr/0002-llvm-support-matrix.md)).

## Licence

MIT. See [LICENSE](LICENSE).
