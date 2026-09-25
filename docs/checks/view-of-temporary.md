# wrocpp-view-of-temporary (configuration, not a new check)

## Mistake

A view type (a project's own slice or span-like class, or `std::string_view`) is constructed from a
temporary owner, so it dangles at the end of the full expression. GCC 16 says nothing about
several of these shapes.

## Evidence

cloudevents-sdk-cpp: c4b4311 ("gcc 16 does not warn"; rvalue overloads deleted to prevent a slicer
over a temporary `std::string`), and a design decision about a reflection helper returning a view
of a prvalue.

## Ladder result: rung 2

This was planned as a query check. Walking the ladder first showed it is not needed:
`bugprone-dangling-handle`, with the project's view classes added to its `HandleClasses` option,
flags every positive case below and none of the negative ones (measured with clang-tidy 23.1.1 on
2026-09-26). Clang's `-Wdangling-gsl` also catches the `std::string_view` case at compile time.

So tidy-agent ships a configuration recipe, [`checks/config/view-of-temporary.yaml`](../../checks/config/view-of-temporary.yaml),
and the tests hold `bugprone-dangling-handle` to it. The finding reports as
`bugprone-dangling-handle`.

## Must flag

`Slice a{std::string{"abc"}};`, `Slice b(make());` where `make()` returns `std::string` by value,
and `std::string_view c = make();`.

## Must not flag

Views of named objects, views of string literals, and member calls on a temporary that return a
value.
