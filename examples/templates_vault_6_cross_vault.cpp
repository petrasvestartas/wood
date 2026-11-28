#include "wood_session.h"
#include "src/templates/vault/cross_vault.h"

using namespace laika_cpp;
using namespace wood_session;

/// The cross vault of the defaults, its voussoirs in contact.
int main() {

    wood_vault::CrossVault cross_vault;

    std::cout << fmt::format("cross vault: {} voussoirs, {} contacts\n", cross_vault.objects.elements->size(), cross_vault.graph.number_of_edges());
    cross_vault.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A cross (groin) vault over a 6 m square bay: two semicircular barrels crossing, every web ten courses per half arch laid in running bond over twelve rings, the course at the crown shifted half a ring, and in every course one groin stone bent along the diagonal through both webs out to the first joint past the groin, the lowest a springer: 144 voussoirs and 40 groin stones, 184 blocks like the reference.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_6_cross_vault --parallel 6 && ./build/templates_vault_6_cross_vault && ../bash/publish-scene.sh --target templates_vault_6_cross_vault

|||||||| WORKFLOW ||||||||
examples/templates_vault_6_cross_vault.cpp
 |
 |-- wood_vault::CrossVault(guide)                       src/templates/vault/cross_vault.cpp
 |    |-- wood_vault::cross_vault(guide)                  src/templates/vault/vault.cpp
 |    |-- add(voussoir) for every voussoir
 |    '-- compute_face_contacts(0)              coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
