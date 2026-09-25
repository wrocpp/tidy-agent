# wrocpp-array-counted-size

## Mistake

A constant table is declared as `std::array<T, N>` with `N` written by hand and an initializer
list. Adding an element past `N` is a compile error, but a list shorter than `N` compiles and
value-initializes the tail, so a forgotten entry becomes a zero.

## Evidence

The proprietary service: one incident broke main (declared size 20, list grew to 21), followed by
a preventive rewrite of every table to `std::to_array`.

## Must flag

A `const` or `constexpr` variable of type `std::array<T, N>` with a non-empty initializer list.

## Must not flag

- `std::array<T, N> buf{};` (empty braces: a zeroed buffer, not a table).
- Non-const arrays.
- Arrays whose size comes from a template parameter or a named constant used elsewhere.

## Form

Query check for detection (verified on Compiler Explorer's clang-tidy trunk on 2026-09-25).
Plugin twin with fix-it to `constexpr auto name = std::to_array<T>({...});`.
