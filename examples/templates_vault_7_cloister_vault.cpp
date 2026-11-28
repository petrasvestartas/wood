#include "wood_session.h"
#include "src/templates/vault/cloister_vault.h"

using namespace laika_cpp;
using namespace wood_session;

/// The cloister vault of the defaults, its voussoirs in contact.
int main() {

    wood_vault::CloisterVault cloister_vault;

    std::cout << fmt::format("cloister vault: {} voussoirs, {} contacts\n", cloister_vault.objects.elements->size(), cloister_vault.graph.number_of_edges());
    cloister_vault.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A cloister (pavilion) vault over a 6 m square bay: four webs rising from the four walls, each eight courses up to the crown by twelve rings along its wall, cut on the bay diagonals where the webs meet.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_7_cloister_vault --parallel 6 && ./build/templates_vault_7_cloister_vault && ../bash/publish-scene.sh --target templates_vault_7_cloister_vault

|||||||| WORKFLOW ||||||||
examples/templates_vault_7_cloister_vault.cpp
 |
 |-- wood_vault::CloisterVault(guide)                       src/templates/vault/cloister_vault.cpp
 |    |-- wood_vault::cloister_vault(guide)                  src/templates/vault/vault.cpp
 |    |-- add(voussoir) for every voussoir
 |    '-- compute_face_contacts(0)              coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
