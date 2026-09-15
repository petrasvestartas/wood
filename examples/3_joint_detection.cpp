#include "wood_session.h"

using namespace wood_session;

// globals::Dataset::Face::<name> for SEARCH = face_to_face, ::Cross::<name> for cross_joint.
const std::string DATASET = globals::Dataset::inplane_hilti;
const SearchType SEARCH = SearchType::face_to_face;  // face_to_face | cross_joint | face_to_face_then_cross <- these must be part of dataset, so we wont type that.

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
    wood_session.compute_joints(SEARCH);

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
|||||||| DESCRIPTION ||||||||
Compute joints between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/3_joint_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 3_joint_detection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/