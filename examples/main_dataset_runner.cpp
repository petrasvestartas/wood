#include "wood_test.h"

int main() {
    return type_plates_name_hexbox_and_corner() ? 0 : 1;
}

/*
|||||||| DESCRIPTION ||||||||
run the selected wood dataset -> data/output/WoodF2F_hexbox_and_corner.pb; edit the call in main to select another dataset.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target main_dataset_runner --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/main_dataset_runner && ../bash/publish-scene.sh --target main_dataset_runner

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
