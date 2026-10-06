#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

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

/// Colours the node and every node nested under it.
static void paint(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& node, const Color& color) {

    session.set_node_color(node, color);

    for (TreeNode* child : node->descendants())
        session.set_node_color(child->shared_from_this(), color);
}

/// The live child of parent named name, null when there is none; a null parent is the tree's root.
static std::shared_ptr<TreeNode> child_named(const wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& parent, const std::string& name) {

    const std::shared_ptr<TreeNode> host = parent ? parent : session.tree.root();

    if (!host)
        return nullptr;

    for (TreeNode* child : host->children())
        if (child->name == name)
            return child->shared_from_this();

    return nullptr;
}

/// The group named name under parent, added after its other children the first time.
static std::shared_ptr<TreeNode> group_named(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& parent, const std::string& name) {

    const std::shared_ptr<TreeNode> group = child_named(session, parent, name);

    return group ? group : add_group(session, name, parent);
}

/// The group of quarter q under the floor's group, made the first time; every member, column and connector of the quarter goes in it.
static std::shared_ptr<TreeNode> quarter_group(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& floor, size_t q) {
    return group_named(session, floor, fmt::format("quarter_{}", q));
}

/// The next free number of a connector name prefix in the session: one past the highest <prefix>_<n> already there, 0 when there is none.
static size_t next_number(const wood_session::WoodSession& session, const std::string& prefix) {

    size_t next = 0;

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        const std::string& name = element->name;

        if (name.size() > prefix.size() + 1 && name.compare(0, prefix.size() + 1, prefix + "_") == 0 && std::all_of(name.begin() + prefix.size() + 1, name.end(), ::isdigit))
            next = std::max(next, static_cast<size_t>(std::stoul(name.substr(prefix.size() + 1))) + 1);
    }

    return next;
}

std::shared_ptr<Element> uncut(const Element& member) {

    const std::shared_ptr<Element> copy = member.clone();

    if (wood_session::BeamVariable* beam = dynamic_cast<wood_session::BeamVariable*>(copy.get())) {
        beam->cuts.clear();
        beam->solid_cuts.clear();
    } else if (wood_session::Plate* plate = dynamic_cast<wood_session::Plate*>(copy.get())) {
        plate->solid_cuts.clear();
    } else if (wood_session::Column* column = dynamic_cast<wood_session::Column*>(copy.get())) {
        // the head carve stays, only what connectors cut goes
        std::erase_if(column->solid_cuts, [](const wood_session::SolidCut& cut) { return !cut.joint_guid.empty(); });
    }

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

    const Xform lift = Xform::translation(0.0, 0.0, view.guide.bay_height);
    const std::string suffix = fmt::format("_{}", view.index);
    const std::vector<std::vector<Outline>> beds = view.beds();
    const std::shared_ptr<TreeNode> bed_group = add_group(session, "beds" + suffix, group);
    QuarterMembers quarter;
    quarter.group = group;

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
    quarter.wedges = to_members(Family::wedges, view.wedges());
    add_family(session, quarter.wedges, lift, "wedges", suffix, group);
    quarter.inner_beams = to_members(Family::inner_beams, view.inner_beams());
    add_family(session, quarter.inner_beams, lift, "inner_beams", suffix, group);

    return quarter;
}

std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<Outline> outlines = guide.oculus();
    std::vector<Member> beams;

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < 4 ? std::static_pointer_cast<Element>(to_beam(outlines[i], {1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(to_plate(outlines[i], "oculus"));
        const std::shared_ptr<TreeNode> host = i < 8 ? group_named(session, quarter_group(session, group, i % 4), fmt::format("oculus_{}", i % 4)) : group_named(session, group, "oculus");
        add_placed(session, member, lift, fmt::format("oculus_{}", i), host);

        if (i < 4)
            beams.push_back({member, outline_thickness(outlines[i])});
    }

    return beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

ColumnModel add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, size_t corner, const std::shared_ptr<TreeNode>& group) {

    const std::string suffix = fmt::format("_{}", corner % 4);
    ColumnModel model;
    model.group = group;
    model.support = to_support(guide.columns[corner % 4]);
    model.column = to_column(guide.columns[corner % 4], guide, *model.support);
    add_named(session, model.support, "support" + suffix, group);
    add_named(session, model.column, "column" + suffix, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*model.support, *model.column);
    session.add(joint, group);
    session.add_joint(joint);

    for (const wood_session::SolidCut& cut : column_cuts(guide.quarter(corner)))
        model.column->solid_cuts.push_back(cut);

    model.column->invalidate_geometry();

    return model;
}

FloorMembers add_floor(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    FloorMembers members;
    members.group = group;

    for (size_t q = 0; q < 4; q++)
        members.quarters[q] = add_quarter_model(session, guide.quarter(q), quarter_group(session, group, q));

    members.ring = add_oculus_model(session, guide, group);
    members.oculus = group_named(session, group, "oculus");

    return members;
}

void add_columns(wood_session::WoodSession& session, const FloorGuide& guide, FloorMembers& members) {

    members.columns.resize(4);

    for (size_t q = 0; q < 4; q++)
        members.columns[q] = add_column_model(session, guide, q, group_named(session, quarter_group(session, members.group, q), fmt::format("column_{}", q)));
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
    else if (ref.family == Family::wedges)
        family = &quarter.wedges;
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

    if (std::find(SCREW_RELATIONS.begin(), SCREW_RELATIONS.end(), kind) != SCREW_RELATIONS.end())
        return "connector_screws";

    return "connector_dowels";
}

/// The group a relationship's connector goes under, made the first time: connectors_q in the group of its quarter q.
static std::shared_ptr<TreeNode> connector_group(wood_session::WoodSession& session, const FloorMembers& members, const Relationship& row) {
    return group_named(session, quarter_group(session, members.group, row.seam_or_corner), fmt::format("connectors_{}", row.seam_or_corner));
}

/// The connector of one contact relationship through its factory: the wedge sized by the thicker member, the plate by the rib's thickness, the tie, the screws and the dowels by their defaults.
static std::shared_ptr<wood_session::JointBeam> connector_of(const Relationship& row, const FloorMembers& members) {

    const std::array<std::shared_ptr<Element>, 2> pair = members.pair(row);
    const wood_session::InteractionContactFace contact(-1, -1, row.type, row.contact);

    if (row.kind == Relation::seam_wedge || row.kind == Relation::oculus_wedge) {
        const double thickness = std::max(members.thickness(row.a), members.thickness(row.b));
        return wood_session::JointBeam::wedge(*pair[0], *pair[1], contact, 1.5 * thickness, 2.0 * thickness / 3.0, row.end);
    }

    if (row.kind == Relation::column_plate)
        return wood_session::JointBeam::rectangle_plate(*pair[0], *pair[1], contact, members.thickness(row.b));

    if (row.kind == Relation::seam_tie)
        return wood_session::JointBeam::tie(*pair[0], *pair[1], contact, TIE_TOP);

    if (!row.screws.empty()) {
        std::vector<const Element*> passed = {pair[0].get(), pair[1].get()};

        for (const MemberRef& ref : row.through)
            passed.push_back(members.get(ref).get());

        return wood_session::JointBeam::screws(passed, row.screws);
    }

    const std::shared_ptr<wood_session::JointBeam> dowels = wood_session::JointBeam::dowels(*pair[0], *pair[1], contact);

    if (!dowels)
        throw std::runtime_error("the inset leaves no room for the dowels of " + row.text());

    return dowels;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(wood_session::WoodSession& session, const FloorGuide& guide, const FloorMembers& members, const std::vector<Relation>& kinds) {

    std::map<size_t, std::vector<std::shared_ptr<wood_session::JointBeam>>> plates_of_corner;
    std::vector<std::pair<Relationship, std::shared_ptr<wood_session::JointBeam>>> built;

    // every connector first, so a missing member throws before anything is added or cut
    for (const Relationship& row : relationships(guide)) {
        if (row.kind == Relation::support || std::find(kinds.begin(), kinds.end(), row.kind) == kinds.end())
            continue;

        std::shared_ptr<wood_session::JointBeam> connector;

        if (row.kind == Relation::cross_lap) {
            const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates = plates_of_corner[row.seam_or_corner];

            if (plates.size() != 2)
                throw std::runtime_error("the cross lap of corner " + std::to_string(row.seam_or_corner) + " needs its two column plates in the same call");

            connector = wood_session::JointBeam::cross_lap(*plates[0], *plates[1]);
        } else
            connector = connector_of(row, members);

        built.push_back({row, connector});

        if (row.kind == Relation::column_plate)
            plates_of_corner[row.seam_or_corner].push_back(connector);
    }

    std::map<std::string, size_t> numbers;
    std::vector<std::shared_ptr<wood_session::JointBeam>> connectors;

    for (const auto& [row, connector] : built) {
        const std::string prefix = connector_prefix(row.kind);

        if (!numbers.count(prefix))
            numbers[prefix] = next_number(session, prefix);

        connector->name = fmt::format("{}_{}", prefix, numbers[prefix]++);
        paint(session, session.add_connector(connector, connector_group(session, members, row)), CONNECTOR_COLOR);
        connectors.push_back(connector);
    }

    return connectors;
}

// ═══════════════════════════════════════════════════════════════════════════
// Model
// ═══════════════════════════════════════════════════════════════════════════

Floor::Floor(const FloorGuide& floor_guide, const std::string& name) : wood_session::WoodSession(name), guide(floor_guide) {
}

void Floor::add_column(size_t corner) {

    if (members.columns.size() < 4)
        members.columns.resize(4);

    members.columns[corner % 4] = add_column_model(*this, guide, corner % 4, group_named(*this, quarter_group(*this, members.group, corner % 4), fmt::format("column_{}", corner % 4)));
}

void Floor::add_columns() {

    for (size_t q = 0; q < 4; q++)
        add_column(q);
}

void Floor::add_quarters() {

    for (size_t q = 0; q < 4; q++)
        members.quarters[q] = add_quarter_model(*this, guide.quarter(q), quarter_group(*this, members.group, q));
}

void Floor::add_oculus() {

    members.ring = add_oculus_model(*this, guide, members.group);
    members.oculus = group_named(*this, members.group, "oculus");
}

void Floor::add_members() {

    add_quarters();
    add_oculus();
    add_columns();
}

void Floor::add_connectors(const std::vector<Relation>& kinds) {

    for (const std::shared_ptr<wood_session::JointBeam>& connector : wood_floor::add_connectors(*this, guide, members, kinds))
        (connector->name.starts_with("connector_screws_") ? screws : connectors).push_back(connector);
}

void Floor::add_screws() {

    add_connectors({SCREW_RELATIONS.begin(), SCREW_RELATIONS.end()});
}

}
