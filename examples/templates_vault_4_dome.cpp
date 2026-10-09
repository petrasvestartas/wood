#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double RADIUS = 4000.0; // intrados
const double BOTTOM = 400.0; // thickness at the springing
const double TOP = 250.0; // thickness at the oculus
const int MERIDIANS = 40;
const int HOOPS = 20;
const double OCULUS = 6.0; // degrees from the zenith
const double SPRINGING = 90.0; // degrees from the zenith: a hemisphere

int main() {

    WoodSession wood_session("vault_dome");
    for (const std::shared_ptr<Element>& element : wood_vault::dome(
        RADIUS,
        BOTTOM,
        TOP,
        MERIDIANS,
        HOOPS,
        OCULUS,
        SPRINGING
    ))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("dome: {} voussoirs, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A hemispherical dome of 4 m intrados radius with an oculus, 20 hoops of 40 voussoirs, every odd hoop shifted half a voussoir and every voussoir a hexagon bending at the joints above and below so the hoops close without gaps, 400 thick at the springing tapering to 250 at the oculus, after compas_dem's DomeTemplate; compute_face_contacts(0) pairs the voussoirs along their meridian joints and with the two voussoirs above and below.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_4_dome --parallel 4 && ./build/templates_vault_4_dome && ../bash/publish-scene.sh --target templates_vault_4_dome

|||||||| WORKFLOW ||||||||
examples/templates_vault_4_dome.cpp
 |
 |-- wood_vault::dome(RADIUS, BOTTOM, TOP, MERIDIANS, HOOPS, OCULUS, SPRINGING)                     src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
