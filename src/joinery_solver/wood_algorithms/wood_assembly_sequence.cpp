#include "pch.h"
#include "wood_assembly_sequence.h"
#include <fmt/ranges.h>

namespace wood_session {

AssemblySequence::AssemblySequence(const WoodSession& session, const std::string& base, double step, double clearance)
    : base(base), step(step), clearance(clearance) {

    // members: every element a joint acts on and every peg or key nested under a joint, and per member the joints on it
    for (const std::shared_ptr<JointElement>& joint : session.get_elements<JointElement>()) {
        for (size_t index = 0; index < joint->targets.size(); index++)
            joints[joint->targets[index]].emplace_back(joint, index);

        const std::shared_ptr<TreeNode> node = session.tree.get_node_by_name(joint->guid());
        const std::shared_ptr<JointBeamElement> beam_joint = std::dynamic_pointer_cast<JointBeamElement>(joint);
        if (!node || !beam_joint || beam_joint->steps.empty())
            continue;
        size_t part = 0;
        for (const TreeNode* child : node->children())
            if (session.get_element<JointConnectorPartElement>(child->name))
                joints[child->name].emplace_back(joint, joint->targets.size() + part++);
    }
    for (const std::shared_ptr<Element>& element : session.elements())
        if (joints.count(element->guid()) && (!std::dynamic_pointer_cast<JointElement>(element) || std::dynamic_pointer_cast<JointConnectorPartElement>(element)))
            members.push_back(element);

    // disassembly: member by member, the base last
    const std::vector<AssemblyStep> removal = compute_disassembly();

    // assembly: the disassembly reversed, every member going in against the way it came out
    sequence.push_back({base, Vector(0.0, 0.0, 0.0), {}, {}, {}});
    for (std::vector<AssemblyStep>::const_reverse_iterator it = removal.rbegin(); it != removal.rend(); ++it)
        sequence.push_back(*it);

    // conflicts: joints of one member whose directions another of its joints' members blocks
    blocked = compute_conflicts();
}

const std::vector<AssemblyStep>& AssemblySequence::steps() const {
    return sequence;
}

const std::vector<std::string>& AssemblySequence::locked() const {
    return stuck;
}

const std::vector<std::string>& AssemblySequence::conflicts() const {
    return blocked;
}

std::vector<std::string> AssemblySequence::partners(const JointElement& joint, size_t index) {

    std::vector<std::string> others;
    for (size_t i = 0; i < joint.targets.size(); i++)
        if (i != index)
            others.push_back(joint.targets[i]);

    return others;
}

std::vector<JointMove> AssemblySequence::out_legs(const std::vector<JointMove>& legs) {

    std::vector<JointMove> out;
    for (std::vector<JointMove>::const_reverse_iterator it = legs.rbegin(); it != legs.rend(); ++it)
        out.push_back(it->reversed());

    return out;
}

bool AssemblySequence::slides_out(const Element& member, const std::vector<JointMove>& legs, const Vector& direction, const std::vector<const Element*>& others) const {

    const Mesh& solid = member.model_geometry_mesh();
    const AABB box = AABB::from_mesh(solid);

    // the placements the member passes: every move in steps no longer than step at its farthest corner, then along direction
    // from where the moves end
    std::vector<Xform> places;
    Xform reached;
    for (const JointMove& leg : legs) {
        const double count = std::max(std::ceil(leg.reach(AABB::from_mesh(solid.transformed(reached))) / step), 1.0);
        for (double i = 1.0; i <= count; i += 1.0)
            places.push_back(leg.at(i / count) * reached);
        reached = leg.at(1.0) * reached;
    }
    for (double distance = 1.0; distance <= box.diagonal(); distance = distance < step ? step : distance + step)
        places.push_back(Xform::translation(direction[0] * distance, direction[1] * distance, direction[2] * distance) * reached);

    for (const Xform& place : places) {
        const Mesh moved = solid.transformed(place);
        const AABB moved_box = AABB::from_mesh(moved);

        for (const Element* other : others)
            if (moved_box.intersects(AABB::from_mesh(other->model_geometry_mesh())) &&
                compute_volume(solid_boolean(moved, other->model_geometry_mesh(), SolidOperation::intersect)) > clearance)
                return false;
    }

    return true;
}

/// Whether left takes the member out farther from its partners' centre than right, by the cosine each stores first.
static bool farther_out(const std::pair<double, Vector>& left, const std::pair<double, Vector>& right) {
    return left.first > right.first;
}

std::vector<Vector> AssemblySequence::candidate_directions(const Element& member, const std::vector<const Element*>& partners) {

    const Mesh& solid = member.model_geometry_mesh();
    std::vector<Vector> found;
    std::vector<Vector> tried;
    for (const size_t face : solid.faces())
        if (solid.face_normal(face))
            tried.push_back(solid.face_normal(face)->normalized());
    for (const Vector& axis : {Vector::x_axis(), Vector::y_axis(), Vector::z_axis()})
        tried.push_back(axis);

    // each normal and its opposite once
    for (const Vector& normal : tried)
        for (const Vector& direction : {normal, normal * -1.0}) {
            bool known = false;
            for (const Vector& other : found)
                known = known || direction.dot(other) > 0.9999;
            if (!known)
                found.push_back(direction);
        }

    // the ways out that lead away from the partners' centre first: a post lifts off its sill, plain fingers slide apart
    const AABB own = AABB::from_mesh(solid);
    Vector away(0.0, 0.0, 0.0);
    for (const Element* partner : partners) {
        const AABB other = AABB::from_mesh(partner->model_geometry_mesh());
        away = away + Vector(own.cx - other.cx, own.cy - other.cy, own.cz - other.cz);
    }
    if (away.magnitude() > 1e-9)
        away = away.normalized();
    std::vector<std::pair<double, Vector>> ranked;
    for (const Vector& direction : found)
        ranked.emplace_back((direction * -1.0).dot(away), direction);
    std::stable_sort(ranked.begin(), ranked.end(), farther_out);
    std::vector<Vector> ordered;
    for (const std::pair<double, Vector>& each : ranked)
        ordered.push_back(each.second);

    return ordered;
}

std::vector<AssemblyStep> AssemblySequence::compute_disassembly() {

    std::vector<std::shared_ptr<Element>> remaining = members;
    std::vector<AssemblyStep> removal;

    while (remaining.size() > 1) {
        bool taken = false;

        for (size_t i = remaining.size(); i-- > 0 && !taken;) {
            const std::shared_ptr<Element> member = remaining[i];
            if (member->name == base)
                continue;

            // the members still in, and the joints on this member that still hold one of them
            std::vector<const Element*> others;
            std::set<std::string> in;
            for (const std::shared_ptr<Element>& other : remaining)
                if (other != member) {
                    others.push_back(other.get());
                    in.insert(other->guid());
                }

            // a member none of whose joints holds one still in is free (the base of a pair set beside another): it stays where it is
            bool held = false;
            for (const std::pair<std::shared_ptr<JointElement>, size_t>& joint : joints[member->guid()])
                for (const std::string& partner : partners(*joint.first, joint.second))
                    held = held || in.count(partner) > 0;
            if (!held) {
                removal.push_back({member->name, Vector(0.0, 0.0, 0.0), {}, {}, {}});
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                taken = true;
                break;
            }

            for (const std::pair<std::shared_ptr<JointElement>, size_t>& joint : joints[member->guid()]) {
                std::vector<std::string> holding;
                for (const std::string& partner : partners(*joint.first, joint.second))
                    if (in.count(partner))
                        holding.push_back(partner);
                if (holding.empty())
                    continue;

                // a joint that names no way in (a plate's fingers, a beam factory's half lap) is tried along the member's own face
                // normals and the axes: the sweep keeps the first that frees it
                const std::vector<Vector> named = joint.first->insertion_directions(joint.second);
                std::vector<const Element*> held;
                for (const std::shared_ptr<Element>& other : remaining)
                    if (std::find(holding.begin(), holding.end(), other->guid()) != holding.end())
                        held.push_back(other.get());
                const std::vector<Vector> directions = named.empty() ? candidate_directions(*member, held) : named;
                for (size_t way = 0; way < directions.size(); way++) {
                    const std::vector<JointMove> legs = joint.first->insertion_legs(joint.second, way);
                    if (!slides_out(*member, out_legs(legs), directions[way] * -1.0, others))
                        continue;

                    AssemblyStep out{member->name, directions[way], legs, {}, {}};
                    for (const std::pair<std::shared_ptr<JointElement>, size_t>& engaged : joints[member->guid()])
                        for (const std::string& partner : partners(*engaged.first, engaged.second))
                            for (const std::shared_ptr<Element>& other : remaining)
                                if (other->guid() == partner && other != member) {
                                    out.joints.push_back(engaged.first->name);
                                    out.partners.push_back(other->name);
                                }
                    removal.push_back(out);
                    remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                    taken = true;
                    break;
                }

                if (taken)
                    break;
            }
        }

        if (!taken) {
            for (const std::shared_ptr<Element>& member : remaining)
                if (member->name != base)
                    stuck.push_back(member->name);
            break;
        }
    }

    return removal;
}

std::vector<std::string> AssemblySequence::compute_conflicts() const {

    std::vector<std::string> found;

    for (const std::shared_ptr<Element>& member : members) {
        const std::vector<std::pair<std::shared_ptr<JointElement>, size_t>>& on_member = joints.at(member->guid());

        for (const std::pair<std::shared_ptr<JointElement>, size_t>& joint : on_member) {
            const std::vector<Vector> directions = joint.first->insertion_directions(joint.second);

            for (const std::pair<std::shared_ptr<JointElement>, size_t>& other : on_member) {
                if (other.first == joint.first || directions.empty())
                    continue;

                std::vector<const Element*> others;
                for (const std::shared_ptr<Element>& candidate : members)
                    for (const std::string& partner : partners(*other.first, other.second))
                        if (candidate->guid() == partner)
                            others.push_back(candidate.get());

                bool every = true;
                for (size_t way = 0; way < directions.size(); way++)
                    every = every && !slides_out(*member, out_legs(joint.first->insertion_legs(joint.second, way)), directions[way] * -1.0, others);
                if (every)
                    found.push_back(fmt::format("{}: {} blocked by {}", member->name, joint.first->name, other.first->name));
            }
        }
    }

    return found;
}

void AssemblySequence::json_dump(const std::string& filename) const {

    nlohmann::ordered_json json;
    json["base"] = base;
    json["steps"] = nlohmann::ordered_json::array();
    for (const AssemblyStep& move : sequence)
        for (const std::shared_ptr<Element>& member : members)
            if (member->name == move.element) {
                const AABB box = AABB::from_mesh(member->model_geometry_mesh());
                nlohmann::ordered_json legs = nlohmann::ordered_json::array();
                for (const JointMove& leg : move.legs) {
                    if (!leg.is_turn()) {
                        legs.push_back({{"shift", {leg.shift[0], leg.shift[1], leg.shift[2]}}});
                        continue;
                    }
                    const Point pivot = leg.axis.start();
                    const Vector along = leg.axis.to_vector().normalized();
                    legs.push_back({{"line", {pivot[0], pivot[1], pivot[2], along[0], along[1], along[2]}}, {"angle", leg.angle}});
                }
                json["steps"].push_back({
                    {"element", move.element},
                    {"insertion", {move.insertion[0], move.insertion[1], move.insertion[2]}},
                    {"legs", legs},
                    {"joints", move.joints},
                    {"partners", move.partners},
                    {"low", {box.cx - box.hx, box.cy - box.hy, box.cz - box.hz}},
                    {"high", {box.cx + box.hx, box.cy + box.hy, box.cz + box.hz}},
                });
            }
    json["locked"] = stuck;
    // each locked member's partners and box, so a drawing can still take it out of them
    json["locked_members"] = nlohmann::ordered_json::array();
    for (const std::string& name : stuck)
        for (const std::shared_ptr<Element>& member : members)
            if (member->name == name) {
                std::vector<std::string> others;
                for (const std::pair<std::shared_ptr<JointElement>, size_t>& joint : joints.at(member->guid()))
                    for (const std::string& partner : partners(*joint.first, joint.second))
                        for (const std::shared_ptr<Element>& other : members)
                            if (other->guid() == partner && other != member)
                                others.push_back(other->name);
                const AABB box = AABB::from_mesh(member->model_geometry_mesh());
                json["locked_members"].push_back({
                    {"element", name},
                    {"partners", others},
                    {"low", {box.cx - box.hx, box.cy - box.hy, box.cz - box.hz}},
                    {"high", {box.cx + box.hx, box.cy + box.hy, box.cz + box.hz}},
                });
            }
    json["conflicts"] = blocked;

    std::ofstream(filename) << json.dump(2);
}

std::string AssemblySequence::str() const {

    std::string text;
    for (size_t i = 0; i < sequence.size(); i++)
        text += fmt::format(
            "{:2d} {:<16} in along ({:6.3f}, {:6.3f}, {:6.3f}) engaging {}\n",
            i,
            sequence[i].element,
            sequence[i].insertion[0],
            sequence[i].insertion[1],
            sequence[i].insertion[2],
            fmt::join(sequence[i].joints, ", ")
        );
    text += fmt::format("locked: {}\n", stuck.empty() ? "none" : fmt::format("{}", fmt::join(stuck, ", ")));
    for (const std::string& conflict : blocked)
        text += fmt::format("conflict: {}\n", conflict);

    return text;
}

}
