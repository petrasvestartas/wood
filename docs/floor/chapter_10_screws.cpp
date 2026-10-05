#include "docs/floor/movie.h"
#include <map>
#include <set>

namespace movie {

namespace {

const std::string CHAPTER = "10_screws";
const double CORNER_LEVELS = 7.0; // floor_screws.cpp:14, internal there: an oculus corner's depth in sevenths.
const double WEDGE_MARGIN = 1.5; // floor_screws.cpp:19, internal there: the wedge's free ends in beam thicknesses.
const double COARSE_STEP = 5.0; // floor_screws.cpp:20, internal there: the coarse head step of the aim search.
const double COARSE_SPAN = 150.0; // floor_screws.cpp:164, internal there: the coarse search's offset centre and span.
const double CORNER_REACH = 450.0; // mm in plan from an oculus corner within which a screw head belongs to that corner's picture.

/// The colour of each screw kind, in SCREW_RELATIONS order, for frame 201 alone, which tells the kinds apart: the family of the member the screws run along, the ring for the ring screws and the beds tint for the oculus screws, whose oculus beam already has the mitre screws' colour.
const std::array<Color, 5> SCREW_COLORS = {FAMILY_COLORS[0], FAMILY_COLORS[2], FAMILY_COLORS[1], RING, FAMILY_COLORS[5]};

const Box CORNER_3D = {-750.0, -1350.0, H - 260.0, 150.0, -450.0, H + 40.0}; // Oculus corner 0 in 3D.
const Box OCULUS_EDGE = {-1150.0, -1150.0, H - 260.0, 150.0, 150.0, H + 40.0}; // Quarter 0's oculus edge.

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The index in rows of the k-th row of a kind at seam or corner q.
size_t row_index(const std::vector<Relationship>& rows, Relation kind, size_t q, size_t k) {

    size_t seen = 0;

    for (size_t i = 0; i < rows.size(); i++)
        if (rows[i].kind == kind && rows[i].seam_or_corner == q && seen++ == k)
            return i;

    throw std::out_of_range(fmt::format("no row {} of {} at {}", k, relation_name(kind), q));
}

/// A value for print, a negative zero or a rounding residue shown as 0.
double tidy(double value) {
    return std::abs(value) < 1e-9 ? 0.0 : value;
}

/// The colour of a screw kind.
const Color& kind_color(Relation kind) {
    return SCREW_COLORS[static_cast<size_t>(std::find(SCREW_RELATIONS.begin(), SCREW_RELATIONS.end(), kind) - SCREW_RELATIONS.begin())];
}

/// The level a lifted screw was built at, below the datum.
double datum_z(const Line& screw) {
    return screw.start()[2] - H;
}

/// The seventh of the depth a level is: the level set index of a corner screw.
size_t seventh(double z, double static_h) {
    return static_cast<size_t>(std::lround(-z * CORNER_LEVELS / static_h));
}

/// Whether a lifted screw's head lies within CORNER_REACH of a datum point in plan.
bool at(const Line& screw, const Point& corner) {
    return std::hypot(screw.start()[0] - corner[0], screw.start()[1] - corner[1]) < CORNER_REACH;
}

/// A member's axis at level z as floor_screws.cpp's axis() takes it: through the middle of the first trace's start and its foot on the second trace, along the first trace.
Line member_axis(const std::array<Plane, 2>& faces, double z) {

    const Line line0 = plane_plane(faces[0], level(z)).value();
    const Line line1 = plane_plane(faces[1], level(z)).value();
    const Point p0 = line0.start();
    const Point p1 = line1.closest_point(p0, false).second;
    const Point centre = p0 + (p1 - p0) * 0.5;

    return Line::from_points(centre, centre + line0.to_direction());
}

/// A plane's horizontal trace at level z, from the foot of a to the foot of b.
Line trace_between(const Plane& plane, double z, const Point& a, const Point& b) {

    const Line line = plane_plane(plane, level(z)).value();

    return Line::from_points(line.closest_point(a, false).second, line.closest_point(b, false).second);
}

/// The distance of a point from a plane, positive on the side of inside, as floor_screws.cpp's depth() measures it.
double depth_of(const Point& point, const Plane& plane, const Point& inside) {
    return signed_distance(inside, plane) < 0.0 ? -signed_distance(point, plane) : signed_distance(point, plane);
}

/// The plane with its origin moved to the foot of a point, so its drawn square sits there.
Plane near(const Plane& plane, const Point& point) {
    return Plane::from_frame(point - plane.z_axis() * signed_distance(point, plane), plane.x_axis(), plane.y_axis(), plane.z_axis());
}

/// A lofted solid as a wireframe: its two loops and the edges between their corners.
void wire(Frame& frame, const std::array<Polyline, 2>& loops, const Color& color, double width) {

    frame.polyline(loops[0], color, width);
    frame.polyline(loops[1], color, width);

    for (size_t i = 0; i < std::min(loops[0].point_count(), loops[1].point_count()); i++)
        frame.line(Line::from_points(loops[0].get_point(i), loops[1].get_point(i)), color, width);
}

/// The two loops of a member outline, lifted, as thin lines: a member seen through.
void loops(Frame& frame, const Outline& outline, const Color& color) {
    frame.polyline(up(outline.top), color, 1.5);
    frame.polyline(up(outline.bottom), color, 1.5);
}

/// Whether a point lies in a box.
bool in(const Box& box, const Point& point) {
    return point[0] >= box[0] && point[0] <= box[3] && point[1] >= box[1] && point[1] <= box[4] && point[2] >= box[2] && point[2] <= box[5];
}

/// Every screw row's screws whose heads sit at a corner, in one colour.
void corner_screws(Frame& frame, const std::vector<Relationship>& rows, const Point& corner, double width, const Color& color) {

    for (const Relationship& row : rows)
        for (const Line& screw : row.screws)
            if (at(screw, corner))
                frame.line(screw, color, width, false, true);
}

/// The ring faces an oculus screw of quarter q at end k reads at level z, as floor_screws.cpp's oculus() and oculus_screw() build them.
struct RingAim {
    Plane inner; // ring.inner: the ring beam's inner face.
    Plane end; // ring.end: the ring beam's end plane at this corner.
    Plane beam_end; // faces.beam_end: the seam beam's inner face.
    Point ring_body; // ring.body.
    Point beam_body; // faces.beam_body.
    Point end_point; // end: the contact's top edge end at this corner.
    Vector along; // ring.along.
    double thickness = 0.0; // The thicker of the oculus beam and the ring beam.
    Point wedge_start; // ring.wedge_start.
    double band = 0.0; // ring.band.
    Point start; // Where ring.inner's trace meets faces.beam_end at z.
    Vector across; // Square to the contact edge, from the ring into the quarter.
};

RingAim ring_aim(const FloorGuide& guide, size_t q, size_t k, double z) {

    const Outline beam = guide.quarter(q).inner_beams()[1];
    const Outline ring = guide.oculus()[q];
    const std::vector<Point> loop = beam.bottom.get_points();
    RingAim result;
    result.inner = guide.oculus_edges[q].ring_inner;
    result.end = k == 0 ? guide.oculus_edges[(q + 1) % 4].tilted : guide.oculus_edges[(q + 3) % 4].ring_inner;
    result.beam_end = guide.geometry[q].planes.inner_beams[k == 0 ? 0 : 2][1];
    result.ring_body = middle(ring);
    result.beam_body = middle(beam);
    result.end_point = k == 0 ? loop[0] : loop[1];
    result.along = ((k == 0 ? loop[1] : loop[0]) - result.end_point).normalized();
    result.thickness = std::max(outline_thickness(beam), outline_thickness(ring));
    result.wedge_start = result.end_point + result.along * (WEDGE_MARGIN * result.thickness);
    result.band = 0.5 * guide.parameters.inner_beams;
    result.start = line_plane(plane_plane(result.inner, level(z)).value(), result.beam_end).value();
    const Vector flat = Vector((result.beam_body - result.ring_body)[0], (result.beam_body - result.ring_body)[1], 0.0);
    result.across = (flat - result.along * flat.dot(result.along)).normalized();

    return result;
}

/// The aim of a lifted oculus screw: its head's offset along ring.along from start and its angle off across, degrees.
std::array<double, 2> aim_of(const RingAim& aim, const Line& screw) {

    const Vector u = screw.to_direction();
    const double offset = (screw.start() - up(aim.start)).dot(aim.along);
    const double angle = std::atan2(-u.dot(aim.along), u.dot(aim.across)) * 180.0 / M_PI;

    return {offset, angle};
}

/// The contact plane of an oculus screw, its normal turned to the ring side.
Vector ring_side(const Plane& contact, const Point& ring_body) {
    return (ring_body - contact.origin()).dot(contact.z_axis()) < 0.0 ? -contact.z_axis() : contact.z_axis();
}

// ═══════════════════════════════════════════════════════════════════════════
// The rows and their levels
// ═══════════════════════════════════════════════════════════════════════════

/// Every row of screw_relationships in its kind's colour, the first of each kind named by its index.
void row_order(const Context& context, const std::vector<Relationship>& rows) {

    size_t count = 0;

    for (const Relationship& row : rows)
        count += row.screws.size();

    Frame frame(CHAPTER, 201, "row_order", fmt::format("screw_relationships: {} rows, {} screws; per quarter six rows, then the ring corners and the oculus", rows.size(), count), "top", BAY);
    frame.key = true;
    plan_context(frame, context.guide, true);

    for (const Relationship& row : rows)
        for (const Line& screw : row.screws)
            frame.line(screw, kind_color(row.kind), 4.0, false, true);

    const std::array<std::array<size_t, 2>, 5> picks = {{{0, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}}};

    for (size_t i = 0; i < SCREW_RELATIONS.size(); i++) {
        const size_t index = row_index(rows, SCREW_RELATIONS[i], picks[i][0], picks[i][1]);
        const Line& screw = rows[index].screws.front();
        frame.label(fmt::format("rows[{}]: {}, q {}", index, relation_name(SCREW_RELATIONS[i]), picks[i][0]), i == 0 ? screw.start() : screw.end());
    }

    frame.write(context.dir);
}

/// The joint depth at oculus corner 0 in elevation, its six seventh levels and the screws that sit on each: the screws built, the levels the variable, the depth read.
void levels(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const double static_h = guide.parameters.static_h();
    const Point corner = guide.oculus_corners[0];
    const Vector side(250.0, 0.0, 0.0);
    Frame frame(CHAPTER, 202, "levels", fmt::format("static_h = height - rise = {:.0f} split in sevenths: six levels, two per corner screw kind", static_h), "front", {corner[0] - 450.0, corner[1] - 100.0, H - 250.0, corner[0] + 450.0, corner[1] + 100.0, H + 50.0});

    const Vector deep(0.0, 0.0, -static_h);
    frame.polyline(up(Polyline({corner - side, corner + side, corner + side + deep, corner - side + deep}).closed()), INPUT, 2.0);
    std::map<size_t, std::set<std::string>> kinds;

    for (const Relationship& row : rows)
        for (const Line& screw : row.screws)
            if (at(screw, corner)) {
                frame.line(screw, BUILT, 3.0, false, true);
                kinds[seventh(datum_z(screw), static_h)].insert(relation_name(row.kind).substr(6));
            }

    for (size_t i = 1; i < static_cast<size_t>(CORNER_LEVELS); i++) {
        const Vector z(0.0, 0.0, -static_h * static_cast<double>(i) / CORNER_LEVELS);
        frame.line(up(Line::from_points(corner - side + z, corner + side + z)), VARIABLE, 1.5, true);
        std::string names;

        for (const std::string& name : kinds[i])
            names += (names.empty() ? "" : ", ") + name;

        frame.label(fmt::format("{}/7 = {:.1f}: {}", i, z[2], names), up(i % 2 == 1 ? corner - side + z : corner + side + z));
    }

    frame.label(fmt::format("static_h = {:.0f}", static_h), up(corner - side + deep));
    frame.write(context.dir);
}

/// One face pair or oculus face of quarter 0 at the datum: its trace between two anchors, its normal and its name.
void face(Frame& frame, const Plane& plane, const Point& a, const Point& b, double t, const std::string& name, const Color& color) {

    const Line trace = trace_between(plane, 0.0, a, b);
    const Point pin = trace.point_at(t);
    frame.line(up(trace), color, 3.0);
    frame.line(up(Line::from_points(pin, pin + plane.z_axis() * 150.0)), INPUT, 2.0, false, true);
    frame.label(name, up(pin));
}

/// The planes of quarter 0 the screw rules read, as traces at the datum with their normals.
void faces(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const std::vector<Point>& polygon = guide.geometry[0].polygon;
    const OculusEdge& oculus = guide.oculus_edges[0];
    Frame frame(CHAPTER, 203, "faces", "Faces the screws read: cp's outer rib and seam beam pairs, the oculus edge's tilted, back and ring_inner", "top", {-1350.0, -3120.0, H - 10.0, 200.0, -250.0, H + 10.0});
    frame.polyline(up(Polyline(polygon).closed()), GREY, 1.0);

    face(frame, cp.outer_ribs[0][0], polygon[1], polygon[0], 0.35, "outer_ribs[0][0]: the bay edge", BUILT);
    face(frame, cp.outer_ribs[0][1], polygon[1], polygon[0], 0.12, fmt::format("outer_ribs[0][1]: + {:.0f}", guide.parameters.outer_ribs), BUILT);
    face(frame, cp.inner_beams[0][0], polygon[1], polygon[2], 0.3, "inner_beams[0][0]: the seam plane", BUILT);
    face(frame, cp.inner_beams[0][1], polygon[1], polygon[2], 0.6, fmt::format("inner_beams[0][1]: + {:.0f}", guide.parameters.inner_beams), BUILT);
    face(frame, oculus.tilted, polygon[2], polygon[3], 0.25, "inner_beams[1][0] = tilted", BUILT);
    face(frame, oculus.back, polygon[2], polygon[3], 0.45, "inner_beams[1][1] = back", BUILT);
    face(frame, oculus.ring_inner, polygon[2], polygon[3], 0.65, "oculus_edges[0].ring_inner", RESULT);
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// The primitives
// ═══════════════════════════════════════════════════════════════════════════

/// The oculus beam's two faces sliced at one level: the traces, p0, its foot p1 and the axis between them.
void trace_axis(const Context& context) {

    const FloorGuide& guide = context.guide;
    const std::array<Plane, 2>& beam_faces = guide.geometry[0].planes.inner_beams[1];
    const Outline beam = guide.quarter(0).inner_beams()[1];
    const double z = -guide.parameters.static_h() * 3.0 / CORNER_LEVELS;
    const Line line0 = plane_plane(beam_faces[0], level(z)).value();
    const Line line1 = plane_plane(beam_faces[1], level(z)).value();
    const Point p0 = line0.start();
    const Point p1 = line1.closest_point(p0, false).second;
    const Line axis = member_axis(beam_faces, z);
    const Vector d = line0.to_direction();
    const Vector across = (p1 - p0).normalized();
    Frame frame(CHAPTER, 204, "trace_axis", fmt::format("trace(plane, z) cuts a face with level(z); axis(faces, z) runs midway between two traces, here z = {:.1f}", z), "iso", {-1100.0, -1100.0, H - 240.0, 0.0, 0.0, H + 40.0});
    frame.orbit = "0,70"; // 20 degrees steeper than iso, so the level rectangle opens and the two traces part.
    frame.distance = 0.8;

    loops(frame, beam, GREY);
    const Point centre = axis.start();
    frame.polyline(up(Polyline({centre - d * 420.0 - across * 160.0, centre + d * 420.0 - across * 160.0, centre + d * 420.0 + across * 160.0, centre - d * 420.0 + across * 160.0}).closed()), VARIABLE, 1.0);
    frame.line(up(Line::from_points(p0 - d * 380.0, p0 + d * 380.0)), RESULT, 2.5);
    frame.line(up(Line::from_points(p1 - d * 380.0, p1 + d * 380.0)), RESULT, 2.5);
    frame.line(up(Line::from_points(p0, p1)), INPUT, 1.5, true);
    frame.line(up(Line::from_points(centre - d * 380.0, centre + d * 380.0)), BUILT, 4.0, false, true);
    frame.point(up(p0), RESULT);
    frame.point(up(p1), RESULT);
    frame.point(up(centre), BUILT);

    frame.label("faces[0] = inner_beams[1][0]: tilted", up(beam.bottom.get_point(2)));
    frame.label("faces[1] = inner_beams[1][1]: back", up(beam.top.get_point(1)));
    frame.label(fmt::format("level(z), z = {:.1f}", z), up(centre - d * 420.0 + across * 160.0));
    frame.label("line0 = trace(faces[0], z)", up(p0 + d * 330.0));
    frame.label("line1 = trace(faces[1], z)", up(p1 - d * 330.0));
    frame.label("p0 = line0.start()", up(p0));
    frame.label("p1: foot of p0 on line1", up(p1));
    frame.label("axis(faces, z)", up(centre + d * 380.0));
    frame.write(context.dir);
}

/// The oculus beam's loop centroids and body, and the + side of a face that depth() measures.
void body_depth(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Outline beam = guide.quarter(0).inner_beams()[1];
    const Point top = area_centroid(beam.top);
    const Point bottom = area_centroid(beam.bottom);
    const Point body = middle(beam);
    const Vector edge = (guide.oculus_corners[3] - guide.oculus_corners[0]).normalized();
    const Plane back = near(guide.geometry[0].planes.inner_beams[1][1], body + edge * 260.0);
    const Vector inward = back.z_axis() * (depth_of(back.origin() + back.z_axis(), back, body) > 0.0 ? 1.0 : -1.0);
    const Point plus = back.origin() + inward * 45.0 + edge * 60.0;
    const Point minus = back.origin() - inward * 60.0 - edge * 60.0;
    Frame frame(CHAPTER, 205, "body_depth", "body(outline): the middle of its loops' area centroids; depth(point, plane, inside) > 0 on inside's side", "top", {-850.0, -850.0, H - 220.0, -200.0, -200.0, H + 30.0});

    loops(frame, beam, GREY);
    frame.line(up(Line::from_points(top, bottom)), INPUT, 1.5, true);
    frame.point(up(top), INPUT);
    frame.point(up(bottom), INPUT);
    frame.point(up(body), BUILT, 16.0);
    frame.plane(up(back), INPUT);
    frame.point(up(plus), VARIABLE);
    frame.point(up(minus), VARIABLE);

    frame.label("area_centroid(outline.top)", up(top));
    frame.label("area_centroid(outline.bottom)", up(bottom));
    frame.label(fmt::format("body(outline) = ({:.1f}, {:.1f}, {:.1f})", body[0], body[1], body[2]), up(body));
    frame.label("plane: inner_beams[1][1], back", up(back.origin()));
    frame.label(fmt::format("+ depth = {:.0f}: the body's side", depth_of(plus, back, body)), up(plus));
    frame.label(fmt::format("- depth = {:.0f}", depth_of(minus, back, body)), up(minus));
    frame.write(context.dir);
}

/// along_axis at a T joint: the oculus beam's axis at a mitre level run back to the seam plane, the head there and the screw.
void along_axis(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Line& screw = rows[row_index(rows, Relation::screw_beam_mitre, 0, 0)].screws.front();
    const double z = datum_z(screw);
    const Line axis = member_axis(cp.inner_beams[1], z);
    const Point head = line_plane(axis, cp.inner_beams[0][0]).value();
    const Vector d = screw.to_direction();
    const Point a = guide.oculus_corners[0] + Vector(0.0, -250.0, 0.0);
    const Point b = guide.oculus_corners[0] + Vector(-450.0, 450.0, 0.0);
    Frame frame(CHAPTER, 206, "along_axis", fmt::format("along_axis: the butting member's axis meets far_face at the head, the screw runs {:.0f} on along it", screw.length()), "top", {-420.0, -1230.0, H - 100.0, 180.0, -700.0, H + 10.0});

    const Line tilted = trace_between(cp.inner_beams[1][0], z, a, b);
    const Line back = trace_between(cp.inner_beams[1][1], z, a, b);
    const Line seam = trace_between(cp.inner_beams[0][0], z, a, guide.oculus_corners[0] + Vector(0.0, 200.0, 0.0));
    frame.line(up(tilted), INPUT, 2.0);
    frame.line(up(back), INPUT, 2.0);
    frame.line(up(seam), VARIABLE, 4.0);
    frame.line(up(Line::from_points(head, head + d * 520.0)), INPUT, 1.5, true);
    frame.point(up(head), BUILT);
    frame.line(screw, BUILT, 4.0, false, true);

    frame.label("butting = inner_beams[1]: axis(butting, z)", up(head + d * 430.0));
    frame.label("trace of butting[0], tilted", up(tilted.point_at(0.55)));
    frame.label("trace of butting[1], back", up(back.point_at(0.25)));
    frame.label("far_face = inner_beams[0][0]", up(seam.point_at(0.15)));
    frame.label("head = line_plane(axis, far_face)", screw.start());
    frame.label("head + d * SCREW_LENGTH", screw.end());
    frame.write(context.dir);
}

/// from_seam_face at outer rib 1 of quarter 0: the rib axis meets the seam plane, the head moves across the rib, the screw runs along it.
void from_seam_face(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const std::vector<Point>& polygon = guide.geometry[0].polygon;
    const std::array<Plane, 2>& rib = cp.outer_ribs[1];
    const std::array<Plane, 2>& beam = cp.inner_beams[2];
    const Line& screw = rows[row_index(rows, Relation::screw_rib_beam, 0, 1)].screws.front();
    const double z = datum_z(screw);
    const Line axis = member_axis(rib, z);
    const Point seam = line_plane(axis, beam[0]).value();
    const Point inner = line_plane(axis, beam[1]).value();
    const double offset = (screw.start() - up(seam)).dot(rib[0].z_axis());
    Frame frame(CHAPTER, 207, "from_seam_face", fmt::format("from_seam_face, outer rib 1 at z = {:.0f}: seam point on the rib axis, head moved {:+.0f} across the rib", z, offset), "top", {-3080.0, -330.0, H - 30.0, -2800.0, 110.0, H + 10.0});

    const Line rib0 = trace_between(rib[0], z, polygon[4], polygon[0]);
    const Line rib1 = trace_between(rib[1], z, polygon[4], polygon[0]);
    const Line beam0 = trace_between(beam[0], z, polygon[4], polygon[3]);
    const Line beam1 = trace_between(beam[1], z, polygon[4], polygon[3]);
    frame.line(up(rib0), INPUT, 3.0);
    frame.line(up(rib1), INPUT, 3.0);
    frame.line(up(beam0), INPUT, 3.0);
    frame.line(up(beam1), INPUT, 3.0);
    frame.line(up(Line::from_points(axis.closest_point(polygon[0], false).second, seam + (seam - inner) * 1.5)), INPUT, 1.5, true);
    frame.line(Line::from_points(up(seam), screw.start()), VARIABLE, 3.0);
    frame.point(up(seam), INPUT);
    frame.point(up(inner), INPUT);
    frame.point(screw.start(), BUILT);
    frame.line(screw, BUILT, 4.0, false, true);

    frame.label("rib[0] = outer_ribs[1][0]", up(rib0.point_at(0.1)));
    frame.label("beam[0] = inner_beams[2][0]: seam plane", up(beam0.point_at(0.08)));
    frame.label("beam[1] = inner_beams[2][1]", up(beam1.point_at(0.08)));
    frame.label("line = axis(rib, z)", up(axis.closest_point(rib0.point_at(0.1), false).second));
    frame.label("seam = line_plane(line, beam[0])", up(seam));
    frame.label(fmt::format("head = seam + rib[0].z_axis() * {:.0f}", offset), screw.start());
    frame.label("along: towards line_plane(line, beam[1])", up(inner));
    frame.label("head + along * SCREW_LENGTH", screw.end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Outer ribs into seam beams
// ═══════════════════════════════════════════════════════════════════════════

/// Seam beam 0 of quarter 0, the beam outer rib 0 meets, with its top loop's corners.
void rib_beam_beam(const Context& context) {

    const FloorGuide& guide = context.guide;
    const Outline beam = guide.quarter(0).inner_beams()[0];
    const Outline rib = guide.quarter(0).outer_ribs()[0];
    const QuarterMembers& quarter = context.members.members.quarters[0];
    Frame frame(CHAPTER, 208, "rib_beam_beam", "rib_beam: outer rib k meets seam beam 0 for k 0, 2 for k 1, lofted between its two seam faces", "iso", {-700.0, -3150.0, H - 700.0, 300.0, -750.0, H + 60.0});

    frame.element(quarter.outer_ribs[0].element, GREY);
    frame.element(quarter.inner_beams[0].element, BUILT);

    for (size_t i = 0; i < 4; i++) {
        frame.point(up(beam.top.get_point(i)), VARIABLE);
        frame.label(fmt::format("top[{}]", i), up(beam.top.get_point(i)));
    }

    frame.label(fmt::format("{}: beam = 0", member_name(Family::inner_beams, 0, 0)), up(middle(beam)));
    frame.label(member_name(Family::outer_ribs, 0, 0), up(rib.top.get_point(rib.top.point_count() - 3)));
    frame.write(context.dir);
}

/// Outer rib 0's end face on the seam beam in elevation: its bottom by end_level and the two screw levels RIB_END_MARGIN inside.
void rib_beam_levels(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const Relationship& row = rows[row_index(rows, Relation::screw_rib_beam, 0, 0)];
    const double bottom = end_level(guide.quarter(0).outer_ribs()[0], guide.quarter(0).rib_seam_ends()[0]);
    const double margin = -datum_z(row.screws[0]);
    const Point centre = area_centroid(row.contact);
    const Vector wide(0.0, 90.0, 0.0);
    Frame frame(CHAPTER, 209, "rib_beam_levels", fmt::format("rib_beam, seam through ribs: screws {:.0f} below the rib top and {:.0f} above its end bottom", margin, margin), "right", {-200.0, -3070.0, H - 280.0, 100.0, -2830.0, H + 30.0});

    frame.element(context.members.members.quarters[0].outer_ribs[0].element, GREY);
    frame.polyline(row.contact, INPUT, 5.0);

    for (const Line& screw : row.screws) {
        const Point at_level(centre[0], centre[1], screw.start()[2]);
        frame.line(Line::from_points(at_level - wide, at_level + wide), VARIABLE, 2.5, true);
    }

    const Point upper(centre[0], centre[1], row.screws[0].start()[2]);
    const Point lower(centre[0], centre[1], row.screws[1].start()[2]);
    frame.label(fmt::format("end_level(rib, rib_seam_ends()[0]) = {:.1f}", bottom), Point(centre[0], centre[1], H + bottom));
    frame.label(fmt::format("-RIB_END_MARGIN = {:.0f}", -margin), upper - wide);
    frame.label(fmt::format("end_level + RIB_END_MARGIN = {:.1f}", datum_z(row.screws[1])), lower + wide);
    frame.label("rib_seam_ends()[0] = inner_beams[0][1]", Point(centre[0], centre[1], H));
    const Outline rib = guide.quarter(0).outer_ribs()[0];
    frame.label(member_name(Family::outer_ribs, 0, 0), up(rib.top.get_point(rib.top.point_count() - 4)));
    frame.write(context.dir);
}

/// The rib axis at the seam end of outer rib 0: where it meets the seam plane and the seam beam's inner face, and along between them.
void rib_beam_seam_point(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const std::vector<Point>& polygon = guide.geometry[0].polygon;
    const double z = datum_z(rows[row_index(rows, Relation::screw_rib_beam, 0, 0)].screws.front());
    const Line axis = member_axis(cp.outer_ribs[0], z);
    const Point seam = line_plane(axis, cp.inner_beams[0][0]).value();
    const Point inner = line_plane(axis, cp.inner_beams[0][1]).value();
    const Vector along = (inner - seam).normalized();
    Frame frame(CHAPTER, 210, "rib_beam_seam_point", fmt::format("rib_beam: the axis of outer_ribs[0], y = {:.0f}, meets the seam plane at seam; along = ({:.0f}, {:.0f}, {:.0f})", seam[1], tidy(along[0]), tidy(along[1]), tidy(along[2])), "top", {-320.0, -3070.0, H - 30.0, 100.0, -2830.0, H + 10.0});

    const Line rib0 = trace_between(cp.outer_ribs[0][0], z, polygon[1], polygon[0]);
    const Line rib1 = trace_between(cp.outer_ribs[0][1], z, polygon[1], polygon[0]);
    const Line beam0 = trace_between(cp.inner_beams[0][0], z, polygon[1], polygon[2]);
    const Line beam1 = trace_between(cp.inner_beams[0][1], z, polygon[1], polygon[2]);
    frame.line(up(rib0), GREY, 3.0);
    frame.line(up(rib1), GREY, 3.0);
    frame.line(up(beam0), INPUT, 3.0);
    frame.line(up(beam1), INPUT, 3.0);
    frame.line(up(Line::from_points(seam - along * 100.0, seam + along * 300.0)), INPUT, 1.5, true);
    frame.line(up(Line::from_points(seam, seam + along * 120.0)), VARIABLE, 4.0, false, true);
    frame.point(up(seam), BUILT);
    frame.point(up(inner), INPUT);

    frame.label(fmt::format("axis(cp.outer_ribs[0], z): y = {:.0f}", seam[1]), up(seam + along * 250.0));
    frame.label("cp.inner_beams[0][0]: x = 0", up(beam0.point_at(0.06)));
    frame.label("cp.inner_beams[0][1]", up(beam1.point_at(-0.02)));
    frame.label(fmt::format("seam = ({:.0f}, {:.0f})", tidy(seam[0]), seam[1]), up(seam));
    frame.label("line_plane(axis, cp.inner_beams[0][1])", up(inner));
    frame.label("along", up(seam + along * 120.0));
    frame.write(context.dir);
}

/// The two outer ribs that meet at seam 0, quarter 0's rib 0 and quarter 1's rib 1: their screws either side of the axis, the heads apart on the seam plane.
void rib_beam_heads(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const size_t index0 = row_index(rows, Relation::screw_rib_beam, 0, 0);
    const size_t index1 = row_index(rows, Relation::screw_rib_beam, 1, 1);
    const Line& screw0 = rows[index0].screws.front();
    const Line& screw1 = rows[index1].screws.front();
    const double z = datum_z(screw0);
    const ConstructionPlanes& cp0 = guide.geometry[0].planes;
    const ConstructionPlanes& cp1 = guide.geometry[1].planes;
    const Point seam0 = line_plane(member_axis(cp0.outer_ribs[0], z), cp0.inner_beams[0][0]).value();
    const Point seam1 = line_plane(member_axis(cp1.outer_ribs[1], z), cp1.inner_beams[2][0]).value();
    const double gap = (screw1.start() - screw0.start()).magnitude();
    const double offset0 = (screw0.start() - up(seam0)).dot(cp0.outer_ribs[0][0].z_axis());
    const double offset1 = (screw1.start() - up(seam1)).dot(cp1.outer_ribs[1][0].z_axis());
    const Point a = guide.edges[0].midpoint + Vector(-400.0, 0.0, 0.0);
    const Point b = guide.edges[0].midpoint + Vector(400.0, 0.0, 0.0);
    const Point c = guide.edges[0].midpoint + Vector(0.0, 300.0, 0.0);
    Frame frame(CHAPTER, 211, "rib_beam_heads", fmt::format("rib_beam at seam 0: offset {:+.0f} for quarter 0's rib 0, {:+.0f} for quarter 1's rib 1, heads {:.0f} apart", offset0, offset1, gap), "top", {-330.0, -3080.0, H - 30.0, 330.0, -2830.0, H + 10.0});

    for (const Plane& plane : {cp0.outer_ribs[0][0], cp0.outer_ribs[0][1]})
        frame.line(up(trace_between(plane, z, a, b)), GREY, 2.0);

    for (const Plane& plane : {cp0.inner_beams[0][1], cp1.inner_beams[2][1]})
        frame.line(up(trace_between(plane, z, guide.edges[0].midpoint, c)), INPUT, 2.0);

    const Line seam_plane = trace_between(cp0.inner_beams[0][0], z, guide.edges[0].midpoint, c);
    frame.line(up(seam_plane), INPUT, 3.0);
    frame.line(screw0, BUILT, 4.0, false, true);
    frame.line(screw1, RESULT, 4.0, true, true);
    frame.line(Line::from_points(screw0.start(), screw1.start()), VARIABLE, 3.0);
    frame.point(screw0.start(), BUILT);
    frame.point(screw1.start(), RESULT);

    frame.label(fmt::format("rows[{}]: quarter 0, rib 0, offset {:+.0f}", index0, offset0), screw0.end());
    frame.label(fmt::format("rows[{}]: quarter 1, rib 1, offset {:+.0f}", index1, offset1), screw1.end());
    frame.label(fmt::format("{:.0f} between the heads", gap), screw0.start() + (screw1.start() - screw0.start()) * 0.5);
    frame.label("the seam plane", up(seam_plane.point_at(0.45)));
    frame.write(context.dir);
}

/// The rib_beam contact: outer rib 0's end face on the seam beam's inner face, both screws crossing it.
void rib_beam_contact(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const Relationship& row = rows[row_index(rows, Relation::screw_rib_beam, 0, 0)];
    const Outline beam = guide.quarter(0).inner_beams()[0];
    const std::array<std::string, 4> corners = {"rib_top[0]", "rib_top[n - 2]", "rib_bottom[n - 2]", "rib_bottom[0]"};
    Frame frame(CHAPTER, 212, "rib_beam_contact", "rib_beam contact: the rib's end {rib_top[0], rib_top[n-2], rib_bottom[n-2], rib_bottom[0]} on x = -60", "iso", {-350.0, -3100.0, H - 260.0, 100.0, -2550.0, H + 40.0});

    frame.element(context.members.members.quarters[0].inner_beams[0].element, GREY);
    loops(frame, guide.quarter(0).outer_ribs()[0], GREY);
    frame.polyline(row.contact, BUILT, 5.0);

    for (const Line& screw : row.screws)
        frame.line(screw, INPUT, 4.0, false, true);

    for (size_t i = 0; i < corners.size(); i++)
        frame.label(fmt::format("contact[{}] = {}", i, corners[i]), row.contact.get_point(i));

    frame.label(fmt::format("b = {}", member_name(Family::inner_beams, 0, 0)), up(beam.top.get_point(0) + (beam.top.get_point(1) - beam.top.get_point(0)) * 0.15));
    frame.label("screws[0]", row.screws[0].end());
    frame.label("screws[1]", row.screws[1].end());
    frame.write(context.dir);
}

/// The tied variant on the 6000 x 4800 bay: the seam beam ends on the rib, the screws run from the bay edge along the beam's axis.
void rib_beam_tied(const Context& context) {

    const FloorGuide& tied = context.tied;
    const ConstructionPlanes& cp = tied.geometry[0].planes;
    const std::vector<Point>& polygon = tied.geometry[0].polygon;
    const std::vector<Relationship> rows = screw_relationships(tied);
    const Relationship& row = rows[row_index(rows, Relation::screw_rib_beam, 0, 0)];
    const Line& screw = row.screws.front();
    const double z = datum_z(screw);
    const Line axis = member_axis(cp.inner_beams[0], z);
    const Vector d = screw.to_direction();
    const Line far_face = trace_between(cp.outer_ribs[0][0], z, polygon[1], polygon[0]);
    Frame frame(CHAPTER, 213, "rib_beam_tied", fmt::format("Tied seam, seam_through_ribs false: along_axis from the bay edge, levels {:.2f} and {:.2f} of static_h", -datum_z(row.screws[0]) / tied.parameters.static_h(), -datum_z(row.screws[1]) / tied.parameters.static_h()), "top", {-330.0, -2470.0, H - 30.0, 170.0, -2130.0, H + 10.0});

    loops(frame, tied.quarter(0).outer_ribs()[0], GREY);
    loops(frame, tied.quarter(0).inner_beams()[0], GREY);
    frame.line(up(Line::from_points(axis.closest_point(polygon[1], false).second, axis.closest_point(polygon[2], false).second)), INPUT, 1.5, true);
    frame.polyline(row.contact, RESULT, 4.0);

    for (const Line& line : row.screws)
        frame.line(line, BUILT, 4.0, false, true);

    frame.point(screw.start(), BUILT);
    frame.label(fmt::format("axis(inner_beams[0], z): x = {:.0f}", screw.start()[0]), screw.start() + d * 250.0);
    frame.label("far_face = outer_ribs[0][0]", up(far_face.closest_point(screw.start() + Vector(-200.0, 0.0, 0.0), false).second));
    frame.label("head", screw.start());
    frame.label("head + d * SCREW_LENGTH", screw.end());
    frame.label("contact: the beam's end on outer_ribs[0][1]", area_centroid(row.contact) + Vector(-30.0, 0.0, 0.0));
    frame.write(context.dir);
}

/// screw_row: the screws built at the datum, read, and lifted by bay_height to the floor, built.
void lift(const Context& context, const std::vector<Relationship>& rows) {

    const Xform down = Xform::translation(0.0, 0.0, -H);
    const Relationship& mitre = rows[row_index(rows, Relation::screw_beam_mitre, 0, 0)];
    Frame frame(CHAPTER, 214, "lift", fmt::format("screw_row: the plane, the contact and every screw of a row lifted by bay_height = {:.0f}", context.guide.parameters.bay_height), "front", {-3200.0, -1100.0, -400.0, 400.0, -900.0, H + 200.0});

    for (const Relationship& row : rows) {
        if (row.seam_or_corner != 0)
            continue;

        frame.polyline(row.contact.transformed(down), INPUT, 1.5);
        frame.polyline(row.contact, BUILT, 1.5);

        for (const Line& screw : row.screws) {
            frame.line(screw.transformed(down), INPUT, 3.0, false, true);
            frame.line(screw, BUILT, 3.0, false, true);
        }
    }

    const Line& screw = mitre.screws.front();
    frame.line(Line::from_points(screw.start().transformed(down), screw.start()), VARIABLE, 2.0, true, true);
    frame.label("the screws at the datum, z 0", screw.end().transformed(down));
    frame.label("lifted(screw, bay_height)", screw.end());
    frame.label(fmt::format("+ bay_height = {:.0f}", context.guide.parameters.bay_height), screw.start() + Vector(0.0, 0.0, -H * 0.5));
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus corners
// ═══════════════════════════════════════════════════════════════════════════

/// The beam_mitre contact: the oculus beam's end on seam beam 0's inner face, leaning with the tilted face.
void mitre_contact(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const Relationship& row = rows[row_index(rows, Relation::screw_beam_mitre, 0, 0)];
    const std::array<std::string, 4> corners = {"top[3]", "top[0]", "bottom[0]", "bottom[3]"};
    Frame frame(CHAPTER, 215, "mitre_contact", "beam_mitre contact, k 0: oculus beam end {top[3], top[0], bottom[0], bottom[3]} on inner_beams[0][1]", "iso", {-450.0, -1300.0, H - 260.0, 150.0, -650.0, H + 40.0});
    frame.distance = 0.6;

    frame.element(context.members.members.quarters[0].inner_beams[0].element, GREY);
    loops(frame, guide.quarter(0).inner_beams()[1], GREY);
    frame.polyline(row.contact, BUILT, 5.0);

    for (size_t i = 0; i < corners.size(); i++)
        frame.label(corners[i], row.contact.get_point(i));

    const Point lean = row.contact.get_point(2) + (row.contact.get_point(3) - row.contact.get_point(2)) * 0.5;
    frame.label(fmt::format("leans by oculus_plane_angle = {:.0f} deg", guide.parameters.oculus_plane_angle), lean);
    frame.write(context.dir);
}

/// The mitre screws at oculus corner 0: quarter 0's k 0 and quarter 1's k 1, heads on the seam plane at different levels.
void mitre_screws(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const double static_h = guide.parameters.static_h();
    const Relationship& own = rows[row_index(rows, Relation::screw_beam_mitre, 0, 0)];
    const Relationship& next = rows[row_index(rows, Relation::screw_beam_mitre, 1, 1)];
    Frame frame(CHAPTER, 216, "mitre_screws", "beam_mitre: along_axis of the oculus beam from the seam plane; MITRE_LEVELS part two quarters' heads", "top", {-260.0, -1200.0, H - 10.0, 260.0, -820.0, H + 10.0});
    frame.key = true;

    for (size_t q = 0; q < 2; q++)
        for (size_t i = 0; i < 3; i++)
            if (!(q == 0 && i == 2) && !(q == 1 && i == 0))
                frame.polyline(up(guide.geometry[q].quads.inner_beams[i].closed()), GREY, 2.0);

    for (const Line& screw : own.screws)
        frame.line(screw, BUILT, 4.0, false, true);

    for (const Line& screw : next.screws)
        frame.line(screw, RESULT, 4.0, true, true);

    frame.label(fmt::format("q 0, k 0 at {}/7: head on x = 0", seventh(datum_z(own.screws[0]), static_h)), own.screws[0].start());
    frame.label(fmt::format("q 0, k 0 at {}/7", seventh(datum_z(own.screws[1]), static_h)), own.screws[1].end());
    frame.label(fmt::format("q 1, k 1 at {}/7", seventh(datum_z(next.screws[0]), static_h)), next.screws[0].end());
    frame.label(fmt::format("q 1, k 1 at {}/7", seventh(datum_z(next.screws[1]), static_h)), next.screws[1].start() + (next.screws[1].end() - next.screws[1].start()) * 0.5);
    frame.write(context.dir);
}

/// corner_faces of quarter 0 at k 0: the oculus beam's faces, the seam beam end, inner rib 0's faces and the two bodies.
void corner_faces(const Context& context) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Point corner = guide.oculus_corners[0] + Vector(0.0, 0.0, -guide.parameters.static_h() * 0.5);
    const Vector edge = (guide.oculus_corners[3] - guide.oculus_corners[0]).normalized();
    const Vector seam = (guide.geometry[0].polygon[1] - guide.geometry[0].polygon[2]).normalized();
    const Vector rib = (guide.columns[0].head[2] - guide.oculus_corners[0]).normalized();
    const Point beam_body = middle(guide.quarter(0).inner_beams()[1]);
    const Point rib_body = middle(guide.quarter(0).inner_ribs()[0]);
    const Plane tilted = near(cp.inner_beams[1][0], corner + edge * 250.0);
    const Plane back = near(cp.inner_beams[1][1], corner + edge * 450.0);
    const Plane beam_end = near(cp.inner_beams[0][1], corner + seam * 250.0);
    const Plane rib0 = near(cp.inner_ribs[0][0], corner + rib * 300.0);
    const Plane rib1 = near(cp.inner_ribs[0][1], corner + rib * 300.0);
    Frame frame(CHAPTER, 217, "corner_faces", "corner_faces(guide, 0, 0): beam, beam_end, rib, beam_body and rib_body, what a corner screw reads", "iso", {-750.0, -1350.0, H - 260.0, 150.0, -400.0, H + 40.0});
    frame.plane_size = 90.0;
    frame.distance = 0.7;

    for (const Plane& plane : {tilted, back, beam_end})
        frame.plane(up(plane), BUILT);

    for (const Plane& plane : {rib0, rib1})
        frame.plane(up(plane), RESULT);

    frame.point(up(beam_body), VARIABLE, 16.0);
    frame.line(up(Line::from_points(rib0.origin(), rib0.origin() + (rib_body - rib0.origin()).normalized() * 300.0)), VARIABLE, 2.0, true, true);

    frame.label("beam[0] = inner_beams[1][0]: tilted", up(tilted.origin()));
    frame.label("beam[1] = inner_beams[1][1]: back", up(back.origin()));
    frame.label("beam_end = inner_beams[0][1]", up(beam_end.origin()));
    frame.label("rib = inner_ribs[0]", up(rib1.origin()));
    frame.label("beam_body: body(inner_beams()[1])", up(beam_body));
    frame.label(fmt::format("to rib_body ({:.0f}, {:.0f}, {:.0f})", rib_body[0], rib_body[1], rib_body[2]), up(rib0.origin() + (rib_body - rib0.origin()).normalized() * 300.0));
    frame.write(context.dir);
}

/// rib_corner: inner rib 0's axis run out to the tilted face, the head there and the two screws into the rib.
void rib_corner(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const double static_h = guide.parameters.static_h();
    const Relationship& row = rows[row_index(rows, Relation::screw_rib_corner, 0, 0)];
    const Line& screw = row.screws.front();
    const double z = datum_z(screw);
    const Line axis = member_axis(cp.inner_ribs[0], z);
    const Point head = line_plane(axis, cp.inner_beams[1][0]).value();
    const Vector d = screw.to_direction();
    const Line tilted = trace_between(cp.inner_beams[1][0], z, guide.oculus_corners[0] + Vector(60.0, -60.0, 0.0), guide.oculus_corners[0] + Vector(-400.0, 400.0, 0.0));
    Frame frame(CHAPTER, 218, "rib_corner", fmt::format("rib_corner: along_axis of inner rib 0 from the tilted face, head at x = {:.1f}, at RIB_CORNER_LEVELS", head[0]), "top", {-340.0, -1170.0, H - 10.0, 80.0, -860.0, H + 10.0});

    for (const Polyline& quad : {guide.geometry[0].quads.inner_beams[0], guide.geometry[0].quads.inner_beams[1], guide.geometry[0].quads.inner_ribs[0]})
        frame.polyline(up(quad.closed()), GREY, 2.0);

    frame.line(up(tilted), INPUT, 3.0);
    frame.line(up(Line::from_points(head, head + d * 360.0)), INPUT, 1.5, true);

    for (const Line& line : row.screws)
        frame.line(line, BUILT, 4.0, false, true);

    frame.point(screw.start(), BUILT);
    frame.label("axis(inner_ribs[0], z)", up(head + d * 340.0));
    frame.label("inner_beams[1][0] at z", up(tilted.point_at(0.0)));
    frame.label(fmt::format("head = ({:.1f}, {:.1f})", screw.start()[0], screw.start()[1]), screw.start());
    frame.label(fmt::format("screws[0] at {}/7", seventh(datum_z(row.screws[0]), static_h)), row.screws[0].end());
    frame.label(fmt::format("screws[1] at {}/7", seventh(datum_z(row.screws[1]), static_h)), row.screws[1].start() + (row.screws[1].end() - row.screws[1].start()) * 0.5);
    frame.write(context.dir);
}

/// rib_corner's through test: the head lies inside the seam beam's end, on the - side of beam_end, so the seam beam is a target too.
void rib_corner_through(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const size_t index = row_index(rows, Relation::screw_rib_corner, 0, 0);
    const Relationship& row = rows[index];
    const Plane& beam_end = cp.inner_beams[0][1];
    const Point beam_body = middle(guide.quarter(0).inner_beams()[1]);
    const Point head = row.screws[0].start().transformed(Xform::translation(0.0, 0.0, -H));
    const double depth = depth_of(head, beam_end, beam_body);
    const Line trace = trace_between(beam_end, datum_z(row.screws[0]), guide.oculus_corners[0] + Vector(0.0, -200.0, 0.0), guide.oculus_corners[0] + Vector(0.0, 100.0, 0.0));
    const Vector inward = beam_end.z_axis() * (depth_of(beam_end.origin() + beam_end.z_axis(), beam_end, beam_body) > 0.0 ? 1.0 : -1.0);
    const std::shared_ptr<wood_session::JointBeam>& connector = context.connected.screws[index];
    Frame frame(CHAPTER, 219, "rib_corner_through", fmt::format("rib_corner: depth(head, beam_end, beam_body) = {:.1f} < 0, so the seam beam joins row.through", depth), "top", {-200.0, -1160.0, H - 100.0, 80.0, -900.0, H + 10.0});

    frame.polyline(up(guide.geometry[0].quads.inner_beams[1].closed()), GREY, 2.0);
    frame.polyline(up(guide.geometry[0].quads.inner_beams[0].closed()), BUILT, 3.0);
    frame.line(up(trace), INPUT, 3.0);

    for (const Line& screw : row.screws) {
        frame.line(screw, GREY, 3.0, false, true);
        frame.point(screw.start(), VARIABLE);
    }

    frame.label("faces.beam_end = inner_beams[0][1]", up(trace.point_at(0.15)));
    frame.label("+: beam_body's side", up(trace.point_at(0.3) + inward * 45.0));
    frame.label(fmt::format("head: depth = {:.1f}", depth), row.screws[0].start());
    frame.label(fmt::format("through: {}", member_name(Family::inner_beams, 0, 0)), up(trace.point_at(0.2) - inward * 30.0));
    frame.label(fmt::format("{}: {} targets", connector->name, connector->targets.size()), row.screws[0].end());
    frame.write(context.dir);
}

/// The rib_corner contact: inner rib 0's end on the back face, kept above the soffit by above().
void rib_corner_contact(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Relationship& row = rows[row_index(rows, Relation::screw_rib_corner, 0, 0)];
    const Vector edge = (guide.oculus_corners[3] - guide.oculus_corners[0]).normalized();
    const Line soffit = trace_between(cp.inner_beams[1][1], guide.soffit, guide.oculus_corners[0] - edge * 150.0, guide.oculus_corners[0] + edge * 450.0);
    const Outline rib = guide.quarter(0).inner_ribs()[0];
    Frame frame(CHAPTER, 220, "rib_corner_contact", fmt::format("rib_corner contact: inner rib 0's end on the back face, above(..., guide.soffit = {:.1f})", guide.soffit), "iso", {-700.0, -1300.0, H - 260.0, 100.0, -500.0, H + 40.0});
    frame.distance = 0.7;

    frame.element(context.members.members.quarters[0].inner_beams[1].element, GREY);
    loops(frame, rib, GREY);
    frame.polyline(row.contact, BUILT, 5.0);
    frame.line(up(soffit), VARIABLE, 2.0, true);

    for (const Line& screw : row.screws)
        frame.line(screw, INPUT, 3.0, false, true);

    frame.label("contact = above(end, guide.soffit)", row.contact.get_point(3));
    frame.label(fmt::format("guide.soffit = {:.1f}", guide.soffit), up(soffit.point_at(0.9)));
    frame.label("plane = inner_beams[1][1]: back", up(trace_between(cp.inner_beams[1][1], -60.0, guide.oculus_corners[0], guide.oculus_corners[3]).point_at(0.35)));
    frame.label(fmt::format("b = {}", member_name(Family::inner_ribs, 0, 0)), up(rib.top.get_point(rib.top.point_count() - 3)));
    frame.write(context.dir);
}

/// The ring corner at oculus corner 0: ring beam 1's axis run back to ring beam 0's tilted face, the screws through ring 0 into ring 1.
void ring_screws(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const double static_h = guide.parameters.static_h();
    const Relationship& row = rows[row_index(rows, Relation::screw_ring, 0, 0)];
    const Line& screw = row.screws.front();
    const double z = datum_z(screw);
    const Line axis = member_axis({guide.oculus_edges[1].tilted, guide.oculus_edges[1].ring_inner}, z);
    const Point head = line_plane(axis, guide.oculus_edges[0].tilted).value();
    const Vector d = screw.to_direction();
    const std::vector<Outline> oculus = guide.oculus();
    Frame frame(CHAPTER, 221, "ring", "ring: ring beam 1 starts on ring beam 0's inner face; along_axis of ring 1 from ring 0's tilted face", "top", {-650.0, -1250.0, H - 10.0, 650.0, -380.0, H + 10.0});

    for (size_t i = 0; i < 4; i++)
        loops(frame, oculus[i], GREY);

    frame.line(up(trace_between(guide.oculus_edges[0].tilted, z, guide.oculus_corners[0], guide.oculus_corners[3])), INPUT, 2.0);
    frame.line(up(Line::from_points(head - d * 120.0, head + d * 520.0)), INPUT, 1.5, true);
    frame.polyline(row.contact, RESULT, 5.0);

    for (const Line& line : row.screws)
        frame.line(line, BUILT, 4.0, false, true);

    frame.label("oculus_0: ring q = 0", up(middle(oculus[0])));
    frame.label("oculus_1: ring next = 1", up(middle(oculus[1])));
    frame.label("axis({tilted, ring_inner} of edge 1, z)", up(head + d * 480.0));
    frame.label("head on oculus_edges[0].tilted", screw.start());
    frame.label("contact on oculus_edges[0].ring_inner", row.contact.get_point(0));
    frame.label(fmt::format("RING_LEVELS {}/7, {}/7", seventh(z, static_h), seventh(datum_z(row.screws[1]), static_h)), screw.end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// Oculus screws
// ═══════════════════════════════════════════════════════════════════════════

/// The ring faces of an oculus screw at quarter 0, k 0, at the datum: end, along, the wedge's free stretch, the band, ring.inner and ring.end.
void ring_faces(const Context& context) {

    const FloorGuide& guide = context.guide;
    const RingAim aim = ring_aim(guide, 0, 0, 0.0);
    const Outline beam = guide.quarter(0).inner_beams()[1];
    const Outline ring = guide.oculus()[0];
    const Plane& contact = guide.geometry[0].planes.inner_beams[1][0];
    const Plane band = contact.translate_by_normal(aim.band * (ring_side(contact, aim.ring_body).dot(contact.z_axis()) > 0.0 ? 1.0 : -1.0));
    const Point corner = guide.oculus_corners[0];
    const Point other = guide.oculus_corners[3];
    Frame frame(CHAPTER, 222, "ring_faces", fmt::format("oculus, k 0: wedge_start = end + along * WEDGE_MARGIN * thickness, {:.1f} along; band = {:.0f}", WEDGE_MARGIN * aim.thickness, aim.band), "top", {-440.0, -1100.0, H - 10.0, 120.0, -600.0, H + 10.0});

    loops(frame, ring, GREY);
    loops(frame, beam, GREY);
    const Line inner = trace_between(aim.inner, 0.0, corner, other);
    const Line end = trace_between(aim.end, 0.0, corner, guide.oculus_corners[1]);
    const Line band_line = trace_between(band, 0.0, corner, other);
    frame.line(up(inner), BUILT, 3.5);
    frame.line(up(end), BUILT, 2.0);
    frame.line(up(band_line), VARIABLE, 2.0, true);
    frame.line(up(Line::from_points(aim.end_point, beam.bottom.get_point(1))), INPUT, 4.0);
    frame.line(up(Line::from_points(aim.end_point, aim.wedge_start)), VARIABLE, 7.0);
    frame.line(up(Line::from_points(aim.end_point, aim.end_point + aim.along * 250.0)), INPUT, 2.0, false, true);
    frame.point(up(aim.end_point), INPUT);
    frame.point(up(aim.wedge_start), RESULT);

    frame.label("end = loop[0]", up(aim.end_point));
    frame.label("ring.along", up(aim.end_point + aim.along * 250.0));
    frame.label("ring.wedge_start", up(aim.wedge_start));
    frame.label(fmt::format("thickness = max({:.1f}, {:.1f})", outline_thickness(beam), outline_thickness(ring)), up(aim.end_point + aim.along * 50.0));
    frame.label(fmt::format("ring.band = {:.0f}", aim.band), up(band_line.point_at(0.3)));
    frame.label("ring.inner = oculus_edges[0].ring_inner", up(inner.point_at(0.2)));
    frame.label("ring.end = oculus_edges[1].tilted", up(end.point_at(0.08)));
    frame.write(context.dir);
}

/// The start of an oculus screw at its level: ring.inner's trace meets beam_end; across and along from there.
void oculus_start(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const Line& screw = rows[row_index(rows, Relation::screw_oculus, 0, 0)].screws.front();
    const double z = datum_z(screw);
    const RingAim aim = ring_aim(guide, 0, 0, z);
    const Line inner = trace_between(aim.inner, z, guide.oculus_corners[0], guide.oculus_corners[3]);
    const Line beam_end = trace_between(aim.beam_end, z, guide.oculus_corners[0] + Vector(0.0, -250.0, 0.0), guide.oculus_corners[0] + Vector(0.0, 300.0, 0.0));
    Frame frame(CHAPTER, 223, "oculus_start", fmt::format("oculus_screw at z = {:.1f}: start where trace(ring.inner, z) meets faces.beam_end; across and ring.along", z), "top", {-320.0, -1080.0, H - 100.0, 80.0, -680.0, H + 10.0});

    loops(frame, guide.oculus()[0], GREY);
    loops(frame, guide.quarter(0).inner_beams()[1], GREY);
    frame.line(up(inner), INPUT, 4.5);
    frame.line(up(beam_end), INPUT, 3.0);
    frame.line(up(Line::from_points(aim.start, aim.start + aim.across * 120.0)), RESULT, 4.0, false, true);
    frame.line(up(Line::from_points(aim.start, aim.start + aim.along * 120.0)), INPUT, 3.0, false, true);
    frame.point(up(aim.start), BUILT);

    frame.label("trace(ring.inner, z)", up(inner.point_at(0.08)));
    frame.label("faces.beam_end = inner_beams[0][1]", up(beam_end.point_at(0.5)));
    frame.label(fmt::format("start = ({:.1f}, {:.1f})", aim.start[0], aim.start[1]), up(aim.start));
    frame.label(fmt::format("across = ({:.3f}, {:.3f}, 0)", aim.across[0], aim.across[1]), up(aim.start + aim.across * 120.0));
    frame.label("ring.along", up(aim.start + aim.along * 120.0));
    frame.write(context.dir);
}

/// An aim: the head slid by offset along ring.along, rays every 10 degrees from across back towards the corner, the chosen one built.
void aim_fan(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const Line& screw = rows[row_index(rows, Relation::screw_oculus, 0, 0)].screws.front();
    const RingAim ring = ring_aim(guide, 0, 0, datum_z(screw));
    const std::array<double, 2> chosen = aim_of(ring, screw);
    const Point head = screw.start();
    Frame frame(CHAPTER, 224, "aim", "Aim (offset, angle): head = start + along * offset, u = across cos(angle) - along sin(angle), 0..80 deg", "top", {-360.0, -1060.0, H - 100.0, 140.0, -700.0, H + 10.0});

    loops(frame, guide.oculus()[0], GREY);
    loops(frame, guide.quarter(0).inner_beams()[1], GREY);
    frame.line(Line::from_points(up(ring.start), head), VARIABLE, 3.0);

    for (double angle = 0.0; angle <= 80.0; angle += 10.0) {
        const Vector u = ring.across * std::cos(angle * M_PI / 180.0) - ring.along * std::sin(angle * M_PI / 180.0);
        frame.line(Line::from_points(head, head + u * screw.length()), INPUT, 1.0, true);
    }

    frame.line(screw, BUILT, 4.0, false, true);
    frame.point(up(ring.start), INPUT);
    frame.point(head, BUILT);

    const Vector across = ring.across * screw.length();
    const Vector back = (ring.across * std::cos(80.0 * M_PI / 180.0) - ring.along * std::sin(80.0 * M_PI / 180.0)) * screw.length();
    const Vector forty = (ring.across * std::cos(40.0 * M_PI / 180.0) - ring.along * std::sin(40.0 * M_PI / 180.0)) * screw.length();
    frame.label("start", up(ring.start));
    frame.label(fmt::format("head = start + ring.along * {:.2f}", chosen[0]), head);
    frame.label("angle 0: across", head + across);
    frame.label("angle 80", head + back);
    frame.label("angle 40", head + forty);
    frame.label(fmt::format("chosen: angle {:.1f}", chosen[1]), screw.end());
    frame.write(context.dir);
}

/// oculus_clearance in a section along the oculus edge: the tilted contact, n towards the ring, the head's height s_head and the crossing.
void clearance_side(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Line& screw = rows[row_index(rows, Relation::screw_oculus, 0, 0)].screws.front();
    const RingAim ring = ring_aim(guide, 0, 0, datum_z(screw));
    const Plane contact = lifted(cp.inner_beams[1][0], H);
    const Vector n = ring_side(contact, up(ring.ring_body));
    const Vector u = screw.to_direction();
    const double s_head = (screw.start() - contact.origin()).dot(n);
    const double s_rate = u.dot(n);
    const Point crossing = screw.start() + u * (s_head / -s_rate);
    const Point foot = screw.start() - n * s_head;
    const Plane section = Plane::from_point_normal(crossing, ring.along);
    Frame frame(CHAPTER, 225, "clearance_side", fmt::format("oculus_clearance: n to the ring side, s_head = {:.1f}, s_rate = u . n = {:.3f} < 0, so the aim is kept", s_head, s_rate), "front", {-160.0, -1060.0, H - 230.0, -40.0, -780.0, H + 30.0});
    frame.orbit = "-157,0";

    std::array<Point, 3> tops;
    const std::array<Plane, 3> planes = {contact, lifted(cp.inner_beams[1][1], H), lifted(guide.oculus_edges[0].ring_inner, H)};

    for (size_t i = 0; i < planes.size(); i++) {
        const Line cut = plane_plane(planes[i], section).value();
        tops[i] = line_plane(cut, level(H)).value();
        frame.line(Line::from_points(tops[i], line_plane(cut, level(H + guide.soffit)).value()), i == 0 ? INPUT : GREY, i == 0 ? 4.0 : 2.0);
    }

    frame.line(Line::from_points(crossing, crossing + n * 80.0), RESULT, 3.0, false, true);
    frame.line(Line::from_points(screw.start(), foot), VARIABLE, 1.5, true);
    frame.line(screw, INPUT, 3.0, false, true);
    frame.point(screw.start(), INPUT);
    frame.point(crossing, BUILT);

    frame.label("contact = faces.beam[0]: tilted", tops[0]);
    frame.label("back", tops[1]);
    frame.label("ring.inner", tops[2]);
    frame.label("n: towards ring.body", crossing + n * 80.0);
    frame.label("head", screw.start());
    frame.label(fmt::format("s_head = {:.1f}", s_head), screw.start() + (foot - screw.start()) * 0.5);
    frame.label("crossing = head + u * s_head / -s_rate", crossing);
    frame.label("u", screw.end());
    frame.write(context.dir);
}

/// oculus_clearance's three test points on the chosen screw and its three margins, the clearance their smallest.
void clearance_points(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const ConstructionPlanes& cp = guide.geometry[0].planes;
    const Line& screw = rows[row_index(rows, Relation::screw_oculus, 0, 0)].screws.front();
    const double z = datum_z(screw);
    const RingAim ring = ring_aim(guide, 0, 0, z);
    const Plane& contact = cp.inner_beams[1][0];
    const Vector n = ring_side(contact, ring.ring_body);
    const Vector u = screw.to_direction();
    const Point head = screw.start().transformed(Xform::translation(0.0, 0.0, -H));
    const double s_head = (head - contact.origin()).dot(n);
    const double s_rate = u.dot(n);
    const Point band_point = head + u * std::max((s_head - ring.band) / -s_rate, 0.0);
    const Point crossing = head + u * (s_head / -s_rate);
    const Point tip = head + u * screw.length();
    const double wedge = (ring.wedge_start - band_point).dot(ring.along);
    const double ring_part = std::min(depth_of(head, ring.end, ring.ring_body), depth_of(crossing, ring.end, ring.ring_body));
    const double beam = std::min({depth_of(tip, cp.inner_beams[1][1], ring.beam_body), depth_of(tip, contact, ring.beam_body), depth_of(tip, ring.beam_end, ring.beam_body)});
    const Point corner = guide.oculus_corners[0];
    const Point other = guide.oculus_corners[3];
    Frame frame(CHAPTER, 226, "clearance_points", fmt::format("oculus_clearance: band_point, crossing, tip; clearance = min(wedge, ring_part, beam) = {:.1f}", std::min({wedge, ring_part, beam})), "top", {-320.0, -1080.0, H - 100.0, 120.0, -700.0, H + 10.0});

    const Line band = trace_between(contact.translate_by_normal(ring.band * n.dot(contact.z_axis())), z, corner, other);
    const Line end = trace_between(ring.end, z, corner + Vector(-100.0, -100.0, 0.0), guide.oculus_corners[1]);
    frame.line(up(trace_between(ring.inner, z, corner, other)), GREY, 2.0);
    frame.line(up(trace_between(contact, z, corner, other)), INPUT, 3.0);
    frame.line(up(trace_between(cp.inner_beams[1][1], z, corner, other)), INPUT, 2.0);
    frame.line(up(trace_between(ring.beam_end, z, corner + Vector(0.0, -200.0, 0.0), corner + Vector(0.0, 100.0, 0.0))), INPUT, 2.0);
    frame.line(up(band), INPUT, 2.0, true);
    frame.line(up(end), INPUT, 2.0, true);
    const Point wedge_start(ring.wedge_start[0], ring.wedge_start[1], z);
    frame.line(up(Line::from_points(band.closest_point(wedge_start, false).second, trace_between(contact, z, corner, other).closest_point(wedge_start, false).second)), INPUT, 3.0);
    frame.line(screw, INPUT, 3.0, false, true);

    for (const Point& point : {band_point, crossing, tip})
        frame.point(up(point), BUILT);

    frame.label("head", screw.start());
    frame.label(fmt::format("band_point: wedge = {:.1f}", wedge), up(band_point));
    frame.label(fmt::format("crossing: ring_part = {:.1f}", ring_part), up(crossing));
    frame.label(fmt::format("tip: beam = {:.1f}", beam), up(tip));
    frame.label("ring.wedge_start", up(wedge_start));
    frame.label("ring.end", up(end.point_at(0.1)));
    frame.label(fmt::format("band: {:.0f} from the contact", ring.band), up(band.point_at(0.2)));
    frame.write(context.dir);
}

/// The coarse heads every COARSE_STEP along the inner face, and the screws the coarse and fine search chose at both ends of quarter 0.
void search(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const double static_h = guide.parameters.static_h();
    Frame frame(CHAPTER, 227, "search", fmt::format("best_aim: offsets 0 to {:.0f} every {:.0f}, angles every 2, then +-5 every 0.25 and +-2 every 0.1", 2.0 * COARSE_SPAN, COARSE_STEP), "top", OCULUS_EDGE);
    frame.key = true;

    loops(frame, guide.oculus()[0], GREY);
    loops(frame, guide.quarter(0).inner_beams()[1], GREY);

    for (size_t k = 0; k < 2; k++) {
        const Relationship& row = rows[row_index(rows, Relation::screw_oculus, 0, k)];
        const RingAim ring = ring_aim(guide, 0, k, datum_z(row.screws[0]));

        for (double offset = 0.0; offset <= 2.0 * COARSE_SPAN; offset += COARSE_STEP)
            frame.point(up(ring.start + ring.along * offset), VARIABLE, 5.0);

        for (size_t i = 0; i < row.screws.size(); i++) {
            const Line& screw = row.screws[i];
            const std::array<double, 2> chosen = aim_of(ring_aim(guide, 0, k, datum_z(screw)), screw);
            frame.line(screw, BUILT, 4.0, false, true);
            frame.label(fmt::format("k {}, {}/7: offset {:.2f}, angle {:.1f}", k, seventh(datum_z(screw), static_h), chosen[0], chosen[1]), i == 0 ? screw.end() : screw.start());
        }

        if (k == 0)
            frame.label(fmt::format("coarse heads, offset 0 to {:.0f}", 2.0 * COARSE_SPAN), up(ring.start + ring.along * (2.0 * COARSE_SPAN)));
    }

    frame.write(context.dir);
}

/// The oculus rows' contact: the oculus beam's tilted-face loop against ring beam 0, two toe screws at each end.
void oculus_contact(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const Relationship& row0 = rows[row_index(rows, Relation::screw_oculus, 0, 0)];
    const Relationship& row1 = rows[row_index(rows, Relation::screw_oculus, 0, 1)];
    const Outline beam = guide.quarter(0).inner_beams()[1];
    Frame frame(CHAPTER, 228, "oculus_contact", "oculus rows: a = oculus_0, b = inner_beams_1_0, the contact the tilted-face loop, OCULUS_LEVELS per end", "iso", OCULUS_EDGE);
    frame.distance = 0.7;

    frame.element(context.members.members.ring[0].element, GREY);
    loops(frame, beam, GREY);
    frame.polyline(row0.contact, BUILT, 5.0);

    for (const Relationship* row : {&row0, &row1})
        for (const Line& screw : row->screws)
            frame.line(screw, INPUT, 4.0, false, true);

    frame.label("contact: the tilted-face loop", area_centroid(row0.contact));
    frame.label("plane = oculus_edges[0].tilted", row0.contact.get_point(1));
    frame.label("a = oculus_0", up(guide.oculus()[0].bottom.get_point(2)));
    frame.label(fmt::format("b = {}", member_name(Family::inner_beams, 1, 0)), up(beam.top.get_point(1)));
    frame.label("k 0: OCULUS_LEVELS[0]", row0.screws[0].end());
    frame.label("k 1: OCULUS_LEVELS[1]", row1.screws[0].end());
    frame.write(context.dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// In the session
// ═══════════════════════════════════════════════════════════════════════════

/// The mitre joint in the session: its two members with their drill features, and every pre-drill line the oculus beam reads.
void pre_drill(const Context& context, const std::vector<Relationship>& rows) {

    const FloorGuide& guide = context.guide;
    const size_t index = row_index(rows, Relation::screw_beam_mitre, 0, 0);
    const std::shared_ptr<wood_session::JointBeam>& connector = context.connected.screws[index];
    const QuarterMembers& quarter = context.connected.members.quarters[0];
    const std::vector<Line> lines = context.connected.pre_drill_lines(quarter.inner_beams[1].element->guid());
    const Line& screw = connector->drill_lines.front();
    const Point entry = line_plane(screw, lifted(guide.geometry[0].planes.inner_beams[0][1], H)).value();
    const Outline seam = guide.quarter(0).inner_beams()[0];
    Frame frame(CHAPTER, 229, "pre_drill", fmt::format("add_pre_drill_joint: drill features where a screw enters and leaves each target; pre_drill_lines: {}", lines.size()), "iso", CORNER_3D);
    frame.features = true;

    frame.element(quarter.inner_beams[0].element, GREY);
    frame.element(quarter.inner_beams[1].element, GREY);

    for (const Line& line : lines)
        frame.line(line, BUILT, 3.0, false, true);

    frame.label(fmt::format("{}: {} pre_drill_lines", member_name(Family::inner_beams, 1, 0), lines.size()), up(middle(guide.quarter(0).inner_beams()[1])));
    frame.label(member_name(Family::inner_beams, 0, 0), up(seam.top.get_point(0) + (seam.top.get_point(1) - seam.top.get_point(0)) * 0.85));
    frame.label(fmt::format("{}: {} targets", connector->name, connector->targets.size()), screw.start());
    frame.label(fmt::format("drill '{} d{:g}'", connector->name, 2.0 * connector->line_radius), entry);
    frame.label(fmt::format("{}_screw_0: a Dowel", connector->name), screw.end());
    frame.write(context.dir);
}

/// A pin on a solid near a point: the corner of its first loop nearest the point, moved a third of the way to the loop's centroid, so a long solid that runs out of the picture is named where it is seen.
Point pin_near(const std::array<Polyline, 2>& solid, const Point& to) {

    Point nearest = solid[0].get_point(0);

    for (const Point& point : solid[0].get_points())
        if ((point - to).magnitude() < (nearest - to).magnitude())
            nearest = point;

    return nearest + (area_centroid(solid[0]) - nearest) * (1.0 / 3.0);
}

/// Every keep-out of the other connectors near oculus corner 0 as a wireframe in solid_color, the bores run on by their overshoot in bore_color.
size_t keep_outs(Frame& frame, const Context& context, const Box& box, std::vector<std::pair<std::string, Point>>& named, const Color& solid_color, const Color& bore_color) {

    const Point corner = up(context.guide.oculus_corners[0]);

    size_t count = 0;

    for (const std::shared_ptr<wood_session::JointBeam>& connector : context.connected.connectors) {
        if (connector->pre_drill)
            continue;

        for (const std::array<Polyline, 2>& part : connector->parts)
            if (in(box, part[0].get_point(0)) || in(box, part[1].get_point(0))) {
                wire(frame, part, solid_color, 2.0);
                named.push_back({fmt::format("{} part", connector->name), pin_near(part, corner)});
                count++;
            }

        for (size_t side = 0; side < connector->cutters.size() && side < connector->targets.size(); side++)
            for (const std::array<Polyline, 2>& cutter : connector->cutters[side])
                if (in(box, cutter[0].get_point(0)) || in(box, cutter[1].get_point(0))) {
                    wire(frame, cutter, solid_color, 1.0);
                    named.push_back({fmt::format("{} cutter, side {}", connector->name, side), area_centroid(cutter[1])});
                    count++;
                }

        for (const Line& dowel : connector->drill_lines)
            if (in(box, dowel.center())) {
                const Vector d = dowel.to_direction() * connector->drill_overshoot;
                frame.line(Line::from_points(dowel.start() - d, dowel.end() + d), bore_color, 3.0);
                named.push_back({fmt::format("{} bore + drill_overshoot", connector->name), dowel.end() + d});
                count++;
            }
    }

    return count;
}

/// check_screws' keep-outs at oculus corner 0: the parts and the cutters of the wedges there, and the screws they are checked against.
void keep_out(const Context& context, const std::vector<Relationship>& rows) {

    Frame frame(CHAPTER, 230, "keep_outs", "check_screws: every other connector's parts and cutters are keep-out solids, its drill lines bores", "iso", CORNER_3D);
    std::vector<std::pair<std::string, Point>> named;
    const size_t count = keep_outs(frame, context, {-750.0, -1350.0, H - 400.0, 250.0, -450.0, H + 60.0}, named, BUILT, RESULT);
    corner_screws(frame, rows, context.guide.oculus_corners[0], 3.0, INPUT);

    std::set<std::string> kinds;
    std::vector<Point> pinned;

    for (const std::pair<std::string, Point>& name : named) {
        const bool apart = std::none_of(pinned.begin(), pinned.end(), [&name](const Point& point) { return (point - name.second).magnitude() < 80.0; });

        if (apart && kinds.size() < 4 && kinds.insert(name.first.substr(name.first.find(' '))).second) {
            frame.label(name.first, name.second);
            pinned.push_back(name.second);
        }
    }

    frame.label(fmt::format("{} keep-outs near oculus corner 0", count), up(context.guide.oculus_corners[0] + Vector(-500.0, 400.0, 0.0)));
    frame.write(context.dir);
}

/// The screws at oculus corner 0 against each other and the keep-outs there: the closest two axes found with the kernel's segment parameters, and what check_screws reports.
void spacing(const Context& context, const std::vector<Relationship>& rows) {

    const ScrewCheck check = check_screws(context.connected, context.guide, context.connected.screws);
    std::vector<std::pair<std::string, Line>> screws;

    for (size_t r = 0; r < rows.size(); r++)
        for (size_t i = 0; i < rows[r].screws.size(); i++)
            if (at(rows[r].screws[i], context.guide.oculus_corners[0]))
                screws.push_back({fmt::format("{} screw {}", context.connected.screws[r]->name, i), rows[r].screws[i]});

    std::array<size_t, 2> pair = {0, 1};
    std::array<Point, 2> closest = {screws[0].second.start(), screws[1].second.start()};

    for (size_t i = 0; i < screws.size(); i++)
        for (size_t j = i + 1; j < screws.size(); j++) {
            double t0 = 0.0;
            double t1 = 0.0;
            Intersection::line_line_parameters(screws[i].second, screws[j].second, t0, t1, 0.0, true, true);
            const Point a = screws[i].second.point_at(t0);
            const Point b = screws[j].second.point_at(t1);

            if ((b - a).magnitude() < (closest[1] - closest[0]).magnitude()) {
                pair = {i, j};
                closest = {a, b};
            }
        }

    Frame frame(CHAPTER, 233, "spacing", fmt::format("check_screws: screw to screw {:.3f}, to bore {:.3f}, to pocket {:.3f} mm; {} misfits", check.screw_screw_mm, check.screw_bore_mm, check.screw_pocket_mm, check.misfits.size()), "top", {-420.0, -1250.0, H - 10.0, 420.0, -750.0, H + 10.0});
    std::vector<std::pair<std::string, Point>> named;
    keep_outs(frame, context, {-750.0, -1350.0, H - 400.0, 250.0, -450.0, H + 60.0}, named, GREY, GREY);
    corner_screws(frame, rows, context.guide.oculus_corners[0], 3.0, INPUT);
    frame.line(Line::from_points(closest[0], closest[1]), VARIABLE, 4.0);
    frame.point(closest[0], VARIABLE);

    frame.label(fmt::format("closest axes: {:.3f}", (closest[1] - closest[0]).magnitude()), closest[0] + (closest[1] - closest[0]) * 0.5);
    frame.label(screws[pair[0]].first, screws[pair[0]].second.end());
    const Line& second = screws[pair[1]].second;
    frame.label(screws[pair[1]].first, (second.end() - screws[pair[0]].second.end()).magnitude() < 40.0 ? second.start() : second.end());
    frame.write(context.dir);
}

}

/// Chapter 10: add_screws, the five screw kinds of screw_relationships, their levels, aim and contacts, the pre-drill joint and check_screws.
void chapter_10_screws(const Context& context) {

    const std::vector<Relationship> rows = screw_relationships(context.guide);
    row_order(context, rows);
    levels(context, rows);
    faces(context);
    trace_axis(context);
    body_depth(context);
    along_axis(context, rows);
    from_seam_face(context, rows);
    rib_beam_beam(context);
    rib_beam_levels(context, rows);
    rib_beam_seam_point(context, rows);
    rib_beam_heads(context, rows);
    rib_beam_contact(context, rows);
    rib_beam_tied(context);
    lift(context, rows);
    mitre_contact(context, rows);
    mitre_screws(context, rows);
    corner_faces(context);
    rib_corner(context, rows);
    rib_corner_through(context, rows);
    rib_corner_contact(context, rows);
    ring_screws(context, rows);
    ring_faces(context);
    oculus_start(context, rows);
    aim_fan(context, rows);
    clearance_side(context, rows);
    clearance_points(context, rows);
    search(context, rows);
    oculus_contact(context, rows);
    pre_drill(context, rows);
    keep_out(context, rows);
    spacing(context, rows);
}

}
