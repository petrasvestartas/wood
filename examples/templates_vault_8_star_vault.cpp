#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 8000.0;
const double RISE = 1000.0;
const double THICKNESS = 150.0;
const double STAR = 0.5;
const double RIB = 200.0;
const int SUBDIVISIONS = 8;

int main() {

    WoodSession wood_session("vault_star_vault");
    for (const std::shared_ptr<Element>& element :
         wood_vault::star_vault(SPAN, RISE, THICKNESS, STAR, RIB, SUBDIVISIONS))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format(
        "star vault: {} elements, {} contacts\n",
        wood_session.objects.elements->size(),
        wood_session.graph.number_of_edges()
    );
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A star (stellar) vault over an 8 m square bay (1 m rise) on intersecting circular barrel cylinders: eighteen single-curved planar ribs in vertical planes - two interlocking continuous diagonals, eight tiercerons from the corners to the star points, four continuous ridge ribs from the wall crowns through the star bosses to the central boss, and four wall arches - four corner support blocks, and the sixteen cylindrical webs between them, each 150 thick: a BRep solid whose intrados is a circular barrel cylinder trimmed along straight plan lines at the beam centers, whose extrados is shifted vertically by thickness and whose sides are vertical ruled surfaces neighbouring webs share.

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
