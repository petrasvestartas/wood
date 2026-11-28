#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace {

double shell_z(double x, double y) {
    return 95.0 + 250.0 * (1.0 - (x * x) / (960.0 * 960.0))
           + 140.0 * (1.0 - (y * y) / (960.0 * 960.0));
}

} // namespace

/// Doubly curved bent raw-wood gridshell joined at nine eccentric crossings by conical cross-laps and pins (Thesis Fig. 5.58 B, 5.59).
int main() {

    WoodSession scene("element_edge_raw_bent_shell");

    const std::array<std::string, 2> long_stocks = {"raw_log_0", "raw_log_1"};
    const std::array<double, 3> coords = {-480.0, 0.0, 480.0};
    const double radius = 72.0;
    const double eccentricity = 76.0;
    const int samples = 14;

    const std::shared_ptr<TreeNode> u_group = scene.group_named("bent_arches_u");
    const std::shared_ptr<TreeNode> v_group = scene.group_named("bent_arches_v");
    const std::shared_ptr<TreeNode> j_group = scene.group_named("conical_cross_joints");

    std::vector<std::shared_ptr<EdgeRawElement>> arches_u;
    std::vector<std::shared_ptr<EdgeRawElement>> arches_v;

    for (size_t j = 0; j < coords.size(); j++) {
        const double y = coords[j];
        std::vector<Point> pts;
        for (int s = 0; s <= samples; s++) {
            const double x = -820.0 + 1640.0 * static_cast<double>(s) / samples;
            pts.emplace_back(x, y, shell_z(x, y));
        }
        const std::shared_ptr<EdgeRawElement> arch = EdgeRawElement::from_bent_stock(
            Polyline(pts),
            radius,
            long_stocks[j % 2],
            35.0 * static_cast<double>(j),
            fmt::format("bent_arch_u_{}", j)
        );
        arch->cuts = {
            Plane::from_point_normal(pts.front(), (pts[1] - pts[0]).normalized()),
            Plane::from_point_normal(pts.back(), (pts[samples - 1] - pts[samples]).normalized()),
        };
        scene.add(arch, u_group);
        arches_u.push_back(arch);
    }

    for (size_t i = 0; i < coords.size(); i++) {
        const double x = coords[i];
        std::vector<Point> pts;
        for (int s = 0; s <= samples; s++) {
            const double y = -820.0 + 1640.0 * static_cast<double>(s) / samples;
            pts.emplace_back(x, y, shell_z(x, y) + eccentricity);
        }
        const std::shared_ptr<EdgeRawElement> arch = EdgeRawElement::from_bent_stock(
            Polyline(pts),
            radius,
            long_stocks[(i + 1) % 2],
            110.0 + 50.0 * static_cast<double>(i),
            fmt::format("bent_arch_v_{}", i)
        );
        arch->cuts = {
            Plane::from_point_normal(pts.front(), (pts[1] - pts[0]).normalized()),
            Plane::from_point_normal(pts.back(), (pts[samples - 1] - pts[samples]).normalized()),
        };
        scene.add(arch, v_group);
        arches_v.push_back(arch);
    }

    // Order (*v_seg, *u_seg) with contact from top_pt down to base_pt so conical pockets open upward (+Z) for collision-free lift!
    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.55;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    for (size_t j = 0; j < coords.size(); j++) {
        for (size_t i = 0; i < coords.size(); i++) {
            const double x = coords[i];
            const double y = coords[j];
            const double z0 = shell_z(x, y);
            const double z1 = z0 + eccentricity;
            const Point base_pt(x, y, z0);
            const Point top_pt(x, y, z1);

            const std::shared_ptr<EdgeRawElement> u_seg = arches_u[j]->segment_proxy(base_pt, 210.0);
            const std::shared_ptr<EdgeRawElement> v_seg = arches_v[i]->segment_proxy(top_pt, 210.0);

            const InteractionContactAxis contact(
                Line::from_points(top_pt, base_pt), 0.5, 0.5, 0, 0, 0, 0
            );
            const std::shared_ptr<JointBeamElement> conical = JointBeamElement::from_contact(
                *v_seg, *u_seg, contact, 280.0, 0.91, 1, conical_settings
            );
            conical->name = fmt::format("bent_conical_{}_{}", j, i);
            conical->line_radius = 8.5;
            conical->target_insertions = {{{0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}}};
            scene.add(conical, j_group);
            scene.add_interaction(conical, arches_v[i], conical->interaction(0));
            scene.add_interaction(conical, arches_u[j], conical->interaction(1));

            const Line pin_axis = Line::from_points({x, y, z0 - 40.0}, {x, y, z1 + 40.0});
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 8.5, 0.04);
            pin->name = fmt::format("bent_pin_{}_{}", j, i);
            scene.add(pin, j_group);
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Doubly curved bent raw-wood gridshell (Thesis Fig. 5.58 B and Fig. 5.59): six scanned spruce trunks warped along arched shell centrelines (EdgeRawElement::from_bent_stock) cross at nine eccentric nodes (dz = 54 mm), where local tangent proxies (segment_proxy) cut 6-plane conical cross-lap saddles and cylindrical hardwood pins while preserving over two-thirds of every curved trunk section.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_bent_shell --parallel 6 && ./build/element_edge_raw_bent_shell && ../bash/publish-scene.sh --target element_edge_raw_bent_shell

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
