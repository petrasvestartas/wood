// Build and run from wood/ root:
//   cmake -B build
//   cmake --build build --config Release --target main_chevron
//   ./build/Release/main_chevron.exe

#include "src/templates/chevron.h"
#include <filesystem>
#include <iostream>

int main() {
    auto out = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
               / "data" / "templates";
    std::filesystem::create_directories(out);

    Chevron ch;

    std::cout << "mesh:     " << ch.mesh.number_of_vertices() << " vertices  "
              << ch.mesh.number_of_faces() << " faces\n";
    std::cout << "elements: " << ch.elements.size()
              << "  (4 plate-pairs per mesh face)\n";

    for (const auto& el : ch.elements) {
        Mesh plate = el.loft_mesh();
        std::cout << plate << "\n";
    }

    ch.mesh.file_json_dump((out / "chevron_mesh.json").string());

    return 0;
}
