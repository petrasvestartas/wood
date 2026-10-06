#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The column model at corner 0 of the square floor.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide);
    floor.add_column(0);
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the timber floor: the support of corner 0, a Sherpa Power Base L 140 C from its datasheet, and the 220 square column standing on it from the support's head plate to the floor at 3500, with the head 120 wider on the two bay sides over the top 730. The support joint lets the head plate into the column end and drills the three column screws; the six column cutters of quarter 0 carve the head down to the outer rib bottoms.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_2_column_model --parallel 6 && ./build/templates_floor_2_column_model && ../bash/publish-scene.sh --target templates_floor_2_column_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
