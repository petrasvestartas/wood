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
            const std::vector<std::shared_ptr<EdgeStraightElement>> members = library_members(design, corner == 0, node);
            const std::shared_ptr<EdgeStraightElement> member = members[0];
            const std::shared_ptr<EdgeStraightElement> carrier = members[1];
            member->name = name + "_member";
            carrier->name = name + "_carrier";
            std::vector<const EdgeStraightElement*> more;
            for (size_t m = 2; m < members.size(); m++) {
                members[m]->name = fmt::format("{}_{}", name, static_cast<char>('a' + m));
                more.push_back(members[m].get());
            }
            for (const std::shared_ptr<EdgeStraightElement>& each : members)
                scene.add(each, scene.group_named(design));

            const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(design, *member, *carrier, {}, 2.0, more);
            joint->name = name;
            scene.add(joint, scene.group_named(design));
            for (size_t m = 0; m < members.size(); m++)
                scene.add_interaction(joint, members[m], joint->interaction(m));

            // drawn apart as the book draws it: an end member out along its own axis whatever way it goes in, a crossing one lifted
            // straight up with the pegs that run across it, each peg or wedge out along its own way and aside from the carrier's axis
            const bool crossing = library_record(design).fields.at("category") == "cross";
            const Vector out = crossing ? Vector(0.0, 0.0, 650.0) : (member->axis.get_point(1) - member->axis.get_point(0)).normalized() * 500.0;
            member->place(Xform::translation(out[0], out[1], out[2]));
            // a more member out along its own axis, away from the node
            for (size_t m = 2; m < members.size(); m++) {
                const Point start = members[m]->axis.get_point(0);
                const Point end = members[m]->axis.get_point(1);
                const Vector away = (Point::distance(start, node) < Point::distance(end, node) ? end - start : start - end).normalized() * 500.0;
                members[m]->place(Xform::translation(away[0], away[1], away[2]));
            }
            size_t k = 0;
            for (const std::shared_ptr<JointConnectorPartElement>& part : scene.get_elements<JointConnectorPartElement>())
                if (part->name.starts_with(name + "_part")) {
                    const Vector way = joint->insertion_directions(joint->targets.size() + k++)[0];
                    const AABB box = AABB::from_mesh(part->model_geometry_mesh());
                    const double clear = box.diagonal() + 150.0;
                    const Point start = carrier->axis.get_point(0);
                    const Vector along_carrier = (carrier->axis.get_point(1) - start).normalized();
                    const Vector off_axis = Point(box.cx, box.cy, box.cz) - start;
                    const Vector away = off_axis - along_carrier * off_axis.dot(along_carrier);
                    const Vector aside = away.magnitude() > 1.0 ? away.normalized() * clear : Vector(0.0, 0.0, 0.0);
                    const Vector pulled = (crossing && std::abs(way[2]) < 0.9 ? out - way * clear : way * -clear) + aside;
                    part->place(Xform::translation(pulled[0], pulled[1], pulled[2]));
                }
        }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Every joint of the beam joint library (data/joints/*.yaml, Bauer and Pauli's Joinery Compendium) built by JointBeamElement::from_library on a pair of 120 x 120 beams, one group and one row per design named after it: the member square on the carrier, then leaning at 50 degrees, its stock running 180 mm past its axis end so a lap can pass through the carrier; each drawn apart as the book draws it, an end member 500 mm out along its own axis, a crossing one lifted 650 mm with the pegs that run across it, every peg or wedge out along its own way by its length and 150 mm and as far aside from the carrier.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_library --parallel 6 && ./build/element_joint_beam_library && ../bash/publish-scene.sh --target element_joint_beam_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
