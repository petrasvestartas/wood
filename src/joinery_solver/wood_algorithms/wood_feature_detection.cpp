#include "pch.h"
#include "wood_feature_detection.h"
#include "wood_contact_detection.h"
#include "wood_feature_construction.h"
using namespace session_cpp;
using namespace wood_session;

constexpr bool TRACE = false;

// ═══════════════════════════════════════════════════════════════════════════
// Feature detection
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Per-pair state shared by every classification stage of face_to_face_wood.
struct F2F {
    Plate& el0;
    Plate& el1;
    const std::pair<int, int> el_ids_in;
    std::pair<int, int> el_ids;
    std::pair<std::array<int, 2>, std::array<int, 2>> face_ids;
    const std::vector<double>& extension;
    const double limit_min_joint_length;
    const double zero_length_squared;
    const double coplanar_tolerance;
    const double dihedral_angle_threshold;
    const bool all_treated_as_rotated;
    const bool rotated_joint_as_average;
    double cos_angle;
    Plane avg_plane_0;
    Plane avg_plane_1;
    std::string dbg_fail_reason;
    FeaturePlate& out_joint;
    bool& out_swap_planes_1;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// el_ids is swapped mid-pass to put the male first; el0/el1 never move, so resolve by the original id.
    const std::string& guid_at(int id) const {
        return id == el_ids_in.first ? el0.guid() : el1.guid();
    }

    /// The [width, height, length] extension this joint type reads.
    std::array<double, 3> ext(int joint_type) const { return wood_session::joint_volume_extension(extension, joint_type); }
};

/// What the alignment stage produces for one face contact and the joint branches consume.
struct FaceCandidate {
    size_t i = 0;
    size_t j = 0;
    Polyline joint_area;
    ContactType ctype = ContactType::unknown;
    int joint_type = 0;
    Line joint_line0 = Line::from_points(Point(0, 0, 0), Point(0, 0, 0));
    Line joint_line1 = Line::from_points(Point(0, 0, 0), Point(0, 0, 0));
    Polyline joint_quads0;
    Polyline joint_quads1;
    bool has_quads0 = false;
    bool has_quads1 = false;
    Vector dir = Vector(0, 0, 0);
    bool dir_set = false;
    std::array<Line, 2> joint_lines = {
        Line::from_points(Point(0, 0, 0), Point(0, 0, 0)),
        Line::from_points(Point(0, 0, 0), Point(0, 0, 0)),
    };
    std::array<std::optional<Polyline>, 4> joint_volumes{};
};

/// Mid-thickness average plane of an element; the xy plane when it has no two outlines.
Plane average_plane(const Plate& el) {

    Plane avg_plane = Plane::xy_plane();
    if (el.polylines.size() >= 2 && !el.planes.empty()) {
        const Point origin = Point::mid_point(el.polylines[0].get_point(0), el.polylines[1].get_point(0));
        const Vector normal = el.planes[0].z_axis();
        avg_plane = Plane::from_point_normal(origin, normal);
    }

    return avg_plane;
}

/// Alignment line and side quad of side face `face` of `el` (side 0 = A, 1 = B); false rejects the pair.
bool alignment_line(
    F2F& s,
    const Plate& el,
    const Plane& avg_plane,
    int side,
    size_t face,
    const FaceCandidate& c,
    Line& joint_line,
    Polyline& joint_quads,
    bool& has_quads) {

    const size_t i = c.i;
    const size_t j = c.j;
    const Point a0 = el.polylines[0].get_point(face - 2);
    const Point a1 = el.polylines[1].get_point(face - 2);
    const Point b0 = el.polylines[0].get_point(face - 1);
    const Point b1 = el.polylines[1].get_point(face - 1);
    const Line segment = Line::from_points(Point::mid_point(a0, a1), Point::mid_point(b0, b1));

    if (!Intersection::polyline_plane_to_line(c.joint_area, avg_plane, segment.start(), joint_line)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("ppl{}_fail f({},{})", side, i, j);
        return false;
    }

    if (joint_line.squared_length() <= s.zero_length_squared) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("jl{}_short f({},{})", side, i, j);
        return false;
    }

    if (!Intersection::quad_from_line_top_bottom_planes(el.planes[face], joint_line, el.planes[0], el.planes[1], joint_quads)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("quad{}_fail f({},{})", side, i, j);
        return false;
    }

    has_quads = true;
    return true;
}

/// One face contact into a candidate: face ids, alignment lines, length checks, insertion direction.
bool prepare_candidate(F2F& s, const wood_session::ContactFace& contact, FaceCandidate& c) {

    c.i = static_cast<size_t>(contact.face_a);
    c.j = static_cast<size_t>(contact.face_b);
    c.joint_area = contact.polygon;
    const size_t i = c.i;
    const size_t j = c.j;

    s.face_ids.first[0] = contact.face_a;
    s.face_ids.first[1] = contact.face_a;
    s.face_ids.second[0] = contact.face_b;
    s.face_ids.second[1] = contact.face_b;

    c.ctype = contact.type;
    c.joint_type = static_cast<int>(c.ctype);

    if (i > 1 && !alignment_line(s, s.el0, s.avg_plane_0, 0, i, c, c.joint_line0, c.joint_quads0, c.has_quads0))
        return false;

    if (j > 1 && !alignment_line(s, s.el1, s.avg_plane_1, 1, j, c, c.joint_line1, c.joint_quads1, c.has_quads1))
        return false;

    if (c.joint_type < 2) {
        const double ext_l = s.ext(c.joint_type == 1 ? 20 : 12)[2];
        const double ext_sq = (ext_l * 2.0) * (ext_l * 2.0);
        const double min_sq = s.limit_min_joint_length * s.limit_min_joint_length;

        if (ext_l < 0.0 && i > 1 && ext_sq > c.joint_line0.squared_length() - min_sq) {
            if (TRACE)
                s.dbg_fail_reason = fmt::format("jl0_ext f({},{})", i, j);
            return false;
        }

        if (ext_l < 0.0 && j > 1 && ext_sq > c.joint_line1.squared_length() - min_sq) {
            if (TRACE)
                s.dbg_fail_reason = fmt::format("jl1_ext f({},{})", i, j);
            return false;
        }

        c.joint_line0.extend_equally(ext_l);
        c.joint_line1.extend_equally(ext_l);
        if (ext_l != 0.0 && i > 1)
            c.has_quads0 = Intersection::quad_from_line_top_bottom_planes(s.el0.planes[i], c.joint_line0, s.el0.planes[0], s.el0.planes[1], c.joint_quads0);
        if (ext_l != 0.0 && j > 1)
            c.has_quads1 = Intersection::quad_from_line_top_bottom_planes(s.el1.planes[j], c.joint_line1, s.el1.planes[0], s.el1.planes[1], c.joint_quads1);
    }

    if (i < s.el0.insertion_vectors().size() && j < s.el1.insertion_vectors().size()) {
        c.dir = (i > j) ? s.el0.insertion_vectors()[i] : s.el1.insertion_vectors()[j];
        c.dir_set = (std::abs(c.dir[0]) + std::abs(c.dir[1]) + std::abs(c.dir[2])) > 0.01;
    }

    return true;
}

/// Fills out_joint from the shared state and a finished candidate; always a successful joint.
bool emit_joint(F2F& s, const FaceCandidate& c) {

    s.out_joint.element_a = s.guid_at(s.el_ids.first);
    s.out_joint.element_b = s.guid_at(s.el_ids.second);
    s.out_joint.contact = wood_session::ContactFace(s.face_ids.first[0], s.face_ids.second[0], c.ctype, c.joint_area);
    s.out_joint.cross_faces = { s.face_ids.first[1], s.face_ids.second[1] };
    s.out_joint.joint_type = c.joint_type;
    s.out_joint.joint_lines = c.joint_lines;
    s.out_joint.joint_volumes = c.joint_volumes;

    return true;
}

/// Parallelism of the two alignment lines within cos_angle: 1 parallel, -1 antiparallel, 0 rotated.
int alignment_lines_parallel(const F2F& s, const Line& joint_line0, const Line& joint_line1) {

    const Vector v0 = joint_line0.start() - joint_line0.end();
    const Vector v1 = joint_line1.start() - joint_line1.end();
    const double ll = v0.magnitude() * v1.magnitude();

    if (ll <= 0.0)
        return 0;

    const double ca = v0.dot(v1) / ll;
    if (ca >= s.cos_angle)
        return 1;
    if (ca <= -s.cos_angle)
        return -1;
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Side-side, rotated / perpendicular elements (type 13)
// ═══════════════════════════════════════════════════════════════════════════

/// Local frame of the rotated joint: origin on the averaged segment, x along it, z the face normal.
void rotated_frame(const F2F& s, const FaceCandidate& c, Point& o, Vector& x, Vector& y, Vector& z) {

    const Line& l0 = c.joint_line0;
    const Line& l1 = c.joint_line1;
    const double d_ss = Point::distance(l0.start(), l1.start());
    const double d_se = Point::distance(l0.start(), l1.end());
    const Line axis = d_ss < d_se
        ? Line::from_points(Point::mid_point(l0.start(), l1.start()), Point::mid_point(l0.end(), l1.end()))
        : Line::from_points(Point::mid_point(l0.start(), l1.end()), Point::mid_point(l0.end(), l1.start()));

    o = axis.start();
    x = axis.to_vector();
    z = s.el0.planes[c.i].z_axis();
    y = x.cross(z);
    y.normalize_self();

    if (!s.rotated_joint_as_average) {
        y = s.el0.planes[0].z_axis();
        z = x.cross(y);
    }

    const Point center = s.el0.polylines[c.i].center();
    const double thick = std::max(
        Point::distance(s.el0.planes[0].origin(), s.el0.planes[1].project(s.el0.planes[0].origin())),
        Point::distance(s.el1.planes[0].origin(), s.el1.planes[1].project(s.el1.planes[0].origin()))
    );
    const Vector probe = y * thick * 2.0;
    const Line probe_line = Line::from_points(center + probe, center - probe);
    Line clipped;
    if (Intersection::line_two_planes(probe_line, s.el0.planes[0], s.el1.planes[1], clipped))
        y = clipped.to_vector();
    x = y.cross(z);
}

/// Bounding rectangle of the joint area in the local frame, extruded by half-thickness into two quads.
bool rotated_volumes(
    F2F& s,
    const FaceCandidate& c,
    const Point& o,
    const Vector& x,
    const Vector& y,
    const Vector& z,
    std::vector<Point>& rect,
    Vector& offset,
    Polyline& vol0,
    Polyline& vol1) {

    const std::array<double, 3> ext = s.ext(13);

    const Xform world_to_local = Xform::world_to_frame(o, x, y, z);
    std::vector<Point> proj;
    proj.reserve(c.joint_area.point_count());
    for (size_t k = 0; k < c.joint_area.point_count(); ++k) {
        Point p = c.joint_area.get_point(k);
        p.transform(world_to_local);
        proj.push_back(p);
    }

    if (proj.empty()) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("proj_empty f({},{})", c.i, c.j);
        return false;
    }

    double xmin = proj[0][0];
    double xmax = xmin;
    double ymin = proj[0][1];
    double ymax = ymin;
    for (size_t k = 1; k < proj.size(); ++k) {
        if (proj[k][0] < xmin)
            xmin = proj[k][0];
        else if (proj[k][0] > xmax)
            xmax = proj[k][0];
        if (proj[k][1] < ymin)
            ymin = proj[k][1];
        else if (proj[k][1] > ymax)
            ymax = proj[k][1];
    }

    const double zmin = proj[0][2];
    rect = {
        Point(xmax, ymax, zmin),
        Point(xmin, ymax, zmin),
        Point(xmin, ymin, zmin),
        Point(xmax, ymin, zmin),
    };
    const Xform local_to_world = Xform::frame_to_world(o, x, y, z);
    for (Point& p : rect)
        p.transform(local_to_world);

    offset = c.dir_set ? c.dir : z;
    offset.normalize_self();
    const double d0 = 0.5 * Point::distance(s.el0.planes[0].origin(), s.el0.planes[1].project(s.el0.planes[0].origin()));
    offset = offset * d0;

    vol0 = Polyline({
        rect[3] + offset,
        rect[3] - offset,
        rect[0] - offset,
        rect[0] + offset,
        rect[3] + offset,
    });
    vol1 = Polyline({
        rect[2] + offset,
        rect[2] - offset,
        rect[1] - offset,
        rect[1] + offset,
        rect[2] + offset,
    });

    vol0.extend_edge_equally(0, ext[0]);
    vol0.extend_edge_equally(2, ext[0]);
    vol1.extend_edge_equally(0, ext[0]);
    vol1.extend_edge_equally(2, ext[0]);
    vol0.extend_edge_equally(1, ext[1]);
    vol0.extend_edge_equally(3, ext[1]);
    vol1.extend_edge_equally(1, ext[1]);
    vol1.extend_edge_equally(3, ext[1]);
    return true;
}

/// WOOD_F2F_DUMP trace of one type-13 joint.
void rotated_dump(
    const F2F& s,
    const FaceCandidate& c,
    const Polyline& vol0,
    const Polyline& vol1,
    const std::vector<Point>& rect_local,
    const Vector& offset_vector) {

    const std::pair<int, int>& el_ids = s.el_ids;
    const size_t i = c.i;
    const size_t j = c.j;
    static const std::string fp = [] { const char* env = std::getenv("WOOD_F2F_DUMP"); return env ? std::string(env) : std::string(); }();
    if (!fp.empty()) {
        std::ofstream flog(fp, std::ios::app);
        flog << "F2F type13 el=(" << el_ids.first << "," << el_ids.second << ") i=" << i << " j=" << j << "\n";
        flog << "  vol0: ";
        for (size_t k=0;k<vol0.point_count();k++) { Point p=vol0.get_point(k); flog<<"("<<p[0]<<","<<p[1]<<","<<p[2]<<") "; }
        flog << "\n  vol1: ";
        for (size_t k=0;k<vol1.point_count();k++) { Point p=vol1.get_point(k); flog<<"("<<p[0]<<","<<p[1]<<","<<p[2]<<") "; }
        flog << "\n  rect_local: ";
        for (const Point& p : rect_local) { flog<<"("<<p[0]<<","<<p[1]<<","<<p[2]<<") "; }
        flog << "\n  offset_vec: ("<<offset_vector[0]<<","<<offset_vector[1]<<","<<offset_vector[2]<<")\n";
    }
}

/// Type 13: frame around the averaged alignment segment, projected bounding rectangle, extruded.
bool side_side_rotated(F2F& s, FaceCandidate& c) {

    Point o;
    Vector x;
    Vector y;
    Vector z;
    rotated_frame(s, c, o, x, y, z);

    std::vector<Point> rect;
    Vector offset;
    Polyline vol0;
    Polyline vol1;
    if (!rotated_volumes(s, c, o, x, y, z, rect, offset, vol0, vol1))
        return false;

    rotated_dump(s, c, vol0, vol1, rect, offset);

    c.joint_volumes[0] = vol0;
    c.joint_volumes[1] = vol1;
    c.joint_type = 13;
    return emit_joint(s, c);
}

// ═══════════════════════════════════════════════════════════════════════════
// Side-side, parallel elements: split on dihedral angle (types 11 and 12)
// ═══════════════════════════════════════════════════════════════════════════

/// Overlap-average of the two alignment lines, taken as both joint lines; false rejects the pair.
bool overlap_average(F2F& s, FaceCandidate& c, Line& lj) {

    const bool ok = c.joint_line0.overlap_average(c.joint_line1, lj);
    if (!ok || lj.squared_length() <= s.zero_length_squared) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("lj_overlap f({},{})", c.i, c.j);
        return false;
    }

    c.joint_lines[0] = lj;
    c.joint_lines[1] = lj;
    return true;
}

/// End-cap planes along the joint axis, oriented by the insertion direction when one is set.
void end_planes(const FaceCandidate& c, const Line& lj, Plane& pl_end0, Plane& pl_end1) {

    const Vector lj_v = lj.to_vector();
    const Point lj_s = lj.start();
    const Point lj_e = lj.end();
    pl_end0 = Plane::from_point_normal(lj_s, lj_v);
    if (c.dir_set) {
        Vector dir = c.dir;
        Point start = lj_s;
        pl_end0 = Plane::from_point_normal(start, dir);
    }

    const Vector normal = pl_end0.z_axis();
    pl_end1 = Plane::from_point_normal(lj_e, normal);
}

/// Dihedral angle (degrees) of the joint edge in the tetrahedron (lj.start, lj.end, center0, center1).
bool dihedral_angle(F2F& s, const FaceCandidate& c, const Line& lj, double& dihedral) {

    const Point center0 = s.avg_plane_0.project(s.el0.polylines[0].center());
    const Point center1 = s.avg_plane_1.project(s.el1.polylines[0].center());
    dihedral = Point::dihedral_angle_deg(lj.start(), lj.end(), center0, center1);

    if (dihedral < 20.0) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("dihedral<20 f({},{})", c.i, c.j);
        return false;
    }

    return true;
}

/// Type 11: probe the joint axis 90° in the face plane to pick the nearer planes, then an open 4-plane quad.
bool side_side_out_of_plane(F2F& s, FaceCandidate& c, const Line& lj, const Plane& pl_end0, const Plane& pl_end1) {

    const std::array<double, 3> ext = s.ext(11);
    const size_t i = c.i;
    const size_t j = c.j;
    const Vector normal = s.el0.planes[i].z_axis();
    const Vector probe = lj.to_vector().cross(normal) * 0.5;
    const Line probe_line = Line::from_points(lj.start(), lj.start() + probe);
    Point p00;
    Point p10;
    Point p11;

    if (!Intersection::line_plane(probe_line, s.el0.planes[0], p00, false)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("lp0 f({},{})", i, j);
        return false;
    }

    if (!Intersection::line_plane(probe_line, s.el1.planes[0], p10, false)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("lp1 f({},{})", i, j);
        return false;
    }

    if (!Intersection::line_plane(probe_line, s.el1.planes[1], p11, false)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("lp2 f({},{})", i, j);
        return false;
    }

    const bool farther = Point::distance(p00, p10) > Point::distance(p00, p11);
    std::array<Plane, 4> planes4;
    if (farther)
        planes4 = { s.el1.planes[1], s.el0.planes[0], s.el1.planes[0], s.el0.planes[1] };
    else
        planes4 = { s.el1.planes[0], s.el0.planes[0], s.el1.planes[1], s.el0.planes[1] };

    Polyline vol0;
    Polyline vol1;

    if (!Intersection::plane_4planes_open(pl_end0, planes4, vol0)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("p4p_open0 f({},{})", i, j);
        return false;
    }

    if (!Intersection::plane_4planes_open(pl_end1, planes4, vol1)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("p4p_open1 f({},{})", i, j);
        return false;
    }

    if (!s.el0.planes[i].has_on_negative_side(vol0.get_point(1))) {
        vol0.shift(2);
        vol1.shift(2);
    }

    vol0.reverse();
    vol1.reverse();
    vol0.shift(3);
    vol1.shift(3);
    std::swap(s.el_ids.first, s.el_ids.second);
    std::swap(s.face_ids.first, s.face_ids.second);
    std::swap(c.joint_lines[0], c.joint_lines[1]);

    vol0 = vol0.closed();
    vol1 = vol1.closed();

    vol0.extend_edge_equally(0, ext[0]);
    vol0.extend_edge_equally(2, ext[0]);
    vol1.extend_edge_equally(0, ext[0]);
    vol1.extend_edge_equally(2, ext[0]);
    vol0.extend_edge_equally(1, ext[1]);
    vol0.extend_edge_equally(3, ext[1]);
    vol1.extend_edge_equally(1, ext[1]);
    vol1.extend_edge_equally(3, ext[1]);

    c.joint_volumes[0] = vol0;
    c.joint_volumes[1] = vol1;
    c.joint_type = 11;
    return emit_joint(s, c);
}

/// Type 12: two planes offset ±half-thickness from the matched face, two 4-plane loops, four volumes.
bool side_side_in_plane(F2F& s, FaceCandidate& c, const Plane& pl_end0, const Plane& pl_end1) {

    const std::array<double, 3> ext = s.ext(12);
    const size_t i = c.i;
    const size_t j = c.j;
    const double d0 = 0.5 * Point::distance(s.el0.planes[0].origin(), s.el0.planes[1].project(s.el0.planes[0].origin()));
    const Plane offset_plane_0 = s.el0.planes[i].translate_by_normal(-d0);
    const Plane offset_plane_1 = s.el0.planes[i].translate_by_normal(d0);

    const Point pt00 = s.el0.planes[0].axis_point();
    const Point proj00 = s.el1.planes[0].project(pt00);
    const Point proj01 = s.el1.planes[1].project(pt00);
    const double w0 = (pt00 - proj00).magnitude_squared();
    const double w1 = (pt00 - proj01).magnitude_squared();
    if (w0 > w1)
        s.out_swap_planes_1 = true;
    const Plane p1_0 = (w0 > w1) ? s.el1.planes[1] : s.el1.planes[0];
    const Plane p1_1 = (w0 > w1) ? s.el1.planes[0] : s.el1.planes[1];

    const std::array<Plane, 4> loop_planes_0 = { offset_plane_0, s.el0.planes[0], offset_plane_1, s.el0.planes[1] };
    const std::array<Plane, 4> loop_planes_1 = { offset_plane_0, p1_0, offset_plane_1, p1_1 };

    Polyline vol0;
    Polyline vol1;
    Polyline vol2;
    Polyline vol3;

    if (!Intersection::plane_4planes(pl_end0, loop_planes_0, vol0)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("p4p0 f({},{})", i, j);
        return false;
    }

    if (!Intersection::plane_4planes(pl_end1, loop_planes_0, vol1)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("p4p1 f({},{})", i, j);
        return false;
    }

    if (!Intersection::plane_4planes(pl_end0, loop_planes_1, vol2)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("p4p2 f({},{})", i, j);
        return false;
    }

    if (!Intersection::plane_4planes(pl_end1, loop_planes_1, vol3)) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("p4p3 f({},{})", i, j);
        return false;
    }

    for (Polyline* vp : {&vol0, &vol1, &vol2, &vol3}) {
        vp->extend_edge_equally(0, ext[0]);
        vp->extend_edge_equally(2, ext[0]);
        vp->extend_edge_equally(1, ext[1]);
        vp->extend_edge_equally(3, ext[1]);
    }

    c.joint_volumes[0] = vol0;
    c.joint_volumes[1] = vol1;
    c.joint_volumes[2] = vol2;
    c.joint_volumes[3] = vol3;
    c.joint_type = 12;
    return emit_joint(s, c);
}

/// Side-side dispatcher: rotated (13) when the alignment lines are not parallel, else 11 or 12 by dihedral.
bool side_side(F2F& s, FaceCandidate& c) {

    c.joint_lines[0] = c.joint_line0;
    c.joint_lines[1] = c.joint_line1;

    const int parallel = alignment_lines_parallel(s, c.joint_line0, c.joint_line1);
    if (parallel == 0 || s.all_treated_as_rotated)
        return side_side_rotated(s, c);

    Line lj;
    if (!overlap_average(s, c, lj))
        return false;

    Plane pl_end0;
    Plane pl_end1;
    end_planes(c, lj, pl_end0, pl_end1);

    double dihedral = 0.0;
    if (!dihedral_angle(s, c, lj, dihedral))
        return false;

    if (dihedral <= s.dihedral_angle_threshold)
        return side_side_out_of_plane(s, c, lj, pl_end0, pl_end1);
    return side_side_in_plane(s, c, pl_end0, pl_end1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Top-side (type 20), top-top (type 40), cross fallback (type 30)
// ═══════════════════════════════════════════════════════════════════════════

/// Type 20: the male's side-face quad extruded along an offset vector spanning the female's thickness.
bool top_side(F2F& s, FaceCandidate& c) {

    const std::array<double, 3> ext = s.ext(20);
    const size_t i = c.i;
    const size_t j = c.j;
    const bool male_first = i > j;
    const Line jline = male_first ? c.joint_line0 : c.joint_line1;
    c.joint_lines[0] = jline;
    c.joint_lines[1] = jline;

    const Plane plane0_0 = male_first ? s.el0.planes[0] : s.el1.planes[0];
    const Plane plane1_0 = !male_first ? s.el0.planes[i] : s.el1.planes[j];
    const size_t other = !male_first ? (i == 0 ? 1 : 0) : (j == 0 ? 1 : 0);
    const Plane plane1_1 = !male_first ? s.el0.planes[other] : s.el1.planes[other];

    const bool quad_available = male_first ? c.has_quads0 : c.has_quads1;
    if (!quad_available) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("no_quad f({},{})", i, j);
        return false;
    }

    const Polyline quad = male_first ? c.joint_quads0 : c.joint_quads1;

    Vector offset(0, 0, 0);
    Intersection::orthogonal_vector_between_two_plane_pairs(plane0_0, plane1_0, plane1_1, offset);
    if (c.dir_set) {
        Vector scaled;
        if (Intersection::scale_vector_to_distance_of_2planes(c.dir, plane1_0, plane1_1, scaled))
            offset = scaled;
    }

    if (!male_first) {
        std::swap(s.el_ids.first, s.el_ids.second);
        std::swap(s.face_ids.first, s.face_ids.second);
    }

    const size_t m_id = male_first ? 0 : 1;
    const size_t f_id = male_first ? 1 : 0;
    const Point q0 = quad.get_point(0);
    const Point q1 = quad.get_point(1);
    const Point q2 = quad.get_point(2);
    const Point q3 = quad.get_point(3);

    Polyline male_vol({ q0, q1, q1 + offset, q0 + offset, q0 });
    Polyline female_vol({ q3, q2, q2 + offset, q3 + offset, q3 });

    male_vol.extend_edge_equally(0, ext[0]);
    male_vol.extend_edge_equally(2, ext[0]);
    female_vol.extend_edge_equally(0, ext[0]);
    female_vol.extend_edge_equally(2, ext[0]);
    male_vol.extend_edge_equally(1, ext[1]);
    male_vol.extend_edge_equally(3, ext[1]);
    female_vol.extend_edge_equally(1, ext[1]);
    female_vol.extend_edge_equally(3, ext[1]);

    c.joint_volumes[m_id] = male_vol;
    c.joint_volumes[f_id] = female_vol;
    c.joint_type = 20;
    return emit_joint(s, c);
}

/// Type 40: bounding rectangle of the joint area, translated ±thickness along each element's normal.
bool top_top(F2F& s, FaceCandidate& c) {

    const std::array<double, 3> ext = s.ext(40);
    const size_t i = c.i;
    const size_t j = c.j;

    const std::optional<Polyline> rect = Polyline::bounding_rectangle(c.joint_area);
    if (!rect) {
        if (TRACE)
            s.dbg_fail_reason = fmt::format("no_rect f({},{})", i, j);
        return false;
    }

    Polyline vol_a = *rect;
    Polyline vol_b = *rect;

    Vector dir = c.dir_set
        ? (i < s.el0.insertion_vectors().size() ? s.el0.insertion_vectors()[i] : s.el0.planes[i].z_axis())
        : s.el0.planes[i].z_axis();
    if (!dir.normalize_self()) {
        dir = s.el0.planes[i].z_axis();
        dir.normalize_self();
    }

    const int next_plane_0 = (i == 0) ? 1 : 0;
    const int next_plane_1 = (j == 0) ? 1 : 0;
    const double dist_0 = Point::distance(s.el0.planes[i].origin(), s.el0.planes[next_plane_0].project(s.el0.planes[i].origin()));
    const double dist_1 = Point::distance(s.el1.planes[j].origin(), s.el1.planes[next_plane_1].project(s.el1.planes[j].origin()));
    const Vector move_a = -dir * dist_0;
    const Vector move_b = dir * dist_1;

    for (size_t k = 0; k < vol_a.point_count(); ++k)
        vol_a.set_point(k, vol_a.get_point(k) + move_a);
    for (size_t k = 0; k < vol_b.point_count(); ++k)
        vol_b.set_point(k, vol_b.get_point(k) + move_b);

    const Point a0 = vol_a.get_point(0);
    const Point a1 = vol_a.get_point(1);
    const Point a2 = vol_a.get_point(2);
    const Point a3 = vol_a.get_point(3);
    const Point b0 = vol_b.get_point(0);
    const Point b1 = vol_b.get_point(1);
    const Point b2 = vol_b.get_point(2);
    const Point b3 = vol_b.get_point(3);

    Polyline temp0({a0, a1, b1, b0, a0});
    Polyline temp1({a3, a2, b2, b3, a3});

    temp0.extend_edge_equally(0, ext[0]);
    temp0.extend_edge_equally(2, ext[0]);
    temp1.extend_edge_equally(0, ext[0]);
    temp1.extend_edge_equally(2, ext[0]);
    temp0.extend_edge_equally(1, ext[1]);
    temp0.extend_edge_equally(3, ext[1]);
    temp1.extend_edge_equally(1, ext[1]);
    temp1.extend_edge_equally(3, ext[1]);

    c.joint_volumes[0] = temp0;
    c.joint_volumes[1] = temp1;
    c.joint_type = 40;
    return emit_joint(s, c);
}

/// Type 30: plane_to_face cross joint when no face contact produced a joint.
bool cross_fallback(F2F& s) {

    wood_session::ContactCross cj;
    const std::array<double, 3> cj_ext = s.ext(30);
    constexpr double CROSS_JOINT_PARALLEL_ANGLE_DEG = 30.0;

    const bool found = wood_session::plane_to_face(
        s.el0.polylines[0], s.el0.polylines[1],
        s.el1.polylines[0], s.el1.polylines[1],
        s.el0.planes[0], s.el0.planes[1],
        s.el1.planes[0], s.el1.planes[1],
        cj, CROSS_JOINT_PARALLEL_ANGLE_DEG, cj_ext
    );
    if (!found)
        return false;

    s.out_joint.element_a = s.guid_at(s.el_ids.first);
    s.out_joint.element_b = s.guid_at(s.el_ids.second);
    s.out_joint.contact = wood_session::ContactFace(cj.faces_a[0], cj.faces_b[0], wood_session::ContactType::unknown, cj.polygon);
    s.out_joint.cross_faces = { cj.faces_a[1], cj.faces_b[1] };
    s.out_joint.joint_type = 30;
    s.out_joint.joint_lines = {{
        Line::from_points(cj.lines[0].get_point(0), cj.lines[0].get_point(1)),
        Line::from_points(cj.lines[1].get_point(0), cj.lines[1].get_point(1)),
    }};
    s.out_joint.joint_volumes = { cj.volumes[0], cj.volumes[1], std::nullopt, std::nullopt };
    return true;
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// face_to_face_wood
// ═══════════════════════════════════════════════════════════════════════════

bool face_to_face_wood(
    Plate& el0,
    Plate& el1,
    std::pair<int, int> el_ids_in,
    const std::vector<double>& joint_volume_extension,
    double limit_min_joint_length,
    double zero_length_squared,
    double coplanar_tolerance,
    double dihedral_angle_threshold,
    bool all_treated_as_rotated,
    bool rotated_joint_as_average,
    int  search_type,
    FeaturePlate& out_joint,
    bool& out_swap_planes_1
) {

    out_swap_planes_1 = false;
    F2F s = {
        el0, el1, el_ids_in, el_ids_in, { {{0,0}}, {{0,0}} },
        joint_volume_extension,
        limit_min_joint_length, zero_length_squared, coplanar_tolerance, dihedral_angle_threshold,
        all_treated_as_rotated, rotated_joint_as_average,
        0.0, Plane::xy_plane(), Plane::xy_plane(), std::string(),
        out_joint, out_swap_planes_1,
    };
    if (search_type != 1) {
        s.cos_angle = std::cos(wood_session::config::ANGLE);
        s.avg_plane_0 = average_plane(el0);
        s.avg_plane_1 = average_plane(el1);

        const std::vector<wood_session::ContactFace> pair_contacts = wood_session::face_contacts_for_pair(el0, el1, s.cos_angle, coplanar_tolerance, &out_joint);

        for (const wood_session::ContactFace& contact : pair_contacts) {
            FaceCandidate c;
            if (!prepare_candidate(s, contact, c))
                continue;

            bool found = false;
            if (c.joint_type == 0)
                found = side_side(s, c);
            else if (c.joint_type == 1)
                found = top_side(s, c);
            else
                found = top_top(s, c);

            if (found)
                return true;
        }
    }

    if (search_type != 0 &&
        el0.polylines.size() >= 2 && el1.polylines.size() >= 2 &&
        el0.planes.size() >= 2 && el1.planes.size() >= 2) {
        if (cross_fallback(s))
            return true;
    }

    if (!s.dbg_fail_reason.empty())
        out_joint.dbg_fail_reason = s.dbg_fail_reason;
    return false;
}
