#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The four column models of the square floor.
int main() {

    wood_floor::Floor floor(wood_floor::FloorGuide::rectangle(3000.0, 3000.0));
    floor.add_columns();
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floor: the column model of step 2 built in place at each of the four bay corners, one group per column with its index on every name.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_3_columns_model --parallel 6 && ./build/templates_floor_3_columns_model && ../bash/publish-scene.sh --target templates_floor_3_columns_model

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
