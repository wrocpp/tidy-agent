# wrocpp-default-then-assign

## Mistake

An object whose whole contents are known at the point of declaration is default-constructed and
then filled in: an aggregate by one member assignment per field, or a container by a run of
`push_back` / `emplace_back` / `insert` / `emplace` / `add` calls. The braced form states the value
once, and naming the same field twice in a designated initializer is a compile error, while
assigning it twice is silent.

## Evidence

- Most frequent agent mistake in both codebases. The proprietary service: about 7 fix commits and
  194 remaining sites in 71 files with three or more field assignments after a default
  declaration. In one case an agent silenced an unrelated clang-tidy finding by replacing
  `operator[]` with a run of `emplace` calls, and review undid it.
- cloudevents-sdk-cpp: 3463507 (test `.add()` runs rewritten as braced lists), 8caec84 and 34c15f1
  (an initializer-list constructor added so tests stop calling `add`), plus two user corrections.

## Must flag

```cpp
Point p;            // aggregate, no initializer
p.x = 1;
p.y = 2;            // >= 2 consecutive member assignments to p, nothing between them reads p

std::vector<int> v;
v.push_back(1);
v.push_back(2);
v.push_back(3);     // >= 3 consecutive mutator calls with arguments known at this point
```

## Must not flag

- A loop body filling a container from runtime data.
- Assignments separated by statements that read the object or have other side effects on it.
- A single assignment after declaration.
- Members assigned from values computed after the declaration that depend on the object itself.

## Form

Plugin (needs the statement sequence of a `CompoundStmt`). Fix-it for aggregates: emit the
designators in declaration order; no fix-it when an assignment's right-hand side mentions the
object. Threshold options: `MinAssignments` (default 2), `MinMutatorCalls` (default 3),
`MutatorNames` (default `push_back;emplace_back;insert;emplace;add`).
