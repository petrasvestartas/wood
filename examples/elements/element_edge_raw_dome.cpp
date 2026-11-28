#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Spatial reciprocal raw-wood topologies: hexagonal conical dome (Thesis Fig. 5.62 A) and hyperboloid reciprocal funnel (Fig. 9.21).
int main() {

    WoodSession scene("element_edge_raw_dome");

    const std::array<std::string, 4> stocks = {"raw_log_0", "raw_log_1", "raw_log_2", "raw_log_3"};
    const std::array<std::string, 2> long_stocks = {"raw_log_0", "raw_log_1"};

    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.55;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    // 1. Left: Single-layer hexagonal / triangular reciprocal dome (Thesis Fig. 5.62 A1-A2)
    const std::shared_ptr<TreeNode> dome_group = scene.group_named("hexagonal_reciprocal_dome");
    std::vector<std::shared_ptr<EdgeRawElement>> skirt_logs;
    std::vector<std::shared_ptr<EdgeRawElement>> crown_logs;

    const int n_skirt = 6;
    for (int i = 0; i < n_skirt; i++) {
        const double a0 = Tolerance::TWO_PI * static_cast<double>(i) / n_skirt;
        const double a1 = a0 + 1.08;
        const Point p0(760.0 * std::cos(a0), 760.0 * std::sin(a0), 55.0);
        const Point p1(175.0 * std::cos(a1), 175.0 * std::sin(a1), 405.0);
        const Vector dir = (p1 - p0).normalized();
        const std::shared_ptr<EdgeRawElement> log = EdgeRawElement::from_stock(
            Line::from_points(p0 - dir * 35.0, p1 + dir * 65.0),
            76.0,
            long_stocks[static_cast<size_t>(i) % 2],
            15.0 + 30.0 * i,
            true,
            fmt::format("dome_skirt_{}", i)
        );
        log->cuts = {
            Plane::from_point_normal(p0, dir),
            Plane::from_point_normal(p1 + dir * 45.0, dir * -1.0),
        };
        scene.add(log, dome_group);
        skirt_logs.push_back(log);
    }

    for (int k = 0; k < 3; k++) {
        const double a = Tolerance::TWO_PI * static_cast<double>(k) / 3.0 + 0.42;
        const Point c(235.0 * std::cos(a), 235.0 * std::sin(a), 492.0);
        const Vector t(-std::sin(a), std::cos(a), 0.0);
        const Vector ut = t.normalized();
        const Point p0 = c - ut * 290.0;
        const Point p1 = c + ut * 290.0;
        const std::shared_ptr<EdgeRawElement> log = EdgeRawElement::from_stock(
            Line::from_points(p0 - ut * 25.0, p1 + ut * 25.0),
            76.0,
            long_stocks[static_cast<size_t>(k + 1) % 2],
            80.0 * k,
            true,
            fmt::format("dome_crown_{}", k)
        );
        log->cuts = {
            Plane::from_point_normal(p0, ut),
            Plane::from_point_normal(p1, ut * -1.0),
        };
        scene.add(log, dome_group);
        crown_logs.push_back(log);
    }

    // Order (*crown_logs[k], *skirt_logs[s]) with contact from pc down to ps so conical pockets open upward (+Z)!
    for (int k = 0; k < 3; k++) {
        for (int s = 0; s < n_skirt; s++) {
            double ts = 0.0;
            double tc = 0.0;
            if (!Intersection::line_line_parameters(
                    skirt_logs[s]->chord(), crown_logs[k]->chord(), ts, tc, 0.0, false, false
                ))
                continue;
            if (ts < 0.06 || ts > 0.96 || tc < 0.06 || tc > 0.94)
                continue;
            const Point ps = skirt_logs[s]->chord().point_at(ts);
            const Point pc = crown_logs[k]->chord().point_at(tc);
            if (Point::distance(ps, pc) > 135.0)
                continue;

            const Point mid_xy(0.5 * (ps[0] + pc[0]), 0.5 * (ps[1] + pc[1]), 0.0);
            const InteractionContactAxis contact(
                Line::from_points({mid_xy[0], mid_xy[1], pc[2]}, {mid_xy[0], mid_xy[1], ps[2]}),
                tc,
                ts,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> conical = JointBeamElement::from_contact(
                *crown_logs[k], *skirt_logs[s], contact, 260.0, 0.91, 1, conical_settings
            );
            conical->name = fmt::format("dome_conical_{}_{}", k, s);
            conical->line_radius = 8.0;
            conical->target_insertions = {{{0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}}};
            scene.add(conical, dome_group);
            scene.add_interaction(conical, crown_logs[k], conical->interaction(0));
            scene.add_interaction(conical, skirt_logs[s], conical->interaction(1));

            const Line pin_axis =
                Line::from_points({mid_xy[0], mid_xy[1], ps[2] - 36.0}, {mid_xy[0], mid_xy[1], pc[2] + 36.0});
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 8.0, 0.04);
            pin->name = fmt::format("dome_pin_{}_{}", k, s);
            scene.add(pin, dome_group);
        }
    }

    // 2. Right: Hyperboloid reciprocal raw-wood funnel tower (Thesis Fig. 9.21 bottom-left)
    const std::shared_ptr<TreeNode> tower_group = scene.group_named("hyperboloid_funnel_tower");
    const Vector shift(1780.0, -1020.0, 0.0);
    const int n_gen = 5;
    const double twist = 1.62;
    std::vector<std::shared_ptr<EdgeRawElement>> inner_gen;
    std::vector<std::shared_ptr<EdgeRawElement>> outer_gen;
    std::vector<Vector> inner_out_dirs;

    for (int i = 0; i < n_gen; i++) {
        const double a0 = Tolerance::TWO_PI * static_cast<double>(i) / n_gen;
        const Point b_in = Point(430.0 * std::cos(a0), 430.0 * std::sin(a0), 45.0) + shift;
        const Point t_in =
            Point(520.0 * std::cos(a0 + twist), 520.0 * std::sin(a0 + twist), 920.0) + shift;
        const Vector d_in = (t_in - b_in).normalized();
        const std::shared_ptr<EdgeRawElement> in_log = EdgeRawElement::from_stock(
            Line::from_points(b_in - d_in * 40.0, t_in + d_in * 40.0),
            78.0,
            long_stocks[static_cast<size_t>(i) % 2],
            80.0 + 25.0 * i,
            true,
            fmt::format("tower_inner_{}", i)
        );
        in_log->cuts = {
            Plane::from_point_normal(b_in, {0.0, 0.0, 1.0}),
            Plane::from_point_normal(t_in, {0.0, 0.0, -1.0}),
        };
        scene.add(in_log, tower_group);
        inner_gen.push_back(in_log);

        const Point mid_i = b_in + (t_in - b_in) * 0.5;
        inner_out_dirs.push_back(Vector(mid_i[0] - shift[0], mid_i[1] - shift[1], 0.0).normalized());
    }

    std::vector<Vector> outer_out_dirs;
    for (int j = 0; j < n_gen; j++) {
        const double b0 = Tolerance::TWO_PI * static_cast<double>(j) / n_gen + 0.34;
        const Point b_out = Point(558.0 * std::cos(b0), 558.0 * std::sin(b0), 45.0) + shift;
        const Point t_out =
            Point(654.0 * std::cos(b0 - twist), 654.0 * std::sin(b0 - twist), 920.0) + shift;
        const Vector d_out = (t_out - b_out).normalized();
        const std::shared_ptr<EdgeRawElement> out_log = EdgeRawElement::from_stock(
            Line::from_points(b_out - d_out * 40.0, t_out + d_out * 40.0),
            78.0,
            long_stocks[static_cast<size_t>(j + 1) % 2],
            10.0 + 35.0 * j,
            true,
            fmt::format("tower_outer_{}", j)
        );
        out_log->cuts = {
            Plane::from_point_normal(b_out, {0.0, 0.0, 1.0}),
            Plane::from_point_normal(t_out, {0.0, 0.0, -1.0}),
        };
        scene.add(out_log, tower_group);
        outer_gen.push_back(out_log);

        const Point mid_j = b_out + (t_out - b_out) * 0.5;
        outer_out_dirs.push_back(Vector(mid_j[0] - shift[0], mid_j[1] - shift[1], 0.0).normalized());
    }

    // Order (*outer_gen[j], *inner_gen[i]) with contact from outside in along `out_dir` so all outer generators withdraw radially outward!
    for (int j = 0; j < n_gen; j++) {
        const Vector out_dir = outer_out_dirs[j];
        for (int i = 0; i < n_gen; i++) {
            double ti = 0.0;
            double tj = 0.0;
            if (!Intersection::line_line_parameters(
                    inner_gen[i]->chord(), outer_gen[j]->chord(), ti, tj, 0.0, false, false
                ))
                continue;
            if (ti < 0.08 || ti > 0.92 || tj < 0.08 || tj > 0.92)
                continue;
            const Point pi = inner_gen[i]->chord().point_at(ti);
            const Point pj = outer_gen[j]->chord().point_at(tj);
            const double sep = Point::distance(pi, pj);
            if (sep > 165.0)
                continue;

            const Point mid = pi + (pj - pi) * 0.5;
            const InteractionContactAxis contact(
                Line::from_points(mid + out_dir * (0.5 * sep), mid - out_dir * (0.5 * sep)),
                tj,
                ti,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> conical = JointBeamElement::from_contact(
                *outer_gen[j], *inner_gen[i], contact, 290.0, 0.91, 1, conical_settings
            );
            conical->name = fmt::format("tower_conical_{}_{}", i, j);
            conical->line_radius = 7.5;
            conical->target_insertions = {{out_dir * -1.0}, {inner_out_dirs[i] * -1.0, out_dir}};
            scene.add(conical, tower_group);
            scene.add_interaction(conical, outer_gen[j], conical->interaction(0));
            scene.add_interaction(conical, inner_gen[i], conical->interaction(1));

            const Line pin_axis = Line::from_points(mid - out_dir * 56.0, mid + out_dir * 56.0);
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 7.5, 0.04);
            pin->name = fmt::format("tower_pin_{}_{}", i, j);
            scene.add(pin, tower_group);
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Spatial single- and double-ring reciprocal raw-wood structures (Thesis Fig. 5.62 A and Fig. 9.21): a nine-log hexagonal reciprocal dome crowned by a three-bar triangular ring (left) beside a ten-log hyperboloid reciprocal funnel tower (right), joined at every eccentric crossing by 6-cut conical cross-laps and radial wooden pins.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_dome --parallel 6 && ./build/element_edge_raw_dome && ../bash/publish-scene.sh --target element_edge_raw_dome

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
