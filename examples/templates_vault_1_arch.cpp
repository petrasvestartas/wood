#include "wood_session.h"
#include "src/templates/vault/arch.h"

using namespace session_cpp;
using namespace wood_session;

/// The semicircular arch of the defaults, its voussoirs in contact.
int main() {

    wood_vault::Arch arch;

    std::cout << fmt::format("arch: {} voussoirs, {} contacts\n", arch.objects.elements->size(), arch.graph.number_of_edges());
    arch.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A semicircular arch of 25 voussoirs over 6 m, 500 thick and 500 deep; compute_face_contacts(0) finds a contact between every two neighbours, 24 in all.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_1_arch --parallel 6 && ./build/templates_vault_1_arch && ../bash/publish-scene.sh --target templates_vault_1_arch

|||||||| WORKFLOW ||||||||
examples/templates_vault_1_arch.cpp
 |
 |-- wood_vault::Arch(guide)                       src/templates/vault/arch.cpp
 |    |-- wood_vault::arch(guide)                  src/templates/vault/vault.cpp
 |    |-- add(voussoir) for every voussoir
 |    '-- compute_face_contacts(0)              coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
