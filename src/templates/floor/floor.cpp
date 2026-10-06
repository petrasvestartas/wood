#include "pch.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════
// FLOOR
// ═══════════════════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════════════════

Floor::Floor(const FloorGuide& floor_guide, const std::string& name) : wood_session::WoodSession(name), guide(floor_guide) {
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

void Floor::add_members() {

    add_quarters();
    add_oculus();
    add_columns();
    add_contacts();
}

void Floor::add_quarters() {

    for (size_t q = 0; q < 4; q++) {
        const Quarter view = guide.quarter(q);
        const std::string suffix = fmt::format("_{}", q);
        const std::shared_ptr<TreeNode> group = quarter_group(q);
        QuarterMembers& members = quarters[q];
        members = QuarterMembers();

        const std::shared_ptr<TreeNode> beds = add_group("beds" + suffix, group);
        const std::vector<std::vector<Loops>> rows = view.beds();

        for (size_t row = 0; row < rows.size(); row++) {
            const std::shared_ptr<TreeNode> node = add_group(fmt::format("beds_{}{}", row, suffix), beds);
            members.beds.push_back({});

            for (size_t i = 0; i < rows[row].size(); i++) {
                members.beds.back().push_back(std::make_shared<wood_session::Plate>(rows[row][i][1], rows[row][i][0], "beds"));
                add_placed(members.beds.back().back(), fmt::format("beds_{}_{}{}", row, i, suffix), node);
            }
        }

        const std::shared_ptr<TreeNode> tsections = add_group("tsections" + suffix, group);
        const std::vector<Loops> tsection_loops = view.tsections();

        for (size_t i = 0; i < tsection_loops.size(); i++) {
            members.tsections.push_back(std::make_shared<wood_session::Plate>(tsection_loops[i][1], tsection_loops[i][0], "tsections"));
            add_placed(members.tsections.back(), fmt::format("tsections_{}{}", i, suffix), tsections);
        }

        const std::shared_ptr<TreeNode> outer = add_group("outer_ribs" + suffix, group);
        const std::vector<Loops> outer_loops = view.outer_ribs();

        for (size_t i = 0; i < outer_loops.size(); i++) {
            members.outer_ribs.push_back(rib(outer_loops[i], "outer_ribs"));
            add_placed(members.outer_ribs.back(), fmt::format("outer_ribs_{}{}", i, suffix), outer);
        }

        const std::shared_ptr<TreeNode> inner = add_group("inner_ribs" + suffix, group);
        const std::vector<Loops> inner_loops = view.inner_ribs();

        for (size_t i = 0; i < inner_loops.size(); i++) {
            members.inner_ribs.push_back(rib(inner_loops[i], "inner_ribs"));
            add_placed(members.inner_ribs.back(), fmt::format("inner_ribs_{}{}", i, suffix), inner);
        }

        const std::shared_ptr<TreeNode> wedges = add_group("wedges" + suffix, group);
        const std::vector<Loops> block_loops = view.wedges();

        for (size_t i = 0; i < block_loops.size(); i++) {
            members.wedges.push_back(std::make_shared<wood_session::Plate>(block_loops[i][1], block_loops[i][0], "wedges"));
            add_placed(members.wedges.back(), fmt::format("wedges_{}{}", i, suffix), wedges);
        }

        const std::shared_ptr<TreeNode> beams = add_group("inner_beams" + suffix, group);
        const std::vector<Loops> beam_loops = view.inner_beams();

        for (size_t i = 0; i < beam_loops.size(); i++) {
            members.inner_beams.push_back(beam(beam_loops[i], {0, 3}, {1, 2}, "inner_beams"));
            add_placed(members.inner_beams.back(), fmt::format("inner_beams_{}{}", i, suffix), beams);
        }
    }
}

void Floor::add_oculus() {

    const std::vector<Loops> loops = guide.oculus();
    ring.clear();
    oculus_plates.clear();

    for (size_t i = 0; i < loops.size(); i++) {
        const std::shared_ptr<TreeNode> group = i < 8 ? group_named(fmt::format("oculus_{}", i % 4), quarter_group(i % 4)) : group_named("oculus");

        if (i < 4) {
            ring.push_back(beam(loops[i], {1, 0}, {2, 3}, "oculus"));
            add_placed(ring.back(), fmt::format("oculus_{}", i), group);
        } else {
            oculus_plates.push_back(std::make_shared<wood_session::Plate>(loops[i][1], loops[i][0], "oculus"));
            add_placed(oculus_plates.back(), fmt::format("oculus_{}", i), group);
        }
    }
}

void Floor::add_columns() {

    for (size_t q = 0; q < 4; q++)
        add_column(q);
}

void Floor::add_column(size_t corner) {

    const size_t k = corner % 4;
    const std::shared_ptr<TreeNode> group = group_named(fmt::format("column_{}", k), quarter_group(k));

    if (columns.size() < 4)
        columns.resize(4);

    ColumnModel& model = columns[k];
    model.support = guide.columns[k].to_support();
    model.support->name = fmt::format("support_{}", k);
    model.column = guide.columns[k].to_column(guide, *model.support);
    model.column->name = fmt::format("column_{}", k);
    add(model.support, group);
    add(model.column, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*model.support, *model.column);
    add(joint, group);
    add_joint(joint);

    for (const wood_session::SolidCut& cut : ColumnCutters(guide.quarter(k)).cuts(guide.bay_height))
        model.column->solid_cuts.push_back(cut);

    model.column->invalidate_geometry();
}

std::shared_ptr<TreeNode> Floor::quarter_group(size_t q) {
    return group_named(fmt::format("quarter_{}", q));
}

void Floor::add_placed(const std::shared_ptr<Element>& element, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(Xform::translation(0.0, 0.0, guide.bay_height));
    element->name = name;
    add(element, group);
}

std::shared_ptr<wood_session::BeamVariable> Floor::rib(const Loops& loops, const std::string& name) {

    const std::vector<Point> near = loops[0].get_points();
    const std::vector<Point> far = loops[1].get_points();
    const size_t stations = near.size() - 3;
    std::vector<Polyline> sections;

    for (size_t i = 0; i < stations; i++) {
        const Point& low = near[2 + i];
        const Point& far_low = far[2 + i];
        Point high(low[0], low[1], 0.0);
        Point far_high(far_low[0], far_low[1], 0.0);

        if (i == 0) {
            high = near[1];
            far_high = far[1];
        } else if (i + 1 == stations) {
            high = near[0];
            far_high = far[0];
        }

        sections.push_back(Polyline({low, high, far_high, far_low}).closed());
    }

    const Line axis = Line::from_points(Line::from_points(near[1], far[1]).center(), Line::from_points(near[0], far[0]).center());

    return std::make_shared<wood_session::BeamVariable>(axis, sections, name);
}

std::shared_ptr<wood_session::BeamVariable> Floor::beam(const Loops& loops, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name) {

    const std::vector<Point> near = loops[0].get_points();
    const std::vector<Point> far = loops[1].get_points();
    const Polyline first = Polyline({near[start[0]], near[start[1]], far[start[1]], far[start[0]]}).closed();
    const Polyline last = Polyline({near[end[0]], near[end[1]], far[end[1]], far[end[0]]}).closed();

    return wood_session::BeamVariable::between(first, last, name);
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

void Floor::add_contacts() {

    const ContactFaces faces(guide);
    const bool have_columns = columns.size() == 4 && columns[0].column;

    for (size_t q = 0; q < 4; q++) {
        const QuarterMembers& members = quarters[q];
        const QuarterMembers& next = quarters[(q + 1) % 4];

        if (members.inner_beams.empty())
            continue;

        add_contact(members.inner_beams[0], next.inner_beams[2], faces.seam_wedge(q));

        if (q < ring.size())
            add_contact(members.inner_beams[1], ring[q], faces.oculus_wedge(q));

        for (size_t k = 0; k < 2 && have_columns; k++)
            add_contact(columns[q].column, members.outer_ribs[k], faces.column_plate(q, k));

        if (!guide.seam_through_ribs)
            add_contact(members.outer_ribs[0], next.outer_ribs[1], faces.seam_tie(q));

        // each column block on the two ribs either side of it
        add_contact(members.outer_ribs[0], members.wedges[0], faces.block_dowels(q, 0, 0));
        add_contact(members.outer_ribs[1], members.wedges[2], faces.block_dowels(q, 2, 1));
        add_contact(members.inner_ribs[0], members.wedges[0], faces.block_dowels(q, 0, 1));
        add_contact(members.inner_ribs[0], members.wedges[1], faces.block_dowels(q, 1, 0));
        add_contact(members.inner_ribs[1], members.wedges[1], faces.block_dowels(q, 1, 1));
        add_contact(members.inner_ribs[1], members.wedges[2], faces.block_dowels(q, 2, 0));
    }
}

void Floor::add_contact(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b, const std::shared_ptr<wood_session::InteractionContactFace>& contact) {

    for (const std::shared_ptr<Interaction>& existing : get_interaction(a, b))
        if (existing->name == contact->name)
            return;

    add_interaction(a, b, contact);
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<wood_session::JointBeam>> Floor::add_connectors(const std::vector<ContactKind>& kinds) {

    // every contact interaction of the kinds asked for, read from the graph with its pair in the order it was added
    std::vector<std::tuple<ContactKind, std::string, std::shared_ptr<Element>, std::shared_ptr<Element>, std::shared_ptr<wood_session::InteractionContactFace>>> found;

    for (const auto& [u, w] : graph.get_edges()) {
        const Edge& edge = graph.edges.at(u).at(w);
        const std::shared_ptr<Element> a = get_element<Element>(edge.v0);
        const std::shared_ptr<Element> b = get_element<Element>(edge.v1);

        if (!a || !b)
            continue;

        for (const std::shared_ptr<Interaction>& interaction : get_interaction(a, b)) {
            const std::shared_ptr<wood_session::InteractionContactFace> contact = std::dynamic_pointer_cast<wood_session::InteractionContactFace>(interaction);

            for (const ContactKind kind : kinds)
                if (contact && contact->name.starts_with(CONTACT_NAMES[static_cast<size_t>(kind)] + "_"))
                    found.push_back({kind, contact->name, a, b, contact});
        }
    }

    std::sort(found.begin(), found.end(), [](const auto& x, const auto& y) { return std::make_pair(std::get<0>(x), std::get<1>(x)) < std::make_pair(std::get<0>(y), std::get<1>(y)); });

    // every connector first, so a failing one throws before anything is added or cut
    std::vector<std::tuple<std::string, size_t, std::shared_ptr<wood_session::JointBeam>>> built;
    std::map<size_t, std::vector<std::shared_ptr<wood_session::JointBeam>>> plates_of_corner;

    for (const auto& [kind, name, a, b, contact] : found) {
        // the place the name ends in: the quarter, then a rib or block index and a side
        std::vector<size_t> place;
        std::stringstream indices(name.substr(CONTACT_NAMES[static_cast<size_t>(kind)].size() + 1));

        for (std::string index; std::getline(indices, index, '_');)
            place.push_back(static_cast<size_t>(std::stoul(index)));

        built.push_back({connector_prefix(kind), place[0], connector_of(kind, place, *a, *b, *contact)});

        if (kind == ContactKind::column_plate)
            plates_of_corner[place[0]].push_back(std::get<2>(built.back()));
    }

    for (const auto& [corner, plates] : plates_of_corner)
        if (plates.size() == 2)
            built.push_back({"connector_cross_lap", corner, wood_session::JointBeam::cross_lap(*plates[0], *plates[1])});

    std::map<std::string, size_t> numbers;
    std::vector<std::shared_ptr<wood_session::JointBeam>> added;

    for (const auto& [prefix, q, connector] : built) {
        add_named_connector(connector, prefix, q, numbers);
        connectors.push_back(connector);
        added.push_back(connector);
    }

    return added;
}

std::shared_ptr<wood_session::JointBeam> Floor::connector_of(ContactKind kind, const std::vector<size_t>& place, const Element& a, const Element& b, const wood_session::InteractionContactFace& contact) const {

    const size_t q = place[0];

    if (kind == ContactKind::seam_wedge) {
        const double size = std::max(Quarter::thickness(guide.quarter(q).inner_beams()[0]), Quarter::thickness(guide.quarter(q + 1).inner_beams()[2]));
        const std::optional<Plane> end = guide.seam_through_ribs ? std::optional<Plane>(guide.edges[q].band[0].transformed(Xform::translation(0.0, 0.0, guide.bay_height))) : std::nullopt;
        return wood_session::JointBeam::wedge(a, b, contact, 1.5 * size, 2.0 * size / 3.0, end);
    }

    if (kind == ContactKind::oculus_wedge) {
        const double size = std::max(Quarter::thickness(guide.quarter(q).inner_beams()[1]), Quarter::thickness(guide.oculus()[q]));
        return wood_session::JointBeam::wedge(a, b, contact, 1.5 * size, 2.0 * size / 3.0);
    }

    if (kind == ContactKind::column_plate)
        return wood_session::JointBeam::rectangle_plate(a, b, contact, Quarter::thickness(guide.quarter(q).outer_ribs()[place[1]]));

    if (kind == ContactKind::seam_tie)
        return wood_session::JointBeam::tie(a, b, contact, TIE_TOP);

    const std::shared_ptr<wood_session::JointBeam> dowels = wood_session::JointBeam::dowels(a, b, contact);

    if (!dowels)
        throw std::runtime_error("the inset leaves no room for the dowels of " + contact.name);

    return dowels;
}

std::string Floor::connector_prefix(ContactKind kind) {

    if (kind == ContactKind::seam_wedge || kind == ContactKind::oculus_wedge)
        return "connector_wedge";

    if (kind == ContactKind::column_plate)
        return "connector";

    if (kind == ContactKind::seam_tie)
        return "outer_rib_connector";

    return "connector_dowels";
}

void Floor::add_named_connector(const std::shared_ptr<wood_session::JointBeam>& connector, const std::string& prefix, size_t q, std::map<std::string, size_t>& numbers) {

    if (!numbers.count(prefix))
        numbers[prefix] = next_number(prefix);

    connector->name = fmt::format("{}_{}", prefix, numbers[prefix]++);
    const std::shared_ptr<TreeNode> group = group_named(fmt::format("connectors_{}", q), quarter_group(q));
    set_node_color(add_connector(connector, group), CONNECTOR_COLOR, true);
}

// ═══════════════════════════════════════════════════════════════════════════
// Screws
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<wood_session::JointBeam>> Floor::add_screws() {

    const ScrewLines lines(guide);
    std::vector<std::pair<size_t, std::shared_ptr<wood_session::JointBeam>>> built;

    // every screw connector first, so a bay too narrow for them throws with nothing added
    for (size_t q = 0; q < 4; q++) {
        const QuarterMembers& members = quarters[q];

        for (size_t k = 0; k < 2; k++)
            built.push_back({q, screws_of({members.outer_ribs[k].get(), members.inner_beams[k == 0 ? 0 : 2].get()}, lines.rib_beam(q, k))});

        for (size_t k = 0; k < 2; k++)
            built.push_back({q, screws_of({members.inner_beams[k == 0 ? 0 : 2].get(), members.inner_beams[1].get()}, lines.beam_mitre(q, k))});

        for (size_t k = 0; k < 2; k++) {
            const std::vector<Line> screw_lines = lines.rib_corner(q, k);
            std::vector<const Element*> passed = {members.inner_beams[1].get(), members.inner_ribs[k].get()};

            if (lines.passes_seam_beam(q, k, screw_lines))
                passed.push_back(members.inner_beams[k == 0 ? 0 : 2].get());

            built.push_back({q, screws_of(passed, screw_lines)});
        }
    }

    for (size_t q = 0; q < 4; q++)
        built.push_back({q, screws_of({ring[q].get(), ring[(q + 1) % 4].get()}, lines.ring(q))});

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            built.push_back({q, screws_of({ring[q].get(), quarters[q].inner_beams[1].get()}, lines.oculus(q, k))});

    std::map<std::string, size_t> numbers;
    std::vector<std::shared_ptr<wood_session::JointBeam>> added;

    for (const auto& [q, connector] : built) {
        add_named_connector(connector, "connector_screws", q, numbers);
        screws.push_back(connector);
        added.push_back(connector);
    }

    return added;
}

std::shared_ptr<wood_session::JointBeam> Floor::screws_of(const std::vector<const Element*>& members, const std::vector<Line>& lines) const {

    for (const Element* member : members)
        if (!member)
            throw std::runtime_error("add_screws needs every member of the floor in the scene first");

    return wood_session::JointBeam::screws(members, lines);
}

}
