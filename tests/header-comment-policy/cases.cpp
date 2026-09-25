// spec: header-comment-policy
// The comment check runs on headers. This translation unit includes the
// header; the runner maps each `expect-line` marker in cases.hpp to an
// expected wrocpp-header-comment-policy finding in that file.
#include "cases.hpp"

int demo::parse(int value) { return value; }
