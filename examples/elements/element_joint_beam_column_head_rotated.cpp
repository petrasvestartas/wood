#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// A horizontal 100 x 185 beam ending at the column head from angle degrees in plan.
static std::shared_ptr<EdgeStraightElement>
beam(double angle, double length, const std::string& name) {

    const double width = 100.0;
    const double height = 185.0;
    const double turn = angle * Tolerance::TO_RADIANS;
    const Polyline rectangle = Polyline::rectangle(
        {-0.5 * width, -0.5 * height, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        width,
        height
    );
    const Polyline axis({
        Point(std::cos(turn) * length, std::sin(turn) * length, -0.5 * height),
        Point(0.0, 0.0, -0.5 * height),
    });
    return std::make_shared<EdgeStraightElement>(
        axis,
        std::vector<Polyline>{rectangle},
        std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
        name
    );
}

/// column_head with its secondary beams turned, each its own way: the same call, every cut read from the beams, so the posts come out unequal.
int main() {

    WoodSession scene("element_joint_beam_column_head_rotated");

    // a round column 200 across and 425 long, its head at the origin
    const std::shared_ptr<EdgeStraightElement> column = EdgeStraightElement::round(
        Line::from_points({0.0, 0.0, -425.0}, {0.0, 0.0, 0.0}),
        100.0,
        0.02,
        "column"
    );
    scene.add(column);

    // four 100 x 185 beams, their axes ending at the column's: the main pair in line, one secondary beam at 75 degrees to it, the other at 80 degrees the other side
    const std::shared_ptr<EdgeStraightElement> first = beam(180.0, 385.0, "first");
    const std::shared_ptr<EdgeStraightElement> second = beam(0.0, 385.0, "second");
    const std::shared_ptr<EdgeStraightElement> third = beam(75.0, 465.0, "third");
    const std::shared_ptr<EdgeStraightElement> fourth = beam(280.0, 465.0, "fourth");
    scene.add(first);
    scene.add(second);
    scene.add(third);
    scene.add(fourth);

    const std::shared_ptr<JointBeamElement> joint =
        JointBeamElement::column_head(*column, *first, *second, *third, *fourth);
    scene.add(joint);
    scene.add_interaction(joint, column, joint->interaction(0));
    scene.add_interaction(joint, first, joint->interaction(1));
    scene.add_interaction(joint, second, joint->interaction(2));
    scene.add_interaction(joint, third, joint->interaction(3));
    scene.add_interaction(joint, fourth, joint->interaction(4));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
column_head with its secondary beams turned, one at 75 degrees to the main pair and the other at 80 degrees the other side: the same call as the square node, every cut read from the beams, so the four posts come out unequal.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_column_head_rotated --parallel 6 && ./build/element_joint_beam_column_head_rotated && ../bash/publish-scene.sh --target element_joint_beam_column_head_rotated

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
