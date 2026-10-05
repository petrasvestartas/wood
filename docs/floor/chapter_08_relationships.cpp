#include "docs/floor/movie.h"

#include <set>

namespace movie {

namespace {

const std::string CHAPTER = "08_relationships";

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The rows of one kind at seam or corner q, in the order relationships() lists them.
std::vector<Relationship> rows_at(const FloorGuide& guide, Relation kind, size_t q) {

    std::vector<Relationship> rows;

    for (const Relationship& row : relationships(guide, kind))
        if (row.seam_or_corner == q)
            rows.push_back(row);

    return rows;
}

/// A ring beam, column or support reference, as relationships() makes it.
MemberRef shared(Family family, size_t index) {
    return MemberRef{-1, family, index, -1};
}

/// The point a fraction t of the way from a to b.
Point along(const Point& a, const Point& b, double t) {
    return a + (b - a) * t;
}

/// The box of half side half about a point.
Box around(const Point& centre, double half) {
    return {centre[0] - half, centre[1] - half, centre[2] - half, centre[0] + half, centre[1] + half, centre[2] + half};
}

/// Three coordinates as text to the decimals given, without a negative zero.
template <typename T>
std::string coordinates(const T& v, int decimals) {

    const double small = 0.5 * std::pow(10.0, -decimals);
    const auto clean = [small](double x) { return std::abs(x) < small ? 0.0 : x; };

    return fmt::format("({:.{}f}, {:.{}f}, {:.{}f})", clean(v[0]), decimals, clean(v[1]), decimals, clean(v[2]), decimals);
}

/// The colour a member of that family is shown in.
Color family_color(Family family) {

    if (family == Family::ring)
        return RING;
    if (family == Family::column || family == Family::support)
        return STEEL;

    return FAMILY_COLORS[static_cast<size_t>(family)];
}

/// The family's name as MemberRef holds it.
std::string family_name(Family family) {

    if (family == Family::ring)
        return "ring";
    if (family == Family::column)
        return "column";
    if (family == Family::support)
        return "support";

    return FAMILY_NAMES[static_cast<size_t>(family)];
}

/// Every member of the quarters, their ring beams and columns in grey, but the ones named in skip.
void grey_members(Frame& frame, const Floor& floor, const std::vector<size_t>& quarters, const std::set<std::string>& skip) {

    for (const size_t q : quarters) {
        for (const Family family : FAMILIES)
            for (const Member& member : members(floor.members.quarters[q], family))
                if (!skip.count(member.element->name))
                    frame.element(member.element, GREY);

        if (q < floor.members.ring.size() && !skip.count(floor.members.ring[q].element->name))
            frame.element(floor.members.ring[q].element, GREY);

        if (q < floor.members.columns.size() && !skip.count(floor.members.columns[q].column->name))
            frame.element(floor.members.columns[q].column, GREY);
    }
}

/// The edge of a contact at its highest level: the two corners at the top, in contact order.
Line datum_edge(const Relationship& row) {

    const std::vector<Point> points = open_points(row.contact);
    double top = -1e300;

    for (const Point& point : points)
        top = std::max(top, point[2]);

    std::vector<Point> high;

    for (const Point& point : points)
        if (std::abs(point[2] - top) <= 1e-6)
            high.push_back(point);

    return Line::from_points(high.front(), high.back());
}

/// The trace of a plane on the floor top through the point of it nearest at, half long either way.
Line floor_trace(const Plane& plane, const Point& at, double half) {

    const Vector direction = plane.z_axis().cross(Vector::z_axis()).normalized();
    const Point middle = plane_plane_plane(plane, level(H), Plane::from_point_normal(at, direction)).value();

    return Line::from_points(middle - direction * half, middle + direction * half);
}

/// The polygon edge verify_contacts compares: the highest by mean z, the longest among those within 1e-6 of it, as top_edge in floor_verify.cpp picks it.
Line highest_edge(const std::vector<Point>& points) {

    Line best;
    double best_height = -1e300;
    double best_length = -1.0;

    for (size_t i = 0; i < points.size(); i++) {
        const Line edge = Line::from_points(points[i], points[(i + 1) % points.size()]);
        const double height = edge.center()[2];

        if (height > best_height + 1e-6 || (std::abs(height - best_height) <= 1e-6 && edge.length() > best_length)) {
            best = edge;
            best_height = height;
            best_length = edge.length();
        }
    }

    return best;
}

/// Whether verify_contacts checks the row with its default kinds: a contact of any kind but the screws.
bool checked(const Relationship& row) {
    return row.contact.point_count() > 0 && std::find(SCREW_RELATIONS.begin(), SCREW_RELATIONS.end(), row.kind) == SCREW_RELATIONS.end();
}

/// The kernel's contact of a row's two members, searched on their uncut copies as verify_contacts does.
std::shared_ptr<InteractionContactFace> search(WoodSession& session, const Floor& floor, const Relationship& row) {

    const std::array<std::shared_ptr<Element>, 2> pair = floor.members.pair(row);

    return session.compute_face_contact(uncut(*pair[0]), uncut(*pair[1]));
}

// ═══════════════════════════════════════════════════════════════════════════
// Names
// ═══════════════════════════════════════════════════════════════════════════

/// The whole floor in plan, grey, with the members the first rows of quarter 0 name in their colours, each labelled by MemberRef::name().
void member_names(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const ColumnCorner& column = guide.columns[0];
    const std::vector<MemberRef> refs = {quarter_member(0, Family::inner_beams, 0), quarter_member(1, Family::inner_beams, 2), shared(Family::ring, 0), shared(Family::column, 0), shared(Family::support, 0), quarter_member(0, Family::outer_ribs, 1), quarter_member(0, Family::wedges, 2)};
    const Polyline beam0 = guide.quarter(0).inner_beams()[0].top;
    const Polyline beam2 = guide.quarter(1).inner_beams()[2].top;
    const std::vector<Point> pins = {
        up(along(beam0.get_point(0), beam0.get_point(1), 0.3)),
        up(along(beam2.get_point(0), beam2.get_point(1), 0.7)),
        up(middle(guide.oculus()[0])),
        up(column.corner + column.x_axis * (guide.parameters.column_head + guide.parameters.column_head_chamfer - 20.0) + column.y_axis * 20.0),
        column.axis_point,
        up(middle(guide.quarter(0).outer_ribs()[1])),
        up(middle(guide.quarter(0).wedges()[2])),
    };

    Frame frame(CHAPTER, 144, "member_names", "MemberRef{quarter, family, index, row} names each member of a row; MemberRef::name() gives its scene name", "top", BAY);
    std::set<std::string> named;

    for (const MemberRef& ref : refs)
        named.insert(ref.name());

    plan_context(frame, guide, true);
    grey_members(frame, floor, {0, 1, 2, 3}, named);

    for (size_t i = 0; i < refs.size(); i++) {
        frame.element(floor.members.get(refs[i]), family_color(refs[i].family));
        frame.label(fmt::format("{} = {{{}, {}, {}, {}}}", refs[i].name(), refs[i].quarter, family_name(refs[i].family), refs[i].index, refs[i].row), pins[i]);
    }

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Seam wedge
// ═══════════════════════════════════════════════════════════════════════════

/// Seam 0 in plan: seam beam 0 of quarter 0 and seam beam 2 of quarter 1 either side of its line.
void seam_members(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::seam_wedge, 0).front();
    const Polyline a = guide.quarter(0).inner_beams()[0].top;
    const Polyline b = guide.quarter(1).inner_beams()[2].top;
    const Seam& seam = guide.seams[0];

    Frame frame(CHAPTER, 146, "seam_wedge_members", "seam_wedge(guide, 0): row.a is inner beam 0 of quarter 0, row.b inner beam 2 of quarter 1, along seams[0]", "top", {-500.0, -3150.0, H - 800.0, 500.0, -500.0, H + 20.0});
    plan_context(frame, guide, false);
    grey_members(frame, floor, {0, 1}, {row.a.name(), row.b.name()});
    frame.element(floor.members.get(row.a), BUILT);
    frame.element(floor.members.get(row.b), RESULT);
    frame.line(up(seam.line), INPUT, 2.0, true);
    frame.point(up(seam.oculus_corner), INPUT);

    frame.label("row.a = quarter_member(0, inner_beams, 0): " + row.a.name(), up(along(a.get_point(0), a.get_point(1), 0.5)));
    frame.label("row.b = quarter_member(1, inner_beams, 2): " + row.b.name(), up(along(b.get_point(0), b.get_point(1), 0.5)));
    frame.label("seams[0].line", up(along(seam.line.start(), seam.line.end(), 0.75)));
    frame.label("seams[0].oculus_corner", up(seam.oculus_corner));
    frame.write(context.dir);
}

/// The seam plane into quarter 0 lifted to the floor, over the two grey seam beams.
void seam_plane(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::seam_wedge, 0).front();

    Frame frame(CHAPTER, 147, "seam_wedge_plane", "row.plane = lifted(seams[0].plane_into(0), bay_height): the vertical seam plane, its normal into quarter 0", "iso", {-700.0, -3200.0, H - 400.0, 700.0, -800.0, H + 150.0});
    frame.plane_size = 300.0;
    frame.distance = 0.65;
    frame.element(floor.members.get(row.a), GREY);
    frame.element(floor.members.get(row.b), GREY);
    frame.line(up(guide.seams[0].line), INPUT, 2.0, true);
    frame.plane(row.plane, BUILT);

    frame.label(fmt::format("row.plane origin {}: the half seam's midpoint, lifted {:.0f}", coordinates(row.plane.origin(), 0), guide.parameters.bay_height), row.plane.origin());
    frame.label(fmt::format("normal {}: into quarter 0", coordinates(row.plane.z_axis(), 0)), row.plane.origin() + row.plane.z_axis() * 300.0);
    frame.label("seams[0].line", up(along(guide.seams[0].line.start(), guide.seams[0].line.end(), 0.1)));
    frame.write(context.dir);
}

/// The seam wedge contact in the seam plane: inner beam 0's loop there, its four corners and the four planes that cut them.
void seam_contact(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Relationship row = rows_at(guide, Relation::seam_wedge, 0).front();
    const std::vector<Point> c = open_points(row.contact);
    const double shift = Vector(c[2][0] - c[1][0], c[2][1] - c[1][1], 0.0).magnitude();

    Frame frame(CHAPTER, 148, "seam_wedge_contact", "row.contact = lifted(open_points(inner_beams()[0].bottom)): beam 0's loop on the seam plane", "right", {-100.0, -3150.0, H - 260.0, 100.0, -850.0, H + 60.0});
    frame.key = true;

    for (size_t i = 0; i < c.size(); i++) {
        const Vector direction = (c[(i + 1) % c.size()] - c[i]).normalized();
        frame.line(Line::from_points(c[i] - direction * 120.0, c[(i + 1) % c.size()] + direction * 120.0), INPUT, 1.5, true);
    }

    frame.polyline(row.contact, BUILT, 5.0);

    frame.label("0: outer_ribs[0][0], the bay edge, meets level(0)", c[0]);
    frame.label("1: level(0) meets the tilted oculus plane", c[1]);
    frame.label(fmt::format("2: tilted plane meets level(soffit), {:.2f} nearer the centre than 1", shift), c[2]);
    frame.label(fmt::format("3: level(soffit = {:.2f}) meets the bay edge", guide.soffit), c[3]);
    frame.label(fmt::format("row.contact, row.area() = {:.0f} mm2", row.area()), area_centroid(row.contact), true);
    frame.write(context.dir);
}

/// The seam wedge's end plane, the bay's outer face, next to its contact at the bay edge.
void seam_end(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::seam_wedge, 0).front();
    const std::vector<Point> c = open_points(row.contact);
    const Plane& end = row.end.value();

    Frame frame(CHAPTER, 149, "seam_wedge_end", "row.type = side_side, row.seam_or_corner = 0, and with seam_through_ribs row.end = the bay's outer face", "iso", {-500.0, -3350.0, H - 350.0, 500.0, -2550.0, H + 100.0});
    frame.plane_size = 200.0;
    frame.element(floor.members.get(row.a), GREY);
    frame.element(floor.members.get(row.b), GREY);
    frame.element(floor.members.get(quarter_member(0, Family::outer_ribs, 0)), GREY);
    frame.element(floor.members.get(quarter_member(1, Family::outer_ribs, 1)), GREY);
    frame.polyline(row.contact, GREY, 4.0);
    frame.plane(end, BUILT);

    frame.label("row.end = lifted(edges[0].band[0], bay_height)", end.origin());
    frame.label(fmt::format("row.end normal {}: into the bay", coordinates(end.z_axis(), 0)), end.origin() + end.z_axis() * 200.0);
    frame.label(fmt::format("row.type = {} ({})", to_string(row.type), static_cast<int>(row.type)), along(c[0], c[1], 0.15));
    frame.label(fmt::format("row.seam_or_corner = {}", row.seam_or_corner), along(c[3], c[2], 0.15));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus wedge
// ═══════════════════════════════════════════════════════════════════════════

/// A section along oculus edge 0: its vertical edge plane, the tilted plane leaned from it, the oculus beam on the quarter side and ring beam 0 on the centre side.
void oculus_plane(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::oculus_wedge, 0).front();
    const OculusEdge& oculus = guide.oculus_edges[0];
    const Point centre = oculus.line.center();
    const Plane section = Plane::from_point_normal(centre, oculus.line.to_direction());
    const Point vertical = plane_plane_plane(edge_plane(oculus.line, -Vector::z_axis()), level(-320.0), section).value();
    const Point tilted = plane_plane_plane(oculus.tilted, level(-320.0), section).value();

    Frame frame(CHAPTER, 150, "oculus_wedge_plane", "oculus_wedge(guide, 0): the oculus beam and ring beam 0 on oculus_edges[0].tilted, seen along the edge", "iso", {-700.0, -700.0, H - 360.0, -300.0, -300.0, H + 60.0});
    frame.orbit = "-262,-105";
    frame.distance = 0.7;
    frame.element(floor.members.get(row.a), BUILT);
    frame.element(floor.members.get(row.b), RESULT);
    frame.line(up(Line::from_points(centre, vertical)), INPUT, 2.0, true);
    frame.line(up(Line::from_points(centre, tilted)), VARIABLE, 4.0);

    frame.label("row.a = " + row.a.name(), up(middle(guide.quarter(0).inner_beams()[1])));
    frame.label("row.b = shared_member(ring, 0): " + row.b.name(), up(middle(guide.oculus()[0])));
    frame.label("edge_plane(oculus_edges[0].line, -z), vertical", up(vertical));
    frame.label(fmt::format("row.plane = oculus_edges[0].tilted, turned {:.0f} deg about the edge", guide.parameters.oculus_plane_angle), up(tilted));
    frame.label(fmt::format("normal {}", coordinates(row.plane.z_axis(), 4)), up(centre));
    frame.write(context.dir);
}

/// The oculus wedge contact: the oculus beam's loop on the tilted plane, cut by the seam beams' far faces, the datum and the soffit.
void oculus_contact(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::oculus_wedge, 0).front();
    const std::vector<Point> c = open_points(row.contact);
    const OculusEdge& oculus = guide.oculus_edges[0];
    const double shift = Vector(c[3][0] - c[0][0], c[3][1] - c[0][1], 0.0).magnitude();
    const Line back = floor_trace(lifted(oculus.back, H), up(oculus.line.center()), 650.0);

    Frame frame(CHAPTER, 151, "oculus_wedge_contact", "row.contact = lifted(open_points(inner_beams()[1].bottom)): the oculus beam's loop on the tilted plane", "iso", {-1100.0, -1100.0, H - 280.0, 0.0, 0.0, H + 40.0});
    frame.element(floor.members.get(quarter_member(0, Family::inner_beams, 0)), GREY);
    frame.element(floor.members.get(quarter_member(0, Family::inner_beams, 2)), GREY);
    frame.element(floor.members.get(row.b), GREY);
    frame.line(up(oculus.line), INPUT, 2.0, true);
    frame.line(back, VARIABLE, 2.0, true);
    frame.polyline(row.contact, BUILT, 5.0);

    frame.label("0: inner_beams[0][1] meets level(0)", c[0]);
    frame.label("1: level(0) meets inner_beams[2][1]", c[1]);
    frame.label("2: inner_beams[2][1] meets level(soffit)", c[2]);
    frame.label(fmt::format("3: level(soffit) meets inner_beams[0][1], {:.2f} nearer the centre than 0", shift), c[3]);
    frame.label("the top edge lies on oculus_edges[0].line", up(oculus.line.center()));
    frame.label(fmt::format("oculus_edges[0].back, inner_beams = {:.0f} behind the edge", guide.parameters.inner_beams), along(back.start(), back.end(), 0.25));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Column plate and cross lap
// ═══════════════════════════════════════════════════════════════════════════

/// Column head 0 in plan with the floor traces of the two fan side planes the column plates lie on.
void column_plane(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ColumnCorner& column = guide.columns[0];
    const std::vector<Relationship> rows = rows_at(guide, Relation::column_plate, 0);

    Frame frame(CHAPTER, 152, "column_plate_plane", "column_plate(guide, 0, k): column_0 and outer rib k on the fan side plane wedge_fan[0][0] or wedge_fan[2][0]", "top", {-3060.0, -3060.0, H - 800.0, -2560.0, -2560.0, H + 20.0});
    frame.polyline(up(Polyline(column.head).closed()), GREY, 3.0);

    for (const Polyline& quad : guide.geometry[0].quads.outer_ribs)
        frame.polyline(up(quad.closed()), GREY, 2.0);

    for (size_t k = 0; k < rows.size(); k++) {
        const Relationship& row = rows[k];
        const Vector direction = row.plane.z_axis().cross(Vector::z_axis()).normalized();
        const Point origin = row.plane.origin();
        const Point far = (origin + direction * 220.0 - up(column.corner)).magnitude() > (origin - direction * 220.0 - up(column.corner)).magnitude() ? origin + direction * 220.0 : origin - direction * 220.0;
        const Polyline rib = guide.quarter(0).outer_ribs()[k].top;
        const Point outside = origin - (far - origin) * (100.0 / 220.0);
        const double lean = std::asin(std::abs(row.plane.z_axis()[2])) * 180.0 / M_PI;

        frame.line(Line::from_points(origin - (far - origin), far), BUILT, 4.0);
        frame.label(fmt::format("k={}: row.plane = wedge_fan[{}][0], {:.2f} deg off vertical", k, k == 0 ? 0 : 2, lean), outside);
        frame.label("row.b = " + row.b.name(), up(along(rib.get_point(1), rib.get_point(0), 0.04)));
    }

    frame.label("row.a = shared_member(column, 0): column_0", up(area_centroid(Polyline(column.head).closed())));
    frame.write(context.dir);
}

/// Outer rib 0's end at column 0 in elevation: the column plate contact edge on, and the middle cutter level that clips it.
void column_contact(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::column_plate, 0).front();
    const std::vector<Point> c = open_points(row.contact);
    const double level1 = guide.columns[0].levels[1];

    Frame frame(CHAPTER, 153, "column_plate_contact", "row.contact = above({top[1], top[2], bottom[2], bottom[1]}, levels[1]): the rib's end face on the fan plane", "front", {-2830.0, -3050.0, H - 740.0, -2560.0, -2850.0, H + 40.0});
    frame.element(floor.members.get(row.a), GREY);
    frame.element(floor.members.get(row.b), GREY);
    frame.line(Line::from_points(Point(-2830.0, c[0][1], H + level1), Point(-2580.0, c[0][1], H + level1)), VARIABLE, 2.0, true);
    frame.polyline(row.contact, BUILT, 6.0);

    frame.label("top[1] = p0: the fan plane at level(0)", c[0]);
    frame.label(fmt::format("top[2]: the first soffit point, z {:.2f}", c[1][2]), c[1]);
    frame.label(fmt::format("columns[0].levels[1] = {:.2f}", level1), Point(-2580.0, c[0][1], H + level1));
    frame.label(fmt::format("row.contact: {} corners, nothing below levels[1] to clip", c.size()), along(c[0], c[1], 0.5));
    frame.write(context.dir);
}

/// Corner 0 in plan: the two column plate contacts and the cross lap row that has no geometry of its own.
void cross_lap(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const std::vector<Relationship> plates = rows_at(guide, Relation::column_plate, 0);
    const Relationship lap = rows_at(guide, Relation::cross_lap, 0).front();
    const Point crossing = plane_plane_plane(plates[0].plane, plates[1].plane, level(H)).value();

    Frame frame(CHAPTER, 154, "cross_lap", "cross_lap(0): outer ribs 0 and 1 of the corner, no plane and no contact; the plates crossing come later", "top", {-3150.0, -3150.0, H - 800.0, -2550.0, -2550.0, H + 20.0});
    frame.element(floor.members.get(shared(Family::column, 0)), GREY);
    frame.element(floor.members.get(lap.a), BUILT);
    frame.element(floor.members.get(lap.b), RESULT);

    for (size_t k = 0; k < plates.size(); k++) {
        const Vector direction = plates[k].plane.z_axis().cross(Vector::z_axis()).normalized();
        frame.polyline(plates[k].contact, GREY, 5.0);
        frame.line(Line::from_points(crossing - direction * 200.0, crossing + direction * 200.0), INPUT, 3.0, true);
        frame.label(fmt::format("column_plate k={}", k), area_centroid(plates[k].contact));
    }

    frame.label(fmt::format("cross_lap: row.a = {}, row.b = {}", lap.a.name(), lap.b.name()), crossing);
    frame.label(fmt::format("row.contact: {} points, row.plane: the default xy plane", lap.contact.point_count()), up(guide.columns[0].corner));
    frame.write(context.dir);
}

/// The outline loops of the four members at the bay edge end of seam 0, lifted and moved by shift.
void seam_end_loops(Frame& frame, const FloorGuide& guide, const Vector& shift) {

    const Xform move = Xform::translation(shift[0], shift[1], shift[2] + guide.parameters.bay_height);
    const std::vector<Outline> outlines = {guide.quarter(0).outer_ribs()[0], guide.quarter(1).outer_ribs()[1], guide.quarter(0).inner_beams()[0], guide.quarter(1).inner_beams()[2]};

    for (const Outline& outline : outlines) {
        frame.polyline(outline.top.transformed(move), GREY, 2.0);
        frame.polyline(outline.bottom.transformed(move), GREY, 2.0);
    }
}

/// Seam 0 at the bay edge twice: the default bay above, its ribs ending on the seam beams and no tie; the tied bay below, the ribs end to end and the tie face on the seam plane.
void seam_tie(const Context& context) {

    const FloorGuide& guide = context.guide;
    const FloorGuide& tied = context.tied;
    const Relationship row = rows_at(tied, Relation::seam_tie, 0).front();
    const Vector shift(0.0, guide.edges[0].midpoint[1] - tied.edges[0].midpoint[1], -1200.0);
    const Polyline rib = guide.quarter(0).outer_ribs()[0].top;
    const Polyline tied_a = tied.quarter(0).outer_ribs()[0].top;
    const Polyline tied_b = tied.quarter(1).outer_ribs()[1].top;
    const Vector span = tied.corners[2] - tied.corners[0];

    Frame frame(CHAPTER, 155, "seam_tie", "seam_tie(guide, q): made only when seam_through_ribs is false; above the default bay, below the tied one", "iso", {-700.0, -3350.0, H - 1600.0, 700.0, -2450.0, H + 80.0});
    seam_end_loops(frame, guide, Vector(0.0, 0.0, 0.0));
    seam_end_loops(frame, tied, shift);
    frame.polyline(row.contact + shift, BUILT, 5.0);

    frame.label(fmt::format("default, seam_through_ribs = true: {} seam_tie rows, the rib ends on x = -{:.0f}", relationships(guide, Relation::seam_tie).size(), guide.parameters.inner_beams), up(rib.get_point(0)));
    frame.label(fmt::format("tied {:.0f} x {:.0f}: row.contact on seams[0].plane_into(0), row.type = {}", span[0], span[1], to_string(row.type)), area_centroid(row.contact) + shift);
    frame.label("row.a = " + row.a.name(), up(along(tied_a.get_point(0), tied_a.get_point(1), 0.12)) + shift);
    frame.label("row.b = " + row.b.name(), up(along(tied_b.get_point(0), tied_b.get_point(1), 0.12)) + shift);
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Block dowels
// ═══════════════════════════════════════════════════════════════════════════

/// Corner 0 in plan: the three block footprints and the floor traces of the six rib faces they sit between.
void block_planes(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionQuads& quads = guide.geometry[0].quads;
    const std::vector<Relationship> rows = rows_at(guide, Relation::block_dowels, 0);
    const std::array<std::string, 3> table = {"ribs[0] = {outer_ribs[0][1], inner_ribs[0][0]}", "ribs[1] = {inner_ribs[0][1], inner_ribs[1][1]}", "ribs[2] = {inner_ribs[1][0], outer_ribs[1][1]}"};

    Frame frame(CHAPTER, 156, "block_dowels_planes", "block_dowels: block k sits between the rib faces ribs[k][0] and ribs[k][1]; row.plane = lifted(ribs[k][side])", "top", {-3050.0, -3050.0, H - 800.0, -2350.0, -2350.0, H + 20.0});
    frame.polyline(up(Polyline(guide.columns[0].head).closed()), GREY, 2.0);

    for (const Relationship& row : rows) {
        const Line edge = datum_edge(row);
        const Vector direction = (edge.end() - edge.start()).normalized();
        const Color& color = row.a.family == Family::outer_ribs ? BUILT : RESULT;
        frame.line(Line::from_points(edge.start() - direction * 120.0, edge.end() + direction * 160.0), color, 3.0);
    }

    for (size_t k = 0; k < quads.wedges.size(); k++) {
        frame.polyline(up(quads.wedges[k].closed()), INPUT, 3.0);
        frame.label(fmt::format("wedges_{}_0: {}", k, table[k]), up(area_centroid(quads.wedges[k].closed())));
    }

    frame.write(context.dir);
}

/// Block 0 with its fan-plane loop numbered and its two faces on the rib planes, the contacts of its two rows.
void block_contact(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Outline block = guide.quarter(0).wedges()[0];
    const std::vector<Relationship> rows = rows_at(guide, Relation::block_dowels, 0);
    const std::array<std::string, 2> corners = {"bottom[0]: ribs[0][0] meets bed_top_planes[0]", "bottom[1]: bed_top_planes[0] meets ribs[0][1]"};

    Frame frame(CHAPTER, 157, "block_dowels_contact", "row.contact: block k's quad on its rib face, side 0 corners 3 and 0 of both loops, side 1 corners 1 and 2", "iso", around(up(middle(block)), 330.0));
    frame.element(floor.members.quarters[0].wedges[0].element, GREY);
    frame.polyline(up(block.bottom), INPUT, 2.0);

    for (size_t i = 0; i < corners.size(); i++)
        frame.label(corners[i], up(block.bottom.get_point(i)));

    frame.label("bottom[2], bottom[3]: ribs[0][1] and ribs[0][0] meet level(0)", up(along(block.bottom.get_point(2), block.bottom.get_point(3), 0.5)));

    for (const Relationship& row : rows)
        if (row.b.index == 0) {
            const bool side0 = row.a.family == Family::outer_ribs;
            frame.polyline(row.contact, side0 ? BUILT : RESULT, 5.0);
            frame.label(side0 ? "side 0: {bottom[3], bottom[0], top[0], top[3]} on outer_ribs[0][1]" : "side 1: {bottom[1], bottom[2], top[2], top[1]} on inner_ribs[0][0]", area_centroid(row.contact));
        }

    frame.label("Outline.top on wedges[0][1], the far face", up(area_centroid(block.top)));
    frame.write(context.dir);
}

/// Corner 0 in plan: the six block dowel contacts, numbered in the order relationships() pushes them.
void block_rows(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const std::vector<Relationship> rows = rows_at(guide, Relation::block_dowels, 0);

    Frame frame(CHAPTER, 158, "block_dowels_rows", "Six block_dowels rows per quarter, in this order: every block face on its rib once, 24 in all", "top", {-3100.0, -3100.0, H - 800.0, -2300.0, -2300.0, H + 20.0});
    frame.key = true;

    for (const Family family : {Family::outer_ribs, Family::inner_ribs, Family::wedges})
        for (const Member& member : members(floor.members.quarters[0], family))
            frame.element(member.element, GREY);

    for (size_t j = 0; j < rows.size(); j++) {
        frame.polyline(rows[j].contact, BUILT, 5.0);
        frame.label(fmt::format("{}: {} - {}", j + 1, rows[j].a.name(), rows[j].b.name()), datum_edge(rows[j]).center());
    }

    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Support
// ═══════════════════════════════════════════════════════════════════════════

/// The four supports on the slab with their column axes and the frame of each support row's plane.
void support(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const std::vector<Relationship> rows = relationships(guide, Relation::support);

    Frame frame(CHAPTER, 159, "support", "support(guide, q): support_q under column_q, row.plane = columns[q].support_plane on the slab, not lifted", "iso", FLOOR);
    frame.distance = 0.9;
    plan_context(frame, guide, false);

    for (const Relationship& row : rows) {
        const Plane& plane = row.plane;
        frame.element(floor.members.get(row.a), INPUT);
        frame.line(guide.columns[row.seam_or_corner].axis, INPUT, 2.0, true);
        frame.line(Line::from_points(plane.origin(), plane.origin() + plane.x_axis() * 600.0), BUILT, 4.0, false, true);
        frame.line(Line::from_points(plane.origin(), plane.origin() + plane.y_axis() * 600.0), RESULT, 4.0, false, true);
        frame.line(Line::from_points(plane.origin(), plane.origin() + plane.z_axis() * 600.0), GREY, 4.0, false, true);
        frame.label(fmt::format("{}: origin {}, x {}", row.a.name(), coordinates(plane.origin(), 0), coordinates(plane.x_axis(), 0)), plane.origin());
    }

    frame.label(fmt::format("row.b = column_0: columns[0].axis, up {:.0f}", guide.parameters.bay_height), guide.columns[0].axis.end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Members in the scene
// ═══════════════════════════════════════════════════════════════════════════

/// The whole floor in family colours, one name per family on a member of quarter 0, but the wedges on quarter 3 and the columns on column 1, clear of the crowd at corner 0.
void scene_members(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Quarter quarter = guide.quarter(0);
    const std::array<size_t, 6> shown = {0, 1, 1, 1, 2, 0};

    Frame frame(CHAPTER, 162, "scene_members", "add_members(): the elements every MemberRef resolves against, by quarter and family, the ring, the columns", "iso", BAY);
    frame.distance = 0.85;

    for (size_t q = 0; q < 4; q++) {
        for (size_t f = 0; f < FAMILIES.size(); f++)
            for (const Member& member : members(floor.members.quarters[q], FAMILIES[f]))
                frame.element(member.element, FAMILY_COLORS[f]);

        frame.element(floor.members.ring[q].element, RING);
        frame.element(floor.members.columns[q].column, STEEL);
        frame.element(floor.members.columns[q].support, STEEL);
    }

    for (size_t f = 0; f < FAMILIES.size(); f++) {
        const Outline outline = FAMILIES[f] == Family::beds ? quarter.beds()[1][0] : FAMILIES[f] == Family::wedges ? guide.quarter(3).wedges()[1] : outlines(quarter, FAMILIES[f])[shown[f]];
        frame.label(fmt::format("{}: {} per quarter", FAMILY_NAMES[f], members(floor.members.quarters[0], FAMILIES[f]).size()), up(middle(outline)));
    }

    frame.label(fmt::format("ring: oculus_0 .. oculus_{}", floor.members.ring.size() - 1), up(guide.oculus()[0].top.get_point(0)));
    frame.label(fmt::format("columns: column_q and support_q, {} models", floor.members.columns.size()), up(guide.columns[1].axis_point) - Vector(0.0, 0.0, 300.0));
    frame.write(context.dir);
}

/// Seam beam 0 of quarter 0 as its two loops in plan, close on the area centroid of each and the distance between them that FloorMembers::thickness returns.
void thickness(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const Relationship row = rows_at(guide, Relation::seam_wedge, 0).front();
    const Outline outline = guide.quarter(0).inner_beams()[0];
    const Point top = up(area_centroid(outline.top));
    const Point bottom = up(area_centroid(outline.bottom));
    const Vector apart = top - bottom;
    const double across = std::abs(apart.dot(row.plane.z_axis()));
    const double lengthwise = std::sqrt(std::max(apart.dot(apart) - across * across, 0.0));
    const Point mid = along(bottom, top, 0.5);

    Frame frame(CHAPTER, 163, "thickness_pair", "members.get(ref), members.thickness(ref) = outline_thickness, members.pair(row): what a connector reads", "top", {mid[0] - 230.0, mid[1] - 140.0, H - 280.0, mid[0] + 230.0, mid[1] + 140.0, H + 20.0});
    frame.element(floor.members.get(row.b), GREY);
    frame.polyline(up(outline.top), INPUT, 2.0);
    frame.polyline(up(outline.bottom), INPUT, 2.0);
    frame.line(Line::from_points(bottom, top), VARIABLE, 4.0);
    frame.point(top, VARIABLE);
    frame.point(bottom, VARIABLE);

    frame.label(fmt::format("area_centroid(bottom), members.thickness(row.a) = {:.2f}", floor.members.thickness(row.a)), bottom);
    frame.label("area_centroid(top), on inner_beams[0][1]", top);
    frame.label(fmt::format("{:.0f} across, {:.1f} along: the top loop runs 60 further toward the centre", across, lengthwise), mid);
    frame.label(fmt::format("pair(row) = {{{}, {}}}", floor.members.get(row.a)->name, floor.members.get(row.b)->name), up(area_centroid(guide.quarter(1).inner_beams()[2].top)));
    frame.write(context.dir);
}

/// Outer rib 0 as the connectors cut it beside its uncut copy, and the column, whose uncut copy keeps its head cuts.
void uncut_copies(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.connected;
    const std::shared_ptr<Element> rib = floor.members.quarters[0].outer_ribs[0].element;
    const std::shared_ptr<Element> copy = uncut(*rib);
    const std::shared_ptr<Element> column = uncut(*floor.members.columns[0].column);
    const Vector shift(0.0, 450.0, 0.0);
    const Polyline loop = guide.quarter(0).outer_ribs()[0].top;
    const Point pin = up(along(loop.get_point(1), loop.get_point(0), 0.08));
    const wood_session::BeamVariable* cut = dynamic_cast<const wood_session::BeamVariable*>(rib.get());
    const wood_session::BeamVariable* clean = dynamic_cast<const wood_session::BeamVariable*>(copy.get());
    const wood_session::Column* carved = dynamic_cast<const wood_session::Column*>(column.get());
    copy->place(Xform::translation(shift[0], shift[1], shift[2]));

    Frame frame(CHAPTER, 164, "uncut", "uncut(member): a copy without its cuts for the contact search; a Column is neither cast and keeps its head", "iso", FAN);
    frame.features = true;
    frame.element(rib, INPUT);
    frame.element(copy, BUILT);
    frame.element(column, RESULT);

    frame.label(fmt::format("{} in the floor: {} cuts, {} solid_cuts", rib->name, cut ? cut->cuts.size() : 0, cut ? cut->solid_cuts.size() : 0), pin);
    frame.label(fmt::format("uncut({}): {} solid_cuts, drawn {:.0f} mm aside", rib->name, clean ? clean->solid_cuts.size() : 0, shift.magnitude()), pin + shift);
    frame.label(fmt::format("uncut({}): keeps its {} solid_cuts", column->name, carved ? carved->solid_cuts.size() : 0), up(guide.columns[0].axis_point) - Vector(0.0, 0.0, 400.0));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Verification
// ═══════════════════════════════════════════════════════════════════════════

/// Quarter 0's checked contacts as built and, over them, the polygons the kernel's search finds on the uncut members.
void verify_search(const Context& context, const ContactCheck& check) {

    const FloorGuide& guide = context.guide;
    WoodSession session("verify_contacts");
    std::set<Relation> named;

    Frame frame(CHAPTER, 165, "verify_search", "verify_contacts: per checked row, compute_face_contact(uncut(a), uncut(b)); slate as built, blue as found", "iso", QUARTER);
    frame.key = true;
    frame.distance = 0.72;
    frame.polyline(up(Polyline(guide.geometry[0].polygon).closed()), GREY, 2.0);

    for (const Relationship& row : relationships(guide)) {
        if (row.seam_or_corner != 0 || !checked(row))
            continue;

        const std::shared_ptr<InteractionContactFace> found = search(session, context.members, row);
        frame.polyline(row.contact, INPUT, 6.0);

        if (found)
            frame.polyline(found->polygon, BUILT, 2.0);

        if (named.insert(row.kind).second)
            frame.label(fmt::format("{}: found {}", relation_name(row.kind), found ? std::string(to_string(found->type)) : "nothing"), area_centroid(row.contact));
    }

    frame.label(fmt::format("check.count = {} over the floor", check.count), up(guide.geometry[0].polygon[0]));
    frame.write(context.dir);
}

/// The seam wedge contact as built and as found: the two normals compared and the top edge whose midpoint and length are compared.
void disagreement(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Relationship row = rows_at(guide, Relation::seam_wedge, 0).front();
    WoodSession session("disagreement");
    const std::shared_ptr<InteractionContactFace> found = search(session, context.members, row);

    Frame frame(CHAPTER, 166, "disagreement", "disagreement(row, found): plane normal, top edge midpoint and length, then area, each within tolerance", "iso", {-400.0, -3150.0, H - 300.0, 300.0, -850.0, H + 250.0});
    frame.polyline(row.contact, INPUT, 4.0);
    frame.line(Line::from_points(row.plane.origin(), row.plane.origin() + row.plane.z_axis() * 300.0), INPUT, 4.0, false, true);
    frame.label("row.plane.z_axis()", row.plane.origin() + row.plane.z_axis() * 300.0);

    if (found) {
        const std::vector<Point> theirs = open_points(found->polygon);
        const Vector normal = compute_newell(theirs).normalized();
        const double angle = std::acos(std::clamp(std::abs(normal.dot(row.plane.z_axis())), 0.0, 1.0));
        const Line mine_edge = highest_edge(open_points(row.contact));
        const Line their_edge = highest_edge(theirs);
        const Point centroid = area_centroid(found->polygon);

        frame.polyline(found->polygon, BUILT, 2.0);
        frame.line(their_edge, VARIABLE, 6.0);
        frame.point(their_edge.center(), VARIABLE);
        frame.line(Line::from_points(centroid, centroid + normal * 300.0), BUILT, 4.0, false, true);

        frame.label(fmt::format("compute_newell(theirs): {:.1e} rad from row.plane", angle), centroid + normal * 300.0);
        frame.label(fmt::format("top edge: {:.1f} long, midpoint {:.1e} mm off, length {:.1e} mm off", their_edge.length(), (mine_edge.center() - their_edge.center()).magnitude(), std::abs(mine_edge.length() - their_edge.length())), their_edge.center());
        frame.label(fmt::format("area: {:.1f} found against row.area() {:.1f} mm2", polygon_area(found->polygon), row.area()), centroid);
    }

    frame.write(context.dir);
}

/// The bay in plan with two outer ribs at opposite corners that do not touch, the pair require_contact refuses, and the summary of the whole check.
void require(const Context& context, const ContactCheck& check) {

    const FloorGuide& guide = context.guide;
    const Floor& floor = context.members;
    const std::shared_ptr<Element> a = floor.members.quarters[0].outer_ribs[0].element;
    const std::shared_ptr<Element> b = floor.members.quarters[2].outer_ribs[0].element;
    const Point pa = up(middle(guide.quarter(0).outer_ribs()[0]));
    const Point pb = up(middle(guide.quarter(2).outer_ribs()[0]));
    const std::string summary = check.str().substr(0, check.str().find('\n'));
    WoodSession session("require_contact");
    std::string thrown = "nothing thrown";

    try {
        require_contact(session, a, b, ContactType::end_end, "no such seam");
    } catch (const std::runtime_error& error) {
        thrown = error.what();
    }

    Frame frame(CHAPTER, 168, "require_contact", "ContactCheck::str() for the floor, and require_contact throwing for two ribs that do not touch", "top", BAY);
    plan_context(frame, guide, true);
    frame.element(a, INPUT);
    frame.element(b, INPUT);
    frame.line(Line::from_points(pa, pb), VARIABLE, 3.0, true);

    frame.label("a = members.quarters[0].outer_ribs[0]: " + a->name, pa);
    frame.label("b = members.quarters[2].outer_ribs[0]: " + b->name, pb);
    frame.label("throws: " + thrown, along(pa, pb, 0.3));
    frame.label(fmt::format("ok() = {}: {}", check.ok(), summary), up(guide.centre));
    frame.write(context.dir);
}

}

/// The frames of relationships(): how each row names, places and verifies what two members share.
void chapter_08_relationships(const Context& context) {

    WoodSession session("verify_contacts");
    const ContactCheck check = verify_contacts(session, context.guide, context.members.members);

    member_names(context);
    seam_members(context);
    seam_plane(context);
    seam_contact(context);
    seam_end(context);
    oculus_plane(context);
    oculus_contact(context);
    column_plane(context);
    column_contact(context);
    cross_lap(context);
    seam_tie(context);
    block_planes(context);
    block_contact(context);
    block_rows(context);
    support(context);
    scene_members(context);
    thickness(context);
    uncut_copies(context);
    verify_search(context, check);
    disagreement(context);
    require(context, check);
}

}
