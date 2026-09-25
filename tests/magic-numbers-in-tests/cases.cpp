// spec: magic-numbers-in-tests
#define CHECK(expr) ((void)(expr))

constexpr int max_latency_ms = 250;

int measure();
double ratio();

void test_latency() {
    CHECK(measure() < 250);                             // expect: custom-wrocpp-magic-numbers-in-tests
    CHECK(ratio() >= 0.75);  // known gap: no floatingLiteral matcher in query checks
    CHECK(measure() == 42);                             // expect: custom-wrocpp-magic-numbers-in-tests
    CHECK(measure() < max_latency_ms);
    CHECK(measure() > 0);
    CHECK(measure() != 1);
    int local = measure() + 7;
    (void)local;
}
