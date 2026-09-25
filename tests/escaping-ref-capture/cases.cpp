// spec: escaping-ref-capture
#include <algorithm>
#include <functional>
#include <vector>

using Handler = std::function<int()>;

auto returned_explicit() {
    std::vector<int> items{1, 2, 3};
    return [&items] { return items.size(); };           // expect: wrocpp-escaping-ref-capture
}

Handler returned_default() {
    int count = 3;
    return [&] { return count; };                       // expect: wrocpp-escaping-ref-capture
}

struct Registry {
    std::vector<Handler> handlers;
    void add(Handler h) { handlers.push_back(std::move(h)); }
};

void stored(Registry& registry) {
    int local = 7;
    Handler h = [&local] { return local; };             // expect: wrocpp-escaping-ref-capture
    registry.add(h);
}

// Must not flag: capture by value.
Handler by_value() {
    int count = 3;
    return [count] { return count; };
}

// Must not flag: called synchronously by an algorithm before the frame ends.
int synchronous(std::vector<int>& v) {
    int total = 0;
    std::for_each(v.begin(), v.end(), [&total](int x) { total += x; });
    return total;
}
