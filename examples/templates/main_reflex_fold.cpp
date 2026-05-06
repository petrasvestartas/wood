// Build and run from wood/ root:
//   cmake -B build
//   cmake --build build --config Release --target main_reflex_fold
//   ./build/Release/main_reflex_fold.exe

#include "src/templates/reflex_fold.h"
#include <filesystem>
#include <iostream>

int main() {
    auto out = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
               / "data" / "templates";
    std::filesystem::create_directories(out);

    ReflexFold rf;

    std::cout << "mesh:     " << rf.mesh.number_of_vertices() << " vertices  "
              << rf.mesh.number_of_faces() << " faces\n";
    std::cout << "elements: " << rf.elements.size() << "\n";

    for (const auto& el : rf.elements) {
        Mesh plate = el.loft_mesh();
        std::cout << plate << "\n";
    }

    rf.mesh.file_json_dump((out / "reflex_fold_mesh.json").string());

    return 0;
}
