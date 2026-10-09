#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 6000.0; // side of the square bay
const double RISE = 3000.0; // semicircular webs
const double THICKNESS = 480.0; // out from the intrados, the compas_model proportion
const int COURSES = 10; // per half arch of each web
const int RINGS = 12; // along the bay side, in running bond

int main() {

    WoodSession wood_session("vault_cross_vault");
    for (const std::shared_ptr<Element>& element : wood_vault::cross_vault(
        SPAN,
        RISE,
        THICKNESS,
        COURSES,
        RINGS
    ))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("cross vault: {} voussoirs, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A cross (groin) vault over a 6 m square bay in the stone pattern of compas_model's cross vault example (data/cross_vault.obj), made parametric: two semicircular barrels crossing, every web ten courses per half arch laid in running bond over twelve rings, the course at the crown shifted half a ring, and in every course one groin stone bent along the diagonal through both webs out to the first joint past the groin, the lowest a springer: 144 voussoirs and 40 groin stones, 184 blocks like the reference.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_6_cross_vault --parallel 4 && ./build/templates_vault_6_cross_vault && ../bash/publish-scene.sh --target templates_vault_6_cross_vault

|||||||| WORKFLOW ||||||||
examples/templates_vault_6_cross_vault.cpp
 |
 |-- wood_vault::cross_vault(SPAN, RISE, THICKNESS, COURSES, RINGS)                                 src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
