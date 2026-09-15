#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int DATASET = 19;                 // globals::DATASET_NAMES

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
|||||||| DESCRIPTION ||||||||
Compute contacts between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/2_contact_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 2_contact_detection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
