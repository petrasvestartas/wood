#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The floor's column as a session: its elements and the features they put on it.
int main() {

    WoodSession scene("element_column_session");

    const wood_floor::FloorGuide guide({
        Point(0.0, 0.0, 0.0),
        Point(6000.0, 0.0, 0.0),
        Point(6000.0, 6000.0, 0.0),
        Point(0.0, 6000.0, 0.0),
    });
    const wood_floor::Floor floor(guide);
    scene.graft(floor.get_branch("column_0"), nullptr);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The floor's column at corner 0 as a session of elements and the features they put on the column through add_interaction: the 220 square shaft, two blocks glued on for the 340 head over its top 730 (InteractionFeatureSolid add, hidden once glued), the support under it with its joint's seat and pins, and six hidden cutter plates of the floor guide each taking away an inclined face the ribs and the column blocks bear on (InteractionFeatureSolid subtract).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_column_session --parallel 6 && ./build/element_column_session && ../bash/publish-scene.sh --target element_column_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
