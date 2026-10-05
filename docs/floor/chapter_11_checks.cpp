#include "docs/floor/movie.h"
#include "convex_hull.h"
#include "wood_brep_drill.h"
#include "wood_element_geometry.h"

namespace movie {

namespace {

const std::string CHAPTER = "11_checks";
const double ISO_YAW = -M_PI / 6.0; // The viewer's iso view turns this far about z from looking along +y.
const double ISO_PITCH = -M_PI / 6.0; // And looks down this far.
const double ORBIT_RADIANS = 0.005; // One orbit mouse pixel in the viewer.
const double BLOCK_SHIFT = 520.0; // How far the faceted copy of the drilled block stands beside its BRep, mm.

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The orbit from the iso view, in mouse pixels, that turns the camera to look along a direction.
std::string orbit_along(const Vector& look) {

    const Vector unit = look.normalized();
    double yaw = std::atan2(-unit[0], unit[1]) - ISO_YAW;

    if (yaw > M_PI)
        yaw -= 2.0 * M_PI;
    if (yaw < -M_PI)
        yaw += 2.0 * M_PI;

    const double pitch = std::asin(unit[2]) - ISO_PITCH;

    return fmt::format("{:.0f},{:.0f}", -yaw / ORBIT_RADIANS, -pitch / ORBIT_RADIANS);
}

/// The box around the points, every side margin away.
Box around(const std::vector<Point>& points, double margin) {

    Box box = {1e300, 1e300, 1e300, -1e300, -1e300, -1e300};

    for (const Point& point : points)
        for (int i = 0; i < 3; i++) {
            box[static_cast<size_t>(i)] = std::min(box[static_cast<size_t>(i)], point[i] - margin);
            box[static_cast<size_t>(i) + 3] = std::max(box[static_cast<size_t>(i) + 3], point[i] + margin);
        }

    return box;
}

/// The plane moved along itself to a point on it, so the viewer draws its square there.
Plane re_origin(const Plane& plane, const Point& point) {
    return Plane::from_frame(point, plane.x_axis(), plane.y_axis(), plane.z_axis());
}

/// Whether a joint cut the element, as check_breps tells a cut member: not a joint, and its model mesh differs from the uncut one.
bool is_cut_by_joint(const std::shared_ptr<Element>& element) {
    return !std::dynamic_pointer_cast<Joint>(element) && element->model_geometry_mesh().number_of_vertices() != element->element_geometry_mesh().number_of_vertices();
}

/// The exact bores check_breps finds in a member, 0 for a member no joint cut.
size_t bores_of(const std::shared_ptr<Element>& element) {
    return is_cut_by_joint(element) ? count_bores(element->model_geometry_brep()) : 0;
}

/// The footprint ring_overlap reads for a ring beam: the convex hull of both loops flattened to z 0.
Polyline footprint(const Outline& beam) {

    std::vector<Point> points = open_points(beam.top);
    const std::vector<Point> bottom = open_points(beam.bottom);
    points.insert(points.end(), bottom.begin(), bottom.end());

    for (Point& point : points)
        point[2] = 0.0;

    return Polyline(ConvexHull::hull_2d(points)).closed();
}

/// The corner of a flange's top loop nearest a bed corner but not on it: the soffit corner under it.
Point soffit_under(const Outline& flange, const Point& corner) {

    Point nearest = corner;
    double best = 1e300;

    for (const Point& point : open_points(flange.top)) {
        const double distance = (point - corner).magnitude();
        if (distance > 1.0 && distance < best) {
            best = distance;
            nearest = point;
        }
    }

    return nearest;
}

/// The connector of the floor that drills the block, preferring the one that also drills the rib.
std::shared_ptr<JointBeam> drilling(const Floor& floor, const std::shared_ptr<Element>& block, const std::shared_ptr<Element>& rib) {

    std::shared_ptr<JointBeam> found;

    for (const std::shared_ptr<JointBeam>& connector : floor.connectors) {
        const std::vector<std::string>& targets = connector->targets;
        const bool on_block = std::find(targets.begin(), targets.end(), block->guid()) != targets.end();
        const bool on_rib = std::find(targets.begin(), targets.end(), rib->guid()) != targets.end();

        if (!on_block || connector->drill_axes().empty())
            continue;
        if (on_rib)
            return connector;
        if (!found)
            found = connector;
    }

    if (!found)
        throw std::runtime_error("no connector drills " + block->name);

    return found;
}

/// The stretches bore_stretches counts for one drill axis in one target: inside its solid, cut by its pockets without drills, and overlapping the axis.
std::vector<std::array<double, 2>> counted_stretches(const std::shared_ptr<Element>& target, const Line& axis) {

    const std::vector<SolidCut>* cuts = solid_cuts_of(*target);
    const Mesh solid = cuts ? apply_solid_cuts(target->element_geometry_mesh(), *cuts, false) : target->element_geometry_mesh();
    std::vector<std::array<double, 2>> counted;

    for (const std::array<double, 2>& stretch : inside_stretches(solid, axis))
        if (stretch[1] > 1e-6 && stretch[0] < axis.length() - 1e-6)
            counted.push_back(stretch);

    return counted;
}

// ═══════════════════════════════════════════════════════════════════════════
// The guide's report
// ═══════════════════════════════════════════════════════════════════════════

/// 234: the oculus ring check() builds, ring[0..3] in plan, each beam between its tilted and its ring_inner plane in a pinwheel.
void ring_loops(const Context& context, const std::vector<Outline>& ring) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 234, "ring", fmt::format("check() calls oculus(): ring[0..3], each from its tilted to its ring_inner plane, datum to soffit {:.1f}", guide.soffit), "top", {-1150.0, -1150.0, H - 250.0, 1150.0, 1150.0, H + 20.0});

    for (const OculusEdge& edge : guide.oculus_edges)
        frame.line(up(edge.line), GREY, 2.0);

    for (size_t i = 0; i < 4; i++) {
        frame.polyline(up(ring[i].top), RING, 3.0);
        frame.polyline(up(ring[i].bottom), RING, 3.0);
        frame.label(fmt::format("ring[{}]", i), up(middle(ring[i])));
    }

    frame.label("ring[0].top on oculus_edges[0].tilted", up(ring[0].top.get_point(1)));
    frame.label("ring[0].bottom on oculus_edges[0].ring_inner", up(ring[0].bottom.get_point(2)));
    frame.write(context.dir);
}

/// 235: seam 0 seen from above, quarter 0's beam 0 and quarter 1's beam 2 back to back on the seam plane, and the oculus corner both polygons name.
void seam_identities(const Context& context, const FloorReport& report) {

    const FloorGuide& guide = context.guide;
    const Outline beam0 = guide.quarter(0).inner_beams()[0];
    const Outline beam2 = guide.quarter(1).inner_beams()[2];
    const Point& corner = guide.geometry[0].polygon[2];
    Frame frame(CHAPTER, 235, "seam_identities", fmt::format("seam_plane_gap[0] = {:.1e} and oculus_corner_gap[0] = {:.1e}: quarters 0 and 1 meet on seam 0", report.seam_plane_gap[0], report.oculus_corner_gap[0]), "top", {-450.0, -3080.0, H - 250.0, 450.0, -880.0, H + 20.0});
    frame.key = true;

    for (size_t q = 0; q < 2; q++)
        frame.polyline(up(Polyline(guide.geometry[q].polygon).closed()), GREY, 1.0);

    for (const Outline* beam : {&beam0, &beam2}) {
        frame.polyline(up(beam->top), FAMILY_COLORS[2], 3.0);
        frame.polyline(up(beam->bottom), FAMILY_COLORS[2], 3.0);
    }

    frame.line(up(Line::from_points(guide.seams[0].line.start(), guide.seams[0].oculus_corner)), INK, 2.0, true);

    for (const Point& point : open_points(beam2.bottom))
        frame.point(up(point), MARK);

    frame.point(up(corner), MARK, 24.0);
    frame.point(up(corner), INK, 10.0);
    frame.label(fmt::format("seam_plane_gap[0] = {:.1e}", report.seam_plane_gap[0]), up(beam2.bottom.get_point(0)));
    frame.label("q0.polygon[2] = q1.polygon[3]", up(corner));
    frame.label("quarter(0).inner_beams()[0]", up(middle(beam0)));
    frame.label("quarter(1).inner_beams()[2]", up(middle(beam2)));
    frame.label("planes.inner_beams[0][0]: the seam plane", up(guide.seams[0].line.start() + (corner - guide.seams[0].line.start()) * 0.75));
    frame.write(context.dir);
}

/// 236: outer rib 0 of quarter 0 with its column end on the fan plane and its seam end on the seam beam's far face, the eight corners measured.
void end_faces(const Context& context, const FloorReport& report) {

    const Quarter quarter = context.guide.quarter(0);
    const ConstructionPlanes& cp = quarter.geometry().planes;
    const Outline rib = quarter.outer_ribs()[0];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const size_t n = top.size();
    const std::vector<Point> column_end = {top[1], top[2], bottom[2], bottom[1]};
    const std::vector<Point> seam_end = {top[0], top[n - 2], bottom[n - 2], bottom[0]};
    Frame frame(CHAPTER, 236, "end_faces", fmt::format("end_face_planarity_mm[0] = {:.1e}: outer rib 0's eight end corners against its two end planes", report.end_face_planarity_mm[0]), "iso", RIB);
    frame.plane_size = 300.0;
    frame.distance = 0.9;

    frame.polyline(up(rib.top), GREY, 2.0);
    frame.polyline(up(rib.bottom), GREY, 2.0);
    frame.plane(up(re_origin(cp.wedges[0][0], Point::centroid(column_end))), FAMILY_COLORS[3]);
    frame.plane(up(re_origin(quarter.rib_seam_ends()[0], Point::centroid(seam_end))), FAMILY_COLORS[2]);

    for (const std::vector<Point>* end : {&column_end, &seam_end})
        for (const Point& point : *end)
            frame.point(up(point), MARK);

    frame.label("top[1]", up(top[1]));
    frame.label("top[2]", up(top[2]));
    frame.label("top[0]", up(top[0]));
    frame.label(fmt::format("top[{}]", n - 2), up(top[n - 2]));
    frame.label("cp.wedges[0][0]", up(Point::centroid(column_end)));
    frame.label("rib_seam_ends()[0] = cp.inner_beams[0][1]", up(Point::centroid(seam_end)));
    frame.write(context.dir);
}

/// 237: one bed of row 0 seen along the rib, its four underside corners on the top loops of the two flanges beside it.
void beds_on_flanges(const Context& context, const FloorReport& report) {

    const Quarter quarter = context.guide.quarter(0);
    const std::vector<Outline> row = quarter.beds()[0];
    const std::vector<Outline> flanges = quarter.tsections();
    const QuarterMembers& placed = context.members.members.quarters[0];
    const size_t index = row.size() / 2;
    const std::vector<Point> under = row[index].bottom.get_points();
    const Point centre = up(middle(row[index]));
    const Vector along = context.guide.edges[0].line.to_vector().normalized();
    Frame frame(CHAPTER, 237, "beds_on_flanges", fmt::format("bed_flange_coincidence_mm[0] = {:.1e}: under[0..3] of a row 0 bed on the flanges tsections[1] and [0]", report.bed_flange_coincidence_mm[0]), "iso", around({centre}, 330.0));
    frame.orbit = orbit_along(Vector(along[0], along[1], -0.45));

    for (size_t i = 0; i < row.size(); i++)
        frame.element(placed.beds[0][i].element, i == index ? FAMILY_COLORS[5] : GREY);

    frame.element(placed.tsections[0].element, FAMILY_COLORS[4]);
    frame.element(placed.tsections[1].element, FAMILY_COLORS[4]);

    for (size_t k = 0; k < 4; k++) {
        frame.point(up(under[k]), MARK);
        frame.label(fmt::format("under[{}]", k), up(under[k]));
    }

    frame.label(placed.tsections[1].element->name, up(soffit_under(flanges[1], under[0])));
    frame.label(placed.tsections[0].element->name, up(soffit_under(flanges[0], under[3])));
    frame.label(placed.beds[0][index].element->name, up(area_centroid(row[index].top)));
    frame.write(context.dir);
}

/// 238: the tilted plane of oculus edge 0 seen face on, ring beam 0's face behind quarter 0's oculus beam face, and what of it the ring leaves uncovered.
void ring_cover(const Context& context, const FloorReport& report, const std::vector<Outline>& ring) {

    const FloorGuide& guide = context.guide;
    const OculusEdge& edge = guide.oculus_edges[0];
    const Polyline face = guide.quarter(0).inner_beams()[1].bottom;
    const std::vector<Polyline> uncovered = Polyline::boolean_op(face, ring[0].top, edge.tilted, 2);
    Vector look = edge.tilted.z_axis();

    if (look.dot(guide.centre - edge.line.center()) < 0.0)
        look = -look;

    std::vector<Point> corners;
    for (const Point& point : open_points(ring[0].top))
        corners.push_back(up(point));

    Frame frame(CHAPTER, 238, "ring_uncovered", fmt::format("ring_uncovered_mm2 = {:.1e}: quarter 0's oculus beam face inside ring[0].top on the tilted plane", report.ring_uncovered_mm2), "iso", around(corners, 120.0));
    frame.orbit = orbit_along(look);
    frame.distance = 0.7;
    frame.polyline(up(ring[0].top), GREY, 5.0);
    frame.polyline(up(face), FAMILY_COLORS[2], 3.0);

    for (const Polyline& piece : uncovered)
        frame.polyline(up(piece), MARK, 4.0);

    frame.label("ring[0].top", up(ring[0].top.get_point(3)));
    frame.label("quarter(0).inner_beams()[1].bottom", up(face.get_point(0)));
    frame.label(fmt::format("{} uncovered pieces", uncovered.size()), up(edge.line.center()));
    frame.write(context.dir);
}

/// 239: the four ring footprints ring_overlap intersects pair by pair, touching end to end without overlap.
void ring_footprints(const Context& context, const FloorReport& report, const std::vector<Outline>& ring) {

    Frame frame(CHAPTER, 239, "ring_overlap", fmt::format("ring_overlap_mm2 = {:.1e}: the convex hulls of the four ring beams in plan, pair by pair", report.ring_overlap_mm2), "top", {-1150.0, -1150.0, H - 250.0, 1150.0, 1150.0, H + 20.0});
    std::vector<Polyline> footprints;

    for (size_t i = 0; i < 4; i++)
        footprints.push_back(footprint(ring[i]));

    for (size_t i = 0; i < footprints.size(); i++) {
        frame.polyline(up(footprints[i]), RING, 3.0);
        frame.label(fmt::format("footprints[{}]", i), up(area_centroid(footprints[i])));

        for (size_t j = i + 1; j < footprints.size(); j++)
            for (const Polyline& overlap : Polyline::boolean_op(footprints[i], footprints[j], level(0.0), 0))
                frame.polyline(up(overlap), MARK, 5.0);
    }

    for (const OculusEdge& edge : context.guide.oculus_edges)
        frame.line(up(edge.line), GREY, 1.0, true);

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// The BReps
// ═══════════════════════════════════════════════════════════════════════════

/// 241: the drilled central wedge block twice, its model mesh with faceted bores beside its BRep with exact cylinders.
void drilled_block(const Context& context) {

    const std::shared_ptr<Element>& block = context.connected.members.quarters[0].wedges[1].element;
    const BRep& brep = block->model_geometry_brep();
    const Point centre = up(middle(context.guide.quarter(0).wedges()[1]));
    const Vector shift = Vector(std::cos(ISO_YAW), std::sin(ISO_YAW), 0.0) * -BLOCK_SHIFT;
    Frame frame(CHAPTER, 241, "drilled_block", fmt::format("compute_breps: {} as its model mesh, bores faceted, and as model_geometry_brep(), bores exact", block->name), "iso", around({centre, centre + shift}, 330.0));
    frame.distance = 0.55;

    frame.scene.set_node_color(frame.scene.add_mesh(block->model_geometry_mesh().transformed(Xform::translation(shift[0], shift[1], shift[2]))), GREY);
    frame.scene.set_node_color(frame.scene.add_brep(brep), FAMILY_COLORS[3]);
    frame.label(fmt::format("model_geometry_mesh(): {} vertices", block->model_geometry_mesh().number_of_vertices()), centre + shift);
    frame.label(fmt::format("model_geometry_brep(): count_bores = {}", count_bores(brep)), centre);
    frame.write(context.dir);
}

/// Quarter 0's members of one family as check_breps sorts them, exact in the family colour, faceted in MARK, uncut grey; the member with the most bores named.
void family_bores(Frame& frame, const Context& context, size_t f, std::set<std::string>& guids) {

    const std::vector<Member> placed = members(context.connected.members.quarters[0], FAMILIES[f]);
    const std::vector<Outline> shapes = outlines(context.guide.quarter(0), FAMILIES[f]);
    size_t most = 0;
    size_t named = placed.size();

    for (size_t i = 0; i < placed.size(); i++) {
        const std::shared_ptr<Element>& element = placed[i].element;
        const size_t bores = bores_of(element);
        guids.insert(element->guid());
        frame.element(element, !is_cut_by_joint(element) ? GREY : bores > 0 ? FAMILY_COLORS[f] : MARK);

        if (is_cut_by_joint(element) && bores == 0 && frame.labels.size() < 8)
            frame.label(element->name + ": faceted", up(middle(shapes[i])));
        if (bores > most) {
            most = bores;
            named = i;
        }
    }

    if (named < placed.size() && frame.labels.size() < 8)
        frame.label(fmt::format("{}: {} bores", placed[named].element->name, most), up(middle(shapes[named])));
}

/// The parts and dowels of every connector on quarter 0's members, the ring beam and the column, in CONNECTOR_COLOR.
void quarter_connectors(Frame& frame, const Floor& floor, const std::set<std::string>& guids) {

    for (const std::shared_ptr<JointBeam>& connector : floor.connectors) {
        const bool on_quarter = std::any_of(connector->targets.begin(), connector->targets.end(), [&guids](const std::string& guid) { return guids.count(guid) > 0; });

        if (on_quarter)
            for (const std::shared_ptr<Joint>& child : connector->children())
                frame.element(child, CONNECTOR_COLOR);
    }
}

/// 242: check_breps over the floor, quarter 0 drawn as it sorts the elements: exact members tinted, faceted red, connector parts blue.
void brep_counts(const Context& context, const BrepCheck& check) {

    const Floor& floor = context.connected;
    Frame frame(CHAPTER, 242, "check_breps", fmt::format("check_breps: exact {}, faceted {}, bores {}, connectors {}, part_bores {}, stretches {}, {:.0f} ms", check.exact, check.faceted.size(), check.bores, check.connectors, check.part_bores, check.stretches, check.ms), "iso", QUARTER);
    frame.distance = 0.72;
    frame.key = true;
    std::set<std::string> guids;

    for (size_t f = 0; f < FAMILIES.size(); f++)
        family_bores(frame, context, f, guids);

    for (const std::shared_ptr<Element>& element : {floor.members.ring[0].element, std::static_pointer_cast<Element>(floor.members.columns[0].column)}) {
        guids.insert(element->guid());
        frame.element(element, !is_cut_by_joint(element) ? GREY : bores_of(element) > 0 ? (element == floor.members.ring[0].element ? RING : STEEL) : MARK);
    }

    quarter_connectors(frame, floor, guids);
    frame.write(context.dir);
}

/// 243: one dowel of the block dowels between wedge block 0 and outer rib 0 in section, each stretch inside a target drawn thick in its family colour and numbered.
void bore_stretch_section(const Context& context) {

    const Floor& floor = context.connected;
    const std::shared_ptr<Element>& block = floor.members.quarters[0].wedges[0].element;
    const std::shared_ptr<Element>& rib = floor.members.quarters[0].outer_ribs[0].element;
    const std::shared_ptr<JointBeam> connector = drilling(floor, block, rib);
    const Line axis = connector->drill_axes()[0];
    const Vector direction = axis.to_vector().normalized();
    std::vector<Point> extent = {axis.start(), axis.end()};
    std::vector<std::pair<std::string, Line>> stretches;

    for (const std::string& guid : connector->targets) {
        const std::shared_ptr<Element> target = floor.get_element<Element>(guid);
        if (target)
            for (const std::array<double, 2>& stretch : counted_stretches(target, axis)) {
                stretches.push_back({target->name, Line::from_points(axis.start() + direction * stretch[0], axis.start() + direction * stretch[1])});
                extent.insert(extent.end(), {stretches.back().second.start(), stretches.back().second.end()});
            }
    }

    const std::string view = std::abs(direction[0]) >= std::abs(direction[1]) ? "front" : "right";
    Frame frame(CHAPTER, 243, "bore_stretches", fmt::format("bore_stretches: drill_axes()[0] of {} through its {} targets, {} stretches counted", connector->name, connector->targets.size(), stretches.size()), view, around(extent, 60.0));
    frame.element(block, GREY);
    frame.element(rib, GREY);
    frame.line(axis, INK, 2.0, false, true);

    double start_clear = 1e300;
    double end_clear = 1e300;

    for (size_t k = 0; k < stretches.size(); k++) {
        const std::string& name = stretches[k].first;
        const Color& color = name == block->name ? FAMILY_COLORS[3] : name == rib->name ? FAMILY_COLORS[0] : MARK;
        frame.line(stretches[k].second, color, 8.0);
        start_clear = std::min(start_clear, (stretches[k].second.center() - axis.start()).magnitude());
        end_clear = std::min(end_clear, (stretches[k].second.center() - axis.end()).magnitude());

        if (frame.labels.size() < 7)
            frame.label(fmt::format("{}: stretch {}, {:.1f} mm", name, k + 1, stretches[k].second.length()), stretches[k].second.center());
    }

    frame.label(start_clear >= end_clear ? "drill_axes()[0].start()" : "drill_axes()[0].end()", start_clear >= end_clear ? axis.start() : axis.end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Example 8
// ═══════════════════════════════════════════════════════════════════════════

/// 245: the tied 6000 x 4800 bay of example 8 in plan, its square oculus, every quarter's outer ribs end to end at the seam ties and quarter 0's two run-ins.
void tied_bay(const Context& context) {

    const FloorGuide& tied = context.tied;
    const QuarterGeometry& geometry = tied.geometry[0];
    Frame frame(CHAPTER, 245, "tied_bay", fmt::format("Example 8: rectangle({:.0f}, {:.0f}), seam_through_ribs false: a seam_tie where two outer ribs meet", -tied.corners[0][0], -tied.corners[0][1]), "top", {-3300.0, -2700.0, H - 800.0, 3300.0, 2700.0, H + 50.0});
    frame.key = true;
    frame.distance = 0.85;
    plan_context(frame, tied, true);

    for (size_t k = 0; k < 4; k++) {
        frame.line(up(tied.oculus_edges[k].line), INK, 3.0);
        frame.line(up(tied.seams[k].line), GREY, 1.0, true);
    }

    for (const QuarterGeometry& quarter : tied.geometry)
        for (const Polyline& quad : quarter.quads.outer_ribs)
            frame.polyline(up(quad.closed()), FAMILY_COLORS[0], 3.0);

    const std::vector<Relationship> ties = relationships(tied, Relation::seam_tie);

    for (const Relationship& row : ties)
        frame.polyline(row.contact, MARK, 6.0);

    if (!ties.empty() && ties.front().contact.point_count() > 2)
        frame.label(fmt::format("seam_tie of seam {}", ties.front().seam_or_corner), area_centroid(ties.front().contact));

    for (size_t k = 0; k < 2; k++) {
        const Point start = geometry.parabolas[k][0].get_point(0);
        frame.label(fmt::format("run_in[{}] = {:.3f}", k, geometry.run_in[k]), up(Point(start[0], start[1], 0.0)));
    }

    frame.label(fmt::format("oculus = {:.0f}", tied.parameters.oculus), up(tied.oculus_corners[0]));
    frame.label(fmt::format("FloorGuide::rectangle({:.0f}, {:.0f})", -tied.corners[0][0], -tied.corners[0][1]), up(tied.centre), true);
    frame.write(context.dir);
}

}

/// Chapter 11: what FloorGuide::check() measures on the guide, what compute_breps and check_breps do with the floor's BReps, and the tied bay of example 8.
void chapter_11_checks(const Context& context) {

    const FloorReport report = context.guide.check();
    const std::vector<Outline> ring = context.guide.oculus();

    ring_loops(context, ring);
    seam_identities(context, report);
    end_faces(context, report);
    beds_on_flanges(context, report);
    ring_cover(context, report, ring);
    ring_footprints(context, report, ring);
    drilled_block(context);
    brep_counts(context, check_breps(context.connected));
    bore_stretch_section(context);
    tied_bay(context);
}

}
