#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The Hilti connector on six pairs of 200 mm CLT slabs folded 0 to 50 degrees, one row per angle as in the test series.
int main() {

    WoodSession scene("element_joint_hilti_angles");

    for (int row = 0; row < 6; row++) {
        const double angle = 10.0 * row;
        const double half_angle = 0.5 * angle * Tolerance::PI / 180.0;
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("fold_{:g}", angle));

        // two 600 x 400 slabs 200 thick falling half the angle each from the ridge, mitred on x = 0, the row 600 along y
        std::array<std::shared_ptr<Plate>, 2> slabs;
        for (int side = 0; side < 2; side++) {
            const double sign = side == 0 ? -1.0 : 1.0;
            const Vector down_slope(sign * std::cos(half_angle), 0.0, -std::sin(half_angle));
            const Vector up(sign * std::sin(half_angle), 0.0, std::cos(half_angle));
            const Vector along(0.0, 400.0, 0.0);
            const Point ridge(0.0, 600.0 * row, 0.0);
            const Point seam_foot = ridge - up * 200.0 + down_slope * (200.0 * std::tan(half_angle));
            const Point far_foot = ridge - up * 200.0 + down_slope * 600.0;
            const Point far_top = ridge + down_slope * 600.0;
            slabs[side] = std::make_shared<Plate>(
                Polyline({seam_foot, far_foot, far_foot + along, seam_foot + along, seam_foot}),
                Polyline({ridge, far_top, far_top + along, ridge + along, ridge}),
                fmt::format("{}_{:g}", side == 0 ? "left" : "right", angle)
            );
            scene.add(slabs[side], group);
        }

        // the connector from their face contact, the same parts at every angle, its pockets and slots following the slabs
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(slabs[0], slabs[1]);
        const std::shared_ptr<JointBeam> hilti = JointBeam::hilti(*slabs[0], *slabs[1], *contact);
        hilti->name = fmt::format("hilti_{:g}", angle);
        scene.add(hilti, group);
        scene.add_interaction(hilti, slabs[0], hilti->interaction(0));
        scene.add_interaction(hilti, slabs[1], hilti->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Six pairs of 600 x 400 CLT slabs 200 thick, folded 0, 10, 20, 30, 40 and 50 degrees on a mitred seam, one row per angle 600 apart along the seam, each pair in its group fold_<angle>; JointBeam::hilti with its defaults on every pair: the halves, discs and rod are the same solids at every angle, never sheared or scaled, while the pockets and the obround access slots milled from the top faces follow the slabs; tests/joint_hilti.cpp checks the parts identical at all six angles, inside their slabs and filling their pockets.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_hilti_angles --parallel 6 && ./build/element_joint_hilti_angles && ../bash/publish-scene.sh --target element_joint_hilti_angles

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
