# wrocpp-magic-numbers-in-tests

## Mistake

Test thresholds and fixture sizes are written as bare literals (`CHECK(latency < 250)`), so the
same number appears in several tests with no name and no stated source, and changing it means
finding every copy.

## Evidence

cloudevents-sdk-cpp: 40e2442 (36 unnamed constants), ae8ac0b, 2a05d72, e78634e, cce2e5e. The
general `readability-magic-numbers` check was disabled there, as it is in many codebases, because
it is noisy in production code.

## Ladder

Rung 1 is `readability-magic-numbers`; this check exists for projects that keep that off in
production code but want tests held to a named-constants rule.

## Must flag

An integer or floating literal other than 0 and 1 used as an argument of a comparison inside a
test-framework assertion macro expansion (`CHECK`, `REQUIRE`, `EXPECT_*`, `ASSERT_*`,
`expect`), in files matching the test path pattern.

## Must not flag

Literals in a `constexpr` variable initializer (that is where the name is given), 0, 1, and
literals outside assertions.

## Form

Query check, integer literals only. The dynamic matcher registry that query checks use has no
`floatingLiteral` (measured with clang-query 23.1.1), so `CHECK(ratio() >= 0.75)` is a known gap of
the query form; the test file marks it.
