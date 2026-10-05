#include "docs/floor/movie.h"

namespace movie {

namespace {

const std::string CHAPTER = "07_elements";
const Box FAN_HEAD = {-3080.0, -3080.0, H - 800.0, -2350.0, -2350.0, H + 40.0}; // Column 0's head and the three wedge blocks.
const Box COLUMN = {-3100.0, -3100.0, 0.0, -2600.0, -2600.0, H + 60.0}; // Column 0 from the slab to the floor top.
const Box BRANCH = {-3300.0, -3300.0, 0.0, 3300.0, 3300.0, H + 3100.0}; // The floor and the branch raised above it.
const double RAISE = 3000.0; // How far the branch is drawn above the floor it was copied from, clear of the floor behind it in the lowered view.

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The colour of a quarter family.
const Color& family_color(Family family) {
    return FAMILY_COLORS[static_cast<size_t>(family)];
}

/// The session's element named name, null when there is none.
std::shared_ptr<Element> named(const Session& session, const std::string& name) {

    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        if (element->name == name)
            return element;

    return nullptr;
}

/// The session's element of a tree node, null for a group.
std::shared_ptr<Element> element_of(const Session& session, const TreeNode& node) {

    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        if (element->guid() == node.name)
            return element;

    return nullptr;
}

/// A rib or beam member as the variable beam it was built as.
std::shared_ptr<BeamVariable> variable(const Member& member) {
    return std::dynamic_pointer_cast<BeamVariable>(member.element);
}

/// Every member of the floor: the quarters' families, the ring, the four oculus plates and, with columns, the columns and their supports.
std::vector<std::shared_ptr<Element>> floor_members(const Floor& floor, bool columns) {

    std::vector<std::shared_ptr<Element>> result;

    for (const QuarterMembers& quarter : floor.members.quarters)
        for (const Family family : FAMILIES)
            for (const Member& member : members(quarter, family))
                result.push_back(member.element);

    for (const Member& member : floor.members.ring)
        result.push_back(member.element);

    for (size_t i = 4; i < 9; i++)
        result.push_back(named(floor, fmt::format("oculus_{}", i)));

    if (columns)
        for (const ColumnModel& model : floor.members.columns) {
            result.push_back(model.column);
            result.push_back(model.support);
        }

    return result;
}

/// Every member of the floor in grey but the ones in own.
void grey_floor(Frame& frame, const Floor& floor, const std::set<const Element*>& own, bool columns) {

    for (const std::shared_ptr<Element>& element : floor_members(floor, columns))
        if (element && !own.count(element.get()))
            frame.element(element, GREY);
}

/// Quarter q's members of the families, each in its family's colour.
void family_members(Frame& frame, const Floor& floor, size_t q, const std::vector<Family>& families, const Color* color = nullptr) {

    for (const Family family : families)
        for (const Member& member : members(floor.members.quarters[q], family))
            frame.element(member.element, color ? *color : family_color(family));
}

/// The elements of members, for a set of what is drawn in colour.
std::set<const Element*> pointers(const std::vector<Member>& list) {

    std::set<const Element*> result;

    for (const Member& member : list)
        result.insert(member.element.get());

    return result;
}

/// The box around the points, grown by margin on every side.
Box around(const std::vector<Point>& points, double margin) {

    Box box = {1e300, 1e300, 1e300, -1e300, -1e300, -1e300};

    for (const Point& point : points)
        for (int i = 0; i < 3; i++) {
            box[i] = std::min(box[i], point[i] - margin);
            box[i + 3] = std::max(box[i + 3], point[i] + margin);
        }

    return box;
}

/// Where a closed loop crosses a plane, by Intersection::polyline_plane.
std::vector<Point> crossings(const Polyline& loop, const Plane& plane) {

    std::vector<Point> points;
    std::vector<int> edges;
    Intersection::polyline_plane(loop, plane, points, edges);

    return points;
}

/// The section of a member outline by a plane: its two loops' crossings as a closed quad, empty when a loop does not cross it twice.
Polyline section_quad(const Outline& outline, const Plane& plane) {

    const std::vector<Point> a = crossings(outline.top, plane);
    const std::vector<Point> b = crossings(outline.bottom, plane);

    if (a.size() != 2 || b.size() != 2)
        return Polyline();

    return Polyline({a[0], a[1], b[1], b[0]}).closed();
}

/// The plane moved to a new origin, its axes kept, so its square is drawn there.
Plane at(const Plane& plane, const Point& origin) {
    return Plane::from_frame(origin, plane.x_axis(), plane.y_axis(), plane.z_axis());
}

/// Up to count indices of n, evenly spread, the first and the last among them.
std::vector<size_t> spread(size_t n, size_t count) {

    std::vector<size_t> result;

    for (size_t k = 0; k < std::min(n, count); k++) {
        const size_t index = n <= count ? k : static_cast<size_t>(std::lround(static_cast<double>(k) * static_cast<double>(n - 1) / static_cast<double>(count - 1)));
        if (result.empty() || result.back() != index)
            result.push_back(index);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// The scene and its groups
// ═══════════════════════════════════════════════════════════════════════════

/// The guide's plan, its bay edges the input the Floor copies: the Floor holds nothing else yet.
void empty_floor(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 111, "empty_floor", "Floor floor(guide): an empty WoodSession; only the guide's plan exists, no member yet", "top", BAY);

    for (const BayEdge& edge : guide.edges)
        frame.line(up(edge.line), INPUT, 2.0);

    for (size_t q = 0; q < 4; q++)
        frame.polyline(up(Polyline(guide.geometry[q].polygon).closed()), GREY, 1.0);

    frame.label(fmt::format("Floor \"{}\": empty WoodSession, root", context.members.name), up(guide.corners[3]));
    frame.label(fmt::format("guide = FloorGuide::rectangle({:.0f}, {:.0f})", guide.corners[2][0], guide.corners[2][1]), up(guide.corners[1]));
    frame.label("members.group = nullptr: the tree root", up(guide.centre));
    frame.write(context.dir);
}

/// The four quarter polygons, quarter 0 in colour, each named by the group quarter_group found or made for it.
void quarter_groups(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 113, "quarter_groups", "quarter_group(q): group_named finds quarter_q under the root or adds it; quarter 0 in colour", "top", BAY);
    plan_context(frame, guide, false);

    for (size_t q = 0; q < 4; q++) {
        const Polyline polygon = Polyline(guide.geometry[q].polygon).closed();
        frame.polyline(up(polygon), q == 0 ? BUILT : GREY, q == 0 ? 4.0 : 2.0);
        frame.label(context.members.members.quarters[q].group->name, up(area_centroid(polygon)), true);
    }

    frame.write(context.dir);
}

/// Quarter 0's outlines at z 0 and its members at bay_height: the lift every member is placed by.
void lift(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const double height = guide.parameters.bay_height;
    Frame frame(CHAPTER, 114, "lift", "add_quarter_model: outlines at z 0, placed by lift = Xform::translation(0, 0, bay_height)", "front", {-3100.0, -3100.0, -750.0, 100.0, 100.0, H + 100.0});

    for (const Family family : FAMILIES)
        for (const Outline& outline : outlines(quarter, family)) {
            frame.polyline(outline.top, INPUT, 1.0);
            frame.polyline(outline.bottom, INPUT, 1.0);
        }

    family_members(frame, context.members, 0, {FAMILIES.begin(), FAMILIES.end()}, &BUILT);

    const Point foot(-1500.0, -1500.0, 0.0);
    frame.line(Line::from_points(foot, foot + Vector(0.0, 0.0, height)), VARIABLE, 4.0, false, true);
    frame.label(fmt::format("lift = Xform::translation(0, 0, {:.0f})", height), foot + Vector(0.0, 0.0, height * 0.5));
    frame.label("outlines at the datum, z = 0", quarter.outer_ribs()[0].top.get_point(1));
    frame.label(fmt::format("quarter_0 members, top at z = {:.0f}", height), up(quarter.outer_ribs()[1].top.get_point(1)));
    frame.label("suffix = \"_0\": every name of quarter 0", up(quarter.inner_beams()[1].top.get_point(1)));
    frame.write(context.dir);
}

/// Outer rib 0's two loops, their area centroids and the distance between them, the member's thickness.
void thickness(const Context& context) {

    const Outline rib = context.guide.quarter(0).outer_ribs()[0];
    const Point a = up(area_centroid(rib.top));
    const Point b = up(area_centroid(rib.bottom));
    Frame frame(CHAPTER, 115, "outline_thickness", "outline_thickness: the distance between the area centroids of the outline's two loops", "iso", RIB);
    frame.distance = 0.85;

    frame.polyline(up(rib.top), INPUT, 3.0);
    frame.polyline(up(rib.bottom), INPUT, 3.0);
    frame.line(Line::from_points(a, b), VARIABLE, 4.0);
    frame.point(a, BUILT);
    frame.point(b, BUILT);
    frame.label("area_centroid(outline.top)", a);
    frame.label(fmt::format("area_centroid(outline.bottom): thickness = {:.3f}", outline_thickness(rib)), b);
    frame.label(fmt::format("outline.top on y = {:.0f}", rib.top.get_point(1)[1]), up(rib.top.get_point(1)));
    frame.label(fmt::format("outline.bottom on y = {:.0f}", rib.bottom.get_point(0)[1]), up(rib.bottom.get_point(0)));
    frame.write(context.dir);
}

/// Quarter 0's members in their family colours, each family tagged with the function that built it.
void dispatch(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    Frame frame(CHAPTER, 116, "to_members", "to_members: ribs by to_rib and inner beams by to_beam as BeamVariable, every other family a Plate", "iso", QUARTER);
    frame.distance = 0.72;
    frame.key = true;
    family_members(frame, context.members, 0, {FAMILIES.begin(), FAMILIES.end()});

    const std::vector<std::vector<Outline>> beds = quarter.beds();
    frame.label("outer_ribs: to_rib -> BeamVariable", up(middle(quarter.outer_ribs()[0])));
    frame.label("inner_ribs: to_rib -> BeamVariable", up(middle(quarter.inner_ribs()[0])));
    frame.label("inner_beams: to_beam(outline, {0, 3}, {1, 2})", up(middle(quarter.inner_beams()[1])));
    frame.label("wedges: to_plate -> Plate", up(middle(quarter.wedges()[1])));
    frame.label("tsections: to_plate -> Plate", up(middle(quarter.tsections()[5])));
    frame.label("beds: to_plate -> Plate", up(middle(beds[1][beds[1].size() / 2])));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════

/// The three bed rows as plates over grey ribs, one plate's +t bottom and +2t top marked.
void bed_plates(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const std::vector<std::vector<Outline>> beds = quarter.beds();
    const Outline& plate = beds[1][beds[1].size() / 2];
    Frame frame(CHAPTER, 117, "bed_plates", "Bed plates: Plate(outline.bottom, outline.top), a slab from the +t layer up to the +2t layer", "iso", QUARTER);
    frame.distance = 0.72;

    family_members(frame, context.members, 0, {Family::outer_ribs, Family::inner_ribs}, &GREY);
    family_members(frame, context.members, 0, {Family::beds}, &BUILT);
    frame.polyline(up(plate.bottom), INPUT, 3.0);
    frame.polyline(up(plate.top), VARIABLE, 3.0);

    frame.label("outline.bottom: the +t layer", up(plate.bottom.get_point(0)));
    frame.label(fmt::format("outline.top: the +2t layer, {:.3f} higher", outline_thickness(plate)), up(plate.top.get_point(2)));

    for (size_t row = 0; row < beds.size(); row++)
        frame.label(fmt::format("beds[{}]", row), up(middle(beds[row][row == 1 ? 0 : beds[row].size() / 2])));

    frame.write(context.dir);
}

/// Bed row 0 before and after the lift, each plate named by add_family.
void family_names(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const std::vector<Outline> row = quarter.beds()[0];
    const std::vector<Member>& placed = context.members.members.quarters[0].beds[0];
    const double height = context.guide.parameters.bay_height;
    Frame frame(CHAPTER, 118, "add_family", "add_family: group beds_0_0, each plate placed by lift and named beds_0_<i>_0", "front", {-3100.0, -3100.0, -750.0, 100.0, 100.0, H + 100.0});

    for (const Outline& outline : row) {
        frame.polyline(outline.top, INPUT, 2.0);
        frame.polyline(outline.bottom, INPUT, 2.0);
    }

    for (const Member& member : placed)
        frame.element(member.element, BUILT);

    const Point base = area_centroid(row[0].top);
    frame.line(Line::from_points(base, up(base)), VARIABLE, 3.0, false, true);
    frame.label(fmt::format("element->place(lift): +{:.0f}", height), base + Vector(0.0, 0.0, height * 0.5));

    for (size_t i : spread(row.size(), 6))
        frame.label(placed[i].element->name, up(middle(row[i])));

    frame.write(context.dir);
}

/// A section across quarter 0's ribs, framed on inner rib 0: the ribs grey, its two t-section strips beside its faces in colour and named.
void tsection_plates(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Quarter quarter = guide.quarter(0);
    const ColumnCorner& column = guide.columns[0];
    const Vector across = guide.centre - column.axis_point;
    const Plane cut = Plane::from_point_normal(up(column.axis_point + across * 0.35), across.normalized());
    const Polyline inner_rib = section_quad({up(quarter.inner_ribs()[0].top), up(quarter.inner_ribs()[0].bottom)}, cut);
    const std::array<size_t, 2> shown = {1, 2}; // The strips on inner rib 0's outer and central faces.
    std::vector<Point> corners = inner_rib.get_points();
    std::vector<Polyline> strips;

    for (const Outline& outline : quarter.tsections())
        strips.push_back(section_quad({up(outline.top), up(outline.bottom)}, cut));

    for (const size_t k : shown)
        for (const Point& point : strips[k].get_points())
            corners.push_back(point);

    Frame frame(CHAPTER, 119, "tsection_plates", fmt::format("T-sections: {:.0f} mm strips from the soffit to +t beside the rib faces; the cut across inner rib 0", guide.parameters.tsections), "iso", around(corners, 60.0));

    for (const Outline& outline : quarter.outer_ribs()) {
        const Polyline rib = section_quad({up(outline.top), up(outline.bottom)}, cut);
        if (rib.point_count() > 0)
            frame.polyline(rib, GREY, 4.0);
    }

    const std::vector<Outline> inner_ribs = quarter.inner_ribs();

    for (size_t i = 0; i < inner_ribs.size(); i++) {
        const Polyline rib = section_quad({up(inner_ribs[i].top), up(inner_ribs[i].bottom)}, cut);
        if (rib.point_count() > 0)
            frame.polyline(rib, i == 0 ? INPUT : GREY, 4.0); // Inner rib 0: the faces its two strips lie beside.
    }

    for (size_t k = 0; k < strips.size(); k++)
        if (strips[k].point_count() > 0)
            frame.polyline(strips[k], BUILT, 5.0);

    for (const size_t k : shown)
        if (strips[k].point_count() > 0)
            frame.label(context.members.members.quarters[0].tsections[k].element->name, area_centroid(strips[k]));

    if (inner_rib.point_count() > 0)
        frame.label(context.members.members.quarters[0].inner_ribs[0].element->name, area_centroid(inner_rib));

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Ribs
// ═══════════════════════════════════════════════════════════════════════════

/// Outer rib 0's first face in elevation: p1, p0 and the soffit stations, and the two Bezier points the trim drops.
void rib_stations(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const Outline rib = context.guide.quarter(0).outer_ribs()[0];
    const std::vector<Point> top = rib.top.get_points();
    const size_t stations = top.size() - 3;
    const Polyline& parabola = geometry.parabolas[0][0];
    const Point start = parabola.get_point(0);
    const Point end = parabola.get_point(parabola.point_count() - 1);
    Frame frame(CHAPTER, 120, "rib_stations", fmt::format("to_rib: outline.top = [p1, p0, soffit points, p1], stations = top.size() - 3 = {}", stations), "front", RIB);

    frame.polyline(up(rib.top), BUILT, 4.0);
    frame.line(up(Line::from_points(parabola.get_point(1), top[2])), INPUT, 2.0, true);
    frame.line(up(Line::from_points(top[1 + stations], end)), INPUT, 2.0, true);
    frame.point(up(start), GREY, 14.0);
    frame.point(up(end), GREY, 14.0);
    frame.point(up(top[0]), RESULT);
    frame.point(up(top[1]), RESULT);

    for (size_t i = 0; i < stations; i++)
        frame.point(up(top[2 + i]), VARIABLE);

    frame.label("top[0] = p1", up(top[0]));
    frame.label("top[1] = p0", up(top[1]));
    frame.label("top[2]: station 0, the fan hit", up(top[2]));
    frame.label("top[3]: station 1", up(top[3]));
    frame.label(fmt::format("top[{}]: station {}", 2 + stations / 2, stations / 2), up(top[2 + stations / 2]));
    frame.label(fmt::format("top[{}]: station {}, the end hit", 1 + stations, stations - 1), up(top[1 + stations]));
    frame.label(fmt::format("Bezier point 0, z {:.0f}: dropped", start[2]), up(start));
    frame.label(fmt::format("Bezier point {}: dropped", parabola.point_count() - 1), up(end));
    frame.write(context.dir);
}

/// Outer rib 0's interior sections hanging from the datum, one named at its corners.
void rib_interior(const Context& context) {

    const Outline rib = context.guide.quarter(0).outer_ribs()[0];
    const std::shared_ptr<BeamVariable> beam = variable(context.members.members.quarters[0].outer_ribs[0]);
    const std::vector<Polyline>& sections = beam->sections;
    const size_t k = sections.size() / 2;
    const std::vector<Point> corners = sections[k].get_points();
    Frame frame(CHAPTER, 121, "rib_interior", "to_rib: an interior section per station, {low, high, far_high, far_low}, from the soffit to z 0", "iso", RIB);
    frame.distance = 0.85;

    frame.polyline(up(rib.top), INPUT, 2.0);
    frame.polyline(up(rib.bottom), INPUT, 2.0);

    for (size_t i = 1; i + 1 < sections.size(); i++)
        frame.polyline(sections[i], BUILT, i == k ? 5.0 : 3.0);

    frame.label(fmt::format("low = top[{}]", 2 + k), corners[0]);
    frame.label("high = at_level(low, 0)", corners[1]);
    frame.label("far_high = at_level(far_low, 0)", corners[2]);
    frame.label(fmt::format("far_low = bottom[{}]", 2 + k), corners[3]);
    frame.label(fmt::format("sections[1] .. sections[{}]", sections.size() - 2), area_centroid(sections[1]));
    frame.write(context.dir);
}

/// Outer rib 0's two end sections, each lying in the plane the rib ends on.
void rib_ends(const Context& context) {

    const Quarter quarter = context.guide.quarter(0);
    const ConstructionPlanes& cp = context.guide.geometry[0].planes;
    const std::shared_ptr<BeamVariable> beam = variable(context.members.members.quarters[0].outer_ribs[0]);
    const std::vector<Polyline>& sections = beam->sections;
    const Polyline& first = sections.front();
    const Polyline& last = sections.back();
    const Plane end = quarter.rib_seam_ends()[0];
    Frame frame(CHAPTER, 122, "rib_ends", "to_rib: the end sections take p0 and p1, so they lie in the fan plane and in the seam end plane", "iso", RIB);
    frame.plane_size = 200.0;
    frame.distance = 0.85;

    for (size_t i = 1; i + 1 < sections.size(); i++)
        frame.polyline(sections[i], GREY, 2.0);

    frame.plane(at(up(cp.wedges[0][0]), area_centroid(first)), INPUT);
    frame.plane(at(up(end), area_centroid(last)), INPUT);
    frame.polyline(first, BUILT, 5.0);
    frame.polyline(last, BUILT, 5.0);

    frame.label("sections[0] in cut_plane0 = cp.wedges[0][0]", area_centroid(first));
    frame.label(fmt::format("sections[{}] in cut_plane1 = rib_seam_ends()[0], x = {:.0f}", sections.size() - 1, end.origin()[0]), area_centroid(last));
    frame.label("top[2] = pts[0]", first.get_point(0));
    frame.label("top[1] = p0", first.get_point(1));
    frame.label("bottom[1]", first.get_point(2));
    frame.label("bottom[2] = far[0]", first.get_point(3));
    frame.write(context.dir);
}

/// Quarter 0's four ribs in plan, each with its axis from the fan end to the far end.
void rib_axes(const Context& context) {

    const QuarterMembers& quarter = context.members.members.quarters[0];
    Frame frame(CHAPTER, 123, "rib_axes", "to_rib: axis from the middle of the two p0 corners to the middle of the two p1 corners, on z 0", "top", QUARTER);
    family_members(frame, context.members, 0, {Family::outer_ribs, Family::inner_ribs}, &GREY);

    for (const Family family : {Family::outer_ribs, Family::inner_ribs})
        for (const Member& member : members(quarter, family)) {
            const Line& axis = variable(member)->axis;
            frame.line(axis, BUILT, 5.0, false, true);
            frame.label(member.element->name, axis.center());
        }

    const Line& axis = variable(quarter.outer_ribs[0])->axis;
    frame.label(fmt::format("axis end: y = {:.0f}, x = {:.0f}", axis.end()[1], axis.end()[0]), axis.end());
    frame.write(context.dir);
}

/// The whole floor grey and quarter 0's two outer ribs in colour, each a variable beam.
void outer_ribs(const Context& context) {

    const QuarterMembers& quarter = context.members.members.quarters[0];
    const std::vector<Outline> ribs = context.guide.quarter(0).outer_ribs();
    Frame frame(CHAPTER, 124, "outer_ribs", "Outer ribs: BeamVariable(axis, sections) lofts the sections; added as outer_ribs_0_0 and outer_ribs_1_0", "iso", QUARTER);
    frame.distance = 0.8; // Framed on quarter 0, the rest of the grey floor runs out behind it.
    grey_floor(frame, context.members, pointers(quarter.outer_ribs), false);

    for (size_t i = 0; i < quarter.outer_ribs.size(); i++) {
        const std::shared_ptr<Element>& element = quarter.outer_ribs[i].element;
        frame.element(element, BUILT);
        frame.label(fmt::format("{}: {} faces", element->name, element->element_geometry_mesh().number_of_faces()), up(middle(ribs[i])));
    }

    frame.write(context.dir);
}

/// Quarter 0's inner ribs in plan, the one sweep of both beside each rib's face normal.
void inner_ribs(const Context& context) {

    const QuarterGeometry& geometry = context.guide.geometry[0];
    const CentralPanel& panel = geometry.central_panel;
    const std::vector<Outline> ribs = context.guide.quarter(0).inner_ribs();
    const QuarterMembers& quarter = context.members.members.quarters[0];
    Frame frame(CHAPTER, 125, "inner_ribs", "Inner ribs: the same to_rib, the second loop swept along central_panel.rib_sweep, not the face normal", "top", QUARTER);
    family_members(frame, context.members, 0, {Family::outer_ribs}, &GREY);
    family_members(frame, context.members, 0, {Family::inner_ribs}, &BUILT);

    for (size_t i = 0; i < ribs.size(); i++) {
        const Point mid = middle(ribs[i]);
        const Point from(mid[0], mid[1], 0.0);
        const Vector face_normal = geometry.planes.inner_ribs[i][0].z_axis();
        const double way = panel.rib_sweep.dot(face_normal) < 0.0 ? -600.0 : 600.0; // Inner rib 1's far points move along -rib_sweep.
        const Point sweep = from + panel.rib_sweep.normalized() * way;
        const Point normal = from + face_normal * 400.0;
        frame.line(up(Line::from_points(from, sweep)), VARIABLE, 4.0, false, true);
        frame.line(up(Line::from_points(from, normal)), INPUT, 2.0, true, true);
        frame.label(quarter.inner_ribs[i].element->name, up(ribs[i].top.get_point(1)));

        if (i == 0) {
            frame.label(fmt::format("central_panel.rib_sweep: {:.3f} deg off the normal", panel.obliqueness[0]), up(sweep));
            frame.label("cp.inner_ribs[0][0].z_axis()", up(normal));
        }
    }

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Wedges and beams
// ═══════════════════════════════════════════════════════════════════════════

/// The three wedge blocks at column 0 between the grey ribs, with their fan planes.
void wedge_blocks(const Context& context) {

    const FloorGuide& guide = context.guide;
    const QuarterGeometry& geometry = guide.geometry[0];
    const ConstructionPlanes& cp = geometry.planes;
    const std::vector<Outline> blocks = guide.quarter(0).wedges();
    const QuarterMembers& quarter = context.members.members.quarters[0];
    Frame frame(CHAPTER, 126, "wedge_blocks", "Wedges: loft_planes of the rib faces, the bed top and z 0, from the fan plane to the far face", "iso", FAN_HEAD);
    frame.plane_size = 90.0;
    family_members(frame, context.members, 0, {Family::outer_ribs, Family::inner_ribs}, &GREY);
    family_members(frame, context.members, 0, {Family::wedges}, &BUILT);

    for (size_t i = 0; i < blocks.size(); i++) {
        frame.plane(up(cp.wedges[i][0]), INPUT);
        frame.label(quarter.wedges[i].element->name, up(middle(blocks[i])));
    }

    frame.label(fmt::format("cp.wedges[1][0]: wedge_plane_angle = {:.0f} deg", guide.parameters.wedge_plane_angle), up(cp.wedges[1][0].origin()));
    frame.label(fmt::format("far face cp.wedges[0][1]: run_in[0] = {:.3f}", geometry.run_in[0]), up(blocks[0].top.get_point(0)));
    frame.label(fmt::format("far face cp.wedges[1][1]: {:.2f} x mean(run_in)", guide.parameters.middle_wedge_factor), up(blocks[1].top.get_point(2)));
    frame.write(context.dir);
}

/// Seam beam 0's two loops numbered, its two end caps and its axis from a to b.
void beam_caps(const Context& context) {

    const Outline beam = context.guide.quarter(0).inner_beams()[0];
    const QuarterMembers& quarter = context.members.members.quarters[0];
    const std::shared_ptr<BeamVariable> element = variable(quarter.inner_beams[0]);
    const std::vector<Point> top = beam.top.get_points();
    const std::vector<Point> bottom = beam.bottom.get_points();
    std::vector<Point> corners;

    for (const Point& point : top)
        corners.push_back(up(point));

    for (const Point& point : bottom)
        corners.push_back(up(point));

    Frame frame(CHAPTER, 127, "beam_caps", "to_beam(outline, {0, 3}, {1, 2}): two end caps and a vertex-centroid axis a -> b, a 6-face BeamVariable", "iso", around(corners, 150.0));
    frame.element(quarter.inner_beams[1].element, GREY);
    frame.polyline(up(beam.top), INPUT, 2.0);
    frame.polyline(up(beam.bottom), INPUT, 2.0);
    frame.polyline(element->sections.front(), BUILT, 6.0);
    frame.polyline(element->sections.back(), BUILT, 6.0);
    frame.line(element->axis, RESULT, 4.0, false, true);

    for (size_t k = 0; k < 4; k++)
        frame.label(fmt::format("top[{}]", k), up(top[k]));

    frame.label("bottom[0]", up(bottom[0]));
    frame.label("bottom[1]", up(bottom[1]));
    frame.label("a: first = {top[0], top[3], bottom[3], bottom[0]}", element->axis.start());
    frame.label("b: last = {top[1], top[2], bottom[2], bottom[1]}", element->axis.end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus
// ═══════════════════════════════════════════════════════════════════════════

/// The oculus outlines grey and the four levels they are built between.
void oculus_levels(const Context& context) {

    const FloorGuide& guide = context.guide;
    const double t = guide.parameters.tsections;
    Frame frame(CHAPTER, 128, "oculus_levels", "guide.oculus(): nine outlines between four levels, z 0, the soffit, soffit + t and soffit + 2t", "front", {650.0, -1300.0, H - 240.0, 1250.0, 1300.0, H + 30.0}); // The right end of the ring, so the 27 mm steps between the levels open up.

    for (const Outline& outline : guide.oculus()) {
        frame.polyline(up(outline.top), BUILT, 1.0);
        frame.polyline(up(outline.bottom), BUILT, 1.0);
    }

    const std::array<std::pair<std::string, double>, 4> levels = {{
        {"side0 = level(0)", 0.0},
        {fmt::format("side3 = soffit + 2 tsections = {:.3f}", guide.soffit + 2.0 * t), guide.soffit + 2.0 * t},
        {fmt::format("side1 = soffit + tsections = {:.3f}", guide.soffit + t), guide.soffit + t},
        {fmt::format("side2 = soffit = {:.3f}", guide.soffit), guide.soffit},
    }};

    for (size_t i = 0; i < levels.size(); i++) {
        frame.line(up(Line::from_points(Point(-1200.0, -1200.0, levels[i].second), Point(1200.0, -1200.0, levels[i].second))), VARIABLE, 2.0, true);
        frame.label(levels[i].first, up(Point(1180.0 - 140.0 * static_cast<double>(i), -1200.0, levels[i].second)));
    }

    frame.write(context.dir);
}

/// The four ring beams in plan, a pinwheel, each start cap on the next edge's tilted plane.
void ring_beams(const Context& context) {

    const Floor& floor = context.members;
    Frame frame(CHAPTER, 129, "ring_beams", "Ring beams: to_beam(outline, {1, 0}, {2, 3}); each starts on tilted[i + 1] and ends on ring_inner[i - 1]", "top", OCULUS);

    for (const QuarterMembers& quarter : floor.members.quarters)
        frame.element(quarter.inner_beams[1].element, GREY);

    for (size_t i = 0; i < floor.members.ring.size(); i++) {
        const std::shared_ptr<BeamVariable> beam = variable(floor.members.ring[i]);
        frame.element(beam, BUILT);
        frame.polyline(beam->sections.front(), RESULT, 7.0);
        frame.label(fmt::format("{} starts on tilted[{}]", beam->name, (i + 1) % 4), area_centroid(beam->sections.front()));
    }

    frame.write(context.dir);
}

/// The four bottom wedges inside the grey ring, the ledge the central plate rests on, framed on the far corner where oculus_6 and oculus_7 meet.
void bottom_wedges(const Context& context) {

    const Floor& floor = context.members;
    const std::vector<Outline> oculus = context.guide.oculus();
    const std::vector<Point> far = up(oculus[6].top).get_points();
    const std::vector<Point> left = up(oculus[7].top).get_points();
    Point corner = far[0];
    double gap = 1e300;

    for (const Point& a : far)
        for (const Point& b : left)
            if (Point::distance(a, b) < gap) {
                gap = Point::distance(a, b);
                corner = a + (b - a) * 0.5;
            }

    const Box box = {corner[0] - 260.0, corner[1] - 260.0, H - 260.0, corner[0] + 260.0, corner[1] + 260.0, H + 30.0};
    Frame frame(CHAPTER, 130, "bottom_wedges", fmt::format("Bottom wedges oculus_4 .. oculus_7: {0:.0f} x {0:.0f} strips inside the ring beams, soffit to soffit + t", context.guide.parameters.tsections), "iso", box);

    for (const Member& member : floor.members.ring)
        frame.element(member.element, GREY);

    for (size_t i = 4; i < 8; i++)
        frame.element(named(floor, fmt::format("oculus_{}", i)), BUILT);

    for (const size_t i : std::array<size_t, 2>{6, 7}) {
        const Point along = corner + (up(middle(oculus[i])) - corner).normalized() * 180.0;
        frame.label(i == 6 ? fmt::format("oculus_6: {:.0f} thick", outline_thickness(oculus[i])) : "oculus_7", along);
    }

    for (const size_t i : std::array<size_t, 2>{2, 3}) {
        const Point along = corner + (up(middle(oculus[i])) - corner).normalized() * 220.0;
        frame.label(fmt::format("{}: ring beam", floor.members.ring[i].element->name), along);
    }

    frame.write(context.dir);
}

/// The central plate on the ledge strips, bounded by the four ring_inner planes.
void central_plate(const Context& context) {

    const Floor& floor = context.members;
    const std::vector<Outline> oculus = context.guide.oculus();
    const Outline& plate = oculus[8];
    Frame frame(CHAPTER, 131, "central_plate", "oculus_8: loft_planes(ring_inner, soffit + t, soffit + 2t), the plate on the four strips", "iso", OCULUS);
    frame.distance = 0.65;

    for (const Member& member : floor.members.ring)
        frame.element(member.element, GREY);

    for (size_t i = 4; i < 8; i++)
        frame.element(named(floor, fmt::format("oculus_{}", i)), INPUT);

    frame.element(named(floor, "oculus_8"), BUILT);
    frame.label("oculus_8", up(middle(plate)));
    frame.label(fmt::format("bottom: z {:.3f}", up(plate.bottom.get_point(0))[2]), up(plate.bottom.get_point(0)));
    frame.label(fmt::format("top: z {:.3f}", up(plate.top.get_point(2))[2]), up(plate.top.get_point(2)));
    frame.label("oculus_4: the ledge", up(middle(oculus[4])));
    frame.write(context.dir);
}

/// The oculus by the group that holds each member: quarter_0/oculus_0 built, oculus at the root the second result, the other quarters' groups grey.
void oculus_groups(const Context& context) {

    const Floor& floor = context.members;
    const std::vector<Outline> oculus = context.guide.oculus();
    Frame frame(CHAPTER, 132, "oculus_groups", "add_oculus_model: ring beam q and strip q + 4 in quarter_q/oculus_q, the central plate in oculus", "top", OCULUS);

    for (size_t q = 0; q < 4; q++)
        for (TreeNode* group : floor.members.quarters[q].group->children()) {
            if (group->name != fmt::format("oculus_{}", q))
                continue;

            std::string names;
            for (TreeNode* node : group->children())
                if (const std::shared_ptr<Element> element = element_of(floor, *node)) {
                    frame.element(element, q == 0 ? BUILT : GREY);
                    names += (names.empty() ? "" : ", ") + element->name;
                }

            frame.label(fmt::format("{}/{}: {}", floor.members.quarters[q].group->name, group->name, names), up(middle(oculus[q])));
        }

    for (TreeNode* node : floor.members.oculus->children())
        if (const std::shared_ptr<Element> element = element_of(floor, *node))
            frame.element(element, RESULT);

    frame.label(fmt::format("{} (root): oculus_8", floor.members.oculus->name), up(middle(oculus[8])), true);
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Columns
// ═══════════════════════════════════════════════════════════════════════════

/// Column 0's support on the slab, the column foot on its axis and the axis leaving the frame up to the floor top.
void support_foot(const Context& context) {

    const ColumnModel& model = context.members.members.columns[0];
    const ColumnCorner& corner = context.guide.columns[0];
    const Point foot = model.support->column_foot();
    const Point top(foot[0], foot[1], context.guide.parameters.bay_height);
    Frame frame(CHAPTER, 134, "support_foot", "to_support on corner.support_plane at z 0; the column axis from column_foot() to bay_height", "front", {foot[0] - 260.0, foot[1] - 260.0, -80.0, foot[0] + 260.0, foot[1] + 260.0, 520.0}); // The support close up; the axis runs on out of the top.
    frame.plane_size = 100.0;

    frame.element(model.support, BUILT);
    frame.plane(corner.support_plane, INPUT);
    frame.line(Line::from_points(foot, top), RESULT, 4.0, false, true);
    frame.point(foot, VARIABLE);
    frame.label(fmt::format("support_plane: origin ({:.0f}, {:.0f}, 0)", corner.support_plane.origin()[0], corner.support_plane.origin()[1]), corner.support_plane.origin());
    frame.label(fmt::format("foot = column_foot(): z {:.0f}", foot[2]), foot);
    frame.label(fmt::format("axis up to z = bay_height = {:.0f}", top[2]), foot + Vector(0.0, 0.0, 380.0));
    frame.write(context.dir);
}

/// Column 0 before its cuts: the shaft square, and the head square from the step to the top.
void shaft_capitel(const Context& context) {

    const ColumnModel& model = context.members.members.columns[0];
    const std::shared_ptr<Column> column = to_column(context.guide.columns[0], context.guide.parameters, *model.support);
    const double step = column->axis.end()[2] - column->head_height;
    const Xform to_step = Xform::translation(0.0, 0.0, step - column->axis.start()[2]);
    const Polyline section = column->section.transformed(to_step);
    const Polyline head = column->head.transformed(to_step);
    Frame frame(CHAPTER, 135, "shaft_capitel", "to_column: the shaft square, and the wider head square over head_height = column_head_depth", "iso", COLUMN);

    frame.element(column, BUILT);
    frame.polyline(section, VARIABLE, 4.0);
    frame.polyline(head, RESULT, 4.0);
    frame.label(fmt::format("section: {:.0f} square", (section.get_point(1) - section.get_point(0)).magnitude()), section.get_point(2));
    frame.label(fmt::format("head: {:.0f} square", (head.get_point(1) - head.get_point(0)).magnitude()), head.get_point(1));
    frame.label(fmt::format("step: z {:.0f}, head_height = {:.0f} below the top", step, column->head_height), Point(column->axis.start()[0], column->axis.start()[1], step));
    frame.label(fmt::format("top: z {:.0f}", column->axis.end()[2]), column->axis.end());
    frame.label(fmt::format("foot: z {:.0f}", column->axis.start()[2]), column->axis.start());
    frame.write(context.dir);
}

/// The support joint at column 0's foot: the head plate disc it pockets and the three screw drills.
void support_joint(const Context& context) {

    const ColumnModel& model = context.members.members.columns[0];
    const Support& support = *model.support;
    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(support, *model.column);
    const Point axis = support.at(0.0);
    const Point foot = support.column_foot();
    Frame frame(CHAPTER, 136, "support_joint", "Joint::support: the head plate disc between two loops and one drill per screw, cut into the column end", "front", {axis[0] - 110.0, axis[1] - 110.0, 60.0, axis[0] + 110.0, axis[1] + 110.0, 340.0});

    frame.element(model.support, INPUT);

    for (const Polyline& loop : joint->loops)
        frame.polyline(loop, BUILT, 3.0);

    for (const Line& drill : joint->drill_lines)
        frame.line(drill, RESULT, 3.0, false, true);

    for (const double z : {joint->loops[0].get_point(0)[2], foot[2], joint->loops[1].get_point(0)[2]})
        frame.line(Line::from_points(Point(axis[0] - 100.0, axis[1], z), Point(axis[0] + 100.0, axis[1], z)), VARIABLE, 1.5, true);

    const Polyline& lower = joint->loops[0];
    const Polyline& upper = joint->loops[1];
    frame.label(fmt::format("loops[0]: r {:.0f}, z {:.0f}", support.head_plate_diameter * 0.5, lower.get_point(0)[2]), lower.get_point(0));
    frame.label(fmt::format("loops[1]: z = height = {:.0f}", upper.get_point(0)[2]), upper.get_point(upper.point_count() / 2));
    frame.label(fmt::format("foot: z {:.0f}", foot[2]), foot);
    frame.label(fmt::format("drill_lines: {}, line_radius {:.0f}", joint->drill_lines.size(), joint->line_radius), joint->drill_lines[0].end());
    frame.write(context.dir);
}

/// Column 0's carved head and the six lifted cutter slabs that carved it.
void head_carving(const Context& context) {

    const ColumnCorner& corner = context.guide.columns[0];
    const std::vector<Outline> cutters = context.guide.quarter(0).column_cutters();
    Frame frame(CHAPTER, 137, "head_carving", "column_cuts: six cutter plates lifted by bay_height, solid difference cuts of column_0", "iso", FAN_HEAD);
    frame.key = true;
    frame.element(context.members.members.columns[0].column, BUILT);

    for (size_t k = 0; k < cutters.size(); k++) {
        frame.polyline(up(cutters[k].top), INPUT, 3.0);
        frame.polyline(up(cutters[k].bottom), INPUT, 1.5);
        frame.label(k == 0 ? fmt::format("column_cutters()[0]: {:.0f} thick", outline_thickness(cutters[0])) : fmt::format("[{}]", k), up(middle(cutters[k])));
    }

    frame.label(fmt::format("levels[1] = {:.3f}", corner.levels[1]), up(corner.axis_point + Vector(0.0, 0.0, corner.levels[1])));
    frame.label(fmt::format("levels[2] = {:.0f}", corner.levels[2]), up(corner.axis_point + Vector(0.0, 0.0, corner.levels[2])));
    frame.write(context.dir);
}

/// The four carved columns on their supports under the grey floor.
void four_columns(const Context& context) {

    const Floor& floor = context.members;
    Frame frame(CHAPTER, 138, "four_columns", "add_columns: add_column(q) for q = 0 .. 3, each carved by its own corner's six head cuts", "iso", FLOOR);
    grey_floor(frame, floor, {}, false);

    for (const ColumnModel& model : floor.members.columns) {
        frame.element(model.column, BUILT);
        frame.element(model.support, RESULT);
        frame.label(model.column->name, model.column->axis.center());
    }

    frame.write(context.dir);
}

/// Every member of the floor: the quarters in their family colours, the oculus ring and plates, and the columns on their supports.
void whole_floor(const Context& context) {

    const Floor& floor = context.members;
    Frame frame(CHAPTER, 139, "whole_floor", "add_members: four quarters in family colours, the oculus ring and plates, four columns", "iso", FLOOR);
    frame.key = true;

    for (size_t q = 0; q < 4; q++) {
        family_members(frame, floor, q, {FAMILIES.begin(), FAMILIES.end()});
        frame.label(floor.members.quarters[q].group->name, up(area_centroid(Polyline(context.guide.geometry[q].polygon).closed())), true);
    }

    for (const Member& member : floor.members.ring)
        frame.element(member.element, RING);

    for (size_t i = 4; i < 9; i++)
        frame.element(named(floor, fmt::format("oculus_{}", i)), RING);

    for (const ColumnModel& model : floor.members.columns) {
        frame.element(model.column, STEEL);
        frame.element(model.support, STEEL);
    }

    frame.label("oculus_0 .. oculus_3: the ring", up(middle(context.guide.oculus()[0])));
    frame.label(floor.members.columns[0].column->name, floor.members.columns[0].column->axis.center());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Branch
// ═══════════════════════════════════════════════════════════════════════════

/// The colour of a branch element by its name: its family, the oculus, the column, else a connector.
const Color& branch_color(const std::string& name) {

    for (const Family family : FAMILIES)
        if (name.rfind(FAMILY_NAMES[static_cast<size_t>(family)] + "_", 0) == 0)
            return family_color(family);

    if (name.rfind("oculus_", 0) == 0)
        return RING;

    if (name.rfind("column_", 0) == 0 || name.rfind("support_", 0) == 0)
        return STEEL;

    return CONNECTOR_COLOR;
}

/// The floor grey without quarter 0, and get_branch("quarter_0") of the connected floor raised above its place.
void branch(const Context& context) {

    const Floor& floor = context.members;
    const WoodSession part = context.connected.get_branch("quarter_0");
    const Xform raise = Xform::translation(0.0, 0.0, RAISE);
    const Polyline polygon = up(Polyline(context.guide.geometry[0].polygon).closed());
    const Point centre = area_centroid(polygon);
    std::set<const Element*> own;

    for (const Family family : FAMILIES)
        for (const Member& member : members(floor.members.quarters[0], family))
            own.insert(member.element.get());

    own.insert(floor.members.ring[0].element.get());
    own.insert(named(floor, "oculus_4").get());
    own.insert(floor.members.columns[0].column.get());
    own.insert(floor.members.columns[0].support.get());

    Frame frame(CHAPTER, 142, "get_branch", "get_branch(\"quarter_0\"): a WoodSession of the quarter_0 subtree alone, drawn raised above its place", "iso", BRANCH);
    frame.orbit = "0,-52"; // Looks down 15 deg instead of 30, so the raised quarter stands clear above the floor behind it.
    grey_floor(frame, floor, own, true);
    frame.polyline(polygon, GREY, 2.0); // Context, as the raise below: in a frame of family colours a role colour would read as a family.
    size_t count = 0;

    for (const std::shared_ptr<Element>& element : *part.objects.elements) {
        if (element->element_type_name() == "Joint")
            continue;

        const std::shared_ptr<Element> copy = element->clone();
        copy->place(raise);
        frame.element(copy, branch_color(element->name));
        count++;
    }

    frame.line(Line::from_points(centre, centre + Vector(0.0, 0.0, RAISE)), GREY, 4.0, true, true);
    frame.label(fmt::format("part: {} elements of quarter_0", part.objects.elements->size()), centre + Vector(0.0, 0.0, RAISE));
    frame.label(fmt::format("{} drawn, the support Joint left out", count), centre + Vector(0.0, 0.0, RAISE * 0.5));
    frame.label("oculus_8 stays: its group oculus is at the root", up(middle(context.guide.oculus()[8])));
    frame.write(context.dir);
}

}

void chapter_07_elements(const Context& context) {

    empty_floor(context);
    quarter_groups(context);
    lift(context);
    thickness(context);
    dispatch(context);
    bed_plates(context);
    family_names(context);
    tsection_plates(context);
    rib_stations(context);
    rib_interior(context);
    rib_ends(context);
    rib_axes(context);
    outer_ribs(context);
    inner_ribs(context);
    wedge_blocks(context);
    beam_caps(context);
    oculus_levels(context);
    ring_beams(context);
    bottom_wedges(context);
    central_plate(context);
    oculus_groups(context);
    support_foot(context);
    shaft_capitel(context);
    support_joint(context);
    head_carving(context);
    four_columns(context);
    whole_floor(context);
    branch(context);
}

}
