#include "docs/floor/movie.h"
#include "wood_brep_drill.h"

namespace movie {

namespace {

const std::string CHAPTER = "09_connectors";

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The corners of a polygon without its closing point.
std::vector<Point> corners(const Polyline& polygon) {

    std::vector<Point> points = polygon.get_points();

    if (polygon.is_closed() && !points.empty())
        points.pop_back();

    return points;
}

/// A camera box of half sizes dx, dy and dz about a point.
Box around(const Point& centre, double dx, double dy, double dz) {
    return {centre[0] - dx, centre[1] - dy, centre[2] - dz, centre[0] + dx, centre[1] + dy, centre[2] + dz};
}

/// The mean of every corner of a loop pair.
Point centre(const std::array<Polyline, 2>& loops) {

    std::vector<Point> points = corners(loops[0]);
    const std::vector<Point> far = corners(loops[1]);
    points.insert(points.end(), far.begin(), far.end());

    return Point::centroid(points);
}

/// A loop pair as edges: both loops and the sides from corner i of one to corner i of the other.
void wire(Frame& frame, const std::array<Polyline, 2>& loops, const Color& color, double width = 2.0, bool dashed = false) {

    const std::vector<Point> a = corners(loops[0]);
    const std::vector<Point> b = corners(loops[1]);

    for (size_t i = 0; i < a.size(); i++)
        frame.line(Line::from_points(a[i], a[(i + 1) % a.size()]), color, width, dashed);

    for (size_t i = 0; i < b.size(); i++)
        frame.line(Line::from_points(b[i], b[(i + 1) % b.size()]), color, width, dashed);

    for (size_t i = 0; i < std::min(a.size(), b.size()); i++)
        frame.line(Line::from_points(a[i], b[i]), color, width, dashed);
}

/// A guide outline lifted to the floor and moved by the transform, as edges.
void outline_wire(Frame& frame, const Outline& outline, const Color& color, const Xform& xform = Xform::identity()) {
    wire(frame, {up(outline.top).transformed(xform), up(outline.bottom).transformed(xform)}, color, 1.5);
}

/// A loop pair lofted into a solid, as a connector part of its own without cuts.
void solid(Frame& frame, const std::array<Polyline, 2>& loops, const Color& color, const std::string& name) {

    JointBeam body;
    body.parts = {loops};
    frame.element(std::make_shared<ConnectorPart>(body, 0, name), color);
}

/// Every part and dowel the connector nests under its node, in one colour.
void nested(Frame& frame, const JointBeam& connector, const Color& color) {

    for (const std::shared_ptr<Joint>& child : connector.children())
        frame.element(child, color);
}

/// The connector of the floor named name, among its connectors and its screws.
std::shared_ptr<JointBeam> named(const Floor& floor, const std::string& name) {

    for (const std::vector<std::shared_ptr<JointBeam>>* list : {&floor.connectors, &floor.screws})
        for (const std::shared_ptr<JointBeam>& connector : *list)
            if (connector->name == name)
                return connector;

    throw std::runtime_error("the floor has no connector named " + name);
}

/// The scene name of the element with that guid.
std::string name_of(const Floor& floor, const std::string& guid) {
    return floor.get_element<Element>(guid)->name;
}

/// The relationships add_connectors makes a connector of, in its order: every row of the kinds but the supports.
std::vector<Relationship> walked(const FloorGuide& guide, const std::vector<Relation>& kinds) {

    std::vector<Relationship> rows;

    for (const Relationship& row : relationships(guide))
        if (row.kind != Relation::support && std::find(kinds.begin(), kinds.end(), row.kind) != kinds.end())
            rows.push_back(row);

    return rows;
}

/// The screw kinds as add_screws asks for them.
std::vector<Relation> screw_kinds() {
    return {SCREW_RELATIONS.begin(), SCREW_RELATIONS.end()};
}

/// Where a relationship sits: its contact's area centroid, or its column axis for a cross lap, which has no contact.
Point pin(const Relationship& row, const FloorGuide& guide) {
    return row.contact.point_count() >= 3 ? area_centroid(row.contact) : up(guide.columns[row.seam_or_corner].axis_point);
}

/// Where a connector sits: its first part's centre, else its dowels' mean centre, else its first cutter's centre.
Point pin(const JointBeam& connector) {

    if (!connector.parts.empty())
        return centre(connector.parts[0]);

    if (!connector.drill_lines.empty()) {
        std::vector<Point> centres;
        for (const Line& line : connector.drill_lines)
            centres.push_back(line.center());
        return Point::centroid(centres);
    }

    return centre(connector.cutters.at(0).at(0));
}

/// The contact's top edge as JointBeam::wedge and JointBeam::dowels pick it: the longest edge whose middle is not below the corners' mean height, in the polygon's winding.
Line top_edge(const std::vector<Point>& points) {

    double mean = 0.0;

    for (const Point& point : points)
        mean += point[2] / static_cast<double>(points.size());

    Line best;
    bool best_top = false;
    double best_length = -1.0;

    for (size_t i = 0; i < points.size(); i++) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % points.size()];
        const bool top = 0.5 * (a[2] + b[2]) >= mean;
        const double length = (b - a).magnitude();

        if ((top && !best_top) || (top == best_top && length > best_length)) {
            best = Line::from_points(a, b);
            best_top = top;
            best_length = length;
        }
    }

    return best;
}

/// The plate's and the tie's origin as top_origin puts it: the centre of the box around the corners within max(1, 0.02 height) of the contact's top.
Point top_origin(const std::vector<Point>& points) {

    double top = -1e300;
    double bottom = 1e300;

    for (const Point& point : points) {
        top = std::max(top, point[2]);
        bottom = std::min(bottom, point[2]);
    }

    const double tolerance = std::max(1.0, 0.02 * (top - bottom));
    std::array<double, 3> low = {1e300, 1e300, 1e300};
    std::array<double, 3> high = {-1e300, -1e300, -1e300};

    for (const Point& point : points)
        if (top - point[2] <= tolerance)
            for (int i = 0; i < 3; i++) {
                low[i] = std::min(low[i], point[i]);
                high[i] = std::max(high[i], point[i]);
            }

    return Point(0.5 * (low[0] + high[0]), 0.5 * (low[1] + high[1]), 0.5 * (low[2] + high[2]));
}

/// How a hole ends against its dowel's end: blind when it stops there, else the run-on.
std::string run_on(const Point& hole, const Point& dowel) {

    const double run = hole.distance(dowel);

    return run < 1e-6 ? std::string("blind") : fmt::format("+ drill_overshoot {:g}", run);
}

/// The solid cut a joint stored on a member, null when it stored none.
const SolidCut* stored_cut(const Element& element, const std::string& joint) {

    const std::vector<SolidCut>* cuts = nullptr;

    if (const BeamVariable* beam = dynamic_cast<const BeamVariable*>(&element))
        cuts = &beam->solid_cuts;
    else if (const Plate* plate = dynamic_cast<const Plate*>(&element))
        cuts = &plate->solid_cuts;
    else if (const Column* column = dynamic_cast<const Column*>(&element))
        cuts = &column->solid_cuts;

    if (cuts)
        for (const SolidCut& cut : *cuts)
            if (cut.joint_guid == joint)
                return &cut;

    return nullptr;
}

/// The middle of the first stretch of the lines inside the solid, the first line's head when none enters it.
Point inside_point(const Mesh& solid, const std::vector<Line>& lines) {

    for (const Line& line : lines)
        for (const std::array<double, 2>& stretch : inside_stretches(solid, line)) {
            const double a = std::max(stretch[0], 0.0);
            const double b = std::min(stretch[1], line.length());

            if (b - a > 1e-6)
                return line.start() + line.to_vector().normalized() * (0.5 * (a + b));
        }

    return lines.front().start();
}

/// The lowest and the highest coordinate of the points along an axis from an origin.
std::array<double, 2> extent(const std::vector<Point>& points, const Point& origin, const Vector& axis) {

    std::array<double, 2> range = {1e300, -1e300};

    for (const Point& point : points) {
        range[0] = std::min(range[0], (point - origin).dot(axis));
        range[1] = std::max(range[1], (point - origin).dot(axis));
    }

    return range;
}

/// The eight corners of a box part.
std::vector<Point> box_corners(const std::array<Polyline, 2>& box) {

    std::vector<Point> points = corners(box[0]);
    const std::vector<Point> far = corners(box[1]);
    points.insert(points.end(), far.begin(), far.end());

    return points;
}

/// A box part's frame as JointBeam::cross_lap reads it: origin at its centre, x along the first side of its first loop, z along the last, y from the first loop to the second.
std::pair<Point, std::array<Vector, 3>> box_frame(const std::array<Polyline, 2>& box) {

    const std::vector<Point> near = corners(box[0]);
    const std::vector<Point> far = corners(box[1]);
    const Vector x = (near[1] - near[0]).normalized();
    const Vector z = (near[3] - near[0]).normalized();
    const Vector y = (Point::centroid(far) - Point::centroid(near)).normalized();

    return {Point::centroid(box_corners(box)), {x, y, z}};
}

/// The wedge factory's frame on a wedge relationship, as connector_of sizes it and JointBeam::wedge builds it.
struct WedgeFrame {
    double thickness = 0.0; // The thicker member.
    Line edge; // The contact's top edge.
    Vector normal; // The contact's Newell normal.
    std::array<Vector, 3> axes; // x along the edge, y the normal without its x part, z = x cross y.
    std::array<double, 2> margins = {0.0, 0.0}; // The stations length_margin in from both ends.
    std::array<double, 2> stations = {0.0, 0.0}; // The same, the one nearer the end plane moved onto it.
    Point origin; // The wedge centre on the top edge.
    double length = 0.0; // The wedge length.

    /// A point of the frame.
    Point at(double x, double y, double z) const {
        return origin + axes[0] * x + axes[1] * y + axes[2] * z;
    }
};

/// The frame JointBeam::wedge computes for the relationship, with length_margin = 1.5 thickness as connector_of passes it.
WedgeFrame wedge_frame(const Relationship& row, const FloorMembers& members) {

    WedgeFrame wedge;
    wedge.thickness = std::max(members.thickness(row.a), members.thickness(row.b));
    const std::vector<Point> points = corners(row.contact);
    wedge.edge = top_edge(points);
    wedge.normal = compute_newell(points).normalized();
    const Vector x = wedge.edge.to_vector().normalized();
    const Vector y = (wedge.normal - x * wedge.normal.dot(x)).normalized();
    wedge.axes = {x, y, x.cross(y)};
    const double margin = 1.5 * wedge.thickness;
    wedge.margins = {-0.5 * wedge.edge.length() + margin, 0.5 * wedge.edge.length() - margin};
    wedge.stations = wedge.margins;

    if (row.end) {
        Point hit;
        Intersection::line_plane(Line::from_points(wedge.edge.center(), wedge.edge.center() + x), *row.end, hit, false);
        const double station = (hit - wedge.edge.center()).dot(x);
        wedge.stations[std::abs(station - wedge.stations[0]) < std::abs(station - wedge.stations[1]) ? 0 : 1] = station;
    }

    wedge.origin = wedge.edge.center() + x * (0.5 * (wedge.stations[0] + wedge.stations[1]));
    wedge.length = std::max(row.end ? wedge.stations[1] - wedge.stations[0] : wedge.edge.length() - 2.0 * margin, 1e-6);

    return wedge;
}

/// The first relationship of a kind.
Relationship first(const FloorGuide& guide, Relation kind) {
    return walked(guide, {kind}).at(0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Entry and walk
// ═══════════════════════════════════════════════════════════════════════════

/// 169: the bay's members grey, the connectors the call makes in CONNECTOR_COLOR, one label per connector kind with its row count, and the total.
void entry(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 169, "add_connectors", fmt::format("add_connectors(kinds): one connector per relationship of the six CONNECTOR_RELATIONS kinds, {} here", context.connected.connectors.size()), "iso", BAY);

    for (size_t q = 0; q < 4; q++)
        quarter_members(frame, context.members, q, GREY);

    for (const std::shared_ptr<JointBeam>& connector : context.connected.connectors)
        nested(frame, *connector, CONNECTOR_COLOR);

    const std::array<std::pair<Relation, size_t>, 4> pinned = {{{Relation::seam_wedge, 0}, {Relation::oculus_wedge, 1}, {Relation::column_plate, 2}, {Relation::block_dowels, 20}}};

    for (const std::pair<Relation, size_t>& kind : pinned) {
        const std::vector<Relationship> rows = relationships(guide, kind.first);
        frame.label(fmt::format("{}: {} rows", relation_name(kind.first), rows.size()), pin(rows.at(kind.second), guide));
    }

    frame.label(fmt::format("cross_lap: {} rows, no contact", relationships(guide, Relation::cross_lap).size()), up(guide.columns[0].axis_point));
    frame.label(fmt::format("seam_tie: {} rows, seam_through_ribs = {}", relationships(guide, Relation::seam_tie).size(), guide.parameters.seam_through_ribs), up(guide.seams[2].line.center()));
    frame.label(fmt::format("Floor::connectors: {}", context.connected.connectors.size()), up(guide.centre));
    frame.write(context.dir);
}

/// 170: a dot per quarter-0 connector row at its contact, joined in the order the loop meets them.
void walk(const Context& context) {

    const FloorGuide& guide = context.guide;
    Frame frame(CHAPTER, 170, "walk", "The loop over relationships(guide), supports and kinds not asked for skipped: quarter 0's rows in order", "top", QUARTER);
    plan_context(frame, guide, true);
    const std::vector<Relationship> rows = walked(guide, CONNECTOR_RELATIONS);
    std::map<Relation, std::vector<size_t>> of_kind;
    std::optional<Point> previous;

    for (size_t n = 0; n < rows.size(); n++) {
        if (rows[n].seam_or_corner != 0)
            continue;

        const Point at = pin(rows[n], guide);
        frame.point(at, MARK, 14.0);

        if (previous)
            frame.line(Line::from_points(*previous, at), INK, 1.5, true, true);

        previous = at;
        of_kind[rows[n].kind].push_back(n);
    }

    for (const std::pair<const Relation, std::vector<size_t>>& kind : of_kind) {
        if (kind.second.size() > 2) {
            frame.label(fmt::format("{}..{}: {}", kind.second.front(), kind.second.back(), relation_name(kind.first)), pin(rows[kind.second.front()], guide));
            continue;
        }

        for (const size_t n : kind.second)
            frame.label(fmt::format("{}: {}", n, relation_name(kind.first)), pin(rows[n], guide));
    }

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Wedges
// ═══════════════════════════════════════════════════════════════════════════

/// 171: seam 0's contact seen along x, the 1.5 t margins at both ends of its top edge and the 2t/3 pocket depth.
void wedge_sizing(const Context& context) {

    const Relationship row = first(context.guide, Relation::seam_wedge);
    const WedgeFrame wedge = wedge_frame(row, context.members.members);
    const Vector x = wedge.axes[0];
    const double margin = 1.5 * wedge.thickness;
    const double pocket_depth = 2.0 * wedge.thickness / 3.0;
    const Point middle = wedge.edge.center();
    Frame frame(CHAPTER, 171, "wedge_sizing", "Seam wedge 0 sized by the thicker beam: 1.5 t cut back at both ends of the top edge, pockets 2t/3 deep", "right", around(middle + Vector(0.0, 0.0, -100.0), 100.0, 1150.0, 260.0));

    frame.polyline(row.contact, GREY, 2.0);
    frame.line(wedge.edge, INK, 3.0);
    frame.line(Line::from_points(wedge.edge.start(), wedge.edge.start() + x * margin), MARK, 7.0);
    frame.line(Line::from_points(wedge.edge.end() - x * margin, wedge.edge.end()), MARK, 7.0);
    frame.line(Line::from_points(middle, middle - Vector(0.0, 0.0, pocket_depth)), MARK, 3.0, false, true);

    frame.label(fmt::format("thickness t = max({:.2f}, {:.2f})", context.members.members.thickness(row.a), context.members.members.thickness(row.b)), area_centroid(row.contact) + x * 400.0);
    frame.label(fmt::format("length_margin = 1.5 t = {:.2f}", margin), wedge.edge.start() + x * (0.5 * margin));
    frame.label("length_margin", wedge.edge.end() - x * (0.5 * margin));
    frame.label(fmt::format("pocket_depth = 2t/3 = {:.2f}", pocket_depth), middle - Vector(0.0, 0.0, pocket_depth));
    frame.write(context.dir);
}

/// 172: seam 0's contact with its top edge and the wedge frame x, y, z at the edge centre.
void wedge_axes(const Context& context) {

    const Relationship row = first(context.guide, Relation::seam_wedge);
    const WedgeFrame wedge = wedge_frame(row, context.members.members);
    const Point at = wedge.edge.center();
    Frame frame(CHAPTER, 172, "wedge_frame", "The wedge frame: x along the contact's top edge, y its Newell normal without the x part, z = x cross y", "iso", around(at + Vector(0.0, 0.0, -80.0), 350.0, 650.0, 260.0));

    frame.polyline(row.contact, GREY, 2.0);
    frame.line(wedge.edge, MARK, 5.0);

    const std::array<std::string, 3> names = {"x = edge direction", "y = normal - x (normal . x)", "z = x cross y"};

    for (size_t i = 0; i < 3; i++) {
        frame.line(Line::from_points(at, at + wedge.axes[i] * 220.0), i == 0 ? MARK : INK, 3.0, false, true);
        frame.label(names[i], at + wedge.axes[i] * 220.0);
    }

    frame.label(fmt::format("edge = top_edge(points), {:.0f} long", wedge.edge.length()), wedge.edge.start() + wedge.axes[0] * (0.2 * wedge.edge.length()));
    frame.write(context.dir);
}

/// 173: the two margin stations on the top edge, the nearer one moved onto the end plane, and the span that is left.
void wedge_stations(const Context& context) {

    const Relationship row = first(context.guide, Relation::seam_wedge);
    const WedgeFrame wedge = wedge_frame(row, context.members.members);
    const Point centre_point = wedge.edge.center();
    const Vector x = wedge.axes[0];
    const Vector rise(0.0, 0.0, 40.0);
    const size_t moved = wedge.stations[0] != wedge.margins[0] ? 0 : 1;
    const double middle_station = 0.5 * (wedge.stations[0] + wedge.stations[1]);
    Frame frame(CHAPTER, 173, "wedge_stations", "Stations 1.5 t in from both edge ends; the one nearer the end plane moves onto it", "right", around(centre_point + Vector(0.0, 0.0, -60.0), 100.0, 1150.0, 220.0));

    frame.line(wedge.edge, GREY, 3.0);

    for (const double station : wedge.margins)
        frame.line(Line::from_points(centre_point + x * station - rise, centre_point + x * station + rise), INK, 3.0);

    const Point hit = centre_point + x * wedge.stations[moved];
    frame.line(Line::from_points(hit - Vector(0.0, 0.0, 260.0), hit + Vector(0.0, 0.0, 80.0)), INK, 2.0, true);
    frame.line(Line::from_points(centre_point + x * wedge.margins[moved] + rise, hit + rise), MARK, 3.0, false, true);
    frame.line(Line::from_points(centre_point + x * wedge.stations[0] - rise * 0.5, centre_point + x * wedge.stations[1] - rise * 0.5), MARK, 7.0);
    frame.point(wedge.origin, MARK, 16.0);

    frame.label(fmt::format("stations[{}] -> end: {:.2f}", moved, wedge.stations[moved]), hit + rise);
    frame.label(fmt::format("stations[{}] = {:.2f}", 1 - moved, wedge.stations[1 - moved]), centre_point + x * wedge.stations[1 - moved] + rise);
    frame.label(fmt::format("origin = edge.center() + x * {:.2f}", middle_station), wedge.origin);
    frame.label(fmt::format("length = {:.2f}", wedge.length), centre_point + x * (0.5 * (wedge.stations[moved] + middle_station)) - rise * 0.5);
    frame.label("end = edges[0].band[0]", hit - Vector(0.0, 0.0, 240.0));
    frame.write(context.dir);
}

/// 174: the full WEDGE_PROFILE dashed at the wedge's near end, the cut at the top edge's level and the kept prism.
void wedge_profile(const Context& context) {

    const WedgeFrame wedge = wedge_frame(first(context.guide, Relation::seam_wedge), context.members.members);
    const std::shared_ptr<JointBeam> connector = named(context.connected, "connector_wedge_0");
    const double front = -0.5 * wedge.length - 5.0;
    const std::array<std::array<double, 2>, 3>& profile = JointBeam::WEDGE_PROFILE;
    Frame frame(CHAPTER, 174, "wedge_profile", "below_top cuts WEDGE_PROFILE level with the top edge; the kept triangle runs the wedge length", "front", around(wedge.at(-0.5 * wedge.length, 0.0, -90.0), 110.0, 60.0, 150.0));

    solid(frame, connector->parts[0], CONNECTOR_COLOR, "connector_wedge_0_part");

    for (size_t i = 0; i < 3; i++)
        frame.line(Line::from_points(wedge.at(front, profile[i][0], profile[i][1]), wedge.at(front, profile[(i + 1) % 3][0], profile[(i + 1) % 3][1])), INK, 2.0, true);

    frame.line(Line::from_points(wedge.at(front, -48.0, 0.0), wedge.at(front, 48.0, 0.0)), MARK, 4.0);

    frame.label(fmt::format("apex: WEDGE_PROFILE[0] = ({:g}, {:g})", profile[0][0], profile[0][1]), wedge.at(front, profile[0][0], profile[0][1]));
    frame.label(fmt::format("corner ({:.2f}, {:.2f}) dropped", profile[1][0], profile[1][1]), wedge.at(front, profile[1][0], profile[1][1]));
    frame.label(fmt::format("cut at the top edge, z {:.0f}", wedge.origin[2]), wedge.at(front, 48.0, 0.0));
    frame.label("joint->parts[0]", wedge.at(front, 0.0, -70.0));
    frame.write(context.dir);
}

/// 175: the seam beams' outlines, the wedge and its dowels across seam 0, spaced length / count.
void wedge_dowels(const Context& context) {

    const WedgeFrame wedge = wedge_frame(first(context.guide, Relation::seam_wedge), context.members.members);
    const std::shared_ptr<JointBeam> connector = named(context.connected, "connector_wedge_0");
    const std::vector<Line>& dowels = connector->drill_lines;
    Frame frame(CHAPTER, 175, "wedge_dowels", "Wedge dowels: count = int(length / dowel_spacing), one in the middle of each share, clipped flush", "top", around(wedge.origin + Vector(0.0, 0.0, -50.0), 260.0, 1000.0, 120.0));
    frame.key = true;

    outline_wire(frame, context.guide.quarter(0).inner_beams()[0], GREY);
    outline_wire(frame, context.guide.quarter(1).inner_beams()[2], GREY);
    solid(frame, connector->parts[0], CONNECTOR_COLOR, "connector_wedge_0_part");

    for (const Line& dowel : dowels)
        frame.line(dowel, MARK, 6.0);

    frame.label(connector->name + "_dowel_0", dowels.front().start());
    frame.label(fmt::format("length / count = {:.2f} / {} = {:.2f}", wedge.length, dowels.size(), wedge.length / static_cast<double>(dowels.size())), Point::centroid({dowels[0].center(), dowels[1].center()}));
    frame.label(fmt::format("flush_dowel: {:.0f} long, line_radius {:g}", dowels.back().length(), connector->line_radius), dowels.back().end());
    frame.write(context.dir);
}

/// 176: the two seam beam sections, the wedge triangle and the pocket box each beam gets, seen along the seam.
void wedge_pockets(const Context& context) {

    const WedgeFrame wedge = wedge_frame(first(context.guide, Relation::seam_wedge), context.members.members);
    const std::shared_ptr<JointBeam> connector = named(context.connected, "connector_wedge_0");
    const Vector ahead = wedge.axes[0] * -5.0;
    Frame frame(CHAPTER, 176, "wedge_pockets", "Wedge pockets: a box under each slanted face, pocket_depth thick; each beam gets the one on its own side", "front", around(wedge.at(-0.5 * wedge.length, 0.0, -80.0), 130.0, 60.0, 140.0));

    outline_wire(frame, context.guide.quarter(0).inner_beams()[0], GREY);
    outline_wire(frame, context.guide.quarter(1).inner_beams()[2], GREY);
    solid(frame, connector->cutters[0][0], MARK, "pocket_a");
    solid(frame, connector->cutters[1][0], CONNECTOR_COLOR, "pocket_b");

    const std::vector<Point> triangle = corners(connector->parts[0][0]);

    for (size_t i = 0; i < triangle.size(); i++)
        frame.line(Line::from_points(triangle[i] + ahead, triangle[(i + 1) % triangle.size()] + ahead), INK, 3.0);

    for (size_t side = 0; side < 2; side++) {
        const std::vector<Point> face = corners(connector->cutters[side][0][0]);
        const Point top = *std::max_element(face.begin(), face.end(), [](const Point& a, const Point& b) { return a[2] < b[2]; });
        frame.label(fmt::format("cutters[{}] -> {}: {:.2f} above the top", side, name_of(context.connected, connector->targets[side]), top[2] - wedge.origin[2]), top + ahead);
    }

    frame.label("joint->parts[0]", triangle.front() + ahead);
    frame.write(context.dir);
}

/// 177: oculus wedge 0 turned so the view looks along oculus edge 0: the tilted contact and the wedge cut flush.
void oculus_wedge(const Context& context) {

    const Relationship row = first(context.guide, Relation::oculus_wedge);
    const std::shared_ptr<JointBeam> connector = named(context.connected, "connector_wedge_4");
    const Line edge = up(context.guide.oculus_edges[0].line);
    const Vector d = edge.to_vector().normalized();
    const Line vertical = Line::from_points(edge.center(), edge.center() + Vector(0.0, 0.0, 1.0));
    Xform turn = Xform::rotation_around_line(vertical, 0.5 * M_PI - std::atan2(d[1], d[0]));

    if (std::abs(d.transformed(turn)[0]) > 1e-6)
        turn = Xform::rotation_around_line(vertical, std::atan2(d[1], d[0]) - 0.5 * M_PI);

    const std::vector<Point> part = corners(connector->parts[0][0]);
    const Point apex = *std::min_element(part.begin(), part.end(), [](const Point& a, const Point& b) { return a[2] < b[2]; });
    const Point top = *std::max_element(part.begin(), part.end(), [](const Point& a, const Point& b) { return a[2] < b[2]; });
    Frame frame(CHAPTER, 177, "oculus_wedge", "Oculus wedge 0 seen along its edge: the tilted contact turns the profile, the cut stays flush", "front", around(edge.center().transformed(turn) + Vector(0.0, 0.0, -100.0), 150.0, 80.0, 160.0));

    outline_wire(frame, context.guide.quarter(0).inner_beams()[1], GREY, turn);
    outline_wire(frame, context.guide.oculus()[0], GREY, turn);
    frame.polyline(row.contact.transformed(turn), MARK, 4.0);
    solid(frame, {connector->parts[0][0].transformed(turn), connector->parts[0][1].transformed(turn)}, CONNECTOR_COLOR, "connector_wedge_4_part");

    frame.label("row.plane = oculus_edges[0].tilted", area_centroid(row.contact).transformed(turn));
    frame.label(fmt::format("flush cut at z {:.2f}", top[2]), top.transformed(turn));
    frame.label(fmt::format("apex z {:.2f}", apex[2]), apex.transformed(turn));
    frame.label(name_of(context.connected, connector->targets[0]), up(middle(context.guide.quarter(0).inner_beams()[1])).transformed(turn));
    frame.label(name_of(context.connected, connector->targets[1]), up(middle(context.guide.oculus()[0])).transformed(turn));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Column plates and cross laps
// ═══════════════════════════════════════════════════════════════════════════

/// 178: corner 0's first plate contact, its Newell normal dashed, the horizontal x toward the rib, y, z and the origin.
void plate_frame(const Context& context) {

    const Relationship row = first(context.guide, Relation::column_plate);
    const std::vector<Point> points = corners(row.contact);
    const Vector normal = compute_newell(points).normalized();
    const Point toward = context.members.members.get(row.b)->model_geometry_mesh().centroid();
    const Point middle_point = Point::centroid(points);
    const Vector inward(toward[0] - middle_point[0], toward[1] - middle_point[1], 0.0);
    Vector x(normal[0], normal[1], 0.0);
    x = x.magnitude() < 1e-9 ? inward.normalized() : x.normalized();

    if (x.dot(inward) < 0.0)
        x = -x;

    const Vector z(0.0, 0.0, 1.0);
    const Vector y = z.cross(x).normalized();
    const Point origin = top_origin(points);
    Frame frame(CHAPTER, 178, "plate_frame", "Plate frame: x the contact normal made horizontal, toward the rib; z up; origin the top edge's centre", "iso", around(origin + Vector(0.0, 0.0, -60.0), 330.0, 330.0, 260.0));

    frame.polyline(row.contact, GREY, 3.0);
    frame.line(Line::from_points(middle_point, middle_point + normal * 220.0), INK, 2.0, true, true);
    frame.line(Line::from_points(origin, origin + x * 260.0), MARK, 4.0, false, true);
    frame.line(Line::from_points(origin, origin + y * 160.0), INK, 3.0, false, true);
    frame.line(Line::from_points(origin, origin + z * 160.0), INK, 3.0, false, true);
    frame.point(origin, MARK, 16.0);

    frame.label("normal = compute_newell(points)", middle_point + normal * 220.0);
    frame.label(fmt::format("x: toward {}", context.members.members.get(row.b)->name), origin + x * 260.0);
    frame.label("y = Z cross x", origin + y * 160.0);
    frame.label("z = Z", origin + z * 160.0);
    frame.label("origin = top_origin(points)", origin);
    frame.write(context.dir);
}

/// 179: the plate box of connector_0 on its contact and its pocket dashed, overshoot higher.
void plate_part(const Context& context) {

    const Relationship row = first(context.guide, Relation::column_plate);
    const std::shared_ptr<JointBeam> plate = named(context.connected, "connector_0");
    const std::vector<Point> near = corners(plate->parts[0][0]);
    const std::vector<Point> far = corners(plate->parts[0][1]);
    const std::vector<Point> pocket = corners(plate->cutters[0][0][0]);
    const Point origin = top_origin(corners(row.contact));
    const Point back_end = Point::centroid({near[3], far[3]});
    const Point front_end = Point::centroid({near[2], far[2]});
    Frame frame(CHAPTER, 179, "plate_part", "frame_box: plate back into the column, front into the rib, height down; pocket overshoot higher", "iso", around(centre(plate->parts[0]), 340.0, 220.0, 230.0));

    frame.polyline(row.contact, GREY, 3.0);
    solid(frame, plate->parts[0], CONNECTOR_COLOR, "connector_0_part");
    wire(frame, plate->cutters[0][0], INK, 1.5, true);

    frame.label(fmt::format("back = {:.0f} into {}", origin.distance(back_end), name_of(context.connected, plate->targets[0])), Point::centroid({origin, back_end}));
    frame.label(fmt::format("front = {:.0f} into {}", origin.distance(front_end), name_of(context.connected, plate->targets[1])), Point::centroid({origin, front_end}));
    frame.label(fmt::format("height = {:.0f}", far[2].distance(far[1])), Point::centroid({far[2], far[1]}));
    frame.label(fmt::format("width = {:.0f}", near[1].distance(far[1])), Point::centroid({near[1], far[1]}));
    frame.label(fmt::format("pocket: overshoot = {:.0f}", pocket[3][2] - near[3][2]), pocket[3]);
    frame.write(context.dir);
}

/// 180: the plate face-on with its four dowels and the margins that place them.
void plate_dowels(const Context& context) {

    const std::shared_ptr<JointBeam> plate = named(context.connected, "connector_0");
    const std::vector<Point> near = corners(plate->parts[0][0]);
    const std::vector<Point> far = corners(plate->parts[0][1]);
    const Vector x = (near[1] - near[0]).normalized();
    const Vector y = (far[0] - near[0]).normalized();
    const Vector z(0.0, 0.0, 1.0);
    const Vector ahead = y * -60.0;
    const Point back_top = Point::centroid({near[3], far[3]});
    const Point dowel = plate->drill_lines[0].center();
    const double along = (dowel - back_top).dot(x);
    const double down = (back_top - dowel).dot(z);
    Frame frame(CHAPTER, 180, "plate_dowels", "Plate dowels: stations margin_x radii in from the ends, levels margin_z radii in from top and bottom", "front", around(centre(plate->parts[0]), 300.0, 150.0, 175.0));

    solid(frame, plate->parts[0], CONNECTOR_COLOR, "connector_0_part");

    for (const Line& line : plate->drill_lines)
        frame.element(std::make_shared<Dowel>(line, plate->line_radius, plate->chord_tolerance), MARK);

    frame.line(Line::from_points(dowel - x * along + ahead, dowel + ahead), INK, 2.0, false, true);
    frame.line(Line::from_points(dowel + z * down + ahead, dowel + ahead), INK, 2.0, false, true);

    frame.label(fmt::format("margin_x * dowel_radius = {:.2f}", along), dowel - x * (0.5 * along) + ahead);
    frame.label(fmt::format("margin_z * dowel_radius = {:.2f}", down), dowel + z * (0.5 * down) + ahead);
    frame.label(fmt::format("{}_dowel_0: d{:g}, {:.0f} long", plate->name, 2.0 * plate->line_radius, plate->drill_lines[0].length()), plate->drill_lines[0].start());
    frame.label(fmt::format("{}_dowel_3", plate->name), plate->drill_lines[3].start());
    frame.write(context.dir);
}

/// 181: corner 0's two plates crossing in the carved column, kept in plates_of_corner for the cross lap.
void corner_plates(const Context& context) {

    const std::shared_ptr<JointBeam> plate_0 = named(context.connected, "connector_0");
    const std::shared_ptr<JointBeam> plate_1 = named(context.connected, "connector_1");
    const Point crossing = centre(named(context.connected, "connector_cross_lap_0")->cutters[0][0]);
    Frame frame(CHAPTER, 181, "plates_of_corner", "Each column plate goes to plates_of_corner[corner]; the cross lap is built from those two, not the ribs", "top", around(Point::centroid({centre(plate_0->parts[0]), centre(plate_1->parts[0])}), 330.0, 330.0, 200.0));

    frame.element(context.connected.members.columns[0].column, GREY);
    solid(frame, plate_0->parts[0], CONNECTOR_COLOR, "connector_0_part");
    solid(frame, plate_1->parts[0], CONNECTOR_COLOR, "connector_1_part");

    frame.label("plates_of_corner[0][0] = connector_0", centre(plate_0->parts[0]));
    frame.label("plates_of_corner[0][1] = connector_1", centre(plate_1->parts[0]));
    frame.label("JointBeam::cross_lap(*plates[0], *plates[1])", crossing);
    frame.write(context.dir);
}

/// 182: both plates seen along y, the common height between low and high and the lap level that splits it.
void cross_lap_level(const Context& context) {

    const std::shared_ptr<JointBeam> a = named(context.connected, "connector_0");
    const std::shared_ptr<JointBeam> b = named(context.connected, "connector_1");
    const std::shared_ptr<JointBeam> cross = named(context.connected, "connector_cross_lap_0");
    const std::pair<Point, std::array<Vector, 3>> frame_a = box_frame(a->parts[0]);
    const std::pair<Point, std::array<Vector, 3>> frame_b = box_frame(b->parts[0]);
    const std::array<double, 2> z_a = extent(box_corners(a->parts[0]), frame_a.first, frame_a.second[2]);
    const std::array<double, 2> z_b = extent(box_corners(b->parts[0]), frame_a.first, frame_a.second[2]);
    const std::array<double, 2> span = extent(box_corners(a->parts[0]), frame_a.first, frame_a.second[0]);
    const double low = std::max(z_a[0], z_b[0]);
    const double high = std::min(z_a[1], z_b[1]);
    const double lap = extent(box_corners(cross->cutters[0][0]), frame_a.first, frame_a.second[2])[0];
    const Vector ahead = frame_a.second[1] * -80.0;
    Frame frame(CHAPTER, 182, "cross_lap_level", "Cross lap: common height low..high along frame_a z, split at lap = low + share (high - low)", "front", around(frame_a.first, 330.0, 200.0, 190.0));

    solid(frame, a->parts[0], CONNECTOR_COLOR, "connector_0_part");
    wire(frame, b->parts[0], CONNECTOR_COLOR, 3.0);

    const std::array<std::pair<double, std::string>, 3> levels = {{{low, fmt::format("low = max(z_a[0], z_b[0]) = {:.1f}", low)}, {high, fmt::format("high = min(z_a[1], z_b[1]) = {:.1f}", high)}, {lap, fmt::format("lap = {:.1f}: z {:.0f}, share {:g}", lap, frame_a.first[2] + lap, (lap - low) / (high - low))}}};

    for (size_t i = 0; i < levels.size(); i++) {
        const Point left = frame_a.first + frame_a.second[0] * (span[0] - 40.0) + frame_a.second[2] * levels[i].first + ahead;
        const Point right = frame_a.first + frame_a.second[0] * (span[1] + 40.0) + frame_a.second[2] * levels[i].first + ahead;
        frame.line(Line::from_points(left, right), i == 2 ? MARK : INK, i == 2 ? 4.0 : 2.0, i != 2);
        frame.label(levels[i].second, i == 2 ? left : right);
    }

    frame.point(frame_a.first + ahead, INK, 12.0);
    frame.point(frame_b.first + ahead, INK, 12.0);
    frame.label("frame_a origin", frame_a.first + ahead);
    frame.label("frame_b origin", frame_b.first + ahead);
    frame.write(context.dir);
}

/// 183: the two plates as edges, the slot cut from the top of connector_0 and the slot cut from the bottom of connector_1.
void cross_lap_slots(const Context& context) {

    const std::shared_ptr<JointBeam> lap = named(context.connected, "connector_cross_lap_0");
    const std::shared_ptr<JointBeam> a = named(context.connected, "connector_0");
    const std::shared_ptr<JointBeam> b = named(context.connected, "connector_1");
    const Point slot_a = centre(lap->cutters[0][0]);
    const Point slot_b = centre(lap->cutters[1][0]);
    const Point meet = Point::centroid({slot_a, slot_b});
    Frame frame(CHAPTER, 183, "cross_lap_slots", "Cross lap slots: connector_0 cut from the lap up through its top, connector_1 from its bottom to the lap", "iso", around(meet, 160.0, 160.0, 170.0));
    frame.key = true;

    wire(frame, a->parts[0], CONNECTOR_COLOR, 2.0);
    wire(frame, b->parts[0], CONNECTOR_COLOR, 2.0);
    solid(frame, lap->cutters[0][0], MARK, "slot_a");
    solid(frame, lap->cutters[1][0], MARK, "slot_b");

    frame.label(fmt::format("cutters[0]: slot in {}, lap to top", name_of(context.connected, lap->targets[0])), slot_a);
    frame.label(fmt::format("cutters[1]: slot in {}, bottom to lap", name_of(context.connected, lap->targets[1])), slot_b);
    frame.label(fmt::format("lap: z {:.0f}", meet[2]), meet);
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Ties, on the tied bay
// ═══════════════════════════════════════════════════════════════════════════

/// The two outer ribs of the tied bay's seam 0 as edges.
void tied_ribs(Frame& frame, const Floor& tied) {

    outline_wire(frame, tied.guide.quarter(0).outer_ribs()[0], GREY);
    outline_wire(frame, tied.guide.quarter(1).outer_ribs()[1], GREY);
}

/// 184: the tied bay's seam 0 contact and the tie frame: x down, y the contact normal made horizontal, z across the rib.
void tie_frame(const Context& context, const Floor& tied) {

    const Relationship row = first(tied.guide, Relation::seam_tie);
    const std::vector<Point> points = corners(row.contact);
    const Vector normal = compute_newell(points).normalized();
    const Vector x(0.0, 0.0, -1.0);
    const Vector y = Vector(normal[0], normal[1], 0.0).normalized();
    const Vector z = x.cross(y);
    const Point origin = top_origin(points);
    Frame frame(CHAPTER, 184, "tie_frame", "Tie frame on the tied bay's seam 0: x down, y the contact normal made horizontal, z = x cross y", "iso", around(origin + Vector(0.0, 0.0, -120.0), 350.0, 300.0, 230.0));

    tied_ribs(frame, tied);
    frame.polyline(row.contact, GREY, 3.0);

    const std::array<Vector, 3> axes = {x, y, z};
    const std::array<std::string, 3> names = {"x = (0, 0, -1): down", "y: contact normal, horizontal", "z = x cross y"};

    for (size_t i = 0; i < 3; i++) {
        frame.line(Line::from_points(origin, origin + axes[i] * 200.0), MARK, 3.0, false, true);
        frame.label(names[i], origin + axes[i] * 200.0);
    }

    frame.point(origin, MARK, 14.0);
    frame.label("origin = top_origin(points)", origin);
    frame.write(context.dir);
}

/// 185: the bow-tie key from above: two heads and the neck over the seam, its underside deepening toward the ends.
void tie_key(const Context& context, const Floor& tied) {

    const std::shared_ptr<JointBeam> tie = tied.connectors.at(0);
    const Relationship row = first(tied.guide, Relation::seam_tie);
    const Point origin = top_origin(corners(row.contact));
    const std::vector<Point> head = corners(tie->parts[0][0]);
    const std::vector<Point> neck_end = corners(tie->parts[1][1]);
    const std::vector<Point> neck = corners(tie->parts[1][0]);
    const Vector across = (head[3] - head[0]).normalized();
    Frame frame(CHAPTER, 185, "tie_key", "The tie key: four lofted pieces, heads at both ends and the neck across the seam, from above", "top", around(origin, 440.0, 80.0, 120.0));

    tied_ribs(frame, tied);
    frame.line(Line::from_points(origin - across * 200.0, origin + across * 200.0), INK, 2.0, true);

    for (size_t i = 0; i < tie->parts.size(); i++)
        solid(frame, tie->parts[i], CONNECTOR_COLOR, fmt::format("{}_part_{}", tie->name, i));

    frame.label(fmt::format("head: head_width {:.0f}, {:.0f} long", head[0].distance(head[3]), Point::centroid(head).distance(Point::centroid(corners(tie->parts[0][1])))), centre(tie->parts[0]));
    frame.label(fmt::format("neck: neck_width {:.0f}", neck[0].distance(neck[3])), centre(tie->parts[1]));
    frame.label(fmt::format("underside {:.2f} deep at the seam, {:.2f} at the ends", neck_end[0].distance(neck_end[1]), head[0].distance(head[1])), centre(tie->parts[3]));
    frame.label("seam", origin + across * 200.0);
    frame.write(context.dir);
}

/// 186: the flat-bottomed pockets, head and neck boxes per rib, the necks overlapping across the seam.
void tie_pockets(const Context& context, const Floor& tied) {

    const std::shared_ptr<JointBeam> tie = tied.connectors.at(0);
    const Relationship row = first(tied.guide, Relation::seam_tie);
    const Point origin = top_origin(corners(row.contact));
    const std::array<Color, 2> colors = {MARK, CONNECTOR_COLOR};
    Frame frame(CHAPTER, 186, "tie_pockets", "Tie pockets: a head box and a neck box per rib down to a flat floor, each neck overshoot past the seam", "top", around(origin, 440.0, 80.0, 120.0));

    tied_ribs(frame, tied);

    for (size_t side = 0; side < 2; side++) {
        for (const std::array<Polyline, 2>& box : tie->cutters[side])
            wire(frame, box, colors[side], 3.0);

        frame.label(fmt::format("cutters[{}] -> {}", side, name_of(tied, tie->targets[side])), centre(tie->cutters[side][0]));
    }

    const Vector normal = compute_newell(corners(row.contact)).normalized();
    const Vector y = Vector(normal[0], normal[1], 0.0).normalized();
    const std::array<double, 2> neck_a = extent(box_corners(tie->cutters[0][1]), origin, y);
    const std::array<double, 2> neck_b = extent(box_corners(tie->cutters[1][1]), origin, y);
    const double overlap = std::min(neck_a[1], neck_b[1]) - std::max(neck_a[0], neck_b[0]);
    frame.label(fmt::format("necks overlap {:.0f} = 2 overshoot", overlap), origin);
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Block dowels and screws
// ═══════════════════════════════════════════════════════════════════════════

/// 187: the first block contact face-on and the ring inset from it, whose corners take the dowels.
void dowels_inset(const Context& context) {

    const Relationship row = first(context.guide, Relation::block_dowels);
    const std::shared_ptr<JointBeam> dowels = named(context.connected, "connector_dowels_0");
    const std::vector<Point> points = corners(row.contact);
    const Point origin = Point::centroid(points);
    std::vector<Point> ring;

    for (const Line& line : dowels->drill_lines)
        ring.push_back(line.center());

    double offset = 1e300;

    for (size_t i = 0; i < points.size(); i++) {
        const Line side = Line::from_points(points[i], points[(i + 1) % points.size()]);
        offset = std::min(offset, ring[0].distance(side.closest_point(ring[0]).second));
    }

    const std::array<double, 2> xs = extent(points, origin, Vector(1.0, 0.0, 0.0));
    const std::array<double, 2> zs = extent(points, origin, Vector(0.0, 0.0, 1.0));
    Frame frame(CHAPTER, 187, "dowels_inset", "Block dowels: the contact inset by offset with Clipper2; the ring's corners take the dowels", "front", {origin[0] + xs[0] - 60.0, origin[1] - 100.0, origin[2] + zs[0] - 60.0, origin[0] + xs[1] + 60.0, origin[1] + 100.0, origin[2] + zs[1] + 60.0});

    frame.polyline(row.contact, GREY, 3.0);
    frame.polyline(Polyline(ring).closed(), MARK, 3.0);
    frame.point(origin, INK, 12.0);

    frame.label(fmt::format("contact: {} | {}", row.a.name(), row.b.name()), points[0]);
    frame.label(fmt::format("inset_polygon: offset = {:.0f}", offset), Point::centroid({ring[0], ring[1]}));
    frame.label("origin = centroid of the corners", origin);
    frame.write(context.dir);
}

/// 188: rib and block as edges, the contact between them and the four dowels crossing it, start in the rib and end in the block.
void dowels_axes(const Context& context) {

    const Relationship row = first(context.guide, Relation::block_dowels);
    const std::shared_ptr<JointBeam> dowels = named(context.connected, "connector_dowels_0");
    const std::vector<Line>& lines = dowels->drill_lines;
    Frame frame(CHAPTER, 188, "dowels_axes", "Dowels at the inset corners, centred on the contact along its normal: half in the rib, half in the block", "iso", around(area_centroid(row.contact), 250.0, 150.0, 340.0));

    outline_wire(frame, outlines(context.guide.quarter(static_cast<size_t>(row.a.quarter)), row.a.family).at(row.a.index), GREY);
    outline_wire(frame, outlines(context.guide.quarter(static_cast<size_t>(row.b.quarter)), row.b.family).at(row.b.index), GREY);
    frame.polyline(row.contact, INK, 2.0);

    for (const Line& line : lines)
        frame.element(std::make_shared<Dowel>(line, dowels->line_radius, dowels->chord_tolerance), MARK);

    frame.label(fmt::format("{}_dowel_0: d{:g}, {:.0f} long", dowels->name, 2.0 * dowels->line_radius, lines[0].length()), lines[0].center());
    frame.label(fmt::format("start: half in {}", row.a.name()), lines[1].start());
    frame.label(fmt::format("end: half in {}", row.b.name()), lines[2].end());
    frame.write(context.dir);
}

/// 189: quarter 0's first rib_corner screw connector: its three targets in their family colours, each named a step from the screws toward its own middle, and its two drill lines.
void screws_factory(const Context& context) {

    const std::vector<Relationship> rows = walked(context.guide, screw_kinds());
    size_t index = 0;

    while (rows.at(index).kind != Relation::screw_rib_corner || rows[index].seam_or_corner != 0)
        index++;

    const Relationship& row = rows[index];
    const std::shared_ptr<JointBeam> screws = context.connected.screws.at(index);
    const std::vector<Line>& lines = screws->drill_lines;
    std::vector<MemberRef> passed = {row.a, row.b};
    passed.insert(passed.end(), row.through.begin(), row.through.end());
    const Point at = lines[0].center();
    Frame frame(CHAPTER, 189, "screws_factory", fmt::format("JointBeam::screws: pre_drill, every passed member a target ({} here), no cutters", passed.size()), "iso", around(at, 280.0, 280.0, 230.0));
    frame.distance = 0.6;

    for (const MemberRef& ref : passed) {
        const std::shared_ptr<Element> member = context.members.members.get(ref);
        const Vector along = (member->model_geometry_mesh().centroid() - at).normalized();
        frame.element(member, static_cast<size_t>(ref.family) < FAMILY_COLORS.size() ? FAMILY_COLORS[static_cast<size_t>(ref.family)] : GREY);
        frame.label(fmt::format("target {}", ref.name()), inside_point(member->element_geometry_mesh(), {Line::from_points(at + along * 150.0, at + along * 400.0)}));
    }

    for (const Line& line : lines)
        frame.line(line, MARK, 4.0, false, true);

    frame.label(fmt::format("{}: {} drill lines, r {:g}, {:.0f} long", screws->name, lines.size(), screws->line_radius, lines[0].length()), lines.back().end());
    frame.write(context.dir);
}

/// 190: quarter 0's connectors from above, each named <prefix>_<n> by its place within its prefix.
void naming(const Context& context) {

    const std::vector<Relationship> rows = walked(context.guide, CONNECTOR_RELATIONS);
    const std::vector<Relationship> screw_rows = walked(context.guide, screw_kinds());
    const std::set<std::string> shown = {"connector_wedge_0", "connector_wedge_4", "connector_0", "connector_1", "connector_cross_lap_0", "connector_dowels_0", "connector_dowels_5", "connector_screws_0"};
    Frame frame(CHAPTER, 190, "naming", "Names: connector_prefix(kind) + _n, n counted per prefix across the quarters in walk order", "top", QUARTER);
    plan_context(frame, context.guide, true);

    for (size_t n = 0; n < rows.size(); n++) {
        if (rows[n].seam_or_corner != 0)
            continue;

        nested(frame, *context.connected.connectors.at(n), CONNECTOR_COLOR);

        if (shown.count(context.connected.connectors[n]->name))
            frame.label(context.connected.connectors[n]->name, pin(*context.connected.connectors[n]));
    }

    for (size_t n = 0; n < screw_rows.size(); n++)
        if (screw_rows[n].seam_or_corner == 0) {
            nested(frame, *context.connected.screws.at(n), CONNECTOR_COLOR);

            if (shown.count(context.connected.screws[n]->name))
                frame.label(context.connected.screws[n]->name, context.connected.screws[n]->drill_lines.front().start());
        }

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// In the session
// ═══════════════════════════════════════════════════════════════════════════

/// 193: connector_wedge_0's children: the part with its bores, the dowels lifted out of it.
void nesting(const Context& context) {

    const std::shared_ptr<JointBeam> wedge = named(context.connected, "connector_wedge_0");
    const std::vector<std::shared_ptr<Joint>> children = wedge->children();
    const Vector lift(0.0, 0.0, 260.0);
    const Point at = centre(wedge->parts[0]);
    Frame frame(CHAPTER, 193, "nest_children", "nest_children: a ConnectorPart per part, carrying the bores, and a Dowel per drill line, lifted out here", "iso", around(at + lift * 0.5, 300.0, 450.0, 260.0));

    frame.element(children[0], CONNECTOR_COLOR);

    for (size_t i = 1; i < children.size(); i++) {
        const Line& axis = wedge->drill_lines[i - 1];
        std::shared_ptr<Dowel> dowel = std::make_shared<Dowel>(axis.transformed(Xform::translation(lift[0], lift[1], lift[2])), wedge->line_radius, wedge->chord_tolerance);
        dowel->name = children[i]->name;
        frame.element(dowel, MARK);
        frame.line(Line::from_points(axis.center(), axis.center() + lift), INK, 1.5, true, true);
    }

    frame.label(fmt::format("{}: SolidCut bores, {} drills", children[0]->name, wedge->drill_lines.size()), Point(at[0], wedge->drill_lines[0].center()[1], at[2]));
    frame.label(children[1]->name, wedge->drill_lines[0].end() + lift);
    frame.label(children[2]->name, wedge->drill_lines[1].end() + lift);
    frame.write(context.dir);
}

/// 194: inner_beams_0_0 after the cut, the lofted pocket it was cut with and the dowel holes target_drills gave it.
void cutter_solid(const Context& context) {

    const std::shared_ptr<JointBeam> wedge = named(context.connected, "connector_wedge_0");
    const std::shared_ptr<Element> beam = context.connected.get_element<Element>(wedge->targets[0]);
    const SolidCut* cut = stored_cut(*beam, wedge->guid());

    if (!cut)
        throw std::runtime_error(beam->name + " holds no cut of " + wedge->name);

    const std::vector<Point> deep = corners(wedge->cutters[0][0][1]);
    Frame frame(CHAPTER, 194, "cutter_solid", "Per target: its cutters lofted into one mesh and the target_drills, stored as one SolidCut in the target", "iso", around(Point::centroid({cut->drills[0].center(), cut->drills[1].center()}) + Vector(0.0, 0.0, 30.0), 260.0, 420.0, 220.0));

    frame.element(beam, GREY);
    wire(frame, wedge->cutters[0][0], MARK, 2.5);

    for (const Line& drill : cut->drills)
        frame.line(drill, INK, 1.5, true);

    frame.label("Mesh::loft(cutters[0][0]), closed", deep[0]);
    frame.label(fmt::format("drills: {} x r {:g}", cut->drills.size(), cut->drill_radius), cut->drills[0].start());
    frame.label(fmt::format("{}: SolidCut, joint_guid = guid of {}", beam->name, wedge->name), cut->drills[1].end());
    frame.write(context.dir);
}

/// 195: one block dowel along its axis: the hole in the rib and the hole in the block, each blind where the dowel stops inside.
void target_holes(const Context& context) {

    const Relationship row = first(context.guide, Relation::block_dowels);
    const std::shared_ptr<JointBeam> dowels = named(context.connected, "connector_dowels_0");
    const Line dowel = dowels->drill_lines[0];
    const std::shared_ptr<Element> rib = context.connected.get_element<Element>(dowels->targets[0]);
    const std::shared_ptr<Element> block = context.connected.get_element<Element>(dowels->targets[1]);
    const Line rib_hole = stored_cut(*rib, dowels->guid())->drills[0];
    const Line block_hole = stored_cut(*block, dowels->guid())->drills[0];
    const Vector shift(0.0, 0.0, 10.0);
    Frame frame(CHAPTER, 195, "target_drills", "target_drills: a hole runs on by drill_overshoot where the dowel leaves the target, blind where it ends", "right", around(dowel.center(), 60.0, 75.0, 45.0));

    frame.polyline(row.contact, GREY, 3.0);
    frame.line(dowel, MARK, 8.0);
    frame.line(rib_hole.transformed(Xform::translation(shift[0], shift[1], shift[2])), FAMILY_COLORS[0], 4.0);
    frame.line(block_hole.transformed(Xform::translation(-shift[0], -shift[1], -shift[2])), FAMILY_COLORS[3], 4.0);

    frame.label(fmt::format("{} hole start: {}", rib->name, run_on(rib_hole.start(), dowel.start())), rib_hole.start() + shift);
    frame.label(run_on(rib_hole.end(), dowel.end()), rib_hole.end() + shift);
    frame.label(fmt::format("{} hole start: {}", block->name, run_on(block_hole.start(), dowel.start())), block_hole.start() - shift);
    frame.label(run_on(block_hole.end(), dowel.end()), block_hole.end() - shift);
    frame.label(fmt::format("dowel {:.0f} long", dowel.length()), dowel.center());
    frame.write(context.dir);
}

/// 196: wedges_1_0 after the cuts with the entry and exit circles of every drill feature hosted on it.
void hosted_drills(const Context& context) {

    const std::shared_ptr<Element> block = context.connected.members.quarters[0].wedges[1].element;
    const Point at = up(middle(context.guide.quarter(0).wedges()[1]));
    std::vector<ElementFeature> drills;

    for (const ElementFeature& feature : block->features())
        if (feature.feature_type == "drill")
            drills.push_back(feature);

    Frame frame(CHAPTER, 196, "host_drills", "host_drills: per stretch of a hole inside the target, a drill feature of two circles at entry and exit", "iso", around(at, 330.0, 330.0, 330.0));
    frame.distance = 0.6;
    frame.element(block, GREY);

    for (const ElementFeature& feature : drills)
        for (const Polyline& circle : feature.outlines)
            frame.polyline(circle, MARK, 3.0);

    frame.label(fmt::format("{}: {} drill features", block->name, drills.size()), at);

    if (!drills.empty())
        frame.label(drills.front().name, Point::centroid(corners(drills.front().outlines.front())));

    if (drills.size() > 4)
        frame.label(drills[4].name, Point::centroid(corners(drills[4].outlines.front())));

    frame.write(context.dir);
}

/// 197: corner 0's plates as their parts draw them after the cross lap: the slots and the dowel bores cut.
void synced_parts(const Context& context) {

    const std::shared_ptr<JointBeam> a = named(context.connected, "connector_0");
    const std::shared_ptr<JointBeam> b = named(context.connected, "connector_1");
    const std::shared_ptr<JointBeam> lap = named(context.connected, "connector_cross_lap_0");
    const std::vector<std::shared_ptr<Joint>> parts_a = a->children();
    const std::vector<std::shared_ptr<Joint>> parts_b = b->children();
    Frame frame(CHAPTER, 197, "sync_parts", "sync_parts: a slot stored on a plate connector after nesting reaches its ConnectorPart, with the bores", "iso", around(Point::centroid({centre(a->parts[0]), centre(b->parts[0])}), 330.0, 330.0, 220.0));
    frame.key = true;
    frame.distance = 0.7;

    frame.element(parts_a[0], CONNECTOR_COLOR);
    frame.element(parts_b[0], CONNECTOR_COLOR);

    frame.label(parts_a[0]->name, centre(a->parts[0]));
    frame.label(parts_b[0]->name, centre(b->parts[0]));
    frame.label(fmt::format("slot of {}", lap->name), centre(lap->cutters[0][0]));
    frame.write(context.dir);
}

/// 198: quarter 0 grey and every part, dowel and screw of its connectors in CONNECTOR_COLOR.
void painted(const Context& context) {

    const std::vector<Relationship> rows = walked(context.guide, CONNECTOR_RELATIONS);
    const std::vector<Relationship> screw_rows = walked(context.guide, screw_kinds());
    Frame frame(CHAPTER, 198, "paint", "paint: every connector node and the parts and dowels under it in CONNECTOR_COLOR, brg_blue", "iso", QUARTER);
    frame.key = true;
    frame.distance = 0.72;
    quarter_members(frame, context.connected, 0, GREY);

    for (size_t n = 0; n < rows.size(); n++)
        if (rows[n].seam_or_corner == 0)
            nested(frame, *context.connected.connectors.at(n), CONNECTOR_COLOR);

    for (size_t n = 0; n < screw_rows.size(); n++)
        if (screw_rows[n].seam_or_corner == 0)
            nested(frame, *context.connected.screws.at(n), CONNECTOR_COLOR);

    frame.write(context.dir);
}

/// 199: connector_0's part and dowels, which draw, and a dashed box where its own node draws nothing.
void empty_node(const Context& context) {

    const std::shared_ptr<JointBeam> plate = named(context.connected, "connector_0");
    const Point at = centre(plate->parts[0]);
    const Xform grow = Xform::scale_uniform(at, 1.25);
    const std::array<Polyline, 2> outline = {plate->parts[0][0].transformed(grow), plate->parts[0][1].transformed(grow)};
    Frame frame(CHAPTER, 199, "empty_node", "The connector node's own mesh and BRep are empty; its ConnectorPart and Dowel children draw it", "iso", around(at, 340.0, 220.0, 230.0));

    nested(frame, *plate, CONNECTOR_COLOR);
    wire(frame, outline, INK, 1.5, true);

    frame.label(fmt::format("{}_part: apply_solid_cuts(part_mesh(0), solid_cuts)", plate->name), at);
    frame.label(fmt::format("{}_dowel_0", plate->name), plate->drill_lines[0].start());
    frame.label(fmt::format("{}: element_geometry_mesh() empty", plate->name), corners(outline[1])[2]);
    frame.write(context.dir);
}

}

void chapter_09_connectors(const Context& context) {

    Floor tied(context.tied);
    tied.add_quarters();
    tied.add_connectors({Relation::seam_tie});

    entry(context);
    walk(context);
    wedge_sizing(context);
    wedge_axes(context);
    wedge_stations(context);
    wedge_profile(context);
    wedge_dowels(context);
    wedge_pockets(context);
    oculus_wedge(context);
    plate_frame(context);
    plate_part(context);
    plate_dowels(context);
    corner_plates(context);
    cross_lap_level(context);
    cross_lap_slots(context);
    tie_frame(context, tied);
    tie_key(context, tied);
    tie_pockets(context, tied);
    dowels_inset(context);
    dowels_axes(context);
    screws_factory(context);
    naming(context);
    nesting(context);
    cutter_solid(context);
    target_holes(context);
    hosted_drills(context);
    synced_parts(context);
    painted(context);
    empty_node(context);
}

}
