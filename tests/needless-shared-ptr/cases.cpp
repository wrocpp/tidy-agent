// spec: needless-shared-ptr
#include <memory>
#include <vector>

struct Widget { int value = 0; int get() const { return value; } };

int local_only() {
    auto w = std::make_shared<Widget>();                // expect: wrocpp-needless-shared-ptr
    w->value = 3;
    return w->get() + (w ? 1 : 0);
}

// Must not flag: returned.
std::shared_ptr<Widget> returned() {
    auto w = std::make_shared<Widget>();
    return w;
}

// Must not flag: copied into a container.
void stored(std::vector<std::shared_ptr<Widget>>& all) {
    auto w = std::make_shared<Widget>();
    all.push_back(w);
}

// Must not flag: captured by a lambda.
auto captured() {
    auto w = std::make_shared<Widget>();
    return [w] { return w->get(); };
}

void take(std::shared_ptr<Widget> w);

// Must not flag: passed by value.
void passed() {
    auto w = std::make_shared<Widget>();
    take(w);
}
