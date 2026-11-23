#include "wood_session.h"
#include "wood_joint_library.h"
#include "wood_assembly_sequence.h"
#include "convex_hull.h"
#include "boolean_polyline.h"

using namespace session_cpp;
using namespace wood_session;

/// A piece's outline on the isometric screen the pictures use (the viewer's View::Iso: turned -30 degrees about z, then
/// -30 degrees about the turned x): the hull of its points seen on the screen, grown 7.5 mm, in mm.
static Polyline on_screen(const Mesh& mesh, const Xform& placed) {
    const Vector right(0.5 * std::sqrt(3.0), -0.5, 0.0);
    const Vector up(0.25, 0.25 * std::sqrt(3.0), 0.5 * std::sqrt(3.0));
    std::vector<Point> seen;
    for (const Point& point : mesh.transformed(placed).to_vertices_and_faces().first) {
        const Vector position(point[0], point[1], point[2]);
        for (int k = 0; k < 4; k++)
            seen.push_back(Point(position.dot(right) + (k & 1 ? 7.5 : -7.5), position.dot(up) + (k & 2 ? 7.5 : -7.5), 0.0));
    }
    std::vector<Point> hull = ConvexHull::hull_2d(seen);
    hull.push_back(hull.front());
    return Polyline(hull);
}

/// Every joint of the YAML library, a group and a row per design: on a square corner and on an inclined one, drawn apart in its assembly order.
int main() {

    WoodSession scene("element_joint_beam_library");
    // per design and corner the order its pieces go in, each piece's way, further moves and where the drawing set it, for
    // tools/render_joint_library_gifs.py
    nlohmann::ordered_json orders;

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

            // drawn apart in the order it goes together: the same joint on fresh members taken apart by an AssemblySequence, the
            // carrier staying, every other member, peg or wedge out along the way it goes in, each later one farther
            WoodSession apart(design);
            const std::vector<std::shared_ptr<EdgeStraightElement>> fresh = library_members(design, corner == 0, node);
            std::vector<const EdgeStraightElement*> fresh_more;
            for (size_t m = 2; m < fresh.size(); m++)
                fresh_more.push_back(fresh[m].get());
            for (const std::shared_ptr<EdgeStraightElement>& each : fresh)
                apart.add(each);
            const std::shared_ptr<JointBeamElement> twin = JointBeamElement::from_library(design, *fresh[0], *fresh[1], {}, 2.0, fresh_more);
            apart.add(twin);
            for (size_t m = 0; m < fresh.size(); m++)
                apart.add_interaction(twin, fresh[m], twin->interaction(m));
            const LibraryRecord record = library_record(design);
            const size_t base = record.fields.count("base") ? static_cast<size_t>(record.fields.at("base")[0] - 'a') : 1;
            const AssemblySequence sequence(apart, fresh[base]->name);
            // a member the sequence cannot take out on its own (a pair hooked together outside the post) is still drawn apart,
            // along the way its record gives it, going in right after the base: what the sequence took out stays in it
            std::vector<AssemblyStep> steps = {sequence.steps().front()};
            for (const std::string& locked : sequence.locked())
                for (size_t m = 0; m < fresh.size(); m++)
                    if (fresh[m]->name == locked && !twin->insertion_directions(m).empty())
                        steps.push_back({locked, twin->insertion_directions(m)[0], twin->insertion_legs(m, 0), {}, {}});
            steps.insert(steps.end(), sequence.steps().begin() + 1, sequence.steps().end());
            // the base stays where it is: with three pieces or more, crowding round it, every piece waits clear of it too (a
            // member alone slides straight out of it and reads apart even where the two overlap on the screen)
            std::vector<Polyline> waiting;
            const std::shared_ptr<Element> staying = scene.get_element_by_name<Element>(name + steps.front().element.substr(design.size()));
            if (staying && steps.size() > 2)
                waiting.push_back(on_screen(staying->model_geometry_mesh(), Xform()));
            for (size_t j = 1; j < steps.size(); j++) {
                const AssemblyStep& step = steps[j];
                const std::shared_ptr<Element> drawn = scene.get_element_by_name<Element>(name + step.element.substr(design.size()));
                if (!drawn)
                    continue;
                // out by its own depth along the way it goes in and 40 mm a step of the order: close, so the drawing stays compact
                const AABB box = AABB::from_mesh(drawn->model_geometry_mesh());
                const Vector way = step.insertion;
                const double behind = box.hx * std::abs(way[0]) + box.hy * std::abs(way[1]) + box.hz * std::abs(way[2]);
                // a piece going in by more moves (in, then along the slot, or a turn) stands where it starts them: the moves undone,
                // the last first, then back along its approach
                Xform undone;
                for (const JointMove& leg : AssemblySequence::out_legs(step.legs))
                    undone = leg.at(1.0) * undone;

                // and clear of every piece already waiting where the picture sees it: one whose outline on the isometric screen
                // would touch another's goes farther out, 10 mm at a time, so two pieces are never drawn as one
                const Mesh shape = drawn->model_geometry_mesh();
                double distance = std::min(2.0 * behind, 160.0) + 40.0 * static_cast<double>(j);
                for (int push = 0; push < 300; push++) {
                    const Vector trial = way * -distance;
                    const Polyline outline = on_screen(shape, Xform::translation(trial[0], trial[1], trial[2]) * undone);
                    bool touches = false;
                    for (const Polyline& earlier : waiting)
                        touches = touches || BooleanPolyline::compute_count(outline, earlier, 0) > 0;
                    if (!touches)
                        break;
                    distance += 10.0;
                }
                const Vector out = way * -distance;
                drawn->place(Xform::translation(out[0], out[1], out[2]) * undone);
                waiting.push_back(on_screen(shape, Xform::translation(out[0], out[1], out[2]) * undone));
                nlohmann::ordered_json legs = nlohmann::ordered_json::array();
                for (const JointMove& leg : step.legs) {
                    if (!leg.is_turn()) {
                        legs.push_back({{"shift", {leg.shift[0], leg.shift[1], leg.shift[2]}}});
                        continue;
                    }
                    const Point pivot = leg.axis.start();
                    const Vector along = leg.axis.to_vector().normalized();
                    legs.push_back({{"line", {pivot[0], pivot[1], pivot[2], along[0], along[1], along[2]}}, {"angle", leg.angle}});
                }
                orders[name].push_back({{"element", drawn->name}, {"way", {way[0], way[1], way[2]}}, {"legs", legs}, {"out", {out[0], out[1], out[2]}}});

                // a black arrow behind it along the way it goes in, its head at the back of the part's box, as the truss draws them,
                // longer the later the piece goes in: 30 mm and 30 mm per step of the order
                const Point centre = Point(box.cx, box.cy, box.cz) + out;
                const double shaft = 30.0 + 30.0 * static_cast<double>(j);
                Line arrow = Line::from_points(centre - way * (behind + 15.0 + shaft), centre - way * (behind + 15.0));
                arrow.width = 6.0;
                arrow.linecolor = Color(0, 0, 0, 255);
                arrow.arrowhead = Arrowhead::END;
                scene.add_line(arrow, scene.group_named(design));

                // then an arrow per later move from the piece's centre: a slide the way it goes, as long as the move and at least
                // 40 mm; a turn as an arc of eight arrows about its line
                Point from = centre;
                for (const JointMove& leg : step.legs) {
                    const int pieces = leg.is_turn() ? 8 : 1;
                    for (int k = 1; k <= pieces; k++) {
                        // a turn drawn about its line moved out with the piece
                        const Xform around = Xform::translation(out[0], out[1], out[2]) * leg.at(static_cast<double>(k) / pieces) * Xform::translation(-out[0], -out[1], -out[2]);
                        const Point to = leg.is_turn() ? around.transform_point(centre) : from + leg.shift.normalized() * std::max(leg.shift.magnitude(), 40.0);
                        Line move = Line::from_points(from, to);
                        move.width = 6.0;
                        move.linecolor = Color(0, 0, 0, 255);
                        move.arrowhead = k == pieces ? Arrowhead::END : Arrowhead::NONE;
                        scene.add_line(move, scene.group_named(design));
                        from = to;
                    }
                }
            }
        }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    std::ofstream(pb_path("joint_library_orders").substr(0, pb_path("joint_library_orders").size() - 3) + ".json") << orders.dump(1);
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Every joint of the beam joint library (data/joints/*.yaml, Bauer and Pauli's Joinery Compendium) built by JointBeamElement::from_library on a pair of 120 x 120 beams, one group and one row per design named after it: the member square on the carrier, then leaning at 50 degrees, its stock running 180 mm past its axis end so a lap can pass through the carrier; each drawn apart in the order it goes together: the joint's AssemblySequence takes a twin apart, the carrier stays, and every other member, peg or wedge moves out along the way it goes in by its own depth (at most 160 mm) and 40 mm per step, so the later ones stand farther out, a black arrow behind each along its way in, 30 mm and 30 mm per step of the order long; a piece that goes in by more moves (in, then along the slot) gets an arrow for each.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_library --parallel 6 && ./build/element_joint_beam_library && ../bash/publish-scene.sh --target element_joint_beam_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
