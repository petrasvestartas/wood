// Build and run from wood/ root:
//   cmake -B build
//   cmake --build build --config Release --target main_translation_shell
//   ./build/Release/main_translation_shell.exe

#include "src/templates/translation_shell.h"
#include <filesystem>
#include <iostream>

int main() {

    auto out = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "templates";
    std::filesystem::create_directories(out);

    // Construct the translation shell with default cross-section and profile.
    TranslationShell ts;


    // plate meshes: loft each element's bottom/top polylines into a solid
    for (const auto& el : ts.elements) {
        Mesh plate = el.loft_mesh();
        std::cout << plate << "\n";
    }

    // Write shell mesh to json
    std::cout << ts.mesh << "\n";
    ts.mesh.file_json_dump((out / "translation_shell_mesh.json").string());

    return 0;
}
