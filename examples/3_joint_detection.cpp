#include "wood_session.h"

using namespace wood_session;

// globals::Dataset::Face::<name> for SEARCH = face_to_face, ::Cross::<name> for cross_joint.
const std::string DATASET = globals::Dataset::Cross::cross_corners;
const SearchType SEARCH = cross_joint;  // face_to_face | cross_joint | face_to_face_then_cross

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);

    // Finds nearby element pairs using OBB/BVH.
    // Tests face planes for coplanarity.
    // Computes the polygon overlap.
    // Stores every valid touching face pair.
    // joint alignment lines
    // minimum joint length
    // insertion direction
    // plate orientation
    // dihedral angle
    // male/female orientation
    // joint volume and cut geometry
    wood_session.compute_joints(face_to_face);

    // TODO: view.add_contacts_by_type | view.add_outlines | view.pb_dump
    // side-side, out-of-plane        orange    11
    // side-side, in-plane            navy      12
    // rotated side-side              dark pink 13
    // top-to-side                              20
    // top-to-top                               40
    // crossing geometry                        30
    add_joints_by_type(wood_session, wood_session.joints());
    pb_dump(wood_session, "live");
    
    return 0;
}

/*
description: one dataset by name -> get_connection_zones over its plates -> every joint the
solver refined (cut volumes, insertion vectors, male/female outlines, linked joints), each on
the graph edge of its pair, drawn as one group per joint_type. The raw contact preview this
detail is refined from - no cut geometry, just where elements touch or cross - is
2_contact_detection.cpp instead.

directory: cd "$(git rev-parse --show-toplevel)"
configure: cmake -S . -B build
build:  cmake --build build --config Release --parallel
run: ./build/3_joint_detection
cloudflare: bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 3_joint_detection
view: https://petrasvestartas.github.io/session/
*/
