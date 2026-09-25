// spec: array-counted-size
#include <array>

const std::array<int, 3> counted{1, 2, 3};              // expect: custom-wrocpp-array-counted-size
constexpr std::array<int, 4> short_list{1, 2};         // expect: custom-wrocpp-array-counted-size

constexpr auto deduced = std::to_array<int>({1, 2, 3});
std::array<int, 3> mutable_table{1, 2, 3};
const std::array<int, 8> zeroed{};

int use() { return counted[0] + short_list[0] + deduced[0] + mutable_table[0] + zeroed[0]; }
