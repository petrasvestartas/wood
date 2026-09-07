#include "src/templates/chevron.h"

#include <filesystem>

const char* OUTPUT = "data/templates/chevron_mesh.json";

int main() {
    std::filesystem::create_directories(std::filesystem::path(OUTPUT).parent_path());
    const Chevron shell;
    shell.mesh.file_json_dump(OUTPUT);
    return 0;
}

/*
description: build the chevron template -> write its mesh to data/templates/chevron_mesh.json.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_chevron -j8 && ./build/main_chevron
*/
