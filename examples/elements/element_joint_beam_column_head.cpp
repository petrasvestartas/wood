#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// column_head: five members in one node, rebuilt from photographs: a round column, two main beams in a scarf and two secondary beams on dovetails, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_column_head");

    // a round column 200 across and 425 long, its head at the origin
    const double width = 100.0;
    const double height = 185.0;
    const std::shared_ptr<Column> column = Column::round(
        Line::from_points({0.0, 0.0, -425.0}, {0.0, 0.0, 0.0}), width, 0.02, "column"
    );
    scene.add(column);

    // four 100 x 185 beams, tops flush with the head: main beams along x reaching 385 from it, secondary along y reaching 465, each axis simply ending at the column's: the joint finds the rest
    const double level = -0.5 * height;
    const std::vector<Polyline> section = {Polyline::rectangle(
        {-0.5 * width, -0.5 * height, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, width, height
    )};
    const auto beam = [&](const Point& from, const Point& to, const std::string& name) {
        return std::make_shared<EdgeStraightElement>(
            Polyline({from, to}), section, std::vector<Vector>{Vector(0.0, 0.0, 1.0)}, name
        );
    };
    const std::shared_ptr<EdgeStraightElement> first = beam({-385.0, 0.0, level}, {0.0, 0.0, level}, "first");
    const std::shared_ptr<EdgeStraightElement> second = beam({385.0, 0.0, level}, {0.0, 0.0, level}, "second");
    const std::shared_ptr<EdgeStraightElement> third = beam({0.0, -465.0, level}, {0.0, 0.0, level}, "third");
    const std::shared_ptr<EdgeStraightElement> fourth = beam({0.0, 465.0, level}, {0.0, 0.0, level}, "fourth");
    scene.add(first);
    scene.add(second);
    scene.add(third);
    scene.add(fourth);

    const std::shared_ptr<JointBeam> joint =
        JointBeam::column_head(*column, *first, *second, *third, *fourth);
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
column_head: five members in one node, rebuilt from photographs. A round column keeps four posts around a cross slot; two main beams meet in it in a scarf of two tapered tongues; two secondary beams end on those tongues in one dovetail each. Drawn apart along the column, the way each comes out.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_column_head --parallel 6 && ./build/element_joint_beam_column_head && ../bash/publish-scene.sh --target element_joint_beam_column_head

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
