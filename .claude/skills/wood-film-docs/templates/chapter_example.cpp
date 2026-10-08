#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "00_vocabulary";
const std::array<double, 6> COLUMN_HEAD = {-3100.0, -3100.0, H - 1300.0, -2300.0, -2300.0, H + 60.0}; // Column 0's head and its cutters.
const std::array<double, 6> HEAD_FAN = {-3060.0, -3060.0, H - 260.0, -2700.0, -2700.0, H + 40.0}; // Column head 0 and its fan, close.
const std::array<double, 6> RIB_PLANES = {-2900.0, -3100.0, H - 200.0, -1600.0, -2800.0, H + 200.0}; // The middle of outer rib 0 in 3D.
const std::array<double, 6> SEAM_BEAMS = {-600.0, -3100.0, H - 450.0, 600.0, -900.0, H + 40.0}; // The two seam beams of seam 0 in 3D.
const std::array<double, 6> OCULUS_EDGE = {-1150.0, -1150.0, H - 350.0, 150.0, 150.0, H + 40.0}; // Oculus edge 0 in 3D.

/// The colour of a quarter family.
const Color& family_color(Family family) {
    return FAMILY_COLORS[static_cast<size_t>(family)];
}

/// Every loop pair of quarter q in grey but the families asked for.
void grey_quarter(Frame& frame, const FloorGuide& guide, size_t q, const std::set<Family>& except) {

    for (const Family family : FAMILIES)
        if (!except.count(family))
            for (const std::array<Polyline, 2>& member : outlines(guide, q, family))
                frame.solid(member, GREY);
}

/// The contact interaction of that name the floor stores between two of its members.
std::shared_ptr<InteractionContactFace> contact_named(const Floor& floor, const std::string& name) {

    for (const auto& [u, w] : floor.graph.get_edges()) {
        const std::shared_ptr<Element> a = floor.get_element<Element>(u);
        const std::shared_ptr<Element> b = floor.get_element<Element>(w);

        if (a && b)
            for (const std::shared_ptr<Interaction>& interaction : floor.get_interaction(a, b))
                if (interaction->name == name)
                    return std::dynamic_pointer_cast<InteractionContactFace>(interaction);
    }

    throw std::runtime_error("no contact " + name);
}

/// The plane at a point, so its drawn square sits there.
Plane at(const Plane& plane, const Point& point) {
    return plane.moved_to(plane.project(point));
}

/// A frame of quarter 0's members of some families in blue, the rest of the quarter grey, the families in hidden left out.
void members_of(const Context& context, size_t number, const std::string& slug, const std::string& caption, const std::set<Family>& families, const std::string& name, const std::array<double, 6>& box, const std::set<Family>& hidden = {}) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, number, slug, caption, "iso", box);
    frame.distance = 0.8;
    std::set<Family> skipped = families;
    skipped.insert(hidden.begin(), hidden.end());
    grey_quarter(frame, guide, 0, skipped);

    for (const Family family : families) {
        const std::vector<std::array<Polyline, 2>> list = outlines(guide, 0, family);

        for (const std::array<Polyline, 2>& member : list)
            frame.solid(member, BUILT);

        frame.label(FAMILY_NAMES[static_cast<size_t>(family)], up(middle(list[list.size() / 2])));
    }

    (void)name;
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// FloorGuide: the geometry
// ═══════════════════════════════════════════════════════════════════════════

void floor_guide(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 901, "floor_guide", "FloorGuide: four corners and the parameters; compute() makes every part below from them, quarter by quarter", "top", BAY);
    frame.polyline(up(Polyline({guide.corners[0], guide.corners[1], guide.corners[2], guide.corners[3]}).closed()), BUILT);
    frame.polyline(up(Polyline({guide.oculus_points[0], guide.oculus_points[1], guide.oculus_points[2], guide.oculus_points[3]}).closed()), BUILT);

    for (size_t q = 0; q < 4; q++)
        frame.line(up(Line::from_points(guide.midpoint(q), guide.centre)), INPUT, PEN, true);

    frame.point(up(guide.centre), BUILT);
    frame.label("corners[0]", up(guide.corners[0]));
    frame.label("corners[2]", up(guide.corners[2]));
    frame.label("centre", up(guide.centre));
    frame.label("oculus_points[1]", up(guide.oculus_points[1]));
    frame.label("size_oculus", up(Line::from_points(guide.centre, guide.oculus_points[3]).center()));
    frame.write(context.dir);
}

void quarter_polygon(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::vector<Point> polygon = guide.quarter_polygon(0);
    Frame frame(CHAPTER, 902, "quarter_polygon", "quarter_polygon: the quarter q at corners[q]; every FloorGuide method takes q, and the pictures show q = 0", "top", BAY);
    plan_context(frame, guide, true);
    frame.polyline(up(Polyline(polygon).closed()), BUILT);
    const std::array<std::string, 5> lines = {"line 0: bay edge", "line 1: seam", "line 2: oculus edge", "line 3: seam", "line 4: bay edge"};

    for (size_t i = 0; i < 5; i++)
        frame.label(lines[i], up(Line::from_points(polygon[i], polygon[(i + 1) % 5]).center()));

    for (size_t q = 0; q < 4; q++)
        frame.label(fmt::format("q = {}", q), up(Polyline(guide.quarter_polygon(q)).center()));

    frame.write(context.dir);
}

void column_polygon(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::vector<Point> head = guide.quarter_column_polygon(0);
    const Plane frame_plane = guide.column_frame(0);
    Frame frame(CHAPTER, 903, "quarter_column_polygon", "quarter_column_polygon: the column head at the quarter's corner in its column_frame; the ribs start from it", "top", HEAD_FAN);
    frame.polyline(up(Polyline(head).closed()), BUILT);
    frame.line(up(Line::from_points(frame_plane.origin(), frame_plane.origin() + frame_plane.x_axis() * 200.0)), INPUT, PEN, false, true);
    frame.line(up(Line::from_points(frame_plane.origin(), frame_plane.origin() + frame_plane.y_axis() * 200.0)), INPUT, PEN, false, true);
    frame.label("corner", up(head[0]));
    frame.label("size_column_head", up(Line::from_points(head[0], head[1]).center()));
    frame.label("size_column_head_chamfer", up(Line::from_points(head[1], head[2]).center()));
    frame.label("chamfer", up(Line::from_points(head[2], head[3]).center()));
    frame.write(context.dir);
}

void construction_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.construction_planes(0);
    const Point middle_of_rib = guide.construction_quads(0).outer_ribs[0].center();
    const std::array<double, 6> around = {middle_of_rib[0] - 500.0, middle_of_rib[1] - 400.0, H - 300.0, middle_of_rib[0] + 500.0, middle_of_rib[1] + 400.0, H + 300.0};
    Frame frame(CHAPTER, 904, "construction_planes", "construction_planes: a plane pair per member, its base face and the face offset by its size; here outer rib 0", "iso", around);
    frame.plane_size = 250.0;
    frame.face(up(guide.construction_quads(0).outer_ribs[0].closed()), GREY);
    frame.plane(up(at(cp.outer_ribs[0][0], middle_of_rib)), BUILT);
    frame.plane(up(at(cp.outer_ribs[0][1], middle_of_rib)), RESULT);
    frame.label("outer_ribs[0][0]: on line 0", up(cp.outer_ribs[0][0].project(middle_of_rib)));
    frame.label("outer_ribs[0][1]: offset by size_outer_ribs", up(cp.outer_ribs[0][1].project(middle_of_rib)));
    frame.write(context.dir);
}

void construction_quads(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionQuads& quads = guide.construction_quads(0);
    Frame frame(CHAPTER, 905, "construction_quads", "construction_quads: where each member's four planes meet the floor datum, its footprint in plan", "top", QUARTER);
    plan_context(frame, guide, true);

    const std::array<std::pair<Family, const std::vector<Polyline>*>, 5> families = {{
        {Family::outer_ribs, &quads.outer_ribs},
        {Family::inner_ribs, &quads.inner_ribs},
        {Family::inner_beams, &quads.inner_beams},
        {Family::wedges, &quads.wedges},
        {Family::tsections, &quads.tsections},
    }};

    for (const auto& [family, list] : families) {
        for (const Polyline& quad : *list)
            frame.face(up(quad.closed()), family_color(family));

        frame.label(FAMILY_NAMES[static_cast<size_t>(family)], up((*list)[0].center()));
    }

    frame.write(context.dir);
}

void boundary_parabolas(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::vector<std::array<Polyline, 3>>& parabolas = guide.boundary_parabolas(0);
    Frame frame(CHAPTER, 906, "boundary_parabolas", "boundary_parabolas: a parabola under each rib axis, from -height at the column to -static_h at the seam, with its two layers", "iso", QUARTER);
    plan_context(frame, guide, true);

    for (const Polyline& quad : guide.construction_quads(0).outer_ribs)
        frame.polyline(up(quad.closed()), INPUT);

    for (const std::array<Polyline, 3>& parabola : parabolas) {
        frame.polyline(up(parabola[0]), BUILT);
        frame.polyline(up(parabola[1]), RESULT);
        frame.polyline(up(parabola[2]), RESULT);
    }

    frame.label("outer 0", up(parabolas[0][0].get_point(parabolas[0][0].point_count() / 2)));
    frame.label("inner 0: projected", up(parabolas[2][0].get_point(parabolas[2][0].point_count() / 2)));
    frame.label("+t, +2t", up(parabolas[1][2].get_point(parabolas[1][2].point_count() / 2)));
    frame.write(context.dir);
}

void central_panel(const Context& context) {

    const FloorGuide& guide = context.guide;
    const CentralPanel& panel = guide.central_panel(0);
    Frame frame(CHAPTER, 907, "central_panel", "central_panel: between the two inner ribs one ruling crosses the panel and one sweep serves both ribs (rule A)", "iso", PANEL);

    for (const Polyline& quad : guide.construction_quads(0).inner_ribs)
        frame.polyline(up(quad.closed()), GREY);

    for (size_t k = 0; k < 2; k++) {
        frame.polyline(up(panel.traces[k][0]), BUILT);
        frame.polyline(up(panel.traces[k][1]), RESULT);
        frame.polyline(up(panel.traces[k][2]), RESULT);
    }

    const Point start = panel.traces[0][0].get_point(0);
    frame.line(up(Line::from_points(start, panel.traces[1][0].get_point(0))), VARIABLE, PEN, false, true);
    frame.label("traces: soffit", up(panel.traces[0][0].get_point(panel.traces[0][0].point_count() / 2)));
    frame.label("+t, +2t", up(panel.traces[1][2].get_point(panel.traces[1][2].point_count() / 2)));
    frame.label("ruling", up(Line::from_points(start, panel.traces[1][0].get_point(0)).center()));
    frame.write(context.dir);
}

void loops(const Context& context) {

    const std::array<Polyline, 2> rib = context.guide.outer_ribs(0)[0];
    Frame frame(CHAPTER, 908, "loops", "Two face loops: every member method returns a member as its two face loops; the Floor lofts the element between them", "iso", RIB);
    frame.distance = 0.85;
    frame.solid(rib, GREY);
    frame.polyline(up(rib[0]), BUILT);
    frame.polyline(up(rib[1]), RESULT);
    frame.label("[0]: top", up(rib[0].get_point(rib[0].point_count() / 2)));
    frame.label("[1]: bottom", up(rib[1].get_point(rib[1].point_count() / 2)));
    frame.write(context.dir);
}

void oculus(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::vector<std::array<Polyline, 2>> ring = guide.oculus();
    Frame frame(CHAPTER, 913, "oculus", "oculus(): four ring beams around the hole, one per oculus edge, meeting each quarter's oculus beam on the tilted plane", "iso", OCULUS);
    frame.distance = 0.6;

    for (size_t i = 4; i < ring.size(); i++)
        frame.solid(ring[i], GREY);

    for (size_t q = 0; q < 4; q++) {
        frame.solid(ring[q], BUILT);
        frame.solid(guide.inner_beams(q)[1], RESULT);
    }

    frame.label("ring beam oculus_2", up(middle(ring[2])));
    frame.label("oculus beam inner_beams[1]", up(middle(guide.inner_beams(0)[1])));
    frame.label("bottom wedges and central plate", up(middle(ring[8])));
    frame.write(context.dir);
}

void column_cutters(const Context& context) {

    const std::vector<std::array<Polyline, 2>> cutters = context.guide.column_cutters(0);
    Frame frame(CHAPTER, 914, "column_cutters", "column_cutters: six plates that carve the column head so the ribs and the column blocks sit on it", "iso", COLUMN_HEAD);
    frame.element(context.members.columns[0].column, GREY);

    for (const std::array<Polyline, 2>& plate : cutters)
        frame.solid(plate, BUILT);

    frame.label("column_cutters", up(middle(cutters[1])));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Floor: the model
// ═══════════════════════════════════════════════════════════════════════════

void elements(const Context& context) {

    const Floor& floor = context.members;
    Frame frame(CHAPTER, 915, "elements", "add_quarters, add_oculus, add_columns: the loops become elements, ribs and beams BeamVariable, t-sections, beds and blocks Plate", "iso", FLOOR);
    frame.distance = 0.75;

    for (size_t q = 0; q < 4; q++) {
        for (const Family family : {Family::outer_ribs, Family::inner_ribs, Family::inner_beams})
            for (const std::shared_ptr<Element>& member : members(floor.quarters[q], family))
                frame.element(member, BUILT);

        for (const Family family : {Family::wedges, Family::tsections, Family::beds})
            for (const std::shared_ptr<Element>& member : members(floor.quarters[q], family))
                frame.element(member, RESULT);
    }

    for (const std::shared_ptr<BeamVariable>& beam : floor.ring)
        frame.element(beam, BUILT);

    for (const std::shared_ptr<Plate>& plate : floor.oculus_plates)
        frame.element(plate, RESULT);

    for (const ColumnModel& model : floor.columns) {
        frame.element(model.column, STEEL);
        frame.element(model.support, STEEL);
    }

    frame.label("BeamVariable outer_ribs_0_0", up(middle(context.guide.outer_ribs(0)[0])));
    frame.label("Plate beds_1_2_0", up(middle(context.guide.beds(0)[1][2])));
    frame.label("Column column_2", floor.columns[2].column->axis.center());
    frame.write(context.dir);
}

void contacts(const Context& context) {

    const Floor& floor = context.members;
    Frame frame(CHAPTER, 916, "contacts", "add_contacts: one contact interaction on the session's edge between every two members that touch, named by its kind", "iso", QUARTER);
    frame.distance = 0.8;

    for (const Family family : FAMILIES)
        if (family != Family::beds)
            for (const std::array<Polyline, 2>& member : outlines(context.guide, 0, family)) {
                frame.polyline(up(member[0]), GREY);
                frame.polyline(up(member[1]), GREY);
            }

    for (const std::string& name : {std::string("seam_wedge_0"), std::string("oculus_wedge_0"), std::string("column_plate_0_0"), std::string("column_plate_0_1"), std::string("block_dowels_0_0_0"), std::string("block_dowels_0_1_0"), std::string("block_dowels_0_2_1")})
        frame.face(contact_named(floor, name)->polygon, BUILT);

    frame.label("column_plate_0_0", contact_named(floor, "column_plate_0_0")->polygon.center());
    frame.label("block_dowels_0_1_0", contact_named(floor, "block_dowels_0_1_0")->polygon.center());
    frame.write(context.dir);
}

void contact(const Context& context) {

    const Floor& floor = context.members;
    const std::shared_ptr<InteractionContactFace> face = contact_named(context.members, "seam_wedge_0");
    const std::array<Polyline, 2> b = context.guide.inner_beams(1)[2];
    Frame frame(CHAPTER, 917, "contact", "add_interaction(a, b, contact): an InteractionContactFace named seam_wedge_0, its polygon where the two seam beams touch", "iso", SEAM_BEAMS);
    frame.element(floor.quarters[0].inner_beams[0], RESULT);
    frame.polyline(up(b[0]), VARIABLE);
    frame.polyline(up(b[1]), VARIABLE);
    frame.face(face->polygon, BUILT);
    frame.label(face->name, face->polygon.center());
    frame.label("a: inner_beams_0_0", up(context.guide.inner_beams(0)[0][0].get_point(2)));
    frame.label("b: inner_beams_2_1", up(b[0].get_point(1)));
    frame.write(context.dir);
}

void connectors(const Context& context) {

    const Floor& floor = context.connected;
    Frame frame(CHAPTER, 918, "connectors", "add_connectors: each contact interaction gets the connector of its kind: wedges, column plates with cross laps, dowels", "iso", QUARTER);
    frame.distance = 0.8;
    quarter_members(frame, floor, 0, GREY);

    // the parts of quarter 0's connectors: those in its quarter of the plan
    for (const std::shared_ptr<ConnectorPart>& part : floor.get_elements<ConnectorPart>()) {
        const std::vector<Point> points = part->element_geometry_mesh().to_vertices_and_faces().first;

        if (!points.empty() && Point::centroid(points)[0] < 0.0 && Point::centroid(points)[1] < 0.0)
            frame.element(part, BUILT);
    }

    frame.write(context.dir);
}

void screws(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor floor(guide);
    Frame frame(CHAPTER, 919, "screws", "add_screws: rib_beam_screws, beam_mitre_screws and rib_corner_screws find the 200 mm screw lines between members that butt; JointBeam::screws pre-drills them", "iso", QUARTER);
    frame.distance = 0.8;

    for (const Family family : {Family::outer_ribs, Family::inner_ribs, Family::inner_beams, Family::wedges})
        for (const std::array<Polyline, 2>& member : outlines(guide, 0, family)) {
            frame.polyline(up(member[0]), GREY);
            frame.polyline(up(member[1]), GREY);
        }

    for (size_t k = 0; k < 2; k++)
        for (const std::vector<Line>& screw_lines : {floor.rib_beam_screws(0, k), floor.beam_mitre_screws(0, k), floor.rib_corner_screws(0, k)})
            for (const Line& screw : screw_lines)
                frame.line(screw, BUILT);

    frame.write(context.dir);
}

void floor_model(const Context& context) {

    const Floor& floor = context.connected;
    Frame frame(CHAPTER, 921, "floor", "Floor: the model, every member placed at bay_height with its contacts, connectors and screws", "iso", FLOOR);
    frame.distance = 0.75;

    for (size_t q = 0; q < 4; q++)
        for (const Family family : FAMILIES)
            for (const std::shared_ptr<Element>& member : members(floor.quarters[q], family))
                frame.element(member, family_color(family));

    for (const std::shared_ptr<BeamVariable>& beam : floor.ring)
        frame.element(beam, RING);

    for (const ColumnModel& model : floor.columns) {
        frame.element(model.column, STEEL);
        frame.element(model.support, STEEL);
    }

    for (const std::shared_ptr<ConnectorPart>& part : floor.get_elements<ConnectorPart>())
        frame.element(part, BUILT);

    frame.write(context.dir);
}

}

void chapter_00_vocabulary(const Context& context) {

    floor_guide(context);
    quarter_polygon(context);
    column_polygon(context);
    construction_planes(context);
    construction_quads(context);
    boundary_parabolas(context);
    central_panel(context);
    loops(context);
    members_of(context, 909, "ribs", "outer_ribs, inner_ribs: each rib's parabola trimmed by the planes it ends on, on both of its faces", {Family::outer_ribs, Family::inner_ribs}, "ribs", QUARTER);
    members_of(context, 910, "tsections", "tsections: flange strips beside the rib faces; the beds rest on them", {Family::tsections}, "tsections", QUARTER, {Family::beds, Family::wedges});
    members_of(context, 911, "beds", "beds: three rows of bed plates between the ribs, each row trimmed alike so every plate stays a quad", {Family::beds}, "beds", QUARTER);
    members_of(context, 912, "wedges_and_beams", "wedges, inner_beams: the three column blocks at the head, and the three beams on the seams and the oculus edge", {Family::wedges, Family::inner_beams}, "", QUARTER);
    oculus(context);
    column_cutters(context);
    elements(context);
    contacts(context);
    contact(context);
    connectors(context);
    screws(context);
    floor_model(context);
}

}
