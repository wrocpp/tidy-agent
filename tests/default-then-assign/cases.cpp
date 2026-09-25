// spec: default-then-assign
#include <string>
#include <vector>

struct Point { int x; int y; };
struct Request { std::string path; int timeout; bool retry; };

int read_int();

Point aggregate() {
    Point p;                                            // expect: wrocpp-default-then-assign
    p.x = 1;
    p.y = 2;
    return p;
}

Request request(const std::string& path) {
    Request r;                                          // expect: wrocpp-default-then-assign
    r.path = path;
    r.timeout = 30;
    r.retry = true;
    return r;
}

std::vector<int> container() {
    std::vector<int> v;                                 // expect: wrocpp-default-then-assign
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    return v;
}

// Must not flag: filled from runtime data in a loop.
std::vector<int> from_input(int n) {
    std::vector<int> v;
    for (int i = 0; i < n; ++i) v.push_back(read_int());
    return v;
}

// Must not flag: a read of the object between the assignments.
Point dependent() {
    Point p;
    p.x = read_int();
    p.y = p.x * 2;
    return p;
}

// Must not flag: one assignment.
Point single() {
    Point p{};
    p.x = 3;
    return p;
}

// Must not flag: two mutator calls is below the threshold of three.
std::vector<int> two() {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    return v;
}
