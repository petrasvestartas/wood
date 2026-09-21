#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::cross_corners};

int main() {

    WoodSession scene = WoodSession::yaml_load(DATASET);
    scene.settings.joint_volume_extension = {0, 2, 0};

    scene.compute_features(cross_joint);
    for (const FeaturePlate& joint : scene.get_plate_features())
        std::cout << fmt::format("{} type {}: side faces ({},{}) and ({},{}), volumes {}\n", joint.name, joint.joint_type, joint.contact.face_a, joint.cross_faces[0], joint.contact.face_b, joint.cross_faces[1], joint.joint_volumes[0].has_value() + joint.joint_volumes[1].has_value());

    scene.compute_features(face_to_face_then_cross);
    std::cout << scene.get_plate_features().size() << " joints with face-to-face first and cross as the fallback\n";

    scene.add_to_tree();
    scene.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
plates passing through each other solved as cross joints (type 30), the search type chosen per solve: cross only, or face-to-face with cross as the fallback.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 12_cross_joints --parallel 4 && ./build/12_cross_joints && ../bash/publish-scene.sh --target 12_cross_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
