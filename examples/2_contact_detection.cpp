#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int DATASET = 19;                 // config::DATASET_NAMES

int main() {

    WoodSession wood_session = WoodSession::yaml_load(config::DATASET_NAMES[DATASET]);
    wood_session.compute_contacts();
    wood_session.add_to_tree(true, true, true, false);
    wood_session.pb_dump(pb_path("live").string());

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

|||||||| WORKFLOW ||||||||
examples/2_contact_detection.cpp
 |
 |-- WoodSession::yaml_load(dataset)             wood_session.cpp   (see 1_io: yml, obj, Plate, add)
 |
 |-- compute_contacts()                          wood_session.cpp
 |    '-- compute_face_contacts()
 |         |-- contact_view(scene)               one ContactElement per element: outlines, planes, name
 |         |-- face_contacts(elements)            src/joinery_solver/wood_face_to_face.cpp
 |         |    |-- adjacency_search              inflated OBB per element, BVH, OBB/OBB test -> candidate pairs
 |         |    '-- face_contacts_for_pair        faces_coplanar -> face_overlap_area (Clipper2)
 |         |                                      -> FaceContact {face_a, face_b, type, area}
 |         '-- set_interaction(a, b, ...)         the contacts onto the graph edge a-b (WoodInteraction)
 |
 |-- add_to_tree(true, true, true, false)        wood_session.cpp
 |    |-- one group per plate: the plate, "outlines"
 |    '-- add_contacts_to -> ring()               "contacts" child group: one coloured ring per FaceContact
 |
 '-- pb_dump(pb_path("live"))                    sync_geometry (Mesh::loft once per plate), Session::pb_dump

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
