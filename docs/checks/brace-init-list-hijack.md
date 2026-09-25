# wrocpp-brace-init-list-hijack

## Mistake

`T{x}` or `return {x};` where `T` has a constructor taking `std::initializer_list`. List
initialization prefers that constructor, so a copy or conversion the author meant becomes a
one-element list. With JSON value types this silently wraps an object in an array.

## Evidence

cloudevents-sdk-cpp: 3ad6200 (`result<json>{ {"a",1} }` produced `[{"a":1}]`), d17af33, be80f5d,
e74a25c. The behaviour depended on the JSON library and its version (nlohmann, Boost.JSON before
1.84, Glaze), so it passed on the machine it was written on.

## Must flag

- A braced list with exactly one element that selects an `initializer_list` constructor, when the
  element's type is the class itself or a type another non-list constructor accepts.
- The same in a `return {x};` statement.

## Must not flag

- Braced lists with two or more elements.
- A one-element list whose element type only the `initializer_list` constructor accepts.
- Types with no `initializer_list` constructor.

## Form

Query check for detection. Plugin twin with fix-it `{x}` to `(x)`.
