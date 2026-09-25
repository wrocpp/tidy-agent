# Check specifications

One file per check. Each states the mistake, where the evidence came from, the cases the check
must flag and must not flag, and which rung of the ladder (ADR-0005) it sits on. Every test file
under `tests/<check>/` names the spec it satisfies in its first line (`// spec: <check>`), and
`tools/run_tests.py` fails if a spec has no tests or a test names no spec.

Evidence comes from two agent-written codebases:

- **cloudevents-sdk-cpp** (public, Apache-2.0, https://github.com/filipsajdak/cloudevents-sdk-cpp).
  Commits are cited by SHA.
- **a proprietary C++23 service of about 160,000 lines.** Only pattern names and counts are
  published; no code or commit text from it appears in this repository.

| Check | Form | Fix-it | Spec |
| --- | --- | --- | --- |
| `wrocpp-default-then-assign` | plugin | yes (aggregates) | [default-then-assign.md](default-then-assign.md) |
| `wrocpp-brace-init-list-hijack` | query + plugin | plugin | [brace-init-list-hijack.md](brace-init-list-hijack.md) |
| `wrocpp-unit-in-integer-name` | query | no | [unit-in-integer-name.md](unit-in-integer-name.md) |
| `wrocpp-array-counted-size` | query + plugin | plugin | [array-counted-size.md](array-counted-size.md) |
| `wrocpp-escaping-ref-capture` | plugin | no | [escaping-ref-capture.md](escaping-ref-capture.md) |
| `wrocpp-view-of-temporary` | config (`bugprone-dangling-handle`) | no | [view-of-temporary.md](view-of-temporary.md) |
| `wrocpp-magic-numbers-in-tests` | query | no | [magic-numbers-in-tests.md](magic-numbers-in-tests.md) |
| `wrocpp-needless-shared-ptr` | plugin | yes | [needless-shared-ptr.md](needless-shared-ptr.md) |
| `wrocpp-header-comment-policy` | plugin | no | [header-comment-policy.md](header-comment-policy.md) |
| `wrocpp-public-function-contracts` | blocked | no | [public-function-contracts.md](public-function-contracts.md) |

Query checks are named `custom-wrocpp-<name>` by clang-tidy (it prefixes `custom-`); the plugin
forms are `wrocpp-<name>`.
