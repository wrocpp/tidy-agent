# wrocpp-header-comment-policy

## Mistake

Comments in public headers narrate history ("used to be", "an earlier version"), point at git
("see commit abc123", "run git blame"), or duplicate the commit message, and go stale when history
is rewritten.

## Evidence

cloudevents-sdk-cpp: ff2cc48, 5e45cd1, 7ef3e13 (1,091 comment lines removed from headers). The rule
was enforced only by an advisory editor hook, which did not block.

## Must flag

In files matching `HeaderRegex`, a comment that:

- contains a git pointer (`see commit`, `git blame`, a 7 to 40 character hex string preceded by
  "commit");
- narrates the past (`used to`, `an earlier version`, `previously`, `version \d+ of`).

## Must not flag

Doc comments that state a contract, `// spec: <id>` markers, `TODO(#NN)`, `NOLINT`, namespace
closers.

## Form

Plugin (a `CommentHandler` on the preprocessor). Patterns are options, so a project can tune them.
