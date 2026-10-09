#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The contacts of the square floor: the face every two joined members share, drawn red by the viewer.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide);
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floor: Floor(guide) on the 6000 x 6000 square, viewed for its contacts. Floor::add_contacts stores, for every two members the design joins, the face they share as a named contact interaction. The viewer draws every contact as a red fill on its faces, so View Xray shows them through the members. Per quarter q there are 17: seam_wedge_q, seam beam 0 beside the next quarter's seam beam; oculus_wedge_q, the oculus beam on its ring beam; column_plate_q_0 and column_plate_q_1, the column on each outer rib; six block_pins_q_b_side, each column block on the rib either side; two pins_outer_rib_q_k, the seam beam and the outer rib ending on it; two pins_seam_beam_q_k, the seam beam and the oculus beam; two pins_inner_rib_q_k, the oculus beam and the inner rib; and pins_ring_corner_q, ring beam q against ring beam q + 1. 68 contacts in all.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_3_contacts --parallel 6 && ./build/templates_floor_3_contacts && ../bash/publish-scene.sh --target templates_floor_3_contacts

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/?cmd=Arctic%20On;View%20Xray;View%20Isometric;Fit
*/
