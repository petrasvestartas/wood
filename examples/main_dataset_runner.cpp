#include "wood_pch.h"
#include "wood_session.h"

int main() {
    return type_plates_name_hexbox_and_corner() ? 0 : 1;
}

/*
description: run the selected wood dataset -> data/output/WoodF2F_hexbox_and_corner.pb; edit the call in main to select another dataset.

directory: cd ~/code/code_cpp/wood_research/wood
run: bash bash/cpp.sh main_dataset_runner
cloudflare: ../bash/publish-scene.sh data/output/WoodF2F_hexbox_and_corner.pb
view: https://petrasvestartas.github.io/session/
*/
