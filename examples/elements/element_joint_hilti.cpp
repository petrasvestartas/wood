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

    // drawn apart: the right slab moved 300 off along the seam's normal, its pocket and slot in view
    slabs[1]->place(Xform::translation(300.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two 600 x 400 CLT slabs 200 thick folded 30 degrees, each falling 15 degrees from the ridge along y, their seam faces mitred on the vertical plane x = 0; JointBeam::hilti on their face contact with its defaults for 200 mm CLT: two identical birch plywood halves, a 140 long trapezoid wing 120 wide at its outer end on a 50 wide neck, 100 deep, the two a straight bow-tie across the seam 20 under the ridge, an M16 threaded rod as its pin through both necks, a 70 mm disc 8 thick taking the nut at each outer end; each slab gets the pocket of its half milled from its top face, the seat of its disc and an obround access slot 80 wide milled from its top face from the wing's end outwards, as long as the half. The connector is added and passed to each slab with add_interaction, its halves, discs and rod nested under it; the right slab is moved 300 off along the seam's normal afterwards so its pocket and slot read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_hilti --parallel 6 && ./build/element_joint_hilti && ../bash/publish-scene.sh --target element_joint_hilti

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
