# ADR-0005: Propose the cheapest rung that catches the mistake

Status: accepted (2026-09-25)

## Context

We mined the fix history of two agent-written C++ codebases before writing any check. Many fixes
needed no new check at all. In one, a strict `.clang-tidy` existed from the first commit but no
build ran it until late in the project; its first run found 76 distinct problems, 5 of them real
bugs. In both, useful existing checks were disabled, or were missing a project type from an option
list.

## Decision

For each recurring mistake, the `mistake-to-check` skill tries these rungs in order and proposes
the first one that flags every positive test case and no negative one:

1. enable an existing clang-tidy check;
2. configure an existing check (for example `bugprone-unused-return-value.CheckedReturnTypes`,
   `bugprone-dangling-handle.HandleClasses`, `bugprone-unsafe-functions.CustomFunctions`);
3. turn on a compiler warning;
4. write a query-based custom check;
5. scaffold a plugin check (when a fix-it or multi-node analysis is needed, per ADR-0001).

## Consequences

The skill's first answer is often a one-line config change, which is cheaper to review and to
maintain than a new check. New checks in this repository are the ones no lower rung could cover.
