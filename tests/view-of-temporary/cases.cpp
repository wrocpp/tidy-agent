// spec: view-of-temporary
#include <span>
#include <string>
#include <string_view>
#include <vector>

// A project view type: holds a pointer and a length into someone else's bytes.
class Slice {
public:
    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}
    const char* data() const { return data_; }
    std::size_t size() const { return size_; }
private:
    const char* data_;
    std::size_t size_;
};

std::string make();

void cases(const std::string& named) {
    Slice a{std::string{"abc"}};                        // expect: bugprone-dangling-handle
    Slice b(make());                                    // expect: bugprone-dangling-handle
    std::string_view c = make();                        // expect: bugprone-dangling-handle
    Slice d{named};
    std::string_view e = named;
    std::string_view f = "literal";
    auto g = make().size();
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
}
