#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int DATASET = 20;                 // globals::DATASET_NAMES

int main() {

    WoodSession wood_session = WoodSession::yaml_load(globals::DATASET_NAMES[DATASET]);
    
    // Finds nearby element pairs using OBB/BVH.
    // Tests face planes for coplanarity.
    // Computes the polygon overlap.
    // Stores every valid touching face pair.
    wood_session.compute_contacts();

    // TODO: view.add_contacts_by_type | view.add_outlines | view.pb_dump
    // side_side - navy
    // side_top - pink
    // top_top - green
    // unknown - grey
    add_contacts_by_type(wood_session, wood_session.contacts());
    add_outlines(wood_session, wood_session);
    pb_dump(wood_session, "live");

    return 0;
}

/*
description: one dataset by index -> face contacts between its elements, or the joints the solver makes between its plates, each on the graph edge of its pair -> drawn as the elements' bottom and top outlines with one colored ring per contact or joint type..

directory: cd "$(git rev-parse --show-toplevel)"
configure: cmake -S . -B build
build:  cmake --build build --config Release --parallel
run: ./build/2_contact_detection
cloudflare: bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 2_contact_detection
view: https://petrasvestartas.github.io/session/
*/
