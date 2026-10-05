#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

const double SCREW_LENGTH = 200.0; // mm, every assembly screw
const std::array<double, 2> RIB_BEAM_LEVELS = {0.25, 0.5}; // fractions of the seam depth: the outer rib's lower part at its seam end holds the tie key and its pocket, 138.5 down
const double CORNER_LEVELS = 7.0; // an oculus corner's depth in sevenths: six levels, one per screw on each side of the corner
const std::array<std::array<double, 2>, 2> MITRE_LEVELS = {{{2.0, 5.0}, {3.0, 6.0}}}; // per mitre k, the levels of its two screws; the two quarters' mitres at a seam put their heads on the seam plane at one point, so they differ
const std::array<double, 2> RIB_CORNER_LEVELS = {1.0, 4.0}; // the inner rib end screws at both corners, apart from that corner's mitre and oculus screws they cross
const std::array<std::array<double, 2>, 2> OCULUS_LEVELS = {{{3.0, 6.0}, {2.0, 5.0}}}; // per end k, the ring into the quarter's oculus beam, apart from that side's mitre and rib end screws
const std::array<double, 2> RING_LEVELS = {3.0, 6.0}; // the ring corner screws, apart from the oculus screws of the next quarter they cross
const double WEDGE_MARGIN = 1.5; // the wedge leaves this many beam thicknesses free at both ends of its contact, add_connectors' margin
const double COARSE_STEP = 5.0; // mm, the head positions an oculus screw first tries along the ring's inner face
const double COARSE_ANGLE = 2.0; // degrees, the directions it first tries
const double SEARCH_STEP = 0.25; // mm, the head positions it then tries around the best
const double ANGLE_STEP = 0.1; // degrees, the directions it then tries

/// The quarter's members a corner screw reads: the faces of the oculus beam, the seam beam end plane at that corner, the inner rib and their middles.
struct CornerFaces {
    std::array<Plane, 2> beam; // The oculus beam: the tilted face it shares with the ring, its back face.
    Plane beam_end; // The seam beam's inner face the oculus beam ends on at this corner.
    std::array<Plane, 2> rib; // The inner rib ending on the back face here.
    Point beam_body; // A point inside the oculus beam.
    Point rib_body; // A point inside the inner rib.
};

// ═══════════════════════════════════════════════════════════════════════════
// Lines in a level
// ═══════════════════════════════════════════════════════════════════════════

/// The horizontal line a plane cuts at level z.
static Line trace(const Plane& plane, double z) {
    return plane_plane(plane, level(z)).value();
}

/// The axis of a member between two faces at level z: the line midway between their traces.
static Line axis(const std::array<Plane, 2>& faces, double z) {

    const Line line0 = trace(faces[0], z);
    const Line line1 = trace(faces[1], z);
    const Vector d = line1.to_direction();
    const Point p0 = line0.start();
    const Point p1 = line1.start() + d * (p0 - line1.start()).dot(d);
    const Point middle = p0 + (p1 - p0) * 0.5;

    return Line::from_points(middle, middle + line0.to_direction());
}

/// The middle of a member outline: the mean of its two loops' area centroids.
static Point body(const Outline& outline) {
    return area_centroid(outline.top) + (area_centroid(outline.bottom) - area_centroid(outline.top)) * 0.5;
}

/// The distance of a point from a plane, positive on the side of inside.
static double depth(const Point& point, const Plane& plane, const Point& inside) {
    return signed_distance(inside, plane) < 0.0 ? -signed_distance(point, plane) : signed_distance(point, plane);
}

/// The level of screw i of a corner's level set: down from the datum in sevenths of the depth.
static double corner_level(double levels, double depth) {
    return -depth * levels / CORNER_LEVELS;
}

// ═══════════════════════════════════════════════════════════════════════════
// Screw rules
// ═══════════════════════════════════════════════════════════════════════════

/// A screw at level z through a side member into the member butting on it, along the butting member's axis: the head where that axis leaves the side member's far face, the tip on towards the butting member's body.
static Line along_axis(const std::array<Plane, 2>& butting, const Plane& far_face, const Point& butting_body, double z) {

    const Line line = axis(butting, z);
    const Point head = line_plane(line, far_face).value();
    Vector d = line.to_direction();

    if (d.dot(butting_body - head) < 0.0)
        d = -d;

    return Line::from_points(head, head + d * SCREW_LENGTH);
}

/// The ring's members an oculus screw reads beside the corner's: the ring beam's inner face and body, its end plane at this corner, and where the wedge starts along the contact.
struct RingFaces {
    Plane inner; // The ring beam's inner face, where the heads sit.
    Plane end; // The ring beam's end plane at this corner.
    Point body; // A point inside the ring beam.
    Point wedge_start; // The contact's top edge end at this corner moved along the edge by the wedge's margin.
    Vector along; // Along the contact's top edge, away from this corner.
    double band = 0.0; // Half the beam thickness: within it of the contact plane the wedge's pocket lies.
};

/// How far an oculus screw keeps inside: the head and the contact crossing inside the ring's end, the part within the pocket band short of the wedge, the tip inside the oculus beam's back face and the seam beam end.
static double oculus_clearance(const Point& head, const Vector& u, const CornerFaces& faces, const RingFaces& ring) {

    const Plane& contact = faces.beam[0];
    const Vector n = (ring.body - contact.origin()).dot(contact.z_axis()) < 0.0 ? -contact.z_axis() : contact.z_axis();
    const double s_head = (head - contact.origin()).dot(n);
    const double s_rate = u.dot(n);

    if (s_rate >= 0.0)
        return -1e300;

    const Point band_point = head + u * std::max((s_head - ring.band) / -s_rate, 0.0);
    const Point crossing = head + u * (s_head / -s_rate);
    const Point tip = head + u * SCREW_LENGTH;
    const double wedge = (ring.wedge_start - band_point).dot(ring.along);
    const double ring_part = std::min(depth(head, ring.end, ring.body), depth(crossing, ring.end, ring.body));
    const double beam = std::min({depth(tip, faces.beam[1], faces.beam_body), depth(tip, faces.beam[0], faces.beam_body), depth(tip, faces.beam_end, faces.beam_body)});

    return std::min({wedge, ring_part, beam});
}

/// The aim of an oculus screw: its head's offset along the ring's inner face from the corner and its angle off square to the contact, in degrees.
struct Aim {
    double offset = 0.0; // mm along the inner face from where the seam beam end plane meets it.
    double angle = 0.0; // Degrees from square to the contact towards the corner.
    double clearance = -1e300; // The aim's clearance.
};

/// The best aim on a grid of offsets and angles around a centre, each within its range.
static Aim best_aim(const Point& start, const Vector& across, const CornerFaces& faces, const RingFaces& ring, const Aim& centre, double offset_span, double angle_span, double offset_step, double angle_step) {

    Aim best = centre;

    for (double offset = std::max(centre.offset - offset_span, 0.0); offset <= centre.offset + offset_span; offset += offset_step)
        for (double angle = std::max(centre.angle - angle_span, 0.0); angle <= std::min(centre.angle + angle_span, 80.0); angle += angle_step) {
            const Point head = start + ring.along * offset;
            const Vector u = across * std::cos(angle * M_PI / 180.0) - ring.along * std::sin(angle * M_PI / 180.0);
            const double clearance = oculus_clearance(head, u, faces, ring);

            if (clearance > best.clearance + 1e-9)
                best = {offset, angle, clearance};
        }

    return best;
}

/// The oculus screw at level z: from the ring beam's inner face through the ring and the contact into the quarter's oculus beam towards the corner, crossing the wedge's band before the wedge starts, the 200 line with the largest clearance, found on a coarse grid and refined around its best.
static Line oculus_screw(const CornerFaces& faces, const RingFaces& ring, double z) {

    const Line inner = trace(ring.inner, z);
    const Point start = line_plane(inner, faces.beam_end).value();
    Vector across = (faces.beam_body - ring.body);
    across = Vector(across[0], across[1], 0.0);
    across = (across - ring.along * across.dot(ring.along)).normalized();

    const Aim coarse = best_aim(start, across, faces, ring, Aim{150.0, 40.0, -1e300}, 150.0, 40.0, COARSE_STEP, COARSE_ANGLE);
    const Aim fine = best_aim(start, across, faces, ring, Aim{coarse.offset, coarse.angle, -1e300}, COARSE_STEP, COARSE_ANGLE, SEARCH_STEP, ANGLE_STEP);
    const Point head = start + ring.along * fine.offset;
    const Vector u = across * std::cos(fine.angle * M_PI / 180.0) - ring.along * std::sin(fine.angle * M_PI / 180.0);

    return Line::from_points(head, head + u * SCREW_LENGTH);
}

// ═══════════════════════════════════════════════════════════════════════════
// Relationships
// ═══════════════════════════════════════════════════════════════════════════

/// A screw relationship: the two members, the face the second ends on and its end face there, the screws lifted to the floor.
static Relationship screw_row(const Floor& floor, Relation kind, const MemberRef& a, const MemberRef& b, const Plane& plane, const std::vector<Point>& contact, const std::vector<Line>& screws, size_t corner) {

    const double lift = floor.sizes.bay_height;
    Relationship row;
    row.kind = kind;
    row.a = a;
    row.b = b;
    row.plane = lifted(plane, lift);
    row.contact = lifted(contact, lift);
    row.seam_or_corner = corner;

    for (const Line& screw : screws)
        row.screws.push_back(lifted(screw, lift));

    return row;
}

/// The corner faces of quarter q at end k: k 0 where seam beam 0 meets the oculus beam, k 1 where seam beam 2 does.
static CornerFaces corner_faces(const Floor& floor, size_t q, size_t k) {

    const ConstructionPlanes& cp = floor.geometry[q].planes;
    CornerFaces faces;
    faces.beam = cp.inner_beams[1];
    faces.beam_end = cp.inner_beams[k == 0 ? 0 : 2][1];
    faces.rib = cp.inner_ribs[k];
    faces.beam_body = body(floor.quarter(q).inner_beams()[1]);
    faces.rib_body = body(floor.quarter(q).inner_ribs()[k]);

    return faces;
}

/// Outer rib k of quarter q into the seam beam it meets: two screws along the seam beam from the rib's outer face, the contact the beam's end on the rib's inner face.
static Relationship rib_beam(const Floor& floor, size_t q, size_t k) {

    const ConstructionPlanes& cp = floor.geometry[q].planes;
    const size_t beam = k == 0 ? 0 : 2;
    const Outline outline = floor.quarter(q).inner_beams()[beam];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    std::vector<Line> screws;

    for (double fraction : RIB_BEAM_LEVELS)
        screws.push_back(along_axis(cp.inner_beams[beam], cp.outer_ribs[k][0], body(outline), -floor.sizes.static_h() * fraction));

    return screw_row(floor, Relation::screw_rib_beam, quarter_member(q, Family::outer_ribs, k), quarter_member(q, Family::inner_beams, beam), cp.outer_ribs[k][1], {top[3], top[0], bottom[0], bottom[3]}, screws, q);
}

/// Seam beam 0 (k 0) or 2 (k 1) of quarter q into the oculus beam ending on it: two screws along the oculus beam from the seam plane, the contact the oculus beam's end.
static Relationship beam_mitre(const Floor& floor, size_t q, size_t k) {

    const ConstructionPlanes& cp = floor.geometry[q].planes;
    const size_t seam = k == 0 ? 0 : 2;
    const Outline outline = floor.quarter(q).inner_beams()[1];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const std::vector<Point> contact = k == 0 ? std::vector<Point>{top[3], top[0], bottom[0], bottom[3]} : std::vector<Point>{top[1], top[2], bottom[2], bottom[1]};
    std::vector<Line> screws;

    for (double levels : MITRE_LEVELS[k])
        screws.push_back(along_axis(cp.inner_beams[1], cp.inner_beams[seam][0], body(outline), corner_level(levels, floor.sizes.static_h())));

    return screw_row(floor, Relation::screw_beam_mitre, quarter_member(q, Family::inner_beams, seam), quarter_member(q, Family::inner_beams, 1), cp.inner_beams[seam][1], contact, screws, q);
}

/// The oculus beam of quarter q into inner rib k ending on its back face: two screws along the rib from where its axis leaves the tilted face, through the beam corner, so they also pass the seam beam's end where the corner needs it; the contact the rib's end face down to the beam's soffit.
static Relationship rib_corner(const Floor& floor, size_t q, size_t k) {

    const ConstructionPlanes& cp = floor.geometry[q].planes;
    const Outline outline = floor.quarter(q).inner_ribs()[k];
    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    const CornerFaces faces = corner_faces(floor, q, k);
    const size_t seam = k == 0 ? 0 : 2;
    std::vector<Line> screws;
    bool through_seam = false;

    for (double levels : RIB_CORNER_LEVELS) {
        screws.push_back(along_axis(cp.inner_ribs[k], cp.inner_beams[1][0], faces.rib_body, corner_level(levels, floor.sizes.static_h())));
        through_seam = through_seam || depth(screws.back().start(), faces.beam_end, faces.beam_body) < 0.0;
    }

    const std::vector<Point> end = above({top[0], top[top.size() - 2], bottom[bottom.size() - 2], bottom[0]}, -floor.sizes.static_h());
    Relationship row = screw_row(floor, Relation::screw_rib_corner, quarter_member(q, Family::inner_beams, 1), quarter_member(q, Family::inner_ribs, k), cp.inner_beams[1][1], end, screws, q);

    if (through_seam)
        row.through.push_back(quarter_member(q, Family::inner_beams, seam));

    return row;
}

/// Ring beam q into ring beam q + 1 starting on its inner face at oculus corner q: two screws along ring beam q + 1 from ring beam q's tilted face, the contact ring beam q + 1's start face.
static Relationship ring(const Floor& floor, size_t q, const std::vector<Outline>& oculus) {

    const size_t next = (q + 1) % 4;
    const std::vector<Point> top = oculus[next].top.get_points();
    const std::vector<Point> bottom = oculus[next].bottom.get_points();
    const std::array<Plane, 2> faces = {floor.oculus_edges[next].tilted, floor.oculus_edges[next].ring_inner};
    std::vector<Line> screws;

    for (double levels : RING_LEVELS)
        screws.push_back(along_axis(faces, floor.oculus_edges[q].tilted, body(oculus[next]), corner_level(levels, floor.sizes.static_h())));

    return screw_row(floor, Relation::screw_ring, MemberRef{-1, Family::ring, q, -1}, MemberRef{-1, Family::ring, next, -1}, floor.oculus_edges[q].ring_inner, {top[2], top[3], bottom[3], bottom[2]}, screws, q);
}

/// Ring beam q into the oculus beam of quarter q at its end k: two aimed screws from the ring's inner face beyond the wedge, the contact the oculus wedge's.
static Relationship oculus(const Floor& floor, size_t q, size_t k, const std::vector<Outline>& oculus) {

    const Outline outline = floor.quarter(q).inner_beams()[1];
    const std::vector<Point> loop = outline.bottom.get_points();
    const CornerFaces faces = corner_faces(floor, q, k);
    const double thickness = std::max(outline_thickness(outline), outline_thickness(oculus[q]));
    RingFaces ring;
    ring.inner = floor.oculus_edges[q].ring_inner;
    ring.end = k == 0 ? floor.oculus_edges[(q + 1) % 4].tilted : floor.oculus_edges[(q + 3) % 4].ring_inner;
    ring.body = body(oculus[q]);
    const Point end = k == 0 ? loop[0] : loop[1];
    ring.along = ((k == 0 ? loop[1] : loop[0]) - end).normalized();
    ring.wedge_start = end + ring.along * (WEDGE_MARGIN * thickness);
    ring.band = 0.5 * floor.sizes.inner_beams;
    std::vector<Line> screws;

    for (double levels : OCULUS_LEVELS[k])
        screws.push_back(oculus_screw(faces, ring, corner_level(levels, floor.sizes.static_h())));

    return screw_row(floor, Relation::screw_oculus, MemberRef{-1, Family::ring, q, -1}, quarter_member(q, Family::inner_beams, 1), floor.oculus_edges[q].tilted, {loop.begin(), loop.end() - 1}, screws, q);
}

std::vector<Relationship> geometry::screw_relationships(const Floor& floor) {

    const std::vector<Outline> rings = floor.oculus();
    std::vector<Relationship> rows;

    for (size_t q = 0; q < 4; q++) {
        for (size_t k = 0; k < 2; k++)
            rows.push_back(rib_beam(floor, q, k));

        for (size_t k = 0; k < 2; k++)
            rows.push_back(beam_mitre(floor, q, k));

        for (size_t k = 0; k < 2; k++)
            rows.push_back(rib_corner(floor, q, k));
    }

    for (size_t q = 0; q < 4; q++)
        rows.push_back(ring(floor, q, rings));

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            rows.push_back(oculus(floor, q, k, rings));

    return rows;
}

}
