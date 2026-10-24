#include "wood_session.h"
#include "src/templates/module/cassette_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// A gable house of six cassettes.
int main() {

    wood_module::CassetteShell house(
        wood_module::CassetteShell::house_mesh(),
        160.0,
        20.0,
        60.0,
        wood_module::CassetteShell::house_windows(),
        625.0,
        wood_module::CassetteShell::house_ranks(),
        "cassette_house"
    );
    std::cout << house << std::endl;

    const size_t contacts = house.get_branch("contacts").objects.meshes->size();
    std::cout << fmt::format(
                     "cassette house: {} cassettes, {} contacts between them",
                     house.mesh.number_of_faces(),
                     contacts
                 )
              << std::endl;
    house.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber modules: a 6000 x 4000 gable house of six cassettes, walls 2700 high and the ridge at 4500, one cassette per whole face: the front with its door a notch from the ground, the two gable ends as pentagons, the back and the two roof slopes. Each cassette is 160 deep: a 20 mm panel on either side with its windows cut out, one in the front, the back and each gable, and between the panels 60 mm edge beams on its boundary, a jamb stud either side, a header and a sill around every window, and the studs and rafters on the 625 mm grid from each face's first corner, a grid stud that falls on an edge beam or a jamb being that member and one beside it staying, cut on the inner faces of the beams over and under them. The cassettes meet in T joints, the roof slopes running over the walls and gable ends and the long walls past the gable ends, the one running through ending flush with the other's outer face and the other cut on its inner face, so the fewest cuts are inclined; inside a cassette the more level beam runs through. Every contact between cassettes is detected and drawn in contacts as a red mesh; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_3_cassette_house --parallel 6 && ./build/templates_module_3_cassette_house && ../bash/publish-scene.sh --target templates_module_3_cassette_house

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
