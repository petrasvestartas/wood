#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 6000.0; // between the springing lines
const double RISE = 2000.0; // intrados crown below half the span: a segmental barrel
const double THICKNESS = 250.0; // out from the intrados
const double LENGTH = 6000.0; // along the barrel axis
const int COURSES = 9; // across the arch
const int RINGS = 6; // along the length
const double STAGGER = 0.5; // odd courses shifted half a ring: running bond

int main() {

    WoodSession wood_session("vault_barrel");
    for (const std::shared_ptr<Element>& element : wood_vault::barrel(
        SPAN,
        RISE,
        THICKNESS,
        LENGTH,
        COURSES,
        RINGS,
        STAGGER
    ))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("barrel: {} voussoirs, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A segmental barrel vault over 6 m, 6 m long, nine courses across and six rings along it, every odd course shifted half a ring with part voussoirs at the ends, after compas_dem's BarrelVaultTemplate; compute_face_contacts(0) pairs the courses along the bed joints and the rings along the head joints.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_2_barrel --parallel 4 && ./build/templates_vault_2_barrel && ../bash/publish-scene.sh --target templates_vault_2_barrel

|||||||| WORKFLOW ||||||||
examples/templates_vault_2_barrel.cpp
 |
 |-- wood_vault::barrel(SPAN, RISE, THICKNESS, LENGTH, COURSES, RINGS, STAGGER)                     src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
