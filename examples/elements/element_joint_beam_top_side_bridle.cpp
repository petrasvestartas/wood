#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// top_side_bridle on beams: a skew T-bridle rebuilt from photographs of a made joint, a fork around a tongue, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_top_side_bridle");
    scene.settings.joint_parameters[7] = 0.3;
    scene.settings.joint_parameters[8] = JointPlateElement::library_id("top_side_bridle");

    // two 100 x 100 beams meeting at 57 degrees, the first running past on both sides, the second ending in it
    const double radius = 50.0;
    const double turn = 57.0 * Tolerance::PI / 180.0;
    const Vector along(std::cos(turn), std::sin(turn), 0.0);
    const Point origin(0.0, 0.0, 0.0);
    const std::shared_ptr<EdgeStraightElement> first = EdgeStraightElement::square(
        Polyline({Point(-220.0, 0.0, 0.0), Point(370.0, 0.0, 0.0)}), radius, "first"
    );
    const std::shared_ptr<EdgeStraightElement> second =
        EdgeStraightElement::square(Polyline({origin, origin + along * 136.0}), radius, "second");
    // extended at both ends: past the axis to the first beam's far face, and on to 236 long from it
    second->extension = 2.0 * radius;
    scene.add(first);
    scene.add(second);

    const InteractionContactAxis contact(
        Line::from_points(origin, origin), 220.0 / 590.0, 0.0, 0, 0, 0, 0
    );
    const std::shared_ptr<JointBeamElement> joint =
        JointBeamElement::from_contact(*first, *second, contact, 500.0, 0.91, 1, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // the first beam's ends sawn parallel to the second beam
    const Vector normal(along[1], -along[0], 0.0);
    const std::shared_ptr<JointCutPlaneElement> long_end = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal(Point(345.0, radius, 0.0), -normal), 300.0, "long_end"
    );
    const std::shared_ptr<JointCutPlaneElement> short_end = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal(Point(-124.0, radius, 0.0), normal), 300.0, "short_end"
    );
    for (const std::shared_ptr<JointCutPlaneElement>& end : {long_end, short_end}) {
        end->is_visible = false;
        scene.add(end);
        scene.add_interaction(end, first, end->interaction(0));
    }

    // drawn apart: the second beam slid back off the tongue, square to the first, the only way it comes out
    second->place(Xform::translation(0.0, 250.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
top_side_bridle on beams: a skew T-bridle rebuilt from photographs of a made joint. The second beam ends in a fork around a tongue left in the first, with a shoulder at its acute corner, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_top_side_bridle --parallel 6 && ./build/element_joint_beam_top_side_bridle && ../bash/publish-scene.sh --target element_joint_beam_top_side_bridle

|||||||| TIME ||||||||
2.34 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
