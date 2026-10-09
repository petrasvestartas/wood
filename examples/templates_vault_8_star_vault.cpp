#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 8000.0; // side of the square bay
const double RISE = 2500.0; // crown of the sail above the corners
const double THICKNESS = 150.0; // of the webs
const double STAR = 0.5; // star points at half the half span on the axes
const double RIB = 200.0; // square rib section
const int SUBDIVISIONS = 8; // sail control net and samples per rib

int main() {

    WoodSession wood_session("vault_star_vault");
    for (const std::shared_ptr<Element>& element : wood_vault::star_vault(
        SPAN,
        RISE,
        THICKNESS,
        STAR,
        RIB,
        SUBDIVISIONS
    ))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("star vault: {} elements, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A star (stellar) vault over an 8 m square bay on the sail surface through its corners: twenty ribs - four diagonals, eight tiercerons from the corners to the star points, four liernes to the crown and four wall arches - and the twelve webs between them, each 150 thick: a BRep solid whose intrados is one cubic NURBS sail surface trimmed by its three ribs, whose extrados is that surface moved out from the sphere centre and whose sides are ruled surfaces neighbouring webs share.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_8_star_vault --parallel 4 && ./build/templates_vault_8_star_vault && ../bash/publish-scene.sh --target templates_vault_8_star_vault

|||||||| WORKFLOW ||||||||
examples/templates_vault_8_star_vault.cpp
 |
 |-- wood_vault::star_vault(SPAN, RISE, THICKNESS, STAR, RIB, SUBDIVISIONS)                         src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
