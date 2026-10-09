#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The quarters and the oculus of the square floor with their wedges.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide, wood_floor::FloorStep::connectors);
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber floor: Floor(guide, FloorStep::connectors), every member with its connectors and no screws; on each seam and oculus contact a wedge connector: a triangular wedge along the contact's top edge with horizontal dowels, a box pocket under the wedge in each beam and the dowel holes. Every contact is read from the members' outlines. The connectors are red and sit in the tree by the members they join: oculus > connectors_oculus, floor_model > seams > seam_k.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_6_contacts_floor --parallel 6 && ./build/templates_floor_6_contacts_floor && ../bash/publish-scene.sh --target templates_floor_6_contacts_floor

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
