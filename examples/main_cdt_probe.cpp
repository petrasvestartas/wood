#include "../src/mesh.h"

#include <fmt/core.h>

#include <array>
#include <utility>
#include <vector>

namespace session_cpp {
std::vector<std::array<int, 3>> cdt_triangulate(
    const std::vector<std::pair<double, double>>&,
    const std::vector<std::vector<std::pair<double, double>>>&
);
}

int main() {
    using namespace session_cpp;
    const std::vector<std::pair<double, double>> outer = {
        {-1172.487, -530.170},
        {318.768, -530.170},
        {318.768, -318.102},
        {414.110, -347.792},
        {414.110, -106.034},
        {318.768, -135.724},
        {318.768, 106.034},
        {414.110, 76.344},
        {414.110, 318.102},
        {318.768, 288.412},
        {318.768, 530.170},
        {-1172.487, 530.170},
    };
    const std::vector<std::vector<std::pair<double, double>>> holes = {
        {{-1006.792, 97.448}, {-1006.792, 0.0}, {-841.097, 0.0}, {-841.097, 97.448}},
        {{-344.012, 97.448}, {-344.012, 0.0}, {-178.317, 0.0}, {-178.317, 97.448}},
    };
    fmt::print("holes {} outer {} one {}\n", cdt_triangulate(outer, holes).size(), cdt_triangulate(outer, {}).size(), cdt_triangulate(outer, {holes[0]}).size());
    return 0;
}

/*
description: triangle counts of one annen plate outline with two, zero and one hole.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_cdt_probe -j8 && ./build/main_cdt_probe
*/
