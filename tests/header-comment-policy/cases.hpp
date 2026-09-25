// spec: header-comment-policy
#pragma once

namespace demo {

// See commit 3ad6200 for why this is parenthesised. expect: wrocpp-header-comment-policy
int parse(int value);

// This used to return a raw pointer. expect: wrocpp-header-comment-policy
int* legacy();

// An earlier version cached the result. expect: wrocpp-header-comment-policy
int cached();

/// Returns the number of events; never negative.
int count();

/// The key used to sign the payload.
int key();

// spec: SWR-EVT-0004
int checked();

// TODO(#42): accept a span.
int span_later();

}  // namespace demo
