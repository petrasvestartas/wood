#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The Hilti connector between two 200 mm CLT slabs folded 30 degrees on a mitred seam, drawn apart.
int main() {

    WoodSession scene("element_joint_hilti");

    // two 600 x 400 slabs 200 thick falling 15 degrees each from the ridge along y, their seam faces mitred on x = 0
    const double half_angle = 15.0 * Tolerance::PI / 180.0;
    std::array<std::shared_ptr<Plate>, 2> slabs;
    for (int side = 0; side < 2; side++) {
        const double sign = side == 0 ? -1.0 : 1.0;
        const Vector down_slope(sign * std::cos(half_angle), 0.0, -std::sin(half_angle));
        const Vector up(sign * std::sin(half_angle), 0.0, std::cos(half_angle));
        const Vector along(0.0, 400.0, 0.0);
        const Point ridge(0.0, 0.0, 0.0);
        const Point seam_foot = ridge - up * 200.0 + down_slope * (200.0 * std::tan(half_angle));
        const Point far_foot = ridge - up * 200.0 + down_slope * 600.0;
        const Point far_top = ridge + down_slope * 600.0;
        slabs[side] = std::make_shared<Plate>(
            Polyline({seam_foot, far_foot, far_foot + along, seam_foot + along, seam_foot}),
            Polyline({ridge, far_top, far_top + along, ridge + along, ridge}),
            side == 0 ? "left" : "right"
        );
        scene.add(slabs[side]);
    }

    // the connector from their face contact, added, and passed to each slab in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(slabs[0], slabs[1]);
    const std::shared_ptr<JointBeam> hilti = JointBeam::hilti(*slabs[0], *slabs[1], *contact);
    hilti->name = "hilti";
    scene.add(hilti);
    scene.add_interaction(hilti, slabs[0], hilti->interaction(0));
    scene.add_interaction(hilti, slabs[1], hilti->interaction(1));

    // drawn apart: the right slab moved 300 off along the seam's normal, its pocket in view
    slabs[1]->place(Xform::translation(300.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two 600 x 400 CLT slabs 200 thick folded 30 degrees, each falling 15 degrees from the ridge along y, their seam faces mitred on the vertical plane x = 0; JointBeam::hilti on their face contact with the product's defaults, the 2024 Hilti joint (ss_e_r_2) with its parts: each slab gets its half of the 240 x 90 bow-tie pocket milled 93 deep from the top, a 27.7 x 40 neck across the seam widening into a 90 wide wing at 74.4, its far corners rounded by a 40 mm router; the parts are a straight bow-tie of two plywood halves 90 deep, a 50 mm steel disc 6 thick recessed in each wing's end and an M12 bolt from disc to disc. The connector is added and passed to each slab with add_interaction, its halves, discs and bolt nested under it; the right slab is moved 300 off along the seam's normal afterwards so its pocket reads.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_hilti --parallel 6 && ./build/element_joint_hilti && ../bash/publish-scene.sh --target element_joint_hilti

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
