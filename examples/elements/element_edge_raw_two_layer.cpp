#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace {

Mesh box_solid(const Point& c, double hx, double hy, double hz) {

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

/// Two-layer vertical reciprocal raw-wood prototype with conical cross-laps on both faces of the columns and a mortised base slab (Thesis Fig. 5.60, 5.61).
int main() {

    WoodSession scene("element_edge_raw_two_layer");

    const std::array<std::string, 2> long_stocks = {"raw_log_0", "raw_log_1"};

    const std::shared_ptr<TreeNode> base_group = scene.group_named("base_plate");
    const Polyline base_loop =
        Polyline::rectangle(Point(-720.0, -220.0, 0.0), Vector::x_axis(), Vector::y_axis(), 1440.0, 440.0);
    const std::shared_ptr<FacePlanarElement> base = std::make_shared<FacePlanarElement>(
        base_loop, base_loop.translated(Vector(0.0, 0.0, 68.0)), "base_slab"
    );
    scene.add(base, base_group);

    const std::shared_ptr<TreeNode> col_group = scene.group_named("columns");
    const std::array<std::pair<double, double>, 4> col_x = {{
        {-430.0, -390.0},
        {-155.0, -195.0},
        {155.0, 195.0},
        {430.0, 390.0},
    }};
    const double z_foot = 24.0;
    const double z_col_top = 1240.0;
    const double tenon_h = 28.0;

    std::vector<std::shared_ptr<EdgeRawElement>> columns;

    for (size_t c = 0; c < 4; c++) {
        const double x0 = col_x[c].first;
        const double x1 = col_x[c].second;
        const std::shared_ptr<EdgeRawElement> col = EdgeRawElement::from_stock(
            Line::from_points({x0, 0.0, z_foot - 20.0}, {x1, 0.0, z_col_top + 20.0}),
            76.0,
            long_stocks[c % 2],
            65.0 + 25.0 * static_cast<double>(c),
            true,
            fmt::format("upright_col_{}", c)
        );
        col->cuts = {
            Plane::from_point_normal({x0, 0.0, z_foot}, {0.0, 0.0, 1.0}),
            Plane::from_point_normal({x1, 0.0, z_col_top}, {0.0, 0.0, -1.0}),
        };
        scene.add(col, col_group);

        subtract_box(
            scene,
            col,
            box_solid({x0 + tenon_h + 65.0, 0.0, 37.5}, 65.0, 115.0, 32.0),
            Vector(0.0, 0.0, -1.0),
            fmt::format("col_cheek_px_{}", c),
            col_group
        );
        subtract_box(
            scene,
            col,
            box_solid({x0, -tenon_h - 65.0, 37.5}, 115.0, 65.0, 32.0),
            Vector(0.0, 0.0, -1.0),
            fmt::format("col_cheek_ny_{}", c),
            col_group
        );
        subtract_box(
            scene,
            col,
            box_solid({x0, tenon_h + 65.0, 37.5}, 115.0, 65.0, 32.0),
            Vector(0.0, 0.0, -1.0),
            fmt::format("col_cheek_py_{}", c),
            col_group
        );

        const std::shared_ptr<JointBeamElement> foot = std::make_shared<JointBeamElement>();
        foot->name = fmt::format("col_foot_joint_{}", c);
        foot->is_visible = false;
        foot->targets = {base->guid(), col->guid()};
        foot->target_insertions = {{{0.0, 0.0, 1.0}}, {{0.0, 0.0, -1.0}}};

        JointCutStep step_slab;
        step_slab.label = "A2";
        step_slab.target = 0;
        step_slab.removal = box_solid({x0, 0.0, 46.0}, tenon_h + 1.8, tenon_h + 1.8, 26.0);

        JointCutStep step_col;
        step_col.label = "B2";
        step_col.target = 1;
        step_col.removal = box_solid({x0 - tenon_h - 65.0, 0.0, 37.5}, 65.0, 115.0, 32.0);

        foot->steps = {step_slab, step_col};
        scene.add(foot, col_group);
        scene.add_interaction(foot, base, foot->interaction(0));
        scene.add_interaction(foot, col, foot->interaction(1));
        columns.push_back(col);
    }

    struct RailSpec {
        double y;
        double z_left;
        double z_right;
        size_t stock;
        std::string name;
    };
    const std::array<RailSpec, 4> rail_specs = {{
        {82.0, 460.0, 530.0, 1, "back_rail_lower"},
        {82.0, 1020.0, 950.0, 0, "back_rail_upper"},
        {-82.0, 345.0, 280.0, 0, "front_rail_lower"},
        {-82.0, 835.0, 905.0, 1, "front_rail_upper"},
    }};

    const std::shared_ptr<TreeNode> rail_group = scene.group_named("horizontal_rails");
    const std::shared_ptr<TreeNode> joint_group = scene.group_named("double_sided_conical_joints");

    // Order (*rail, *columns[c]) with contact from spec.y toward 0.0 + CrossDrills::one so conical pockets open outward along +-Y!
    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.55;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    for (size_t r = 0; r < rail_specs.size(); r++) {
        const RailSpec& spec = rail_specs[r];
        const std::shared_ptr<EdgeRawElement> rail = EdgeRawElement::from_stock(
            Line::from_points({-660.0, spec.y, spec.z_left}, {660.0, spec.y, spec.z_right}),
            76.0,
            long_stocks[spec.stock],
            40.0 * static_cast<double>(r),
            true,
            spec.name
        );
        rail->cuts = {
            Plane::from_point_normal({-640.0, spec.y, spec.z_left}, {1.0, 0.0, 0.0}),
            Plane::from_point_normal({640.0, spec.y, spec.z_right}, {-1.0, 0.0, 0.0}),
        };
        scene.add(rail, rail_group);

        const double y_sign = spec.y > 0.0 ? 1.0 : -1.0;
        for (size_t c = 0; c < columns.size(); c++) {
            const Point c0 = columns[c]->chord().start();
            const Point c1 = columns[c]->chord().end();
            const Point r0 = rail->chord().start();
            const Point r1 = rail->chord().end();
            const Line col_xz = Line::from_points({c0[0], 0.0, c0[2]}, {c1[0], 0.0, c1[2]});
            const Line rail_xz = Line::from_points({r0[0], 0.0, r0[2]}, {r1[0], 0.0, r1[2]});
            double tc = 0.5;
            double tr = 0.5;
            Intersection::line_line_parameters(col_xz, rail_xz, tc, tr, 0.0, false, false);
            const Point p_xz = col_xz.point_at(tc);

            const InteractionContactAxis contact(
                Line::from_points({p_xz[0], spec.y, p_xz[2]}, {p_xz[0], 0.0, p_xz[2]}),
                tr,
                tc,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> conical = JointBeamElement::from_contact(
                *rail, *columns[c], contact, 295.0, 0.91, 1, conical_settings
            );
            conical->name = fmt::format("conical_{}_{}", r, c);
            conical->line_radius = 8.5;
            conical->target_insertions = {
                {{0.0, -y_sign, 0.0}}, {{0.0, 0.0, -1.0}, {0.0, y_sign, 0.0}}
            };
            scene.add(conical, joint_group);
            scene.add_interaction(conical, rail, conical->interaction(0));
            scene.add_interaction(conical, columns[c], conical->interaction(1));

            const Line pin_axis = Line::from_points(
                {p_xz[0], -y_sign * 34.0, p_xz[2]}, {p_xz[0], spec.y + y_sign * 38.0, p_xz[2]}
            );
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 8.5, 0.04);
            pin->name = fmt::format("pin_{}_{}", r, c);
            scene.add(pin, joint_group);
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two-layer vertical reciprocal raw-wood prototype (Thesis Fig. 5.60 and Fig. 5.61): four upright scanned spruce columns mortised into a timber base plate receive two front (-Y) and two back (+Y) inclined horizontal raw-wood rails across sixteen staggered double-sided 6-cut conical cross-lap joints and transverse hardwood pins.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_two_layer --parallel 6 && ./build/element_edge_raw_two_layer && ../bash/publish-scene.sh --target element_edge_raw_two_layer

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
