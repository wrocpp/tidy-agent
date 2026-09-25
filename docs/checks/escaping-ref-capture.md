# wrocpp-escaping-ref-capture

## Mistake

A lambda captures by reference and then outlives the frame it captured from: it is returned,
stored in a `std::function`, or handed to a registry that calls it later.

## Evidence

The proprietary service: a returned step handler captured a local container by reference while 20
of its 21 siblings captured by value. Host tests stayed green; it failed only on the target device,
because the sanitizer build did not configure. Two earlier incidents had the same shape.

## Must flag

- A lambda with any by-reference capture (explicit `&x` or default `&`) that is the operand of a
  `return`.
- The same lambda converted to `std::function` or stored in a member or container.

## Must not flag

- Lambdas passed directly to algorithms that call them before returning (`std::ranges::for_each`,
  `std::sort` comparators).
- By-reference captures of `this`-owned members when the lambda is stored in the same object.

## Form

Plugin. The configurable `SafeCallees` list names functions that are known to call their callable
synchronously.
