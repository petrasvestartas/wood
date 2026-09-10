#include "wood_session.h"

using namespace wood_session;

// globals::Dataset::Face::<name> for SEARCH = face_to_face, ::Cross::<name> for cross_joint.
const std::string DATASET = globals::Dataset::Cross::cross_corners;
const SearchType SEARCH = cross_joint;  // face_to_face | cross_joint | face_to_face_then_cross

int main() {
    WoodSession scene = WoodSession::yaml_load(DATASET);
    scene.compute_joints(SEARCH);
    add_joints_by_type(scene, scene.joints());
    pb_dump(scene, "live");
    return 0;
}

/*
description: one dataset by name -> get_connection_zones over its plates -> every joint the
solver refined (cut volumes, insertion vectors, male/female outlines, linked joints), each on
the graph edge of its pair, drawn as one group per joint_type. The raw contact preview this
detail is refined from - no cut geometry, just where elements touch or cross - is
2_contact_detection.cpp instead.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 3_joint_detection -j8 && ./build/3_joint_detection
cloudflare: ../bash/publish-scene.sh --target 3_joint_detection
view: https://petrasvestartas.github.io/session/
*/
