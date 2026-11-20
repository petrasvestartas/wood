#include "wood_session.h"
#include "wood_joint_library.h"

using namespace session_cpp;
using namespace wood_session;

/// Every joint of the YAML library, a group and a row per design: on a square corner and on an inclined one, drawn apart as the book draws it.
int main() {

    WoodSession scene("element_joint_beam_library");

    // a design to a row, standing in the xz plane: the member square on the carrier, then leaning at 50 degrees, at the corners its record names
    const std::vector<std::string> designs = library_designs();
    for (const std::string& design : designs)
        std::cout << fmt::format("DESIGN {}", design) << std::endl;

    for (size_t i = 0; i < designs.size(); i++)
        for (size_t corner = 0; corner < 2; corner++) {
            const std::string& design = designs[i];
            if (library_record(design).fields.at("corners").find(corner == 0 ? "square" : "inclined") == std::string::npos)
                continue;
            const Point node(1400.0 * static_cast<double>(corner), -1000.0 * static_cast<double>(i), 0.0);
            const std::string name = fmt::format("{}_{}", design, corner == 0 ? "square" : "inclined");
            const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = library_pair(design, corner == 0, node);
            const std::shared_ptr<EdgeStraightElement> member = pair[0];
            const std::shared_ptr<EdgeStraightElement> carrier = pair[1];
            member->name = name + "_member";
            carrier->name = name + "_carrier";
            scene.add(carrier, scene.group_named(design));
            scene.add(member, scene.group_named(design));

            const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(design, *member, *carrier);
            joint->name = name;
            scene.add(joint, scene.group_named(design));
            scene.add_interaction(joint, member, joint->interaction(0));
            scene.add_interaction(joint, carrier, joint->interaction(1));

            // drawn apart as the book draws it: an end member out along its own axis whatever way it goes in, a crossing one lifted
            // straight up with the pegs that run across it, each peg out along its own way
            const bool crossing = library_record(design).fields.at("category") == "cross";
            const Vector out = crossing ? Vector(0.0, 0.0, 650.0) : (member->axis.get_point(1) - member->axis.get_point(0)).normalized() * 350.0;
            member->place(Xform::translation(out[0], out[1], out[2]));
            for (const std::shared_ptr<JointConnectorPartElement>& part : scene.get_elements<JointConnectorPartElement>())
                if (part->name.starts_with(name + "_part")) {
                    const Vector way = joint->insertion_directions(2)[0];
                    const Vector pulled = crossing && std::abs(way[2]) < 0.9 ? out - way * 200.0 : way * -200.0;
                    part->place(Xform::translation(pulled[0], pulled[1], pulled[2]));
                }
        }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Every joint of the beam joint library (data/joints/*.yaml, Bauer and Pauli's Joinery Compendium) built by JointBeamElement::from_library on a pair of 120 x 120 beams, one group and one row per design named after it: the member square on the carrier, then leaning at 50 degrees, its stock running 180 mm past its axis end so a lap can pass through the carrier; each drawn apart as the book draws it, an end member 350 mm out along its own axis, a crossing one lifted 650 mm with the pegs that run across it, every peg 200 mm out along its own way.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_library --parallel 6 && ./build/element_joint_beam_library && ../bash/publish-scene.sh --target element_joint_beam_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
