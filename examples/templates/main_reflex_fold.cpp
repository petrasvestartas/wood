#include "src/templates/reflex_fold.h"

#include <filesystem>

const char* OUTPUT = "data/templates/reflex_fold_mesh.json";

int main() {
    std::filesystem::create_directories(std::filesystem::path(OUTPUT).parent_path());
    const ReflexFold shell;
    shell.mesh.file_json_dump(OUTPUT);
    return 0;
}

/*
description: build the reflex_fold template -> write its mesh to data/templates/reflex_fold_mesh.json.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_reflex_fold -j8 && ./build/main_reflex_fold
*/
