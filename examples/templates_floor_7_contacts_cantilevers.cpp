#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool BREPS = true; // write every cut member, connector part and pin as its BRep, the pin bores exact cylinders, instead of its mesh

/// The square bay with its columns, every connector and the assembly pins.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide);

    if constexpr (BREPS)
        floor.compute_breps();

    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the timber floor: the whole square bay, the four quarters, the oculus and the four columns on their supports, with every connector: the wedges of step 6, cut flush with the floor top; a rectangle plate on every column-to-outer-rib contact (30 thick, 220 into the column, 265 into the rib, four pins), the two plates of a column head half-lapped; four Ø8 x 30 pins on every wedge block to rib contact; and last the 48 assembly pins (200 x d4, pre-drilled, no member cut). The seam beams and their wedge run on through the outer rib band to the bay's outer face, the outer ribs ending on the beams with horizontal pins from the beam's seam face; the oculus has no pins. Every member carries a drill feature per hole, every column its head cuts as cut features. BREPS writes every cut member, connector part and pin as its BRep. Connectors are BRG blue, each in connectors_q of its quarter_q, the oculus wedges in connectors of oculus.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_7_contacts_cantilevers --parallel 6 && ./build/templates_floor_7_contacts_cantilevers && ../bash/publish-scene.sh --target templates_floor_7_contacts_cantilevers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
