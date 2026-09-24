#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_corner};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_contacts();
    wood_session.compute_features();

    const std::shared_ptr<Plate> plate = wood_session.plates().front();
    for (const ElementFeature& feature : plate->Element::features())
        std::cout << fmt::format("{} {} on face {}: {} outlines\n", feature.feature_type, feature.name, feature.face_index, feature.outlines.size());

    wood_session.pb_dump(pb_path("live"));
    wood_session.set_features_visible("contact", false);
    wood_session.pb_dump(pb_path(wood_session.name));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
what the viewer sees of a wood session: every contact and joint on its elements as a feature in the colour of its type, put there as it was computed, beside the outline, axis and section features; set_features_visible("contact", false) switches one kind off without deleting it.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 11_viewer --parallel 4 && ./build/11_viewer && ../bash/publish-scene.sh --target 11_viewer

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
