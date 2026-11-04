#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// column_head with its secondary beams turned, each its own way: the same call, every cut read from the beams, so the posts come out unequal. Drawn apart.
int main() {

    WoodSession scene("element_joint_beam_column_head_rotated");

    // a round column 200 across and 425 long, its head at the origin
    const double width = 100.0;
    const double height = 185.0;
    const std::shared_ptr<Column> column = Column::round(
        Line::from_points({0.0, 0.0, -425.0}, {0.0, 0.0, 0.0}), width, 0.02, "column"
    );
    scene.add(column);

    // four 100 x 185 beams, their axes ending at the column's: the main pair in line, one secondary beam at 75 degrees to it, the other at 80 degrees the other side
    const std::vector<Polyline> section = {Polyline::rectangle(
        {-0.5 * width, -0.5 * height, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, width, height
    )};
    const auto beam = [&](double angle, double length, const std::string& name) {
        const double turn = angle * Tolerance::PI / 180.0;
        return std::make_shared<EdgeStraightElement>(
            Polyline(
                {Point(std::cos(turn) * length, std::sin(turn) * length, -0.5 * height),
                 Point(0.0, 0.0, -0.5 * height)}
            ),
            section,
            std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
            name
        );
    };
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

    // drawn apart: every beam lifted off the head along the column, the way it comes out, the secondary ones last in and highest
    first->place(Xform::translation(0.0, 0.0, 260.0));
    second->place(Xform::translation(0.0, 0.0, 260.0));
    third->place(Xform::translation(0.0, 0.0, 520.0));
    fourth->place(Xform::translation(0.0, 0.0, 520.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
column_head with its secondary beams turned, one at 75 degrees to the main pair and the other at 80 degrees the other side: the same call as the square node, every cut read from the beams, so the four posts come out unequal. Drawn apart along the column.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_column_head_rotated --parallel 6 && ./build/element_joint_beam_column_head_rotated && ../bash/publish-scene.sh --target element_joint_beam_column_head_rotated

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
