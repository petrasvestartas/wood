#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 6000.0;
const double RISE = 2000.0;
const double THICKNESS = 250.0;
const double LENGTH = 6000.0;
const int COURSES = 9;
const int RINGS = 6;
const double STAGGER = 0.2;
const bool CLOSED = true;

int main() {

    WoodSession wood_session("vault_barrel_staggered");
    for (const std::shared_ptr<Element>& element :
         wood_vault::barrel(SPAN, RISE, THICKNESS, LENGTH, COURSES, RINGS, STAGGER, CLOSED))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format(
        "barrel staggered: {} voussoirs, {} contacts\n",
        wood_session.objects.elements->size(),
        wood_session.graph.number_of_edges()
    );
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The barrel of templates_vault_2_barrel with its odd courses shifted a fifth of a ring and a closing ring of aligned voussoirs at both ends.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_3_barrel_staggered --parallel 4 && ./build/templates_vault_3_barrel_staggered && ../bash/publish-scene.sh --target templates_vault_3_barrel_staggered

|||||||| WORKFLOW ||||||||
examples/templates_vault_3_barrel_staggered.cpp
 |
 |-- wood_vault::barrel(SPAN, RISE, THICKNESS, LENGTH, COURSES, RINGS, STAGGER, CLOSED)             src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
