#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace {

Mesh axis_box(const Point& c, double hx, double hy, double hz) {

    const Polyline loop({
        Point(c[0] - hx, c[1] - hy, c[2] - hz),
        Point(c[0] + hx, c[1] - hy, c[2] - hz),
        Point(c[0] + hx, c[1] + hy, c[2] - hz),
        Point(c[0] - hx, c[1] + hy, c[2] - hz),
        Point(c[0] - hx, c[1] - hy, c[2] - hz),
    });
    return Mesh::loft({loop}, {loop.translated(Vector(0.0, 0.0, 2.0 * hz))}, true);
}

void subtract_box(
    WoodSession& scene,
    const std::shared_ptr<Element>& target,
    const Mesh& cutter,
    const Vector& way_in,
    const std::string& name,
    const std::shared_ptr<TreeNode>& group
) {
    const std::shared_ptr<JointElement> cut =
        std::make_shared<JointElement>(cutter, SolidOperation::subtract);
    cut->name = name;
    cut->is_visible = false;
    cut->target_insertions = {{way_in}};
    scene.add(cut, group);
    scene.add_interaction(cut, target, cut->interaction(0));
}

} // namespace

/// Cantilevered raw-wood stave bench (Thesis Fig. 5.22, 5.23, 5.44): grooved and dowelled bark staves mortised onto four minimally milled vertical log tenons.
int main() {

    WoodSession scene("element_edge_raw_bench");

    const std::array<std::string, 4> stocks = {"raw_log_0", "raw_log_1", "raw_log_2", "raw_log_3"};
    const std::array<double, 4> y_centers = {165.0, 55.0, -55.0, -165.0};
    const double x_wall = -210.0;
    const double z_shoulder = 528.0;
    const double z_top = 620.0;
    const double tenon_hx = 30.0;
    const double tenon_hy = 22.0;

    const std::shared_ptr<TreeNode> wall_group = scene.group_named("wall_legs");
    const std::shared_ptr<TreeNode> deck_group = scene.group_named("cantilever_deck");
    const std::shared_ptr<TreeNode> dowel_group = scene.group_named("dowels");

    std::vector<std::shared_ptr<EdgeRawElement>> legs;
    std::vector<std::shared_ptr<EdgeRawElement>> staves;

    // 1. Four vertical raw-wood wall logs with surfaced side cheeks and minimally milled top tenons (Fig. 5.44)
    for (size_t i = 0; i < 4; i++) {
        const double yc = y_centers[i];
        const std::shared_ptr<EdgeRawElement> leg = EdgeRawElement::from_stock(
            Line::from_points({x_wall, yc, -30.0}, {x_wall, yc, z_top + 40.0}),
            60.0,
            stocks[i],
            35.0 * static_cast<double>(i),
            true,
            fmt::format("wall_leg_{}", i)
        );
        leg->cuts = {
            Plane::from_point_normal({x_wall, yc, 0.0}, {0.0, 0.0, 1.0}),
            Plane::from_point_normal({x_wall, yc, z_top}, {0.0, 0.0, -1.0}),
        };
        if (i > 0)
            leg->cuts.push_back(
                Plane::from_point_normal({x_wall, yc + 54.7, 260.0}, {0.0, -1.0, 0.0})
            );
        if (i + 1 < 4)
            leg->cuts.push_back(
                Plane::from_point_normal({x_wall, yc - 54.7, 260.0}, {0.0, 1.0, 0.0})
            );
        scene.add(leg, wall_group);
        legs.push_back(leg);
    }

    // 2. Four cantilevered raw-wood deck staves (840 mm <= 902 mm stock): surfaced flat on top, natural bark below, stepped longitudinal grooves
    for (size_t i = 0; i < 4; i++) {
        const double yc = y_centers[i];
        const std::shared_ptr<EdgeRawElement> stave = EdgeRawElement::from_stock(
            Line::from_points({-320.0, yc, 574.0}, {560.0, yc, 574.0}),
            60.0,
            stocks[(i + 1) % 4],
            180.0 + 40.0 * static_cast<double>(i),
            true,
            fmt::format("deck_stave_{}", i)
        );
        stave->cuts = {
            Plane::from_point_normal({-300.0, yc, 574.0}, {1.0, 0.0, 0.0}),
            Plane::from_point_normal({540.0, yc, 574.0}, {-1.0, 0.0, 0.0}),
            Plane::from_point_normal({120.0, yc, z_top}, {0.0, 0.0, -1.0}),
        };
        if (i > 0)
            stave->cuts.push_back(
                Plane::from_point_normal({120.0, yc + 66.4, 574.0}, {0.0, -1.0, 0.0})
            );
        if (i + 1 < 4)
            stave->cuts.push_back(
                Plane::from_point_normal({120.0, yc - 54.4, 574.0}, {0.0, 1.0, 0.0})
            );
        scene.add(stave, deck_group);
        staves.push_back(stave);

        // Four individual closed hexahedral cheek cuts forming the rectangular top tenon on wall_leg_i
        const double z_mid_t = 0.5 * (z_shoulder + z_top + 50.0);
        const double hz_t = 0.5 * (z_top + 50.0 - z_shoulder);
        subtract_box(
            scene,
            legs[i],
            axis_box({x_wall + tenon_hx + 60.0, yc, z_mid_t}, 60.0, 110.0, hz_t),
            Vector(0.0, 1.0, 0.0),
            fmt::format("leg_cheek_px_{}", i),
            wall_group
        );
        subtract_box(
            scene,
            legs[i],
            axis_box({x_wall, yc - tenon_hy - 60.0, z_mid_t}, 110.0, 60.0, hz_t),
            Vector(0.0, 1.0, 0.0),
            fmt::format("leg_cheek_ny_{}", i),
            wall_group
        );
        subtract_box(
            scene,
            legs[i],
            axis_box({x_wall, yc + tenon_hy + 60.0, z_mid_t}, 110.0, 60.0, hz_t),
            Vector(0.0, 1.0, 0.0),
            fmt::format("leg_cheek_py_{}", i),
            wall_group
        );

        // Spot-face underside bearing & longitudinal rabbet grooves on deck_stave_i
        subtract_box(
            scene,
            stave,
            axis_box({x_wall, yc, z_shoulder - 44.2}, 78.0, 75.0, 45.0),
            Vector(0.0, 0.0, -1.0),
            fmt::format("stave_bearing_{}", i),
            deck_group
        );
        if (i > 0) {
            subtract_box(
                scene,
                stave,
                axis_box({120.0, yc + 67.0, 526.2}, 460.0, 12.8, 52.0),
                Vector(0.0, 0.0, -1.0),
                fmt::format("stave_rabbet_py_{}", i),
                deck_group
            );
        }
        if (i + 1 < 4) {
            subtract_box(
                scene,
                stave,
                axis_box({120.0, yc - 55.0, 608.0}, 460.0, 12.8, 30.0),
                Vector(0.0, 0.0, -1.0),
                fmt::format("stave_rabbet_ny_{}", i),
                deck_group
            );
        }

        // Binary tenon-and-mortise joint linking wall_leg_i (-X cheek) and deck_stave_i (through-mortise)
        const std::shared_ptr<JointBeamElement> corner = std::make_shared<JointBeamElement>();
        corner->name = fmt::format("bench_tenon_mortise_{}", i);
        corner->is_visible = false;
        corner->targets = {legs[i]->guid(), stave->guid()};
        corner->target_insertions = {{{0.0, 1.0, 0.0}}, {{0.0, 0.0, -1.0}}};

        JointCutStep step_leg;
        step_leg.label = "A2";
        step_leg.target = 0;
        step_leg.removal = axis_box({x_wall - tenon_hx - 60.0, yc, z_mid_t}, 60.0, 110.0, hz_t);

        JointCutStep step_stave;
        step_stave.label = "B2";
        step_stave.target = 1;
        step_stave.removal = axis_box({x_wall, yc, 575.0}, tenon_hx + 0.9, tenon_hy + 0.9, 80.0);

        corner->steps = {step_leg, step_stave};
        scene.add(corner, deck_group);
        scene.add_interaction(corner, legs[i], corner->interaction(0));
        scene.add_interaction(corner, stave, corner->interaction(1));
    }

    // 3. Transverse 10 mm radius wooden dowels through both the vertical wall logs and the cantilevered deck staves (Fig. 5.23)
    const std::array<double, 2> z_wall_dowels = {95.0, 430.0};
    for (size_t k = 0; k < z_wall_dowels.size(); k++) {
        const Line pin_line =
            Line::from_points({x_wall, -195.0, z_wall_dowels[k]}, {x_wall, 195.0, z_wall_dowels[k]});
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(pin_line, 10.0, 0.02);
        pin->name = fmt::format("wall_dowel_{}", k);
        scene.add(pin, dowel_group);

        Line bore_line = pin_line;
        bore_line.extend_equally(45.0);
        for (size_t i = 0; i < 4; i++) {
            const std::shared_ptr<JointElement> bore = JointElement::drill(bore_line, 10.0);
            bore->name = fmt::format("wall_bore_{}_{}", k, i);
            bore->is_visible = false;
            bore->target_insertions = {{{0.0, 1.0, 0.0}}};
            scene.add(bore, dowel_group);
            scene.add_interaction(bore, legs[i], bore->interaction(0));
        }
    }

    const std::array<double, 3> x_deck_dowels = {20.0, 240.0, 440.0};
    for (size_t k = 0; k < x_deck_dowels.size(); k++) {
        const Line pin_line =
            Line::from_points({x_deck_dowels[k], -195.0, 578.0}, {x_deck_dowels[k], 195.0, 578.0});
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(pin_line, 10.0, 0.02);
        pin->name = fmt::format("deck_dowel_{}", k);
        scene.add(pin, dowel_group);

        Line bore_line = pin_line;
        bore_line.extend_equally(45.0);
        for (size_t i = 0; i < 4; i++) {
            const std::shared_ptr<JointElement> bore = JointElement::drill(bore_line, 10.0);
            bore->name = fmt::format("deck_bore_{}_{}", k, i);
            bore->is_visible = false;
            bore->target_insertions = {{{0.0, 0.0, -1.0}}};
            scene.add(bore, dowel_group);
            scene.add_interaction(bore, staves[i], bore->interaction(0));
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Cantilevered raw-wood stave bench (Thesis Fig. 5.22, 5.23 and 5.44): four vertical scanned spruce wall logs with surfaced cheeks and minimally milled rectangular top tenons support four cantilevered deck staves surfaced flat on top while keeping natural round bark underneath, interlocked along every longitudinal seam by stepped rabbet grooves and 10 mm radius transverse wooden dowels.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_bench --parallel 6 && ./build/element_edge_raw_bench && ../bash/publish-scene.sh --target element_edge_raw_bench

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
