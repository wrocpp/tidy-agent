// spec: unit-in-integer-name
#include <chrono>
#include <cstdint>

struct Config {
    int timeoutMs;                                      // expect: custom-wrocpp-unit-in-integer-name
    std::int64_t ttl_sec;                               // expect: custom-wrocpp-unit-in-integer-name
    double latencySeconds;                              // expect: custom-wrocpp-unit-in-integer-name
    std::size_t buffer_bytes;                           // expect: custom-wrocpp-unit-in-integer-name
    std::chrono::milliseconds timeout;
    std::chrono::seconds ttlSec;
    bool enabledMs;
    int items;
};

int wait(int delayMs) { return delayMs; }               // expect: custom-wrocpp-unit-in-integer-name

int use(Config c) { return wait(c.timeoutMs) + c.items; }
