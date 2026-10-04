#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double LENGTH = 4000.0; // along x
const double HEIGHT = 2000.0;
const double THICKNESS = 200.0;
const double BRICK = 400.0; // brick length
const double COURSE = 200.0; // course height
const double STAGGER = 0.5; // running bond

int main() {

    WoodSession wood_session("vault_wall");
    for (const std::shared_ptr<Element>& element : wood_vault::wall(LENGTH, HEIGHT, THICKNESS, BRICK, COURSE, STAGGER))
        wood_session.add(element);

    wood_session.compute_face_contacts(0);

    std::cout << fmt::format("wall: {} bricks, {} contacts\n", wood_session.objects.elements->size(), wood_session.graph.number_of_edges());
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A running bond wall 4 m long and 2 m high of 400 by 200 bricks, half bricks closing every odd course, the wall compas_dem's WallTemplate leaves unwritten; compute_face_contacts(0) pairs the bricks along the bed and head joints.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_5_wall --parallel 4 && ./build/templates_vault_5_wall && ../bash/publish-scene.sh --target templates_vault_5_wall

|||||||| WORKFLOW ||||||||
examples/templates_vault_5_wall.cpp
 |
 |-- wood_vault::wall(LENGTH, HEIGHT, THICKNESS, BRICK, COURSE, STAGGER)                            src/templates/vault/vault.cpp
 |-- add(element) for every voussoir
 |-- compute_face_contacts(0)                    coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
