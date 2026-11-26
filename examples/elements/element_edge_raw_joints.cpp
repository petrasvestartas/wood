#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Two raw logs modelled as round beams, attached to scanned trunks and joined by a pegged cross-lap.
int main() {

    WoodSession scene("element_edge_raw_joints");

    // 1. Model the crossing pair as round-section beams along their axes
    const std::shared_ptr<EdgeRawElement> member = EdgeRawElement::round(
        Line::from_points({-850.0, 0.0, 0.0}, {850.0, 0.0, 0.0}), 52.0, 0.2, "member"
    );
    const std::shared_ptr<EdgeRawElement> carrier = EdgeRawElement::round(
        Line::from_points({0.0, -850.0, 0.0}, {0.0, 850.0, 0.0}), 52.0, 0.2, "carrier"
    );

    // 2. Attach scanned trunks from the stock database by transformation matrix
    member->attach_stock("raw_log_1", 0.0, true);
    carrier->attach_stock("raw_log_0", 0.0, true);
    scene.add(member);
    scene.add(carrier);

    // 3. Apply a pegged cross-lap: boolean cutters extend past the irregular bark
    const std::shared_ptr<JointBeamElement> joint =
        JointBeamElement::from_library("pegged_cross_lap", *member, *carrier);
    joint->name = "pegged_cross_lap";
    scene.add(joint);
    scene.add_interaction(joint, member, joint->interaction(0));
    scene.add_interaction(joint, carrier, joint->interaction(1));

    for (const std::shared_ptr<EdgeRawElement>& log : {member, carrier})
        std::cout << fmt::format(
            "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
            log->name,
            log->model_geometry_brep().face_count(),
            log->model_geometry_brep().is_solid(),
            log->model_geometry_brep().volume_exact(),
            log->element_geometry_brep().volume_exact()
        );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two raw spruce logs modelled first as round-section beams, connected to scanned trunks by transformation matrix and joined by JointBeamElement::from_library("pegged_cross_lap"): boolean cutters extend past the irregular bark to carve flat half-lap seats and a pegged mortise.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_joints --parallel 6 && ./build/element_edge_raw_joints && ../bash/publish-scene.sh --target element_edge_raw_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
