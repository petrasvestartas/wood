// Build and run from wood/ root:
//   cmake -B build
//   cmake --build build --config Release --target main_reciprocal_move
//   ./build/Release/main_reciprocal_move.exe

#include "src/templates/reciprocal_move.h"
#include <filesystem>
#include <iostream>

int main() {
    auto out = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
               / "data" / "templates";
    std::filesystem::create_directories(out);

    // Default 12×10 sinusoidal dome, 50mm translation offset, 100mm beam width
    ReciprocalMove rm(12, 10, 12000.0, 10000.0, 3000.0, 50.0, 100.0, 200.0);

    std::cout << "dome:   " << rm.dome_mesh.number_of_vertices() << " vertices  "
              << rm.dome_mesh.number_of_faces() << " faces\n";
    std::cout << "beams:  " << rm.beams.size() << "\n";
    std::cout << "side0:  " << rm.side0.size() << " outlines\n";
    std::cout << "side1:  " << rm.side1.size() << " outlines\n";

    rm.dome_mesh.file_json_dump((out / "reciprocal_move_dome.json").string());

    return 0;
}
