#include <cstdlib>
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
#include <fstream>
#include <utility>
#include <vector>
#include <map>
#include <filesystem>

using namespace session_cpp;
using wood_session::WoodJoint;
using wood_session::Plate;

namespace {

bool has_valid_frame(const Vector& direction, const Vector& normal) {
    const double area = direction.cross(normal).magnitude_squared();
    return area > 0.0 && std::isfinite(area);
}

bool compute_trimmed_rectangles(Polyline& first, Polyline& second, const Plane& plane) {
    auto top = first.get_points();
    auto bottom = second.get_points();
    if (top.size() != 5 || bottom.size() != 5)
        return false;
    std::array<Point, 4> points;
    if (!Intersection::line_plane(Line::from_points(top[0], top[1]), plane, points[0], false) ||
        !Intersection::line_plane(Line::from_points(top[3], top[2]), plane, points[1], false) ||
        !Intersection::line_plane(Line::from_points(bottom[0], bottom[1]), plane, points[2], false) ||
        !Intersection::line_plane(Line::from_points(bottom[3], bottom[2]), plane, points[3], false))
        return false;
    for (const Point& point : points)
        for (size_t i = 0; i < 3; ++i)
            if (!std::isfinite(point[i]))
                return false;
    if (plane.has_on_negative_side(top[0])) {
        top[0] = points[0];
        top[3] = points[1];
        top[4] = top[0];
        bottom[0] = points[2];
        bottom[3] = points[3];
        bottom[4] = bottom[0];
    } else {
        top[1] = points[0];
        top[2] = points[1];
        bottom[1] = points[2];
        bottom[2] = points[3];
    }
    first = Polyline(top);
    second = Polyline(bottom);
    return true;
}

}

void beam_volumes_pipeline(
    const std::vector<Polyline>& axes,
    const std::vector<std::vector<double>>& segment_radii,
    const std::vector<std::vector<Vector>>& segment_direction,
    const std::vector<int>& allowed_types_per_polyline,
    double min_distance,
    double volume_length,
    double cross_or_side_to_end,
    int    flip_male)
{
    using namespace wood_session::globals;

    const std::string pb_name = DATA_SET_OUTPUT_FILE;
    auto base = internal::output_dir();

    Session session("WoodF2F");
    auto g_axes = session.add_group("BeamAxes");
    auto g_vols = session.add_group("JointVolumes");
    g_axes->color = Color(0.70f, 0.70f, 0.70f, 1.0f, "grey");
    g_vols->color = Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta");

    for (size_t i = 0; i < axes.size(); i++) {
        auto pl = std::make_shared<Polyline>(axes[i]);
        pl->name = fmt::format("axis_{}", i);
        session.add_polyline(pl, g_axes);
    }

    struct Contact {
        double dist_sq;
        int pid0, sid0, pid1, sid1;
    };
    std::map<uint64_t, Contact> contacts;
    std::vector<std::vector<Point>> all_axis_pts;
    all_axis_pts.reserve(axes.size());
    for (const auto& ax : axes)
        all_axis_pts.push_back(ax.get_points());
    for (size_t a = 0; a < axes.size(); a++) {
        const auto& pa = all_axis_pts[a];
        for (size_t sa = 0; sa + 1 < pa.size(); sa++) {
            const Line la = Line::from_points(pa[sa], pa[sa+1]);
            if (!(la.squared_length() > 0.0))
                continue;
            for (size_t b = a + 1; b < axes.size(); b++) {
                const auto& pb = all_axis_pts[b];
                for (size_t sb = 0; sb + 1 < pb.size(); sb++) {
                    const Line lb = Line::from_points(pb[sb], pb[sb+1]);
                    if (!(lb.squared_length() > 0.0))
                        continue;
                    double t0, t1;
                    if (!Intersection::line_line_parameters(la, lb, t0, t1, 0.0, true, true))
                        continue;
                    if (!std::isfinite(t0) || !std::isfinite(t1))
                        continue;
                    Point q0 = la.point_at(t0);
                    Point q1 = lb.point_at(t1);
                    double dx=q0[0]-q1[0], dy=q0[1]-q1[1], dz=q0[2]-q1[2];
                    double d2 = dx*dx + dy*dy + dz*dz;
                    if (!std::isfinite(d2) || d2 > min_distance*min_distance) {
                        continue;
                    }
                    uint64_t id = ((uint64_t)b << 32) | (uint64_t)a;
                    Contact c{d2, (int)a,(int)sa,(int)b,(int)sb};
                    auto it = contacts.find(id);
                    if (it == contacts.end() || d2 < it->second.dist_sq) {
                        contacts[id] = c;
                    }
                }
            }
        }
    }

    int n_pairs = 0, n_success = 0, n_failed = 0;
    int counts[6] = {0,0,0,0,0,0};

    std::vector<std::array<Polyline, 4>> joint_rects;
    joint_rects.reserve(contacts.size());

    for (const auto& entry : contacts) {
        const auto& c = entry.second;
        n_pairs++;
        const auto& pa_pts = all_axis_pts[c.pid0];
        const auto& pb_pts = all_axis_pts[c.pid1];
        Line s0 = Line::from_points(pa_pts[c.sid0], pa_pts[c.sid0+1]);
        Line s1 = Line::from_points(pb_pts[c.sid1], pb_pts[c.sid1+1]);

        Point p0, p1;
        Vector v0, v1, normal;
        bool type0=false, type1=false, is_parallel=false;
        bool ok = Intersection::line_line_classified(
            s0, s1,
            (int)(pa_pts.size() - 1), (int)(pb_pts.size() - 1),
            c.sid0, c.sid1,
            cross_or_side_to_end,
            p0, p1, v0, v1, normal,
            type0, type1, is_parallel);
        if (!ok) { n_failed++; continue; }

        auto is_valid = [](int sum, int allowed) {
            switch (allowed) {
                case 0:  return sum == 0;
                case 1:  return sum == 1 || sum == 2;
                case -1: return true;
                default: return false;
            }
        };
        int sum = (int)type0 + (int)type1;
        if (!allowed_types_per_polyline.empty()) {
            if (allowed_types_per_polyline.size() == 1) {
                if (!is_valid(sum, allowed_types_per_polyline[0])) {
                    continue;
                }
            } else if (allowed_types_per_polyline.size() == axes.size()) {
                if (!is_valid(sum, allowed_types_per_polyline[c.pid0]) ||
                    !is_valid(sum, allowed_types_per_polyline[c.pid1])) {
                    continue;
                }
            }
        }

        auto seg_dir_ok = [&](int pid, int sid) {
            return !segment_direction.empty() &&
                   pid >= 0 && pid < (int)segment_direction.size() &&
                   sid >= 0 && sid < (int)segment_direction[pid].size();
        };
        auto seg_rad = [&](int pid, int sid) -> double {
            if (pid < 0 || pid >= (int)segment_radii.size()) return 0.0;
            if (sid < 0 || sid >= (int)segment_radii[pid].size()) return 0.0;
            return segment_radii[pid][sid];
        };
        Vector sn0 = seg_dir_ok(c.pid0, c.sid0) ? segment_direction[c.pid0][c.sid0] : normal;
        Vector sn1 = seg_dir_ok(c.pid1, c.sid1) ? segment_direction[c.pid1][c.sid1] : normal;
        double r0 = seg_rad(c.pid0, c.sid0);
        double r1 = seg_rad(c.pid1, c.sid1);
        if (!(r0 > 0.0) || !(r1 > 0.0) || !std::isfinite(r0) || !std::isfinite(r1) ||
            !(volume_length > 0.0) || !std::isfinite(volume_length) ||
            !has_valid_frame(v0, sn0) || !has_valid_frame(v1, sn1)) {
            n_failed++;
            continue;
        }

        std::array<Polyline, 4> beam_vol;
        Polyline::two_rects_from_frame(
            p0, v0, sn0, type0 == 1, r0, volume_length, flip_male,
            beam_vol[0], beam_vol[1]);
        Polyline::two_rects_from_frame(
            p1, v1, sn1, type1 == 1, r1, volume_length, flip_male,
            beam_vol[2], beam_vol[3]);

        if (sum == 0) {
            Point pm((p0[0]+p1[0])*0.5, (p0[1]+p1[1])*0.5, (p0[2]+p1[2])*0.5);
            Point pm_plus_v0(pm[0]+v0[0], pm[1]+v0[1], pm[2]+v0[2]);
            Vector bisector = is_parallel ? v0 : Vector(v0[0]-v1[0], v0[1]-v1[1], v0[2]-v1[2]);
            Point  pm_cp = pm;
            Vector bis   = bisector;
            Plane  cp    = Plane::from_point_normal(pm_cp, bis);
            bool toward_v0 = !cp.has_on_negative_side(pm_plus_v0);
            Vector npos = cp.z_axis();
            Vector nneg(-npos[0], -npos[1], -npos[2]);
            Point pa_cp = pm;  Vector npa = toward_v0 ? npos : nneg;
            Point pb_cp = pm;  Vector npb = toward_v0 ? nneg : npos;
            Plane cut_plane0 = Plane::from_point_normal(pa_cp, npa);
            Plane cut_plane1 = Plane::from_point_normal(pb_cp, npb);
            for (int lid = 0; lid < 2; lid++) {
                int shift = lid == 0 ? 0 : 2;
                const Plane& cutpl = lid == 0 ? cut_plane0 : cut_plane1;
                if (!compute_trimmed_rectangles(beam_vol[shift], beam_vol[shift + 1], cutpl)) {
                    ok = false;
                    break;
                }
            }
        } else if (sum == 1) {
            int closer_rect, farrer_rect;
            if (type0 == 0) {
                auto q20 = beam_vol[2].get_points()[0];
                auto q30 = beam_vol[3].get_points()[0];
                Point pp(p0[0]+v0[0], p0[1]+v0[1], p0[2]+v0[2]);
                bool closer = Point::distance(pp, q20) < Point::distance(pp, q30);
                closer_rect = closer ? 2 : 3;
                farrer_rect = closer ? 3 : 2;
            } else {
                auto q00 = beam_vol[0].get_points()[0];
                auto q10 = beam_vol[1].get_points()[0];
                Point pp(p1[0]+v1[0], p1[1]+v1[1], p1[2]+v1[2]);
                bool closer = Point::distance(pp, q00) < Point::distance(pp, q10);
                closer_rect = closer ? 0 : 1;
                farrer_rect = closer ? 1 : 0;
            }
            auto qc = beam_vol[closer_rect].get_points();
            Vector rv0(qc[1][0]-qc[0][0], qc[1][1]-qc[0][1], qc[1][2]-qc[0][2]);
            Vector rv1(qc[2][0]-qc[0][0], qc[2][1]-qc[0][1], qc[2][2]-qc[0][2]);
            Vector rnrm = rv0.cross(rv1);
            Point  rorig = qc[0];
            Plane  cutpl = Plane::from_point_normal(rorig, rnrm);
            auto qf = beam_vol[farrer_rect].get_points();
            if (!cutpl.has_on_negative_side(qf[0])) {
                Vector nneg(-rnrm[0], -rnrm[1], -rnrm[2]);
                Point  rorig2 = qc[0];
                cutpl = Plane::from_point_normal(rorig2, nneg);
            }
            int shift = type0 == 0 ? 0 : 2;
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

        Plate el0(beam_vol[0], beam_vol[1]);
        Plate el1(beam_vol[2], beam_vol[3]);

        WoodJoint jt;
        bool swap_planes_1 = false;
        bool jok = face_to_face_wood(
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
            swap_planes_1);
        if (!jok) { n_failed++; continue; }
        n_success++;
        int t = jt.joint_type;
        if (t == 11) {
            counts[0]++;
        } else if (t == 12) {
            counts[1]++;
        } else if (t == 13) {
            counts[2]++;
        } else if (t == 20) {
            counts[3]++;
        } else if (t == 30) {
            counts[4]++;
        } else if (t == 40) {
            counts[5]++;
        }

        joint_rects.push_back(beam_vol);
    }

    if (std::getenv("WOOD_F2F_DUMP") != nullptr) {
        std::ofstream meta_out((base / (pb_name + "_meta.txt")).string());
        std::ofstream coord_out((base / (pb_name + "_coords.txt")).string());
        auto emit = [&](int ei, const Polyline& pl) {
            meta_out << 1 << ' ' << pl.point_count() << '\n';
            coord_out << "element " << ei << "\n";
            coord_out << "  poly 0:";
            for (size_t pi = 0; pi < pl.point_count(); pi++) {
                Point p = pl.get_point(pi);
                coord_out << " " << p[0] << " " << p[1] << " " << p[2];
            }
            coord_out << "\n";
        };
        int ei = 0;
        for (const auto& pl : axes) {
            emit(ei++, pl);
        }
        for (const auto& rects : joint_rects) {
            for (int k = 0; k < 4; k++) {
                emit(ei++, rects[k]);
            }
        }
    }

    session.pb_dump((base / pb_name).string());
    if (std::getenv("WOOD_VERBOSE")) {
        fmt::print("\n=== beam_volumes_pipeline ===\n");
        fmt::print("{} axes -> {} contacts -> {} volumes ({} failed)\n",
                   axes.size(), n_pairs, n_success, n_failed);
        fmt::print("  by type: 11={} 12={} 13={} 20={} 30={} 40={}\n",
                   counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
    }
}
