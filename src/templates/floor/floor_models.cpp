#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

const std::array<std::string, 6> FAMILY_NAMES = {"outer_ribs", "inner_ribs", "inner_beams", "wedges_inner_beams", "tsections", "beds"}; // the group each quarter family is named after, in Family order

/// The member outlines of one family as members: ribs and inner beams as variable beams, every other family as plates, each with its outline's thickness.
static std::vector<Member> to_members(Family family, const std::vector<Outline>& outlines) {

    const std::string& name = FAMILY_NAMES[static_cast<size_t>(family)];
    std::vector<Member> members;

    for (const Outline& outline : outlines) {
        Member member;
        member.thickness = outline_thickness(outline);

        if (family == Family::outer_ribs || family == Family::inner_ribs)
            member.element = to_rib(outline, name);
        else if (family == Family::inner_beams)
            member.element = to_beam(outline, {0, 3}, {1, 2}, name);
        else
            member.element = to_plate(outline, name);

        members.push_back(member);
    }

    return members;
}

/// Names an element and adds it under the group.
static void add_named(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->name = name;
    session.add(element, group);
}

/// Lifts an element to the floor, names it and adds it under the group.
static void add_placed(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const Xform& lift, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(lift);
    add_named(session, element, name, group);
}

/// Places the members under a new group <prefix><suffix>, each named <prefix>_<i><suffix>.
static void add_family(wood_session::WoodSession& session, const std::vector<Member>& members, const Xform& placement, const std::string& prefix, const std::string& suffix, const std::shared_ptr<TreeNode>& group) {

    const std::shared_ptr<TreeNode> node = add_group(session, prefix + suffix, group);

    for (size_t i = 0; i < members.size(); i++)
        add_placed(session, members[i].element, placement, fmt::format("{}_{}{}", prefix, i, suffix), node);
}

/// Names the connector <prefix>_<i> by its place among the connectors, adds it under the group and appends it.
static void add_named(wood_session::WoodSession& session, std::vector<std::shared_ptr<wood_session::JointBeam>>& connectors, const std::shared_ptr<wood_session::JointBeam>& connector, const std::string& prefix, const std::shared_ptr<TreeNode>& group) {

    connector->name = fmt::format("{}_{}", prefix, connectors.size());
    session.add_connector(connector, group);
    connectors.push_back(connector);
}

std::shared_ptr<Element> uncut(const Element& member) {

    const std::shared_ptr<Element> copy = member.clone();

    if (wood_session::BeamVariable* beam = dynamic_cast<wood_session::BeamVariable*>(copy.get())) {
        beam->cuts.clear();
        beam->solid_cuts.clear();
    } else if (wood_session::Plate* plate = dynamic_cast<wood_session::Plate*>(copy.get()))
        plate->solid_cuts.clear();

    copy->invalidate_geometry();

    return copy;
}

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<TreeNode>& parent) {

    std::shared_ptr<TreeNode> node = std::make_shared<TreeNode>(name);
    session.Session::add(node, parent);

    return node;
}

QuarterMembers add_quarter_model(wood_session::WoodSession& session, const Quarter& view, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, view.sizes().bay_height);
    const std::string suffix = fmt::format("_{}", view.index);
    const std::vector<std::vector<Outline>> beds = view.beds();
    const std::shared_ptr<TreeNode> bed_group = add_group(session, "beds" + suffix, group);
    QuarterMembers quarter;

    for (size_t row = 0; row < beds.size(); row++) {
        quarter.beds.push_back(to_members(Family::beds, beds[row]));
        add_family(session, quarter.beds.back(), lift, fmt::format("beds_{}", row), suffix, bed_group);
    }

    quarter.tsections = to_members(Family::tsections, view.tsections());
    add_family(session, quarter.tsections, lift, "tsections", suffix, group);
    quarter.outer_ribs = to_members(Family::outer_ribs, view.outer_ribs());
    add_family(session, quarter.outer_ribs, lift, "outer_ribs", suffix, group);
    quarter.inner_ribs = to_members(Family::inner_ribs, view.inner_ribs());
    add_family(session, quarter.inner_ribs, lift, "inner_ribs", suffix, group);
    quarter.blocks = to_members(Family::wedges_inner_beams, view.wedges_inner_beams());
    add_family(session, quarter.blocks, lift, "wedges_inner_beams", suffix, group);
    quarter.inner_beams = to_members(Family::inner_beams, view.inner_beams());
    add_family(session, quarter.inner_beams, lift, "inner_beams", suffix, group);

    return quarter;
}

std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, floor.sizes.bay_height);
    const std::vector<Outline> outlines = floor.oculus();
    std::vector<Member> beams;

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < 4 ? std::static_pointer_cast<Element>(to_beam(outlines[i], {1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(to_plate(outlines[i], "oculus"));
        add_placed(session, member, lift, fmt::format("oculus_{}", i), group);

        if (i < 4)
            beams.push_back({member, outline_thickness(outlines[i])});
    }

    return beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

ColumnModel add_column_model(wood_session::WoodSession& session, const Floor& floor, size_t corner, const std::shared_ptr<TreeNode>& group) {

    const std::string suffix = fmt::format("_{}", corner % 4);
    ColumnModel model;
    model.support = to_support(floor.columns[corner % 4]);
    model.column = to_column(floor.columns[corner % 4], floor.sizes, *model.support);
    add_named(session, model.support, "support" + suffix, group);
    add_named(session, model.column, "column" + suffix, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*model.support, *model.column);
    session.add(joint, group);
    session.add_joint(joint);
    model.cutters = to_column_cutters(floor.quarter(corner), *model.column);

    for (const std::shared_ptr<wood_session::Joint>& cutter : model.cutters) {
        session.add(cutter, group);
        session.add_joint(cutter);
    }

    return model;
}

FloorMembers add_floor(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<TreeNode>& group) {

    const std::shared_ptr<TreeNode> quarters = add_group(session, "quarters_model", group);
    FloorMembers members;

    for (size_t q = 0; q < 4; q++)
        members.quarters[q] = add_quarter_model(session, floor.quarter(q), add_group(session, fmt::format("quarter_model_{}", q), quarters));

    members.ring = add_oculus_model(session, floor, add_group(session, "oculus", group));

    return members;
}

void add_columns(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<TreeNode>& group, FloorMembers& members) {

    members.columns.clear();

    for (size_t q = 0; q < 4; q++)
        members.columns.push_back(add_column_model(session, floor, q, add_group(session, fmt::format("column_model_{}", q), group)));
}

/// The member a quarter reference names, null when the family or index is not in the quarter.
static const Member* quarter_member(const QuarterMembers& quarter, const MemberRef& ref) {

    const std::vector<Member>* family = nullptr;

    if (ref.family == Family::outer_ribs)
        family = &quarter.outer_ribs;
    else if (ref.family == Family::inner_ribs)
        family = &quarter.inner_ribs;
    else if (ref.family == Family::inner_beams)
        family = &quarter.inner_beams;
    else if (ref.family == Family::wedges_inner_beams)
        family = &quarter.blocks;
    else if (ref.family == Family::tsections)
        family = &quarter.tsections;
    else if (ref.family == Family::beds && ref.row >= 0 && static_cast<size_t>(ref.row) < quarter.beds.size())
        family = &quarter.beds[static_cast<size_t>(ref.row)];

    return family && ref.index < family->size() ? &(*family)[ref.index] : nullptr;
}

std::shared_ptr<Element> FloorMembers::get(const MemberRef& ref) const {

    if (ref.family == Family::ring)
        return ref.index < ring.size() ? ring[ref.index].element : nullptr;

    if (ref.family == Family::column)
        return ref.index < columns.size() ? columns[ref.index].column : nullptr;

    if (ref.family == Family::support)
        return ref.index < columns.size() ? columns[ref.index].support : nullptr;

    if (ref.family == Family::cutter)
        return ref.quarter >= 0 && static_cast<size_t>(ref.quarter) < columns.size() && ref.index < columns[static_cast<size_t>(ref.quarter)].cutters.size() ? columns[static_cast<size_t>(ref.quarter)].cutters[ref.index] : nullptr;

    if (ref.quarter < 0 || ref.quarter > 3)
        return nullptr;

    const Member* member = quarter_member(quarters[static_cast<size_t>(ref.quarter)], ref);

    return member ? member->element : nullptr;
}

double FloorMembers::thickness(const MemberRef& ref) const {

    if (ref.family == Family::ring)
        return ref.index < ring.size() ? ring[ref.index].thickness : 0.0;

    if (ref.quarter < 0 || ref.quarter > 3)
        return 0.0;

    const Member* member = quarter_member(quarters[static_cast<size_t>(ref.quarter)], ref);

    return member ? member->thickness : 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

std::array<std::shared_ptr<Element>, 2> FloorMembers::pair(const Relationship& row) const {

    const std::shared_ptr<Element> a = get(row.a);
    const std::shared_ptr<Element> b = get(row.b);

    if (!a || !b)
        throw std::runtime_error("the floor members do not hold both members of " + row.text());

    return {a, b};
}

/// The name prefix of a connector of that kind, as the examples name them.
static std::string connector_prefix(Relation kind) {

    if (kind == Relation::seam_wedge || kind == Relation::oculus_wedge)
        return "connector_wedge";

    if (kind == Relation::column_plate)
        return "connector";

    if (kind == Relation::cross_lap)
        return "connector_cross_lap";

    if (kind == Relation::seam_tie)
        return "outer_rib_connector";

    return "connector_dowels";
}

/// The connector of one contact relationship through its factory: the wedge sized by the thicker member, the plate by the rib's thickness, the tie and the dowels by their defaults.
static std::shared_ptr<wood_session::JointBeam> connector_of(const Relationship& row, const FloorMembers& members) {

    const std::array<std::shared_ptr<Element>, 2> pair = members.pair(row);
    const wood_session::InteractionContactFace contact(-1, -1, row.type, row.contact);

    if (row.kind == Relation::seam_wedge || row.kind == Relation::oculus_wedge) {
        const double thickness = std::max(members.thickness(row.a), members.thickness(row.b));
        return wood_session::JointBeam::wedge(*pair[0], *pair[1], contact, 1.5 * thickness, 2.0 * thickness / 3.0);
    }

    if (row.kind == Relation::column_plate)
        return wood_session::JointBeam::rectangle_plate(*pair[0], *pair[1], contact, members.thickness(row.b));

    if (row.kind == Relation::seam_tie)
        return wood_session::JointBeam::tie(*pair[0], *pair[1], contact);

    const std::shared_ptr<wood_session::JointBeam> dowels = wood_session::JointBeam::dowels(*pair[0], *pair[1], contact);

    if (!dowels)
        throw std::runtime_error("the inset leaves no room for the dowels of " + row.text());

    return dowels;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(wood_session::WoodSession& session, const Floor& floor, const FloorMembers& members, const std::shared_ptr<TreeNode>& group, const std::vector<Relation>& kinds) {

    std::map<std::string, std::vector<std::shared_ptr<wood_session::JointBeam>>> by_prefix;
    std::map<size_t, std::vector<std::shared_ptr<wood_session::JointBeam>>> plates_of_corner;
    std::vector<std::shared_ptr<wood_session::JointBeam>> connectors;

    for (const Relationship& row : relationships(floor)) {
        if (row.kind == Relation::support || row.kind == Relation::cutter || std::find(kinds.begin(), kinds.end(), row.kind) == kinds.end())
            continue;

        std::shared_ptr<wood_session::JointBeam> connector;

        if (row.kind == Relation::cross_lap) {
            const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates = plates_of_corner[row.seam_or_corner];

            if (plates.size() != 2)
                throw std::runtime_error("the cross lap of corner " + std::to_string(row.seam_or_corner) + " needs its two column plates in the same call");

            connector = wood_session::JointBeam::cross_lap(*plates[0], *plates[1]);
        } else
            connector = connector_of(row, members);

        add_named(session, by_prefix[connector_prefix(row.kind)], connector, connector_prefix(row.kind), group);
        connectors.push_back(connector);

        if (row.kind == Relation::column_plate)
            plates_of_corner[row.seam_or_corner].push_back(connector);
    }

    return connectors;
}

}
