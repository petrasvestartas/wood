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

std::vector<Vector> AssemblySequence::out_legs(const std::vector<Vector>& legs) {

    std::vector<Vector> out;
    for (std::vector<Vector>::const_reverse_iterator it = legs.rbegin(); it != legs.rend(); ++it)
        out.push_back(*it * -1.0);

    return out;
}

bool AssemblySequence::slides_out(const Element& member, const std::vector<Vector>& legs, const Vector& direction, const std::vector<const Element*>& others) const {

    const Mesh& solid = member.model_geometry_mesh();
    const AABB box = AABB::from_mesh(solid);

    // the places the member passes: along every leg in steps, then along direction from where the legs end
    std::vector<Vector> places;
    Vector reached(0.0, 0.0, 0.0);
    for (const Vector& leg : legs) {
        const double count = std::max(std::ceil(leg.magnitude() / step), 1.0);
        for (double i = 1.0; i <= count; i += 1.0)
            places.push_back(reached + leg * (i / count));
        reached = reached + leg;
    }
    for (double distance = 1.0; distance <= box.diagonal(); distance = distance < step ? step : distance + step)
        places.push_back(reached + direction * distance);

    for (const Vector& move : places) {
        const AABB moved_box(box.cx + move[0], box.cy + move[1], box.cz + move[2], box.hx, box.hy, box.hz);
        const Mesh moved = solid.transformed(Xform::translation(move[0], move[1], move[2]));

        for (const Element* other : others)
            if (moved_box.intersects(AABB::from_mesh(other->model_geometry_mesh())) &&
                compute_volume(solid_boolean(moved, other->model_geometry_mesh(), SolidOperation::intersect)) > clearance)
                return false;
    }

    return true;
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

            for (const std::pair<std::shared_ptr<JointElement>, size_t>& joint : joints[member->guid()]) {
                std::vector<std::string> holding;
                for (const std::string& partner : partners(*joint.first, joint.second))
                    if (in.count(partner))
                        holding.push_back(partner);
                if (holding.empty())
                    continue;

                const std::vector<Vector> directions = joint.first->insertion_directions(joint.second);
                for (size_t way = 0; way < directions.size(); way++) {
                    const std::vector<Vector> legs = joint.first->insertion_legs(joint.second, way);
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
                json["steps"].push_back({
                    {"element", move.element},
                    {"insertion", {move.insertion[0], move.insertion[1], move.insertion[2]}},
                    {"joints", move.joints},
                    {"partners", move.partners},
                    {"low", {box.cx - box.hx, box.cy - box.hy, box.cz - box.hz}},
                    {"high", {box.cx + box.hx, box.cy + box.hy, box.cz + box.hz}},
                });
            }
    json["locked"] = stuck;
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
