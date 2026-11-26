#include "wood_session.h"
#include "src/templates/vault/wall.h"

using namespace session_cpp;
using namespace wood_session;

/// The brick wall of the defaults, its bricks in contact.
int main() {

    wood_vault::Wall wall;

    std::cout << fmt::format("wall: {} bricks, {} contacts\n", wall.objects.elements->size(), wall.graph.number_of_edges());
    wall.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A running bond wall 4 m long and 2 m high of 400 by 200 bricks, half bricks closing every odd course; compute_face_contacts(0) pairs the bricks along the bed and head joints.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_5_wall --parallel 6 && ./build/templates_vault_5_wall && ../bash/publish-scene.sh --target templates_vault_5_wall

|||||||| WORKFLOW ||||||||
examples/templates_vault_5_wall.cpp
 |
 |-- wood_vault::Wall(guide)                       src/templates/vault/wall.cpp
 |    |-- wood_vault::wall(guide)                  src/templates/vault/vault.cpp
 |    |-- add(brick) for every brick
 |    '-- compute_face_contacts(0)              coplanar face overlaps between every pair
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
