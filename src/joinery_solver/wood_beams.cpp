#include "wood_session.h"
#include "wood_element_plate.h"
#include "wood_face_to_face.h"
#include "../src/session.h"
#include "../src/element.h"
#include "../src/intersection.h"
#include "../src/polyline.h"
#include "../src/line.h"
#include "../src/vector.h"
#include "../src/point.h"
#include "../src/plane.h"
#include "../src/color.h"
#include <fmt/core.h>
#include <cmath>
#include <array>
#include <filesystem>
#include <map>
#include <utility>
#include <vector>

using namespace session_cpp;
using wood_session::WoodJoint;
using wood_session::Plate;

constexpr bool TRACE = false;

namespace {

/// Closest points of two axis segments and where they sit on their polylines.
struct Contact {
    double dist_sq;
    int pid0;
    int sid0;
    int pid1;
    int sid1;
};

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

bool has_direction(const std::vector<std::vector<Vector>>& directions, const int pid, const int sid) {
    return !directions.empty() && pid >= 0 && pid < (int)directions.size() && sid >= 0 && sid < (int)directions[pid].size();
}

double radius_of(const std::vector<std::vector<double>>& radii, const int pid, const int sid) {
    if (pid < 0 || pid >= (int)radii.size())
        return 0.0;
    if (sid < 0 || sid >= (int)radii[pid].size())
        return 0.0;
    return radii[pid][sid];
}

/// Cuts both rectangles of one beam volume at the plane; false when a corner misses it.
bool compute_trimmed_rectangles(Polyline& first, Polyline& second, const Plane& plane) {
    if (first.point_count() != 5 || second.point_count() != 5)
        return false;
    std::array<Point, 4> points;
    if (!Intersection::line_plane(Line::from_points(first[0], first[1]), plane, points[0], false) ||
        !Intersection::line_plane(Line::from_points(first[3], first[2]), plane, points[1], false) ||
        !Intersection::line_plane(Line::from_points(second[0], second[1]), plane, points[2], false) ||
        !Intersection::line_plane(Line::from_points(second[3], second[2]), plane, points[3], false))
        return false;
    for (const Point& point : points)
        for (size_t i = 0; i < 3; ++i)
            if (!std::isfinite(point[i]))
                return false;
    if (plane.has_on_negative_side(first[0])) {
        first.set_point(0, points[0]);
        first.set_point(3, points[1]);
        first.set_point(4, points[0]);
        second.set_point(0, points[2]);
        second.set_point(3, points[3]);
        second.set_point(4, points[2]);
    } else {
        first.set_point(1, points[0]);
        first.set_point(2, points[1]);
        second.set_point(1, points[2]);
        second.set_point(2, points[3]);
    }
    return true;
}

/// The closest segment pair of every two axes within min_distance, keyed by axis pair.
std::map<uint64_t, Contact> compute_contacts(const std::vector<std::vector<Line>>& lines, const double min_distance) {
    std::map<uint64_t, Contact> contacts;
    for (size_t a = 0; a < lines.size(); a++) {
        for (size_t sa = 0; sa < lines[a].size(); sa++) {
            const Line& la = lines[a][sa];
            if (!(la.squared_length() > 0.0))
                continue;
            for (size_t b = a + 1; b < lines.size(); b++) {
                for (size_t sb = 0; sb < lines[b].size(); sb++) {
                    const Line& lb = lines[b][sb];
                    if (!(lb.squared_length() > 0.0))
                        continue;
                    double t0;
                    double t1;
                    if (!Intersection::line_line_parameters(la, lb, t0, t1, 0.0, true, true))
                        continue;
                    if (!std::isfinite(t0) || !std::isfinite(t1))
                        continue;
                    const Point q0 = la.point_at(t0);
                    const Point q1 = lb.point_at(t1);
                    const double d2 = (q0 - q1).magnitude_squared();
                    if (!std::isfinite(d2) || d2 > min_distance * min_distance)
                        continue;
                    const uint64_t id = ((uint64_t)b << 32) | (uint64_t)a;
                    const Contact c{d2, (int)a, (int)sa, (int)b, (int)sb};
                    const auto it = contacts.find(id);
                    if (it == contacts.end() || d2 < it->second.dist_sq)
                        contacts[id] = c;
                }
            }
        }
    }
    return contacts;
}

}  // namespace

void beam_volumes_pipeline(
    const std::vector<Polyline>& axes,
    const std::vector<std::vector<double>>& segment_radii,
    const std::vector<std::vector<Vector>>& segment_direction,
    const std::vector<int>& allowed_types_per_polyline,
    double min_distance,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male
) {
    using namespace wood_session::globals;

    const std::string pb_name = DATA_SET_OUTPUT_FILE;
    const std::filesystem::path base = internal::output_dir();

    Session session("WoodF2F");
    const auto g_axes = session.add_group("BeamAxes");
    const auto g_vols = session.add_group("JointVolumes");
    g_axes->color = Color(0.70f, 0.70f, 0.70f, 1.0f, "grey");
    g_vols->color = Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta");

    std::vector<std::vector<Line>> lines;
    lines.reserve(axes.size());
    for (size_t i = 0; i < axes.size(); i++) {
        auto pl = std::make_shared<Polyline>(axes[i]);
        pl->name = fmt::format("axis_{}", i);
        session.add_polyline(pl, g_axes);
        lines.push_back(axes[i].get_lines());
    }

    const std::map<uint64_t, Contact> contacts = compute_contacts(lines, min_distance);

    int n_pairs = 0;
    int n_success = 0;
    int n_failed = 0;
    int counts[6] = {0, 0, 0, 0, 0, 0};

    for (const auto& entry : contacts) {
        const Contact& c = entry.second;
        n_pairs++;
        const Polyline& pa_pts = axes[c.pid0];
        const Polyline& pb_pts = axes[c.pid1];
        const Line s0 = lines[c.pid0][c.sid0];
        const Line s1 = lines[c.pid1][c.sid1];

        Point p0;
        Point p1;
        Vector v0;
        Vector v1;
        Vector normal;
        bool type0 = false;
        bool type1 = false;
        bool is_parallel = false;
        bool ok = Intersection::line_line_classified(
            s0, s1,
            (int)(pa_pts.point_count() - 1), (int)(pb_pts.point_count() - 1),
            c.sid0, c.sid1,
            cross_or_side_to_end,
            p0, p1, v0, v1, normal,
            type0, type1, is_parallel
        );
        if (!ok) {
            n_failed++;
            continue;
        }

        const int sum = (int)type0 + (int)type1;
        if (allowed_types_per_polyline.size() == 1) {
            if (!type_allowed(sum, allowed_types_per_polyline[0]))
                continue;
        } else if (!allowed_types_per_polyline.empty() && allowed_types_per_polyline.size() == axes.size()) {
            if (!type_allowed(sum, allowed_types_per_polyline[c.pid0]) || !type_allowed(sum, allowed_types_per_polyline[c.pid1]))
                continue;
        }

        const Vector sn0 = has_direction(segment_direction, c.pid0, c.sid0) ? segment_direction[c.pid0][c.sid0] : normal;
        const Vector sn1 = has_direction(segment_direction, c.pid1, c.sid1) ? segment_direction[c.pid1][c.sid1] : normal;
        const double r0 = radius_of(segment_radii, c.pid0, c.sid0);
        const double r1 = radius_of(segment_radii, c.pid1, c.sid1);
        if (!(r0 > 0.0) || !(r1 > 0.0) || !std::isfinite(r0) || !std::isfinite(r1) ||
            !(volume_length > 0.0) || !std::isfinite(volume_length) ||
            !has_valid_frame(v0, sn0) || !has_valid_frame(v1, sn1)) {
            n_failed++;
            continue;
        }

        std::array<Polyline, 4> beam_vol;
        Polyline::two_rects_from_frame(p0, v0, sn0, type0 == 1, r0, volume_length, flip_male, beam_vol[0], beam_vol[1]);
        Polyline::two_rects_from_frame(p1, v1, sn1, type1 == 1, r1, volume_length, flip_male, beam_vol[2], beam_vol[3]);

        if (sum == 0) {
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
                if (!compute_trimmed_rectangles(beam_vol[shift], beam_vol[shift + 1], cutpl)) {
                    ok = false;
                    break;
                }
            }
        } else if (sum == 1) {
            int closer_rect;
            int farrer_rect;
            if (type0 == 0) {
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
            const int shift = type0 == 0 ? 0 : 2;
            ok = compute_trimmed_rectangles(beam_vol[shift], beam_vol[shift + 1], cutpl);
        }
        if (!ok) {
            n_failed++;
            continue;
        }

        for (int k = 0; k < 4; k++) {
            auto rect = std::make_shared<Polyline>(beam_vol[k]);
            rect->name = fmt::format("beam_{}_{}_rect{}", c.pid0, c.pid1, k);
            session.add_polyline(rect, g_vols);
        }

        const Plate el0(beam_vol[0], beam_vol[1]);
        const Plate el1(beam_vol[2], beam_vol[3]);

        WoodJoint jt;
        bool swap_planes_1 = false;
        const bool jok = face_to_face_wood(
            (size_t)(n_success + n_failed),
            el0,
            el1,
            {c.pid0, c.pid1},
            JOINT_VOLUME_EXTENSION,
            0.0,
            1e-6,
            DISTANCE_SQUARED,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE,
            (sum == 2 ? 1 : 0),
            jt,
            swap_planes_1
        );
        if (!jok) {
            n_failed++;
            continue;
        }
        n_success++;
        switch (jt.joint_type) {
            case 11: counts[0]++; break;
            case 12: counts[1]++; break;
            case 13: counts[2]++; break;
            case 20: counts[3]++; break;
            case 30: counts[4]++; break;
            case 40: counts[5]++; break;
            default: break;
        }
    }

    session.pb_dump((base / pb_name).string());
    if (TRACE) {
        fmt::print("\n=== beam_volumes_pipeline ===\n");
        fmt::print("{} axes -> {} contacts -> {} volumes ({} failed)\n", axes.size(), n_pairs, n_success, n_failed);
        fmt::print("  by type: 11={} 12={} 13={} 20={} 30={} 40={}\n", counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
    }
}
