#include "wood_session.h"
#include "wood_joint_library.h"

using namespace session_cpp;
using namespace wood_session;

/// Every joint of the YAML library on a pair of beams, one group per design, four to a row, each chapter on new rows, the member it ends drawn out along its insertion.
int main() {

    WoodSession scene("element_joint_beam_library");

    size_t row = 0;
    for (const std::string chapter : {"angled", "t"}) {
        const std::vector<std::string> designs = library_designs(chapter);
        for (size_t i = 0; i < designs.size(); i++) {
            const std::string& design = designs[i];
            const double turn = (chapter == "t" ? 90.0 : 130.0) * Tolerance::PI / 180.0;
            const Point node(1400.0 * static_cast<double>(i % 4), -1200.0 * static_cast<double>(row + i / 4), 0.0);
            const std::shared_ptr<EdgeStraightElement> carrier = EdgeStraightElement::square(Polyline({node + Vector(-450.0, 0.0, 0.0), node + Vector(450.0, 0.0, 0.0)}), 60.0, design + "_carrier");
            const std::shared_ptr<EdgeStraightElement> member = EdgeStraightElement::square(Polyline({node, node + Vector(600.0 * std::cos(turn), 600.0 * std::sin(turn), 0.0)}), 60.0, design + "_member");
            scene.add(carrier, scene.group_named(design));
            scene.add(member, scene.group_named(design));

            const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(design, *member, *carrier);
            scene.add(joint, scene.group_named(design));
            scene.add_interaction(joint, member, joint->interaction(0));
            scene.add_interaction(joint, carrier, joint->interaction(1));

            const Vector out = joint->insertion_directions(0)[0] * -150.0;
            member->place(Xform::translation(out[0], out[1], out[2]));
        }
        row += (designs.size() + 3) / 4;
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Every joint of the beam joint library (data/joints/*.yaml, Bauer and Pauli's Joinery Compendium) built by JointBeamElement::from_library on a pair of 120 x 120 beams, one group per design named after it, four to a row, each chapter starting a new row: the angled joints with the member leaning at 50 degrees, the T-joints standing square; each member drawn 150 mm out along its first insertion direction.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_library --parallel 6 && ./build/element_joint_beam_library && ../bash/publish-scene.sh --target element_joint_beam_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
