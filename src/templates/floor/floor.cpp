#include "pch.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Column
// ═══════════════════════════════════════════════════════════════════════════

WoodSession column(const FloorGuide& guide, size_t q) {

    const size_t k = q % 4;
    const std::string name = fmt::format("column_{}", k);

    const std::shared_ptr<Support> support = std::make_shared<Support>(guide.support_plane(k), "support");
    support->name = fmt::format("support_{}", k);
    const Line axis = support->column_axis(guide.bay_height);
    const Plane frame = guide.column_frame(k);
    const std::shared_ptr<Column> shaft = Column::square(
        axis,
        frame,
        guide.size_column_head,
        name
    );

    const std::array<std::array<Polyline, 2>, 6>& loops = guide.column_cutters(k);
    std::vector<std::shared_ptr<Plate>> cutters;

    for (size_t i = 0; i < loops.size(); i++) {
        cutters.push_back(std::make_shared<Plate>(loops[i][1], loops[i][0], fmt::format("column_cutters_{}_{}", i, k)));
        cutters.back()->place(Xform::translation(0.0, 0.0, guide.bay_height));
    }

    // the head glued on as wide as the chamfer reaches, as deep as the carved head
    const double head_width = guide.size_column_head + guide.size_column_head_chamfer;
    WoodSession session(name);
    session.add_column(
        shaft,
        head_width,
        guide.column_head_depth,
        support,
        cutters
    );
    return session;
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

Floor::Floor(const FloorGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // quarters: every quarter's members, lifted to bay_height and grouped by family
    add_quarters();

    // oculus: the four ring beams, the oculus beams, the bottom wedges and the central plate
    add_oculus();

    // columns: the column at every corner, its head carved by the guide's cutters
    add_columns();

    // contacts: per quarter an interaction between every two members that touch, named by its kind and place
    const std::array<QuarterContacts, 4> contacts = add_contacts();

    // connectors: per quarter its wedges, column plates with their cross lap and dowels, all built on uncut members
    const std::array<QuarterConnectors, 4> connectors = compute_connectors(contacts);
    add_connectors(connectors);

    // screws: the assembly screws, after every other connector so nothing before them changes
    const std::array<QuarterScrews, 4> screws = compute_screws();
    add_screws(screws);
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

        // inner_beams: BeamVariable, the two seam beams, inner_beams_<i>_<q>; the guide's oculus edge beam between them goes with the oculus
        const std::shared_ptr<TreeNode> beams = add_group(fmt::format("inner_beams_{}", q), group);
        const std::array<std::array<Polyline, 2>, 3>& beam_loops = guide.inner_beams(q);
        const std::array<size_t, 2> seams = {0, 2};

        for (size_t i = 0; i < seams.size(); i++) {
            const std::shared_ptr<BeamVariable> inner_beam = beam(
                beam_loops[seams[i]],
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
    const std::shared_ptr<TreeNode> group = oculus_group();

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

    // oculus_beams: BeamVariable, the inner beam along oculus edge q, oculus_beam_<q>
    const std::shared_ptr<TreeNode> oculus_beams = add_group("oculus_beams", group);

    for (size_t q = 0; q < 4; q++) {
        const std::array<Polyline, 2>& loops = guide.inner_beams(q)[1];
        const std::shared_ptr<BeamVariable> oculus_beam = beam(
            loops,
            {0, 3},
            {1, 2},
            fmt::format("oculus_beam_{}", q)
        );
        oculus_beam->place(lift);
        add(oculus_beam, oculus_beams);
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

    const size_t k = corner % 4;
    const std::shared_ptr<TreeNode> group = group_named(fmt::format("column_{}", k), quarter_group(k));
    graft(column(guide, k), group);
}

std::shared_ptr<TreeNode> Floor::quarter_group(size_t q) {
    return group_named(fmt::format("quarter_{}", q));
}

std::shared_ptr<TreeNode> Floor::oculus_group() {
    return group_named("oculus");
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

        // seam: this quarter's seam beam 0 beside the next quarter's seam beam 1
        const Contact seam = add_contact(fmt::format("seam_wedge_{}", q), fmt::format("inner_beams_0_{}", q), fmt::format("inner_beams_1_{}", next));
        contacts[q].seam_wedge = seam;

        // oculus: the oculus beam's back face on its ring beam
        const Contact oculus = add_contact(fmt::format("oculus_wedge_{}", q), fmt::format("oculus_beam_{}", q), fmt::format("oculus_{}", q));
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
                const Contact dowels = add_contact(fmt::format("block_dowels_{}_{}_{}", q, b, side), ribs[b + side], fmt::format("wedges_{}_{}", b, q));
                contacts[q].block_dowels[b][side] = dowels;
            }
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
        const QuarterContacts& c = contacts[q];
        QuarterConnectors& made = connectors[q];

        // seam wedge: sized by the inner beams, running on to the bay's outer face
        const Contact& seam = c.seam_wedge;
        const double beam = guide.size_inner_beams;
        const Plane outer_face = guide.construction_planes(q).outer_ribs[0][0].transformed(lift);
        made.seam_wedge = JointBeam::wedge(
            *seam.a,
            *seam.b,
            *seam.face,
            1.5 * beam,
            2.0 * beam / 3.0,
            outer_face
        );

        // oculus wedge: sized by the thicker of the oculus beam and its ring beam
        const Contact& oculus = c.oculus_wedge;
        const double oculus_beam = FloorGuide::thickness(guide.inner_beams(q)[1]);
        const double ring_beam = FloorGuide::thickness(guide.oculus()[q]);
        const double thicker = std::max(oculus_beam, ring_beam);
        made.oculus_wedge = JointBeam::wedge(
            *oculus.a,
            *oculus.b,
            *oculus.face,
            1.5 * thicker,
            2.0 * thicker / 3.0
        );

        // column plates: a rectangle plate as wide as the outer rib on each, and the cross lap where the two cross
        for (size_t k = 0; k < 2; k++) {
            const Contact& plate = c.column_plates[k];
            made.column_plates[k] = JointBeam::rectangle_plate(
                *plate.a,
                *plate.b,
                *plate.face,
                guide.size_outer_ribs
            );
        }

        made.cross_lap = JointBeam::cross_lap(*made.column_plates[0], *made.column_plates[1]);

        // block dowels: dowels between each column block and the rib either side
        for (size_t b = 0; b < 3; b++)
            for (size_t side = 0; side < 2; side++) {
                const Contact& contact = c.block_dowels[b][side];
                const std::shared_ptr<JointBeam> dowels = JointBeam::dowels(*contact.a, *contact.b, *contact.face);

                if (!dowels)
                    throw std::runtime_error("the inset leaves no room for the dowels of " + contact.face->name);

                made.block_dowels[b][side] = dowels;
            }
    }

    return connectors;
}

void Floor::add_connectors(const std::array<QuarterConnectors, 4>& connectors) {

    // seam wedges
    for (size_t q = 0; q < 4; q++)
        add_named_connector(connectors[q].seam_wedge, "connector_seam_wedge", connectors_group(q));

    // oculus wedges
    for (size_t q = 0; q < 4; q++)
        add_named_connector(connectors[q].oculus_wedge, "connector_oculus_wedge", group_named("connectors", oculus_group()));

    // column plates
    for (size_t q = 0; q < 4; q++)
        for (const std::shared_ptr<JointBeam>& plate : connectors[q].column_plates)
            add_named_connector(plate, "connector_column_plate", connectors_group(q));

    // block dowels
    for (size_t q = 0; q < 4; q++)
        for (const std::array<std::shared_ptr<JointBeam>, 2>& block : connectors[q].block_dowels)
            for (const std::shared_ptr<JointBeam>& dowels : block)
                add_named_connector(dowels, "connector_block_dowels", connectors_group(q));

    // cross laps, last, over the plates they join
    for (size_t q = 0; q < 4; q++)
        add_named_connector(connectors[q].cross_lap, "connector_cross_lap", connectors_group(q));
}

void Floor::add_named_connector(const std::shared_ptr<JointBeam>& connector, const std::string& prefix, const std::shared_ptr<TreeNode>& group) {

    connector->name = fmt::format("{}_{}", prefix, next_number(prefix));
    set_node_color(add_connector(connector, group), CONNECTOR_COLOR, true);
}

// ═══════════════════════════════════════════════════════════════════════════
// Screws
// ═══════════════════════════════════════════════════════════════════════════

std::array<QuarterScrews, 4> Floor::compute_screws() const {

    std::array<QuarterScrews, 4> screws;

    for (size_t q = 0; q < 4; q++) {
        // the members the screws join, by name: the seam beams either side, the oculus beam between them, the outer and inner ribs
        const auto member = [this, q](const std::string& family, size_t i) {
            return get_element_by_name<Element>(fmt::format("{}_{}_{}", family, i, q)).get();
        };
        const Element* oculus_beam = get_element_by_name<Element>(fmt::format("oculus_beam_{}", q)).get();

        for (size_t k = 0; k < 2; k++) {
            // outer_rib_seam_beam: the outer rib into the seam beam it ends on
            screws[q].outer_rib_seam_beam[k] = screws_of({member("outer_ribs", k), member("inner_beams", k)}, outer_rib_seam_beam_screws(q, k));

            // seam_beam_oculus_beam: the seam beam into the oculus beam ending on it
            screws[q].seam_beam_oculus_beam[k] = screws_of({member("inner_beams", k), oculus_beam}, seam_beam_oculus_beam_screws(q, k));

            // oculus_beam_inner_rib: the oculus beam into the inner rib ending on its back face, through the seam beam too when the screws pass it
            const std::vector<Line> lines = oculus_beam_inner_rib_screws(q, k);
            std::vector<const Element*> passed = {oculus_beam, member("inner_ribs", k)};

            if (passes_seam_beam(q, k, lines))
                passed.push_back(member("inner_beams", k));

            screws[q].oculus_beam_inner_rib[k] = screws_of(passed, lines);
        }
    }

    return screws;
}

void Floor::add_screws(const std::array<QuarterScrews, 4>& screws) {

    for (size_t q = 0; q < 4; q++)
        for (const std::array<std::shared_ptr<JointBeam>, 2>& pair : {screws[q].outer_rib_seam_beam, screws[q].seam_beam_oculus_beam, screws[q].oculus_beam_inner_rib})
            for (const std::shared_ptr<JointBeam>& screw : pair)
                add_named_connector(screw, "connector_screws", connectors_group(q));
}

std::shared_ptr<JointBeam> Floor::screws_of(const std::vector<const Element*>& members, const std::vector<Line>& lines) const {

    return JointBeam::screws(
        members,
        lines,
        2.0,
        SCREW_LENGTH
    );
}

std::vector<Line> Floor::outer_rib_seam_beam_screws(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.construction_planes(q);
    const Plane& seam_face = cp.inner_beams[SEAM_BEAMS[k]][0];
    const std::array<Polyline, 2>& rib = guide.outer_ribs(q)[k];
    const Point body = FloorGuide::body(rib);
    // the two ribs of a seam on opposite sides of their axes, so the heads on the seam plane stay apart
    const double offset = k == 0 ? -SEAM_SCREW_OFFSET : SEAM_SCREW_OFFSET;
    const double bottom = FloorGuide::end_level(rib, guide.rib_seam_ends(q)[k]);

    return {
        screw(
            cp.outer_ribs[k],
            seam_face,
            body,
            -RIB_END_MARGIN,
            offset
        ),
        screw(
            cp.outer_ribs[k],
            seam_face,
            body,
            bottom + RIB_END_MARGIN,
            offset
        ),
    };
}

std::vector<Line> Floor::seam_beam_oculus_beam_screws(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.construction_planes(q);
    const Plane& seam_plane = cp.inner_beams[SEAM_BEAMS[k]][0];
    const Point body = FloorGuide::body(guide.inner_beams(q)[1]);

    return {
        screw(
            cp.inner_beams[1],
            seam_plane,
            body,
            corner_level(SEAM_BEAM_OCULUS_BEAM_LEVELS[k][0])
        ),
        screw(
            cp.inner_beams[1],
            seam_plane,
            body,
            corner_level(SEAM_BEAM_OCULUS_BEAM_LEVELS[k][1])
        ),
    };
}

std::vector<Line> Floor::oculus_beam_inner_rib_screws(size_t q, size_t k) const {

    const ConstructionPlanes& cp = guide.construction_planes(q);
    const Plane& back_face = cp.inner_beams[1][0];
    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const Plane seam_plane = cp.inner_beams[SEAM_BEAMS[k]][0].transformed(lift);
    const Point body = FloorGuide::body(guide.inner_ribs(q)[k]);
    std::vector<Line> screws;

    for (double levels : OCULUS_BEAM_INNER_RIB_LEVELS) {
        screws.push_back(
            screw(
                cp.inner_ribs[k],
                back_face,
                body,
                corner_level(levels)
            )
        );

        // the next quarter's screws meet the seam plane from the other side: a head closer than half the spacing would touch them
        const double from_seam = seam_plane.signed_distance(screws.back().start());

        if (from_seam < 0.5 * SCREW_SPACING)
            throw std::runtime_error(fmt::format("quarter {}'s inner rib {} screw starts {:.3f} mm from the seam plane, less than half the screw spacing, where the next quarter's meets it: the bay is too narrow for the corner screws", q, k, from_seam));
    }

    return screws;
}

bool Floor::passes_seam_beam(size_t q, size_t k, const std::vector<Line>& screws) const {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const ConstructionPlanes& cp = guide.construction_planes(q);
    const Plane beam_end = cp.inner_beams[SEAM_BEAMS[k]][1].transformed(lift);
    const Point oculus_beam = FloorGuide::body(guide.inner_beams(q)[1]).transformed(lift);
    const double oculus_side = beam_end.signed_distance(oculus_beam) < 0.0 ? -1.0 : 1.0;

    for (const Line& screw : screws)
        if (oculus_side * beam_end.signed_distance(screw.start()) < 0.0)
            return true;

    return false;
}

Line Floor::screw(
    const std::array<Plane, 2>& member,
    const Plane& from,
    const Point& toward,
    double z,
    double offset
) const {

    const Line line = axis(member, z) + member[0].z_axis() * offset;
    const Point head = Intersection::line_plane(line, from, false).value();
    Vector along = line.to_direction().normalized();

    if (along.dot(toward - head) < 0.0)
        along = -along;

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const Line at_datum = Line::from_points(head, head + along * SCREW_LENGTH);

    return at_datum.transformed(lift);
}

Line Floor::axis(const std::array<Plane, 2>& faces, double z) {

    const Line line0 = Intersection::plane_plane(Plane::xy_plane_at(z), faces[0]).value();
    const Line line1 = Intersection::plane_plane(Plane::xy_plane_at(z), faces[1]).value();
    const Point p0 = line0.start();
    const Point p1 = line1.closest_point(p0, false).second;
    const Point middle = Point::mid_point(p0, p1);

    return Line::from_points(middle, middle + line0.to_direction());
}

double Floor::corner_level(double levels) const {
    return -guide.static_h() * levels / CORNER_LEVELS;
}

}
