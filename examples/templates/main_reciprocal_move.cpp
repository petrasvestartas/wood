#include "src/templates/reciprocal_move.h"

#include <filesystem>

const char* OUTPUT = "data/templates/reciprocal_move_dome.json";

int main() {
    std::filesystem::create_directories(std::filesystem::path(OUTPUT).parent_path());
    const ReciprocalMove shell(12, 10, 12000.0, 10000.0, 3000.0, 50.0, 100.0, 200.0);
    shell.dome_mesh.file_json_dump(OUTPUT);
    return 0;
}

/*
description: build the reciprocal_move template -> write its mesh to data/templates/reciprocal_move_dome.json.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_reciprocal_move -j8 && ./build/main_reciprocal_move
*/
