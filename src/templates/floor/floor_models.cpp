#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

/// Places an element, names it and adds it under the group.
void add_placed(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const Xform& placement, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(placement);
    element->name = name;
    session.add(element, group);
}

/// The member outlines of one group as elements: ribs and inner beams as variable beams, every other group as plates.
std::vector<std::shared_ptr<Element>> to_members(const std::string& group, const std::vector<Outline>& outlines) {

    std::vector<std::shared_ptr<Element>> members;

    for (const Outline& outline : outlines)
        if (group == "outer_ribs" || group == "inner_ribs")
            members.push_back(to_rib(outline, group));
        else if (group == "inner_beams")
            members.push_back(to_beam(outline, {0, 3}, {1, 2}, group));
        else
            members.push_back(to_plate(outline, group));

    return members;
}

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<TreeNode>& parent) {

    std::shared_ptr<TreeNode> node = std::make_shared<TreeNode>(name);
    session.Session::add(node, parent);

    return node;
}

std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, const Xform& placement, const std::shared_ptr<TreeNode>& group, const std::string& suffix) {

    const std::shared_ptr<wood_session::Support> support = to_support(guide);
    const std::shared_ptr<wood_session::Column> column = to_column(guide, *support);
    add_placed(session, support, placement, "support" + suffix, group);
    add_placed(session, column, placement, "column" + suffix, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*support, *column);
    session.add(joint, group);
    session.add_joint(joint);

    for (const std::shared_ptr<wood_session::Joint>& cutter : to_column_cutters(guide, *column)) {
        cutter->place(placement);
        session.add(cutter, group);
        session.add_joint(cutter);
    }

    return column;
}

void add_quarter_model(wood_session::WoodSession& session, const FloorGuide& guide, const Xform& placement, const std::shared_ptr<TreeNode>& group, const std::string& suffix) {

    const Xform lift = placement * Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<std::pair<std::string, std::vector<Outline>>> sections = {
        {"beds", guide.beds()},
        {"tsections", guide.tsections()},
        {"outer_ribs", guide.outer_ribs()},
        {"inner_ribs", guide.inner_ribs()},
        {"wedges_inner_beams", guide.wedges_inner_beams()},
        {"inner_beams", guide.inner_beams()},
    };

    for (const std::pair<std::string, std::vector<Outline>>& section : sections) {
        const std::shared_ptr<TreeNode> node = add_group(session, section.first + suffix, group);
        const std::vector<std::shared_ptr<Element>> members = to_members(section.first, section.second);
        std::map<int, std::shared_ptr<TreeNode>> rows;
        std::map<int, int> counts;

        for (size_t i = 0; i < members.size(); i++) {
            const int row = section.second[i].row;

            if (row < 0) {
                add_placed(session, members[i], lift, fmt::format("{}_{}{}", section.first, i, suffix), node);
                continue;
            }

            if (!rows.count(row))
                rows[row] = add_group(session, fmt::format("{}_{}{}", section.first, row, suffix), node);

            add_placed(session, members[i], lift, fmt::format("{}_{}_{}{}", section.first, row, counts[row]++, suffix), rows[row]);
        }
    }
}

void add_oculus_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<Outline> outlines = guide.oculus();

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < 4 ? std::static_pointer_cast<Element>(to_beam(outlines[i], {1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(to_plate(outlines[i], "oculus"));
        add_placed(session, member, lift, fmt::format("oculus_{}", i), group);
    }
}

}
