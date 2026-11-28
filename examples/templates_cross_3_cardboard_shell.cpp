#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// The corrugated cardboard shell of the VDA pavilion: the dataset's plates and connectors with their cross joints.
int main() {

    WoodSession cardboard_shell = WoodSession::yaml_load(config::Dataset::cross_vda_shell);
    cardboard_shell.compute_features();
    std::cout << cardboard_shell.get_plate_features().size() << " cross joints in "
              << cardboard_shell.get_elements<FacePlanarElement>().size() << " plates" << std::endl;
    cardboard_shell.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The corrugated cardboard shell of the VDA pavilion, data/cross_vda_shell.yml: 1516 cardboard plates about 3.5 thick, two layers of quads 90 apart and the connector plates through both layers, solved as 2938 cross joints (type 30), each a half-lap slot merged into both outlines.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cross_3_cardboard_shell --parallel 6 && ./build/templates_cross_3_cardboard_shell && ../bash/publish-scene.sh --target templates_cross_3_cardboard_shell

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
