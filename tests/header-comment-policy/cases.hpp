// spec: header-comment-policy
#pragma once

namespace demo {

// See commit 3ad6200 for why this is parenthesised.                expect-line
int parse(int value);

// This used to return a raw pointer.                                expect-line
int* legacy();

// An earlier version cached the result.                             expect-line
int cached();

/// Returns the number of events; never negative.
int count();

// spec: SWR-EVT-0004
int checked();

// TODO(#42): accept a span.
int span_later();

}  // namespace demo
