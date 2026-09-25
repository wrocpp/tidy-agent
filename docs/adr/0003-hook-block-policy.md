# ADR-0003: The hook feeds warnings back and blocks only on WarningsAsErrors

Status: accepted (2026-09-25)

## Context

The Claude Code plugin runs clang-tidy on every C or C++ file the agent edits (a `PostToolUse` hook
on `Edit|Write`). The edit has already happened when the hook runs. The hook can either tell the
agent about findings (JSON `additionalContext`) or make the agent stop and fix them (exit code 2,
diagnostics on stderr).

Blocking on everything is unusable on legacy code, where an edit to one line surfaces every old
finding in the file. Never blocking turns the checks back into advice the agent may ignore, which
is the failure this project exists to remove.

## Decision

- Checks the project lists in `WarningsAsErrors` block: the hook exits 2 and the agent must fix
  them before it continues.
- Every other finding is returned as `additionalContext`.
- Every finding, blocking or not, is appended to `.tidy-agent/findings.jsonl`, which the
  `mistake-to-check` skill reads.
- Only the edited file is checked, with the project's `compile_commands.json`.

## Consequences

The project, not the plugin, decides which checks are hard. That is the same switch CI already
uses, so an agent is blocked on exactly what CI would reject.
