#include "src/templates/translation_shell.h"

#include <filesystem>

const char* OUTPUT = "data/templates/translation_shell_mesh.json";

int main() {
    std::filesystem::create_directories(std::filesystem::path(OUTPUT).parent_path());
    const TranslationShell shell;
    shell.mesh.file_json_dump(OUTPUT);
    return 0;
}

/*
description: build the translation_shell template -> write its mesh to data/templates/translation_shell_mesh.json.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_translation_shell -j8 && ./build/main_translation_shell
*/
