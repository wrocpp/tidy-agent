---
name: mistake-to-check
description: Turn a C++ mistake that an AI agent keeps making into a deterministic, tested clang-tidy rule. Use when the user says "the agent keeps doing X", asks to "make a check for this", "stop this from coming back", "turn this review comment into a lint", or asks which recurring mistakes in the history should become checks. Collects evidence from review-fix commits, the tidy-agent findings log and (only with --transcripts) local session transcripts, then proposes the cheapest rule that catches every positive case and no negative one.
argument-hint: "[description of the mistake] [--since <rev>] [--transcripts]"
allowed-tools: Read Write Edit Glob Grep Bash(python3 *) Bash(git *) Bash(clang-tidy *) Bash(clang-query *) AskUserQuestion
---

# mistake-to-check

A prose rule in CLAUDE.md is applied when the model remembers it. A clang-tidy check is applied
every time. This skill converts the first into the second, and stops at the cheapest form that
works.

## 1. Collect the evidence

Run the collector from the repository being analysed:

```bash
python3 "${CLAUDE_SKILL_DIR}/scripts/collect_signals.py" --since <rev> [--transcripts] > /tmp/tidy-agent-signals.json
```

It gathers, deterministically:

- **review-fix commits**: commits whose subject or body says fix, review, address, per review,
  follow-up, or that amend C++ lines an earlier commit in the range added; each with its C++ hunks;
- **the findings log**: `.tidy-agent/findings.jsonl`, grouped by check, with counts;
- **transcripts** (only with `--transcripts`): user turns in local Claude Code sessions for this
  project that correct the agent ("no", "don't", "instead", "wrong", "why did you"), each paired
  with the file the agent edited just before. Per ADR-0004 this stays on the machine: you read it
  to understand the pattern, and nothing from it (code, names, wording) goes into your output.

If the user described the mistake in the arguments, that description is the primary signal and
the collector output is supporting evidence.

## 2. Name the pattern

Cluster the evidence into recurring mistakes. For each one write down:

- one sentence stating the mistake as a rule about code shape;
- how many times it occurs in the evidence, and where;
- **positive cases**: 2 to 5 minimal, synthetic C++ snippets that must be flagged;
- **negative cases**: 2 to 5 snippets that look similar and must not be flagged.

Write the cases yourself. Never paste code from a transcript or from a private repository.

Confirm the list with the user (AskUserQuestion, one option per pattern, recommended first)
before building anything.

## 3. Walk the ladder

For each confirmed pattern, try the rungs in order and stop at the first that flags every positive
case and none of the negative ones (ADR-0005):

1. **Enable an existing check.** Search the clang-tidy check list (`clang-tidy -list-checks
   -checks='*'`) and its docs for one that already covers the shape. Check whether the project's
   `.clang-tidy` disables it, or whether its header filter excludes the files involved.
2. **Configure an existing check.** Many checks take lists: `bugprone-unused-return-value.CheckedReturnTypes`,
   `bugprone-dangling-handle.HandleClasses`, `bugprone-unsafe-functions.CustomFunctions`,
   `readability-identifier-naming.*`.
3. **Turn on a compiler warning**, when the compiler already diagnoses the shape (`-Wshadow-all`,
   `-Wmissing-designated-field-initializers`, `-Wdangling*`).
4. **Write a query check.** Prototype the matcher with `clang-query` on the positive and negative
   files until it matches exactly the positives, then write it as a `CustomChecks` entry.
5. **Scaffold a plugin check**, when the rule needs a fix-it, a sequence of statements, data flow
   or comments. Produce the spec and tests; implement it in the tidy-agent plugin sources.

Verify each rung by running it, not by reading documentation:

```bash
python3 "${CLAUDE_SKILL_DIR}/scripts/try_rung.py" --config <candidate.clang-tidy> --pos pos.cpp --neg neg.cpp
```

## 4. Measure it on the real code

Run the chosen rule over the whole project and report the hit count, with three sample hits. A
rule with zero hits in a codebase that produced the evidence is wrong; a rule with thousands of
hits needs a narrower matcher or a rollout plan (a baseline, or `clang-tidy-diff` on changed lines).

## 5. Hand over a diff, never enable it silently

Produce, as files the user can review:

- the `.clang-tidy` change (enable, configure, or a `CustomChecks` entry);
- the positive and negative test files, each starting with `// spec: <name>` and marking every line
  that must be flagged with `// expect: <diagnostic>`;
- a spec in the project's docs stating the mistake, the evidence (counts, public commit ids only),
  the cases and the rung.

Do not add the check to `WarningsAsErrors` yourself. Recommend it when the hit count on current
code is zero or has been fixed, and let the user decide.
