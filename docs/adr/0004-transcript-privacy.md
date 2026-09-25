# ADR-0004: Session transcripts are opt-in, local, and never leave as code

Status: accepted (2026-09-25)

## Context

The richest record of agent mistakes is the Claude Code session transcript: the user saying "no,
don't copy that" next to the code the agent wrote. Transcripts also contain private source code,
customer context and everything else the user typed.

## Decision

The `mistake-to-check` skill reads transcripts only when the user passes an explicit flag, and only
from the local machine. From a transcript it extracts correction turns and the code they refer to,
and it emits only:

- the generalized mistake pattern, in prose;
- synthetic positive and negative test cases written for the check.

It never copies original code, file names or conversation text into its output, its proposals, or
the findings log.

## Consequences

A proposal built from a transcript can be committed to a public repository without review for
leaked content. The cost is that the synthetic test cases may miss a detail of the original code;
the hit count over the real codebase (which the skill reports) is what catches that.
