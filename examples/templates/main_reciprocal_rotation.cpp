#include "src/templates/reciprocal_rotation.h"

#include <filesystem>

const char* OUTPUT = "data/templates/reciprocal_dome.json";

int main() {
    std::filesystem::create_directories(std::filesystem::path(OUTPUT).parent_path());
    const ReciprocalRotation shell;
    shell.dome_mesh.file_json_dump(OUTPUT);
    return 0;
}

/*
description: build the reciprocal_rotation template -> write its mesh to data/templates/reciprocal_dome.json.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_reciprocal_rotation -j8 && ./build/main_reciprocal_rotation
*/
