#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_features();

    for (const FeaturePlate& joint : wood_session.get_plate_features())
        std::cout << fmt::format("{} type {} between {} f{} and {} f{}: {} male, {} female outlines, {} divisions\n", joint.name, joint.joint_type, joint.element_a.substr(0, 8), joint.contact.face_a, joint.element_b.substr(0, 8), joint.contact.face_b, joint.male_outlines[0].size(), joint.female_outlines[0].size(), joint.divisions);

    const std::shared_ptr<Plate> plate = wood_session.plates().front();
    std::cout << fmt::format("{}: {} faces alone, {} faces with joints, {} joint features\n", plate->name, std::get<Mesh>(plate->element_geometry()).number_of_faces(), std::get<Mesh>(plate->model_geometry()).number_of_faces(), plate->Element::features().size());

    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the joinery pipeline on one dataset: every plate joint with its type, faces and cut outlines; a plate's geometry alone and with its joints cut in; the joint features the hosts carry.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 4_features --parallel 4 && ./build/4_features && ../bash/publish-scene.sh --target 4_features

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
