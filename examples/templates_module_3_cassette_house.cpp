#include "wood_session.h"
#include "src/templates/module/cassette_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// A gable house of cassettes: walls with a door and windows, two gable triangles and two roof slopes, the contacts between cassettes detected.
int main() {

    wood_module::CassetteShell house(wood_module::CassetteShell::house_mesh(), 160.0, 20.0, 60.0, "cassette_house");
    std::cout << house << std::endl;

    const size_t contacts = house.get_branch("contacts").objects.polylines->size();
    std::cout << fmt::format("cassette house: {} cassettes, {} contacts between them", house.mesh.number_of_faces(), contacts) << std::endl;
    house.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber modules: a 6000 x 4000 gable house of cassettes, walls 2700 high and the ridge at 4500. The four walls are split into panels on a grid with a door and a window in the front, a window in the back and one in each gable, each opening a cell left out so the cassettes around it close it with their edge beams. A triangle stands over each end wall and two roof slopes run to the ridge. Every cassette is 160 deep, two 20 mm plates and 60 mm edge beams, cut on the plane it shares with each neighbour: square where two panels of a wall meet in one plane, beveled at the wall corners, the eaves and the ridge. Every contact between cassettes is detected and drawn in contacts; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_3_cassette_house --parallel 6 && ./build/templates_module_3_cassette_house && ../bash/publish-scene.sh --target templates_module_3_cassette_house

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
