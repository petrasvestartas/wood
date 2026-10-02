#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

/// The member families of a quarter.
enum class Family {
    outer_ribs, // Variable beams under the two outer parabolas.
    inner_ribs, // Variable beams under the two inner parabolas.
    inner_beams, // Variable beams between two slanted end faces.
    wedges_inner_beams, // Wedge block plates.
    tsections, // T-section plates.
    beds, // Bed plates.
};

const std::array<std::string, 6> FAMILY_NAMES = {"outer_ribs", "inner_ribs", "inner_beams", "wedges_inner_beams", "tsections", "beds"}; // the group each family is named after, in Family order

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

/// Places an element, names it and adds it under the group.
static void add_placed(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const Xform& placement, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(placement);
    element->name = name;
    session.add(element, group);
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

/// The member as it was before any cut, for the contact search: a copy without its plane and solid cuts, so a pocket or a hole on the cut model neither splits nor loses a contact.
static std::shared_ptr<Element> uncut(const Element& member) {

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

std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group, const std::string& suffix) {

    const std::shared_ptr<wood_session::Support> support = to_support(guide);
    const std::shared_ptr<wood_session::Column> column = to_column(guide, *support);
    support->name = "support" + suffix;
    column->name = "column" + suffix;
    session.add(support, group);
    session.add(column, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*support, *column);
    session.add(joint, group);
    session.add_joint(joint);

    for (const std::shared_ptr<wood_session::Joint>& cutter : to_column_cutters(guide, *column)) {
        session.add(cutter, group);
        session.add_joint(cutter);
    }

    return column;
}

Quarter add_quarter_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group, const std::string& suffix) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<std::vector<Outline>> beds = guide.beds();
    const std::shared_ptr<TreeNode> bed_group = add_group(session, "beds" + suffix, group);
    Quarter quarter;

    for (size_t row = 0; row < beds.size(); row++) {
        quarter.beds.push_back(to_members(Family::beds, beds[row]));
        add_family(session, quarter.beds.back(), lift, fmt::format("beds_{}", row), suffix, bed_group);
    }

    quarter.tsections = to_members(Family::tsections, guide.tsections());
    add_family(session, quarter.tsections, lift, "tsections", suffix, group);
    quarter.outer_ribs = to_members(Family::outer_ribs, guide.outer_ribs());
    add_family(session, quarter.outer_ribs, lift, "outer_ribs", suffix, group);
    quarter.inner_ribs = to_members(Family::inner_ribs, guide.inner_ribs());
    add_family(session, quarter.inner_ribs, lift, "inner_ribs", suffix, group);
    quarter.blocks = to_members(Family::wedges_inner_beams, guide.wedges_inner_beams());
    add_family(session, quarter.blocks, lift, "wedges_inner_beams", suffix, group);
    quarter.inner_beams = to_members(Family::inner_beams, guide.inner_beams());
    add_family(session, quarter.inner_beams, lift, "inner_beams", suffix, group);

    return quarter;
}

std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const std::vector<FloorGuide>& quarters, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, quarters.front().bay_height);
    const std::vector<Outline> outlines = FloorGuide::oculus(quarters);
    const size_t n = quarters.size();
    std::vector<Member> beams;

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < n ? std::static_pointer_cast<Element>(to_beam(outlines[i], {1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(to_plate(outlines[i], "oculus"));
        add_placed(session, member, lift, fmt::format("oculus_{}", i), group);

        if (i < n)
            beams.push_back({member, outline_thickness(outlines[i])});
    }

    return beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<wood_session::JointBeam>> add_wedges(wood_session::WoodSession& session, const std::vector<Member>& ring, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> wedges;

    for (size_t i = 0; i < ring.size(); i++)
        for (size_t j = i + 1; j < ring.size(); j++) {
            const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(ring[i].element, ring[j].element);

            if (!contact || contact->type != wood_session::ContactType::side_side)
                continue;

            const double thickness = std::max(ring[i].thickness, ring[j].thickness);
            add_named(session, wedges, wood_session::JointBeam::wedge(*ring[i].element, *ring[j].element, *contact, 1.5 * thickness, 2.0 * thickness / 3.0), "connector_wedge", group);
        }

    return wedges;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_rectangle_plates(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::Column>>& columns, const std::vector<Member>& outer_ribs, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> plates;

    for (const std::shared_ptr<wood_session::Column>& column : columns)
        for (const Member& rib : outer_ribs) {
            const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(column, rib.element);

            if (!contact)
                continue;

            add_named(session, plates, wood_session::JointBeam::rectangle_plate(*column, *rib.element, *contact, rib.thickness), "connector", group);
        }

    return plates;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_ties(wood_session::WoodSession& session, const std::vector<Member>& outer_ribs, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> ties;

    for (size_t i = 0; i < outer_ribs.size(); i++)
        for (size_t j = i + 1; j < outer_ribs.size(); j++) {
            const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(outer_ribs[i].element, outer_ribs[j].element);

            if (!contact || contact->type != wood_session::ContactType::end_end)
                continue;

            add_named(session, ties, wood_session::JointBeam::tie(*outer_ribs[i].element, *outer_ribs[j].element, *contact), "outer_rib_connector", group);
        }

    return ties;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_quarter_dowels(wood_session::WoodSession& session, const std::vector<Quarter>& quarters, const std::shared_ptr<TreeNode>& group, double radius, double length, double offset) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> joints;

    for (const Quarter& quarter : quarters) {
        std::vector<Member> ribs = quarter.outer_ribs;
        ribs.insert(ribs.end(), quarter.inner_ribs.begin(), quarter.inner_ribs.end());

        for (const Member& rib : ribs)
            for (const Member& block : quarter.blocks) {
                const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(uncut(*rib.element), uncut(*block.element));
                const std::shared_ptr<wood_session::JointBeam> dowels = contact ? wood_session::JointBeam::dowels(*rib.element, *block.element, *contact, radius, length, offset) : nullptr;

                if (dowels)
                    add_named(session, joints, dowels, "connector_dowels", group);
            }
    }

    return joints;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_cross_laps(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> laps;

    for (size_t i = 0; i < plates.size(); i++)
        for (size_t j = i + 1; j < plates.size(); j++) {
            if (plates[i]->targets.front() != plates[j]->targets.front())
                continue;

            add_named(session, laps, wood_session::JointBeam::cross_lap(*plates[i], *plates[j]), "connector_cross_lap", group);
        }

    return laps;
}

}
