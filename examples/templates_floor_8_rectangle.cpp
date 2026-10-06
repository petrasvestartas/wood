#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool BREPS = true; // write every cut member, connector part and dowel as its BRep, the dowel and screw bores exact cylinders, instead of its mesh
const double HALF_X = 3000.0; // half span along x: the bay is 6000 long
const double HALF_Y = 2400.0; // half span along y: the bay is 4800 wide

/// The rectangular bay with its columns, every connector and the assembly screws.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-HALF_X, -HALF_Y, 0.0),
        Point(HALF_X, -HALF_Y, 0.0),
        Point(HALF_X, HALF_Y, 0.0),
        Point(-HALF_X, HALF_Y, 0.0),
    });
    wood_floor::Floor floor(guide);
    floor.add_members();
    floor.add_connectors();
    floor.add_screws();

    if constexpr (BREPS)
        floor.compute_breps();

    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floor: the same model on a 6000 x 4800 bay, a FloorGuide on the corners (-HALF_X, -HALF_Y) to (-HALF_X, HALF_Y) counter-clockwise. The quarters are built in place at their own corners and mirror each other; the oculus is a square diamond of half-diagonal 1000; the central panel follows rule A, its inner ribs swept along one direction so the central bed is one planar-faced cylinder with every layer 27 thick; both outer ribs of a corner end at one level (the short ribs' run-in solved to 187.667), which is the middle cutter level, so every rib meets its column head within 0.307 mm. The connectors and screws are those of step 7. BREPS writes the BReps.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_8_rectangle --parallel 6 && ./build/templates_floor_8_rectangle && ../bash/publish-scene.sh --target templates_floor_8_rectangle

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
