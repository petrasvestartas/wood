#include "pch.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

Floor::Floor(const FloorGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // quarters: every quarter's members, lifted to bay_height and grouped by family
    add_quarters();

    // oculus: the four ring beams, the bottom wedges and the central plate
    add_oculus();

    // columns: the column at every corner, its head carved by the guide's cutters
    add_columns();

    // contacts: per quarter an interaction between every two members that touch, named by its kind and place
    const std::array<QuarterContacts, 4> contacts = add_contacts();

    // connectors: per quarter its wedges, column plates with their cross lap, centred and headed pins, all built on uncut members
    const std::array<QuarterConnectors, 4> connectors = compute_connectors(contacts);
    add_connectors(connectors, contacts);
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

void Floor::add_quarters() {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);

    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<TreeNode> group = quarter_group(q);

        // beds: Plate, three rows of plates between the bed rails, beds_<row>_<i>_<q> under beds_<row>_<q>
        const std::shared_ptr<TreeNode> beds = add_group(fmt::format("beds_{}", q), group);
        const std::array<std::array<std::array<Polyline, 2>, 2>, 3>& rows = guide.bed_rails(q);

        for (size_t row = 0; row < rows.size(); row++) {
            const std::shared_ptr<TreeNode> node = add_group(fmt::format("beds_{}_{}", row, q), beds);
            const std::vector<std::shared_ptr<Plate>> plates = Plate::row_between(rows[row][0], rows[row][1]);

            for (size_t i = 0; i < plates.size(); i++) {
                plates[i]->name = fmt::format("beds_{}_{}_{}", row, i, q);
                plates[i]->place(lift);
                add(plates[i], node);
            }
        }

        // tsections: Plate, the flanges beside the ribs, tsections_<i>_<q>
        const std::shared_ptr<TreeNode> tsections = add_group(fmt::format("tsections_{}", q), group);
        const std::array<std::array<Polyline, 2>, 6>& tsection_loops = guide.tsections(q);

        for (size_t i = 0; i < tsection_loops.size(); i++) {
            const std::shared_ptr<Plate> tsection = std::make_shared<Plate>(tsection_loops[i][1], tsection_loops[i][0], fmt::format("tsections_{}_{}", i, q));
            tsection->place(lift);
            add(tsection, tsections);
        }

        // outer_ribs: BeamVariable, the two ribs along the bay edges, outer_ribs_<i>_<q>
        const std::shared_ptr<TreeNode> outer = add_group(fmt::format("outer_ribs_{}", q), group);
        const std::array<std::array<Polyline, 2>, 2>& outer_loops = guide.outer_ribs(q);

        for (size_t i = 0; i < outer_loops.size(); i++) {
            const std::shared_ptr<BeamVariable> outer_rib = rib(outer_loops[i], fmt::format("outer_ribs_{}_{}", i, q));
            outer_rib->place(lift);
            add(outer_rib, outer);
        }

        // inner_ribs: BeamVariable, the two ribs from the column head to the inner beam corners, inner_ribs_<i>_<q>
        const std::shared_ptr<TreeNode> inner = add_group(fmt::format("inner_ribs_{}", q), group);
        const std::array<std::array<Polyline, 2>, 2>& inner_loops = guide.inner_ribs(q);

        for (size_t i = 0; i < inner_loops.size(); i++) {
            const std::shared_ptr<BeamVariable> inner_rib = rib(inner_loops[i], fmt::format("inner_ribs_{}_{}", i, q));
            inner_rib->place(lift);
            add(inner_rib, inner);
        }

        // wedges: Plate, the three column blocks of the column head fan, wedges_<i>_<q>
        const std::shared_ptr<TreeNode> wedges = add_group(fmt::format("wedges_{}", q), group);
        const std::array<std::array<Polyline, 2>, 3>& block_loops = guide.wedges(q);

        for (size_t i = 0; i < block_loops.size(); i++) {
            const std::shared_ptr<Plate> block = std::make_shared<Plate>(block_loops[i][1], block_loops[i][0], fmt::format("wedges_{}_{}", i, q));
            block->place(lift);
            add(block, wedges);
        }

        // inner_beams: BeamVariable, seam beam 0, the beam along the oculus edge and seam beam 1, inner_beams_<i>_<q>
        const std::shared_ptr<TreeNode> beams = add_group(fmt::format("inner_beams_{}", q), group);
        const std::array<std::array<Polyline, 2>, 3>& beam_loops = guide.inner_beams(q);

        for (size_t i = 0; i < beam_loops.size(); i++) {
            const std::shared_ptr<BeamVariable> inner_beam = beam(
                beam_loops[i],
                {0, 3},
                {1, 2},
                fmt::format("inner_beams_{}_{}", i, q)
            );
            inner_beam->place(lift);
            add(inner_beam, beams);
        }
    }
}

void Floor::add_oculus() {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const std::array<std::array<Polyline, 2>, 9>& loops = guide.oculus();
    const std::shared_ptr<TreeNode> group = group_named("oculus");

    // ring_beams: BeamVariable, the four ring beams, oculus_<q>
    const std::shared_ptr<TreeNode> ring_beams = add_group("ring_beams", group);

    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<BeamVariable> ring_beam = beam(
            loops[q],
            {1, 0},
            {2, 3},
            fmt::format("oculus_{}", q)
        );
        ring_beam->place(lift);
        add(ring_beam, ring_beams);
    }

    // bottom_wedges: Plate, the plate under ring beam q, oculus_<4 + q>
    const std::shared_ptr<TreeNode> bottom_wedges = add_group("bottom_wedges", group);

    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<Plate> bottom_wedge = std::make_shared<Plate>(loops[4 + q][1], loops[4 + q][0], fmt::format("oculus_{}", 4 + q));
        bottom_wedge->place(lift);
        add(bottom_wedge, bottom_wedges);
    }

    // central_plate: Plate, oculus_8
    const std::shared_ptr<TreeNode> central_plate = add_group("central_plate", group);
    const std::shared_ptr<Plate> plate = std::make_shared<Plate>(loops[8][1], loops[8][0], "oculus_8");
    plate->place(lift);
    add(plate, central_plate);
}

void Floor::add_columns() {

    for (size_t q = 0; q < 4; q++)
        add_column(q);
}

void Floor::add_column(size_t corner) {

    const std::string name = fmt::format("column_{}", corner);
    const std::shared_ptr<TreeNode> group = group_named(name, quarter_group(corner));

    const std::shared_ptr<Support> support = std::make_shared<Support>(guide.support_plane(corner), "support");
    support->name = fmt::format("support_{}", corner);
    const Line axis = support->column_axis(guide.bay_height);
    const Plane frame = guide.column_frame(corner);
    const std::shared_ptr<Column> shaft = Column::square(
        axis,
        frame,
        guide.size_column_head,
        name
    );

    const std::array<std::array<Polyline, 2>, 6>& loops = guide.column_cutters(corner);
    std::vector<std::shared_ptr<Plate>> cutters;

    for (size_t i = 0; i < loops.size(); i++) {
        cutters.push_back(std::make_shared<Plate>(loops[i][1], loops[i][0], fmt::format("column_cutters_{}_{}", i, corner)));
        cutters.back()->place(Xform::translation(0.0, 0.0, guide.bay_height));
    }

    add(shaft, group);

    // the head: blocks glued on as wide as the chamfer reaches, as deep as the carved head
    const double head_width = guide.size_column_head + guide.size_column_head_chamfer;

    for (const std::shared_ptr<Block>& block : shaft->head_blocks(head_width, guide.column_head_depth)) {
        add(block, group);
        const std::shared_ptr<InteractionFeatureSolid> glue = std::make_shared<InteractionFeatureSolid>(block->element_geometry_mesh(), SolidOperation::add);
        add_interaction(block, shaft, glue);
    }

    // the support under it, its joint let into the column end and drilled
    add(support, group);
    const std::shared_ptr<Joint> seat = Joint::support(*support, *shaft);
    add(seat, group);
    add_interaction(seat, shaft, seat->interaction(0));

    // the six cutters, hidden, take the head's inclined faces away
    for (const std::shared_ptr<Plate>& cutter : cutters) {
        cutter->is_visible = false;
        add(cutter, group);
        const std::shared_ptr<InteractionFeatureSolid> cut = std::make_shared<InteractionFeatureSolid>(cutter->element_geometry_mesh(), SolidOperation::subtract);
        add_interaction(cutter, shaft, cut);
    }
}

std::shared_ptr<TreeNode> Floor::quarter_group(size_t q) {
    return group_named(fmt::format("quarter_{}", q));
}

std::shared_ptr<TreeNode> Floor::connectors_group(size_t q) {
    return group_named(fmt::format("connectors_{}", q), quarter_group(q));
}

std::shared_ptr<BeamVariable> Floor::rib(const std::array<Polyline, 2>& loops, const std::string& name) {

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

    const Point start = Point::mid_point(near[1], far[1]);
    const Point end = Point::mid_point(near[0], far[0]);
    const Line axis = Line::from_points(start, end);

    return std::make_shared<BeamVariable>(axis, sections, name);
}

std::shared_ptr<BeamVariable> Floor::beam(
    const std::array<Polyline, 2>& loops,
    const std::array<size_t, 2>& start,
    const std::array<size_t, 2>& end,
    const std::string& name
) {

    const std::vector<Point> near = loops[0].get_points();
    const std::vector<Point> far = loops[1].get_points();
    const Polyline first = Polyline({near[start[0]], near[start[1]], far[start[1]], far[start[0]]}).closed();
    const Polyline last = Polyline({near[end[0]], near[end[1]], far[end[1]], far[end[0]]}).closed();

    return BeamVariable::between(first, last, name);
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

std::array<QuarterContacts, 4> Floor::add_contacts() {

    std::array<QuarterContacts, 4> contacts;

    for (size_t q = 0; q < 4; q++) {
        const size_t next = (q + 1) % 4;

        // seam: this quarter's seam beam 0 beside the next quarter's seam beam 2
        const Contact seam = add_contact(fmt::format("seam_wedge_{}", q), fmt::format("inner_beams_0_{}", q), fmt::format("inner_beams_2_{}", next));
        contacts[q].seam_wedge = seam;

        // oculus: the oculus beam's back face on its ring beam
        const Contact oculus = add_contact(fmt::format("oculus_wedge_{}", q), fmt::format("inner_beams_1_{}", q), fmt::format("oculus_{}", q));
        contacts[q].oculus_wedge = oculus;

        // column head: the column against each of its two outer ribs
        for (size_t k = 0; k < 2; k++) {
            const Contact plate = add_contact(fmt::format("column_plate_{}_{}", q, k), fmt::format("column_{}", q), fmt::format("outer_ribs_{}_{}", k, q));
            contacts[q].column_plates[k] = plate;
        }

        // column blocks: outer_rib 0 | block 0 | inner_rib 0 | block 1 | inner_rib 1 | block 2 | outer_rib 1, each block on the rib either side
        const std::array<std::string, 4> ribs = {fmt::format("outer_ribs_0_{}", q), fmt::format("inner_ribs_0_{}", q), fmt::format("inner_ribs_1_{}", q), fmt::format("outer_ribs_1_{}", q)};

        for (size_t b = 0; b < 3; b++)
            for (size_t side = 0; side < 2; side++) {
                const Contact pins = add_contact(fmt::format("block_pins_{}_{}_{}", q, b, side), ribs[b + side], fmt::format("wedges_{}_{}", b, q));
                contacts[q].block_pins[b][side] = pins;
            }

        // butt joints held by pins: the outer rib ending on its seam beam, the seam beam on the oculus beam, the oculus beam on the inner rib
        for (size_t k = 0; k < 2; k++) {
            const std::string seam_beam = fmt::format("inner_beams_{}_{}", SEAM_BEAMS[k], q);
            const std::string oculus_beam = fmt::format("inner_beams_1_{}", q);
            contacts[q].outer_rib_seam_beam[k] = add_contact(fmt::format("pins_outer_rib_{}_{}", q, k), seam_beam, fmt::format("outer_ribs_{}_{}", k, q));
            contacts[q].seam_beam_oculus_beam[k] = add_contact(fmt::format("pins_seam_beam_{}_{}", q, k), seam_beam, oculus_beam);
            contacts[q].oculus_beam_inner_rib[k] = add_contact(fmt::format("pins_inner_rib_{}_{}", q, k), oculus_beam, fmt::format("inner_ribs_{}_{}", k, q));
        }

        // ring corner: ring beam q against the next, at the oculus corner they share
        const Contact ring_corner = add_contact(fmt::format("pins_ring_corner_{}", q), fmt::format("oculus_{}", q), fmt::format("oculus_{}", (q + 1) % 4));
        contacts[q].ring_corner = ring_corner;
    }

    return contacts;
}

Contact Floor::add_contact(const std::string& name, const std::string& a_name, const std::string& b_name) {

    const std::shared_ptr<Element> a = get_element_by_name<Element>(a_name);
    const std::shared_ptr<Element> b = get_element_by_name<Element>(b_name);
    const std::shared_ptr<InteractionContactFace> face = compute_face_contact(a, b);

    if (!face)
        throw std::runtime_error(fmt::format("no contact {} between {} and {}", name, a_name, b_name));

    face->name = name;
    add_interaction(a, b, face);

    return {a, b, face};
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

std::array<QuarterConnectors, 4> Floor::compute_connectors(const std::array<QuarterContacts, 4>& contacts) const {

    std::array<QuarterConnectors, 4> connectors;
    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);

    for (size_t q = 0; q < 4; q++) {
        const QuarterContacts& quarter_contacts = contacts[q];
        QuarterConnectors& quarter_connectors = connectors[q];

        // seam wedge: sized by the inner beams, running on to the bay's outer face
        const Contact& seam = quarter_contacts.seam_wedge;
        const double beam = guide.size_inner_beams;
        const Plane outer_face = guide.construction_planes(q).outer_ribs[0][0].transformed(lift);
        quarter_connectors.seam_wedge = JointBeam::wedge(
            *seam.a,
            *seam.b,
            *seam.face,
            1.5 * beam,
            2.0 * beam / 3.0,
            outer_face
        );

        // oculus wedge: sized by the thicker of the oculus beam and its ring beam
        const Contact& oculus = quarter_contacts.oculus_wedge;
        const double oculus_beam = FloorGuide::thickness(guide.inner_beams(q)[1]);
        const double ring_beam = FloorGuide::thickness(guide.oculus()[q]);
        const double thicker = std::max(oculus_beam, ring_beam);
        quarter_connectors.oculus_wedge = JointBeam::wedge(
            *oculus.a,
            *oculus.b,
            *oculus.face,
            1.5 * thicker,
            2.0 * thicker / 3.0
        );

        // column plates: a plate on each outer rib, let into the column and the rib by a pocket, two pins in each
        for (size_t k = 0; k < 2; k++) {
            const Contact& column = quarter_contacts.column_plates[k];
            quarter_connectors.column_plates[k] = JointBeam::let_in_plate(*column.b, *column.face);
            quarter_connectors.column_plate_pins[k] = JointBeam::rectangle_plate(
                *column.a,
                *column.b,
                *quarter_connectors.column_plates[k],
                *column.face,
                guide.size_outer_ribs
            );
        }

        // cross lap: the half lap where the two column plates cross, a plate joint merged into both outlines
        const std::array<std::shared_ptr<Plate>, 2>& plates = quarter_connectors.column_plates;
        const std::shared_ptr<InteractionContactCross> crossing = compute_cross_contact(plates[0], plates[1]);

        if (!crossing)
            throw std::runtime_error(fmt::format("quarter {}: the two column plates do not cross", q));

        quarter_connectors.cross_lap = JointPlate::cr_c_ip_0();
        quarter_connectors.cross_lap->orient(crossing, {plates[0], plates[1]});
        quarter_connectors.cross_lap->name = fmt::format("connector_cross_lap_{}", q);

        // block pins: pins between each column block and the rib either side
        for (size_t b = 0; b < 3; b++)
            for (size_t side = 0; side < 2; side++) {
                const Contact& contact = quarter_contacts.block_pins[b][side];
                const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(*contact.a, *contact.b, *contact.face);

                if (!pins)
                    throw std::runtime_error("the inset leaves no room for the pins of " + contact.face->name);

                quarter_connectors.block_pins[b][side] = pins;
            }

        // pins: two in a column across each butt joint, along the member that ends on it; the two quarters' pins at a seam either side of its middle
        for (size_t k = 0; k < 2; k++) {
            const double shift = k == 0 ? -PIN_SHIFT : PIN_SHIFT;
            const Contact& outer = quarter_contacts.outer_rib_seam_beam[k];
            const Contact& seam = quarter_contacts.seam_beam_oculus_beam[k];
            const Contact& inner = quarter_contacts.oculus_beam_inner_rib[k];
            quarter_connectors.outer_rib_seam_beam[k] = JointBeam::headed_pins(
                *outer.a,
                *outer.b,
                *outer.face,
                PinLayout::vertical,
                2,
                PIN_INSET,
                shift,
                PIN_RADIUS,
                PIN_LENGTH
            );
            quarter_connectors.seam_beam_oculus_beam[k] = JointBeam::headed_pins(
                *seam.a,
                *seam.b,
                *seam.face,
                PinLayout::vertical,
                2,
                PIN_INSET,
                shift,
                PIN_RADIUS,
                PIN_LENGTH
            );
            quarter_connectors.oculus_beam_inner_rib[k] = JointBeam::headed_pins(
                *inner.a,
                *inner.b,
                *inner.face,
                PinLayout::vertical,
                2,
                PIN_INSET,
                0.0,
                PIN_RADIUS,
                PIN_LENGTH
            );

            if (!quarter_connectors.outer_rib_seam_beam[k] || !quarter_connectors.seam_beam_oculus_beam[k] || !quarter_connectors.oculus_beam_inner_rib[k])
                throw std::runtime_error(fmt::format("quarter {} side {}: a butt joint leaves no room for its pins, the bay is too narrow", q, k));
        }

        // ring corner pins: two in a column across the corner of ring beam q and the next
        const Contact& ring = quarter_contacts.ring_corner;
        quarter_connectors.ring_corner = JointBeam::headed_pins(
            *ring.a,
            *ring.b,
            *ring.face,
            PinLayout::vertical,
            2,
            PIN_INSET,
            0.0,
            PIN_RADIUS,
            PIN_LENGTH
        );

        if (!quarter_connectors.ring_corner)
            throw std::runtime_error("the inset leaves no room for the pins of " + ring.face->name);
    }

    return connectors;
}

void Floor::add_connectors(const std::array<QuarterConnectors, 4>& connectors, const std::array<QuarterContacts, 4>& contacts) {

    // seam wedges: each wedge added, then into the two seam beams it joins
    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<JointBeam>& wedge = connectors[q].seam_wedge;
        const Contact& seam = contacts[q].seam_wedge;
        add(wedge, connectors_group(q));
        add_interaction(wedge, seam.a, wedge->interaction(0));
        add_interaction(wedge, seam.b, wedge->interaction(1));
    }

    // oculus wedges: into the oculus beam and its ring beam
    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<JointBeam>& wedge = connectors[q].oculus_wedge;
        const Contact& oculus = contacts[q].oculus_wedge;
        add(wedge, group_named("connectors", group_named("oculus")));
        add_interaction(wedge, oculus.a, wedge->interaction(0));
        add_interaction(wedge, oculus.b, wedge->interaction(1));
    }

    // column plates: each plate added, then let into the column and the outer rib, the pins bored through it too
    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::shared_ptr<Plate>& plate = connectors[q].column_plates[k];
            const std::shared_ptr<JointBeam>& pins = connectors[q].column_plate_pins[k];
            const Contact& column = contacts[q].column_plates[k];
            add(plate, connectors_group(q));
            add(pins, connectors_group(q));
            add_interaction(pins, column.a, pins->interaction(0));
            add_interaction(pins, column.b, pins->interaction(1));
            add_interaction(pins, plate, pins->interaction(2));
        }

    // block pins: into the rib and the column block
    for (size_t q = 0; q < 4; q++)
        for (size_t b = 0; b < 3; b++)
            for (size_t side = 0; side < 2; side++) {
                const std::shared_ptr<JointBeam>& pins = connectors[q].block_pins[b][side];
                const Contact& block = contacts[q].block_pins[b][side];
                add(pins, connectors_group(q));
                add_interaction(pins, block.a, pins->interaction(0));
                add_interaction(pins, block.b, pins->interaction(1));
            }

    // pins: seam beam into the outer rib ending on it
    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::shared_ptr<JointBeam>& pins = connectors[q].outer_rib_seam_beam[k];
            const Contact& joint = contacts[q].outer_rib_seam_beam[k];
            add(pins, connectors_group(q));
            add_interaction(pins, joint.a, pins->interaction(0));
            add_interaction(pins, joint.b, pins->interaction(1));
        }

    // pins: seam beam into the oculus beam
    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::shared_ptr<JointBeam>& pins = connectors[q].seam_beam_oculus_beam[k];
            const Contact& joint = contacts[q].seam_beam_oculus_beam[k];
            add(pins, connectors_group(q));
            add_interaction(pins, joint.a, pins->interaction(0));
            add_interaction(pins, joint.b, pins->interaction(1));
        }

    // pins: oculus beam into the inner rib
    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::shared_ptr<JointBeam>& pins = connectors[q].oculus_beam_inner_rib[k];
            const Contact& joint = contacts[q].oculus_beam_inner_rib[k];
            add(pins, connectors_group(q));
            add_interaction(pins, joint.a, pins->interaction(0));
            add_interaction(pins, joint.b, pins->interaction(1));
        }

    // pins: ring beam q into the next at their corner
    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<JointBeam>& pins = connectors[q].ring_corner;
        const Contact& joint = contacts[q].ring_corner;
        add(pins, group_named("connectors", group_named("oculus")));
        add_interaction(pins, joint.a, pins->interaction(0));
        add_interaction(pins, joint.b, pins->interaction(1));
    }

    // cross laps, last: the half lap merged into the outlines of the two column plates
    for (size_t q = 0; q < 4; q++) {
        const std::shared_ptr<JointPlate>& lap = connectors[q].cross_lap;
        const std::array<std::shared_ptr<Plate>, 2>& plates = connectors[q].column_plates;
        add(lap, connectors_group(q));
        add_interaction(lap, plates[0], lap->interaction(0));
        add_interaction(lap, plates[1], lap->interaction(1));
    }
}

}
