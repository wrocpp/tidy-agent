# wrocpp-needless-shared-ptr

## Mistake

A local `std::shared_ptr` is created with `std::make_shared` and never copied, moved out, captured
or stored, so shared ownership buys nothing and costs an atomic reference count and a control
block.

## Evidence

No incident in the two mined codebases. Kept on request as a common agent habit in general and as
the worked example for a data-flow check in the plugin.

## Must flag

A local `shared_ptr` initialized from `make_shared` whose every use is a dereference, `get()`, or
`operator bool`, within one function.

## Must not flag

Any copy, move, capture, return, or pass by value; passing to a function taking
`const std::shared_ptr<T>&` whose body is not visible (conservative).

## Form

Plugin. Fix-it: `std::make_shared<T>(...)` to `std::make_unique<T>(...)` and the declared type to
`std::unique_ptr<T>`, only when the variable is declared with `auto` or the fix can rewrite the
type.
