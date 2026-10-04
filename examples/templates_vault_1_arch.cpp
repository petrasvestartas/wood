#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN = 6000.0; // between the springings
const double RISE = 3000.0; // intrados crown, half the span: a semicircle
const double THICKNESS = 500.0; // out from the intrados
const double DEPTH = 500.0; // across the arch
const int VOUSSOIRS = 25;

int main() {

    WoodSession wood_session("vault_arch");
    for (const std::shared_ptr<Element>& element : wood_vault::arch(SPAN, RISE, THICKNESS, DEPTH, VOUSSOIRS))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("arch: {} voussoirs, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A semicircular arch of 25 voussoirs over 6 m, 500 thick and 500 deep, after compas_dem's ArchTemplate; compute_face_contacts(0) finds a contact between every two neighbours, 24 in all.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_1_arch --parallel 4 && ./build/templates_vault_1_arch && ../bash/publish-scene.sh --target templates_vault_1_arch

|||||||| WORKFLOW ||||||||
examples/templates_vault_1_arch.cpp
 |
 |-- wood_vault::arch(SPAN, RISE, THICKNESS, DEPTH, VOUSSOIRS)                                      src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
