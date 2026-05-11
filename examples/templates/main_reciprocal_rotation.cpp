// Build and run from wood/ root:
//   cmake -B build
//   cmake --build build --config Release --target main_reciprocal_rotation
//   ./build/Release/main_reciprocal_rotation.exe

#include "src/templates/reciprocal_rotation.h"
#include <filesystem>
#include <iostream>

int main() {
    auto out = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
               / "data" / "templates";
    std::filesystem::create_directories(out);

    ReciprocalRotation rb;

    std::cout << "dome:   " << rb.dome_mesh.number_of_vertices() << " vertices  "
              << rb.dome_mesh.number_of_faces() << " faces\n";
    std::cout << "beams:  " << rb.beams.size() << "\n";
    std::cout << "side0:  " << rb.side0.size() << " outlines\n";
    std::cout << "side1:  " << rb.side1.size() << " outlines\n";

    rb.dome_mesh.file_json_dump((out / "reciprocal_dome.json").string());

    return 0;
}
