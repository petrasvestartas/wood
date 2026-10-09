#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The four column models of the square floor.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide, wood_floor::FloorStep::columns);
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floor: Floor(guide, FloorStep::columns), the floor up to its columns: the quarters, the oculus, and the column model of step 2 built in place at each of the four bay corners, one group per column with its index on every name.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_3_columns_model --parallel 6 && ./build/templates_floor_3_columns_model && ../bash/publish-scene.sh --target templates_floor_3_columns_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
