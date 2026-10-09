#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The oculus of the square floor.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide, wood_floor::FloorStep::oculus);
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the timber floor: Floor(guide, FloorStep::oculus), the quarters and the oculus ring built from the four quarters' oculus edges as a pinwheel, each ring beam between its edge's tilted bearing plane and the ring's inner plane, lifted to the floor at 3500: four ring beams as variable beams, four bottom wedges and the inner plate as plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_5_oculus --parallel 6 && ./build/templates_floor_5_oculus && ../bash/publish-scene.sh --target templates_floor_5_oculus

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
