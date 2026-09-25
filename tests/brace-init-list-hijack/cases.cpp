// spec: brace-init-list-hijack
#include <initializer_list>
#include <string>
#include <vector>

// A JSON-like value type: an initializer_list constructor next to a copy
// constructor, the shape that turned an object into a one-element array.
struct Value {
    Value() = default;
    Value(int) {}
    Value(std::initializer_list<Value>) {}
};

Value wrap(const Value& v) { return {v}; }              // expect: custom-wrocpp-brace-init-list-hijack

void cases(const Value& v) {
    Value a{v};                                         // expect: custom-wrocpp-brace-init-list-hijack
    Value b(v);
    Value c{1, 2};
    Value d{Value{}, Value{}};
    std::vector<int> e{5};
    std::vector<std::string> f{"x"};
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
}
