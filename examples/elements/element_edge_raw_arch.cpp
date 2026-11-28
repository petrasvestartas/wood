#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace {

Mesh frame_box(
    const Point& origin,
    const Vector& ex,
    const Vector& ey,
    const Vector& ez,
    double hx,
    double hy,
    double hz
) {
    const Point base = origin - ez * hz;
    const Polyline loop({
        base - ex * hx - ey * hy,
        base + ex * hx - ey * hy,
        base + ex * hx + ey * hy,
        base - ex * hx + ey * hy,
        base - ex * hx - ey * hy,
    });
    return Mesh::loft({loop}, {loop.translated(ez * (2.0 * hz))}, true);
}

std::shared_ptr<JointBeamElement> make_solid_joint(
    const Element& a,
    const Element& b,
    const std::vector<Mesh>& cuts_a,
    const std::vector<Mesh>& cuts_b,
    const std::vector<Vector>& ways_a,
    const std::vector<Vector>& ways_b,
    const std::string& name
) {
    const std::shared_ptr<JointBeamElement> joint = std::make_shared<JointBeamElement>();
    joint->name = name;
    joint->is_visible = false;
    joint->targets = {a.guid(), b.guid()};
    joint->target_insertions = {ways_a, ways_b};

    if (!cuts_a.empty()) {
        JointCutStep sa;
        sa.label = "A2";
        sa.target = 0;
        for (const Mesh& m : cuts_a)
            sa.removal = sa.removal.number_of_faces() == 0
                             ? m
                             : solid_boolean(sa.removal, m, SolidOperation::add);
        joint->steps.push_back(sa);
    }
    if (!cuts_b.empty()) {
        JointCutStep sb;
        sb.label = "B2";
        sb.target = 1;
        for (const Mesh& m : cuts_b)
            sb.removal = sb.removal.number_of_faces() == 0
                             ? m
                             : solid_boolean(sb.removal, m, SolidOperation::add);
        joint->steps.push_back(sb);
    }
    return joint;
}

} // namespace

/// Segmented three-row staggered raw-timber arch (Thesis Fig. 8.7, 8.9, 8.16, 8.17): bark-extrados/intrados logs joined by radial multi-tenon fingers and longitudinal cheek grooves.
int main() {

    WoodSession scene("element_edge_raw_arch");

    const std::array<std::string, 4> stocks = {"raw_log_0", "raw_log_1", "raw_log_2", "raw_log_3"};

    const double R = 1366.67;
    const Point center(0.0, 0.0, 560.0 - R);
    const double half_angle = std::asin(1040.0 / R);
    const double ply_pitch = 84.0;
    const double half_thick = 41.6;

    const auto arch_pt = [&](double u, double y) -> Point {
        const double a = -half_angle + 2.0 * half_angle * u;
        return Point(center[0] + R * std::sin(a), y, center[2] + R * std::cos(a));
    };
    const auto arch_radial = [&](double u) -> Vector {
        const double a = -half_angle + 2.0 * half_angle * u;
        return Vector(std::sin(a), 0.0, std::cos(a));
    };
    const auto arch_tangent = [&](double u) -> Vector {
        const double a = -half_angle + 2.0 * half_angle * u;
        return Vector(std::cos(a), 0.0, -std::sin(a));
    };

    const std::shared_ptr<TreeNode> mid_group = scene.group_named("ply_middle");
    const std::shared_ptr<TreeNode> front_group = scene.group_named("ply_front");
    const std::shared_ptr<TreeNode> back_group = scene.group_named("ply_back");
    const std::shared_ptr<TreeNode> joint_group = scene.group_named("arch_joints");

    auto build_ply = [&](const std::vector<double>& u_breaks,
                         double y_center,
                         const std::string& prefix,
                         const std::shared_ptr<TreeNode>& group,
                         size_t stock_offset) {
        std::vector<std::shared_ptr<EdgeRawElement>> segs;
        const double finger_ext = 15.0;
        for (size_t i = 0; i + 1 < u_breaks.size(); i++) {
            const double u0 = u_breaks[i];
            const double u1 = u_breaks[i + 1];
            const Point p0 = arch_pt(u0, y_center);
            const Point p1 = arch_pt(u1, y_center);
            const Vector dir = (p1 - p0).normalized();
            const Vector t0 = arch_tangent(u0);
            const Vector t1 = arch_tangent(u1);

            const double ext0 = i > 0 ? finger_ext : 0.0;
            const double ext1 = (i + 2 < u_breaks.size()) ? finger_ext : 0.0;

            const std::shared_ptr<EdgeRawElement> seg = EdgeRawElement::from_stock(
                Line::from_points(p0 - dir * (ext0 + 35.0), p1 + dir * (ext1 + 35.0)),
                62.0,
                stocks[(i + stock_offset) % 4],
                40.0 * static_cast<double>(i + stock_offset),
                true,
                fmt::format("{}_{}", prefix, i)
            );
            seg->cuts = {
                Plane::from_point_normal(p0 - t0 * ext0, t0),
                Plane::from_point_normal(p1 + t1 * ext1, t1 * -1.0),
                Plane::from_point_normal({0.0, y_center - half_thick, 320.0}, {0.0, 1.0, 0.0}),
                Plane::from_point_normal({0.0, y_center + half_thick, 320.0}, {0.0, -1.0, 0.0}),
            };
            scene.add(seg, group);
            segs.push_back(seg);
        }

        for (size_t i = 0; i + 1 < segs.size(); i++) {
            const double um = u_breaks[i + 1];
            const Point pm = arch_pt(um, y_center);
            const Vector er = arch_radial(um);
            const Vector et = arch_tangent(um);
            const Vector ey = Vector::y_axis();

            const std::array<std::pair<double, double>, 5> bands = {{
                {-75.0, -27.0},
                {-27.0, -9.0},
                {-9.0, 9.0},
                {9.0, 27.0},
                {27.0, 75.0},
            }};
            std::vector<Mesh> cut_left;
            std::vector<Mesh> cut_right;
            for (size_t b = 0; b < bands.size(); b++) {
                const double r_mid = 0.5 * (bands[b].first + bands[b].second);
                const double r_half = 0.5 * (bands[b].second - bands[b].first) + 0.45;
                if (b % 2 == 1) {
                    cut_left.push_back(
                        frame_box(pm + er * r_mid, et, ey, er, finger_ext + 0.6, 55.0, r_half)
                    );
                } else {
                    cut_right.push_back(
                        frame_box(pm + er * r_mid, et, ey, er, finger_ext + 0.6, 55.0, r_half)
                    );
                }
            }

            const std::shared_ptr<JointBeamElement> finger = make_solid_joint(
                *segs[i],
                *segs[i + 1],
                cut_left,
                cut_right,
                {et, Vector(0.0, y_center < 0.0 ? 1.0 : -1.0, 0.0)},
                {et * -1.0, Vector(0.0, y_center < 0.0 ? 1.0 : -1.0, 0.0)},
                fmt::format("{}_radial_fingers_{}", prefix, i)
            );
            scene.add(finger, joint_group);
            scene.add_interaction(finger, segs[i], finger->interaction(0));
            scene.add_interaction(finger, segs[i + 1], finger->interaction(1));
        }
        return segs;
    };

    const std::vector<double> u_mid = {0.0, 0.25, 0.50, 0.75, 1.0};
    const std::vector<double> u_outer = {0.07, 0.36, 0.64, 0.93};

    const std::vector<std::shared_ptr<EdgeRawElement>> ply_mid =
        build_ply(u_mid, 0.0, "arch_mid", mid_group, 0);
    const std::vector<std::shared_ptr<EdgeRawElement>> ply_front =
        build_ply(u_outer, -ply_pitch, "arch_front", front_group, 1);
    const std::vector<std::shared_ptr<EdgeRawElement>> ply_back =
        build_ply(u_outer, ply_pitch, "arch_back", back_group, 2);

    auto add_cheek_grooves = [&](const std::vector<std::shared_ptr<EdgeRawElement>>& outer_ply,
                                 const std::vector<double>& u_out,
                                 double y_seam,
                                 double y_out_sign,
                                 const std::string& tag) {
        for (size_t o = 0; o < outer_ply.size(); o++) {
            const double ua0 = u_out[o];
            const double ua1 = u_out[o + 1];
            for (size_t m = 0; m < ply_mid.size(); m++) {
                const double ub0 = u_mid[m];
                const double ub1 = u_mid[m + 1];
                const double u_lo = std::max(ua0, ub0) + 0.025;
                const double u_hi = std::min(ua1, ub1) - 0.025;
                if (u_hi - u_lo < 0.06)
                    continue;

                const double u_c = 0.5 * (u_lo + u_hi);
                const Point pc = arch_pt(u_c, y_seam);
                const Vector et = arch_tangent(u_c);
                const Vector er = arch_radial(u_c);
                const Vector ey = Vector::y_axis();
                const double half_chord =
                    0.5 * Point::distance(arch_pt(u_lo, y_seam), arch_pt(u_hi, y_seam));

                std::vector<Mesh> mid_grooves;
                std::vector<Mesh> out_grooves;
                for (double roff : {-22.0, 0.0, 22.0}) {
                    mid_grooves.push_back(frame_box(
                        pc + er * roff - ey * (y_out_sign * 4.0), et, ey, er, half_chord, 4.8, 5.2
                    ));
                    out_grooves.push_back(frame_box(
                        pc + er * roff + ey * (y_out_sign * 4.0), et, ey, er, half_chord, 4.8, 5.2
                    ));
                }

                const std::shared_ptr<JointBeamElement> seam = make_solid_joint(
                    *ply_mid[m],
                    *outer_ply[o],
                    mid_grooves,
                    out_grooves,
                    {{0.0, y_out_sign, 0.0}},
                    {{0.0, -y_out_sign, 0.0}},
                    fmt::format("{}_grooves_{}_{}", tag, o, m)
                );
                scene.add(seam, joint_group);
                scene.add_interaction(seam, ply_mid[m], seam->interaction(0));
                scene.add_interaction(seam, outer_ply[o], seam->interaction(1));

                const Point pd = arch_pt(u_c, 0.0);
                const Line d_axis = Line::from_points(
                    Point(pd[0], y_out_sign > 0.0 ? -18.0 : -ply_pitch - 32.0, pd[2]),
                    Point(pd[0], y_out_sign > 0.0 ? ply_pitch + 32.0 : 18.0, pd[2])
                );
                const std::shared_ptr<JointPinElement> dowel =
                    std::make_shared<JointPinElement>(d_axis, 8.0, 0.04);
                dowel->name = fmt::format("{}_dowel_{}_{}", tag, o, m);
                scene.add(dowel, joint_group);

                Line b_axis = d_axis;
                b_axis.extend_equally(30.0);
                for (size_t k = 0; k < 2; k++) {
                    const std::shared_ptr<EdgeRawElement>& target =
                        k == 0 ? ply_mid[m] : outer_ply[o];
                    const std::shared_ptr<JointElement> bore = JointElement::drill(b_axis, 8.0);
                    bore->name = fmt::format("{}_bore_{}_{}_{}", tag, o, m, k);
                    bore->is_visible = false;
                    bore->target_insertions = {
                        {{0.0, k == 0 ? y_out_sign : -y_out_sign, 0.0}}
                    };
                    scene.add(bore, joint_group);
                    scene.add_interaction(bore, target, bore->interaction(0));
                }
            }
        }
    };

    add_cheek_grooves(ply_front, u_outer, -0.5 * ply_pitch, -1.0, "front_cheek");
    add_cheek_grooves(ply_back, u_outer, 0.5 * ply_pitch, 1.0, "back_cheek");

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Segmented three-row staggered raw-timber arch (Thesis Fig. 8.7, 8.9, 8.16 and 8.17): ten scanned spruce log segments sawn flat on their two vertical side cheeks while preserving natural round bark along both extrados and intrados, interlocked end-to-end by three-tenon radial finger butt joints and side-by-side across every staggered overlap by three parallel longitudinal cheek grooves and transverse wooden dowels.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_arch --parallel 6 && ./build/element_edge_raw_arch && ../bash/publish-scene.sh --target element_edge_raw_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
