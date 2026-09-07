#include "file_obj.h"
#include "pair_polylines.h"
#include "polyline.h"

#include <fmt/core.h>

#include <filesystem>
#include <fstream>

using namespace session_cpp;

const char* INPUT = "data/annen_polylines.obj";
const char* OUTPUT = "data/annen_for_wood.xml";

int main() {
    const std::vector<Polyline> polylines = file_obj::read_file_obj_polylines(INPUT);
    std::ofstream out(OUTPUT);
    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?><input_polylines>";
    for (const auto& [a, b] : wood::pair_polylines(polylines))
        for (const int index : {a, b}) {
            out << "<polyline>";
            for (size_t k = 0; k < polylines[index].point_count(); k++) {
                const Point point = polylines[index].get_point(k);
                out << fmt::format("<point><x>{}</x><y>{}</y><z>{}</z></point>", point[0], point[1], point[2]);
            }
            out << "</polyline>";
        }
    out << "</input_polylines>";
    return 0;
}

/*
description: pair the annen polylines from an .obj -> write them as a wood input XML.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_export_xml -j8 && ./build/main_export_xml
*/
