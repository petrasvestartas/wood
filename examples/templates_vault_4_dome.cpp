#include "wood_session.h"
#include "src/templates/vault/dome.h"

using namespace session_cpp;
using namespace wood_session;

/// The dome of the defaults, its staggered hoops in contact.
int main() {

    wood_vault::Dome dome;

    std::cout << fmt::format("dome: {} voussoirs, {} contacts\n", dome.objects.elements->size(), dome.graph.number_of_edges());
    dome.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A hemispherical dome of 4 m intrados radius with an oculus, 20 hoops of 40 voussoirs, every odd hoop shifted half a voussoir and every voussoir a hexagon bending at the joints above and below so the hoops close without gaps, 400 thick at the springing tapering to 250 at the oculus; compute_face_contacts(0) pairs the voussoirs along their meridian joints and with the two voussoirs above and below.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_4_dome --parallel 6 && ./build/templates_vault_4_dome && ../bash/publish-scene.sh --target templates_vault_4_dome

|||||||| WORKFLOW ||||||||
examples/templates_vault_4_dome.cpp
 |
 |-- wood_vault::Dome(guide)                       src/templates/vault/dome.cpp
 |    |-- wood_vault::dome(guide)                  src/templates/vault/vault.cpp
 |    |-- add(voussoir) for every voussoir
 |    '-- compute_face_contacts(0)              coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
