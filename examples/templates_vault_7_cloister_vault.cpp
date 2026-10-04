#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 6000.0; // side of the square bay
const double RISE = 3000.0; // semicircular webs
const double THICKNESS = 300.0; // out from the intrados
const int COURSES = 8; // from each wall up to the crown
const int RINGS = 12; // along each wall

int main() {

    WoodSession wood_session("vault_cloister_vault");
    for (const std::shared_ptr<Element>& element : wood_vault::cloister_vault(SPAN, RISE, THICKNESS, COURSES, RINGS))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("cloister vault: {} voussoirs, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A cloister (pavilion) vault over a 6 m square bay after compas_tna's pavillionvault envelope: four webs rising from the four walls, each eight courses up to the crown by twelve rings along its wall, cut on the bay diagonals where the webs meet.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_7_cloister_vault --parallel 4 && ./build/templates_vault_7_cloister_vault && ../bash/publish-scene.sh --target templates_vault_7_cloister_vault

|||||||| WORKFLOW ||||||||
examples/templates_vault_7_cloister_vault.cpp
 |
 |-- wood_vault::cloister_vault(SPAN, RISE, THICKNESS, COURSES, RINGS)                              src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
