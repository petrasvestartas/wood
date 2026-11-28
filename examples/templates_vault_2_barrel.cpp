#include "wood_session.h"
#include "src/templates/vault/barrel.h"

using namespace laika_cpp;
using namespace wood_session;

/// The segmental barrel of the defaults, its staggered voussoirs in contact.
int main() {

    wood_vault::Barrel barrel;

    std::cout << fmt::format("barrel: {} voussoirs, {} contacts\n", barrel.objects.elements->size(), barrel.graph.number_of_edges());
    barrel.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A segmental barrel vault over 6 m, 6 m long, nine courses across and six rings along it, every odd course shifted half a ring with part voussoirs at the ends; compute_face_contacts(0) pairs the courses along the bed joints and the rings along the head joints.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_2_barrel --parallel 6 && ./build/templates_vault_2_barrel && ../bash/publish-scene.sh --target templates_vault_2_barrel

|||||||| WORKFLOW ||||||||
examples/templates_vault_2_barrel.cpp
 |
 |-- wood_vault::Barrel(guide)                       src/templates/vault/barrel.cpp
 |    |-- wood_vault::barrel(guide)                  src/templates/vault/vault.cpp
 |    |-- add(voussoir) for every voussoir
 |    '-- compute_face_contacts(0)              coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
