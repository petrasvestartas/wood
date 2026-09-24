#include "pch.h"
#include "wood_feature_detection_beam.h"
using namespace session_cpp;

namespace wood_session {

namespace {

bool has_valid_frame(const Vector& direction, const Vector& normal) {
    const double area = direction.cross(normal).magnitude_squared();
    return area > 0.0 && std::isfinite(area);
}

/// Whether a pair's end-type sum (0 cross, 1 side-to-end, 2 end-to-end) passes the dataset's allowed type: 0, 1 or -1 for any.
bool type_allowed(const int sum, const int allowed) {
    switch (allowed) {
        case 0: return sum == 0;
        case 1: return sum == 1 || sum == 2;
        case -1: return true;
        default: return false;
    }
}

/// Two crossing beams: both volume pairs trimmed by the bisector plane at the midpoint, each keeping its own side; false when a trim fails.
bool trim_crossing(std::array<Polyline, 4>& beam_vol, const Point& p0, const Point& p1, const Vector& v0, const Vector& v1, bool is_parallel) {

    const Point pm = Point::mid_point(p0, p1);
    const Vector bisector = is_parallel ? v0 : v0 - v1;
    const Plane cp = Plane::from_point_normal(pm, bisector);
    const bool toward_v0 = !cp.has_on_negative_side(pm + v0);
    const Vector npos = cp.z_axis();
    const Vector nneg = -npos;
    const Plane cut_plane0 = Plane::from_point_normal(pm, toward_v0 ? npos : nneg);
    const Plane cut_plane1 = Plane::from_point_normal(pm, toward_v0 ? nneg : npos);

    for (int lid = 0; lid < 2; lid++) {
        const int shift = lid == 0 ? 0 : 2;
        const Plane& cutpl = lid == 0 ? cut_plane0 : cut_plane1;
        if (!Polyline::trim_rectangles_by_plane(beam_vol[shift], beam_vol[shift + 1], cutpl))
            return false;
    }

    return true;
}

/// A beam ending on the side of another: the side beam's volume pair trimmed by the end beam's nearer rectangle, facing the farther one; false when the trim fails.
bool trim_side_to_end(std::array<Polyline, 4>& beam_vol, bool type0, const Point& p0, const Point& p1, const Vector& v0, const Vector& v1) {

    int closer_rect;
    int farrer_rect;
    if (!type0) {
        const Point pp = p0 + v0;
        const bool closer = Point::distance(pp, beam_vol[2].get_point(0)) < Point::distance(pp, beam_vol[3].get_point(0));
        closer_rect = closer ? 2 : 3;
        farrer_rect = closer ? 3 : 2;
    } else {
        const Point pp = p1 + v1;
        const bool closer = Point::distance(pp, beam_vol[0].get_point(0)) < Point::distance(pp, beam_vol[1].get_point(0));
        closer_rect = closer ? 0 : 1;
        farrer_rect = closer ? 1 : 0;
    }

    const Polyline& qc = beam_vol[closer_rect];
    const Vector rv0 = qc[1] - qc[0];
    const Vector rv1 = qc[2] - qc[0];
    const Vector rnrm = rv0.cross(rv1);
    Plane cutpl = Plane::from_point_normal(qc[0], rnrm);
    if (!cutpl.has_on_negative_side(beam_vol[farrer_rect][0]))
        cutpl = Plane::from_point_normal(qc[0], -rnrm);

    const int shift = type0 ? 2 : 0;

    return Polyline::trim_rectangles_by_plane(beam_vol[shift], beam_vol[shift + 1], cutpl);
}

}  // namespace

bool beam_to_beam(const Beam& beam0, const Beam& beam1, const ContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male, FeatureBeam& out) {

    const Polyline& pa_pts = beam0.axis;
    const Polyline& pb_pts = beam1.axis;
    const std::vector<Line> lines0 = pa_pts.get_lines();
    const std::vector<Line> lines1 = pb_pts.get_lines();
    if (contact.segment_a < 0 || contact.segment_a >= (int)lines0.size() || contact.segment_b < 0 || contact.segment_b >= (int)lines1.size())
        return false;

    const Line s0 = lines0[contact.segment_a];
    const Line s1 = lines1[contact.segment_b];
    const int sid0 = contact.segment_a;
    const int sid1 = contact.segment_b;

    Point p0;
    Point p1;
    Vector v0;
    Vector v1;
    Vector normal;
    bool type0 = false;
    bool type1 = false;
    bool is_parallel = false;
    const bool ok = Intersection::line_line_classified(
        s0, s1,
        (int)(pa_pts.point_count() - 1), (int)(pb_pts.point_count() - 1),
        sid0, sid1,
        cross_or_side_to_end,
        p0, p1, v0, v1, normal,
        type0, type1, is_parallel
    );
    if (!ok)
        return false;

    const int sum = (int)type0 + (int)type1;
    if (!type_allowed(sum, beam0.allowed_type) || !type_allowed(sum, beam1.allowed_type))
        return false;

    const Vector sn0 = beam0.has_direction(sid0) ? beam0.directions[sid0] : normal;
    const Vector sn1 = beam1.has_direction(sid1) ? beam1.directions[sid1] : normal;
    const double r0 = beam0.radius(sid0);
    const double r1 = beam1.radius(sid1);
    if (!(r0 > 0.0) || !(r1 > 0.0) || !std::isfinite(r0) || !std::isfinite(r1) ||
        !(volume_length > 0.0) || !std::isfinite(volume_length) ||
        !has_valid_frame(v0, sn0) || !has_valid_frame(v1, sn1)) {
        return false;
    }

    std::array<Polyline, 4> beam_vol;
    Polyline::two_rects_from_frame(p0, v0, sn0, type0 == 1, r0, volume_length, flip_male, beam_vol[0], beam_vol[1]);
    Polyline::two_rects_from_frame(p1, v1, sn1, type1 == 1, r1, volume_length, flip_male, beam_vol[2], beam_vol[3]);

    if (sum == 0 && !trim_crossing(beam_vol, p0, p1, v0, v1, is_parallel))
        return false;
    if (sum == 1 && !trim_side_to_end(beam_vol, type0, p0, p1, v0, v1))
        return false;

    out.end_type = sum;
    out.volumes = beam_vol;
    return true;
}

} // namespace wood_session
