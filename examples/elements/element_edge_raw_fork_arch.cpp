#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Two-bay pointed Gothic tree-fork portal nave (Thesis Appendix A Fig. A.3 B-C): bifurcated Y-fork columns triangulating pointed raw-wood arches and longitudinal purlins.
int main() {

    WoodSession scene("element_edge_raw_fork_arch");

    const std::shared_ptr<TreeNode> fork_group = scene.group_named("fork_columns");
    const std::shared_ptr<TreeNode> rafter_group = scene.group_named("pointed_arch_rafters");
    const std::shared_ptr<TreeNode> purlin_group = scene.group_named("purlins");
    const std::shared_ptr<TreeNode> joint_group = scene.group_named("nave_joints");

    const std::array<double, 2> bay_y = {-380.0, 380.0};
    const double tilt_rad = 22.0 * Tolerance::TO_RADIANS;
    const double cs = std::cos(tilt_rad);
    const double sn = std::sin(tilt_rad);

    std::vector<std::shared_ptr<EdgeRawElement>> rafters_left;
    std::vector<std::shared_ptr<EdgeRawElement>> rafters_right;

    for (size_t b = 0; b < bay_y.size(); b++) {
        const double yb = bay_y[b];

        for (int side : {-1, 1}) {
            const double s = static_cast<double>(side);
            const Point base_origin(s * 580.0, yb, -35.0);
            const Vector x_col(-s * cs, 0.0, sn);
            const Vector y_col(0.0, -s, 0.0);
            const Vector z_col(s * sn, 0.0, cs);
            const Xform xf = Xform::frame_to_world(base_origin, x_col, y_col, z_col);

            const Point stem_p0 = Point(0.0, 0.0, 40.0).transformed(xf);
            const Point stem_p1 = Point(0.0, 0.0, 705.0).transformed(xf);
            const std::shared_ptr<EdgeRawElement> fork_col = EdgeRawElement::from_registered_stock(
                "raw_fork_1",
                xf,
                56.0,
                Line::from_points(stem_p0, stem_p1),
                fmt::format("fork_col_{}_{}", b, side < 0 ? "left" : "right")
            );
            fork_col->cuts = {
                Plane::from_point_normal({s * 580.0, yb, 0.0}, {0.0, 0.0, 1.0}),
            };
            scene.add(fork_col, fork_group);

            const Point eave(s * 915.0, yb, 546.6);
            const Point crown(0.0, yb, 1242.4);
            const Vector r_dir = (crown - eave).normalized();
            const std::shared_ptr<EdgeRawElement> rafter = EdgeRawElement::from_stock(
                Line::from_points(eave - r_dir * 30.0, crown + r_dir * 70.0),
                68.0,
                side < 0 ? "raw_log_0" : "raw_log_1",
                45.0 * static_cast<double>(b) + (side < 0 ? 0.0 : 90.0),
                true,
                fmt::format("arch_rafter_{}_{}", b, side < 0 ? "left" : "right")
            );
            rafter->cuts = {
                Plane::from_point_normal(eave, r_dir),
                Plane::from_point_normal(crown + r_dir * 52.0, r_dir * -1.0),
            };
            scene.add(rafter, rafter_group);

            const std::shared_ptr<JointBeamElement> stem_joint = JointBeamElement::from_library(
                "pegged_angled_stopped_dovetail_half_lap",
                *fork_col,
                *rafter,
                {{"flare", 0.24}, {"stop", 0.72}}
            );
            stem_joint->name = fmt::format("fork_stem_dovetail_{}_{}", b, side < 0 ? "L" : "R");
            scene.add(stem_joint, joint_group);
            scene.add_interaction(stem_joint, fork_col, stem_joint->interaction(0));
            scene.add_interaction(stem_joint, rafter, stem_joint->interaction(1));

            const Point br0 = Point(110.0, 0.0, 460.0).transformed(xf);
            const Point br1 = Point(384.5, 0.0, 794.9).transformed(xf);
            const std::shared_ptr<EdgeRawElement> branch =
                fork_col->branch_proxy(44.0, Line::from_points(br0, br1));
            const std::shared_ptr<JointBeamElement> branch_joint = JointBeamElement::from_library(
                "pegged_angled_stopped_dovetail_half_lap",
                *branch,
                *rafter,
                {{"flare", 0.24}, {"stop", 0.72}}
            );
            branch_joint->name = fmt::format("fork_branch_dovetail_{}_{}", b, side < 0 ? "L" : "R");
            scene.add(branch_joint, joint_group);
            scene.add_interaction(branch_joint, fork_col, branch_joint->interaction(0));
            scene.add_interaction(branch_joint, rafter, branch_joint->interaction(1));

            if (side < 0)
                rafters_left.push_back(rafter);
            else
                rafters_right.push_back(rafter);
        }

        const std::shared_ptr<JointBeamElement> apex = JointBeamElement::from_library(
            "pegged_angled_stopped_dovetail_half_lap",
            *rafters_left[b],
            *rafters_right[b],
            {{"flare", 0.22}, {"stop", 0.78}}
        );
        apex->name = fmt::format("pointed_crown_apex_{}", b);
        scene.add(apex, joint_group);
        scene.add_interaction(apex, rafters_left[b], apex->interaction(0));
        scene.add_interaction(apex, rafters_right[b], apex->interaction(1));
    }

    // Order (*purlin, *rafter) with contact from z_purlin down to z_rafter + CrossDrills::one so purlins lift cleanly along +Z!
    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.55;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    struct PurlinSpec {
        double x;
        double z_rafter;
        double z_purlin;
        bool on_left;
        std::string name;
    };
    const std::array<PurlinSpec, 2> purlin_specs = {{
        {-320.0, 999.1, 1082.0, true, "nave_purlin_left"},
        {320.0, 999.1, 1082.0, false, "nave_purlin_right"},
    }};

    for (size_t p = 0; p < purlin_specs.size(); p++) {
        const PurlinSpec& ps = purlin_specs[p];
        const std::shared_ptr<EdgeRawElement> purlin = EdgeRawElement::from_stock(
            Line::from_points({ps.x, -620.0, ps.z_purlin}, {ps.x, 620.0, ps.z_purlin}),
            76.0,
            p == 0 ? "raw_log_0" : "raw_log_1",
            35.0 + 160.0 * static_cast<double>(p),
            true,
            ps.name
        );
        purlin->cuts = {
            Plane::from_point_normal({ps.x, -600.0, ps.z_purlin}, {0.0, 1.0, 0.0}),
            Plane::from_point_normal({ps.x, 600.0, ps.z_purlin}, {0.0, -1.0, 0.0}),
        };
        scene.add(purlin, purlin_group);

        for (size_t b = 0; b < bay_y.size(); b++) {
            const double yb = bay_y[b];
            const std::shared_ptr<EdgeRawElement>& rafter =
                ps.on_left ? rafters_left[b] : rafters_right[b];
            double tr = 0.5;
            double tp = 0.5;
            Intersection::line_line_parameters(rafter->chord(), purlin->chord(), tr, tp, 0.0, false, false);

            const InteractionContactAxis contact(
                Line::from_points({ps.x, yb, ps.z_purlin}, {ps.x, yb, ps.z_rafter}),
                tp,
                tr,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> conical = JointBeamElement::from_contact(
                *purlin, *rafter, contact, 275.0, 0.91, 1, conical_settings
            );
            conical->name = fmt::format("purlin_conical_{}_{}", p, b);
            conical->line_radius = 8.0;
            conical->target_insertions = {{{0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}}};
            scene.add(conical, joint_group);
            scene.add_interaction(conical, purlin, conical->interaction(0));
            scene.add_interaction(conical, rafter, conical->interaction(1));

            const Line pin_axis =
                Line::from_points({ps.x, yb, ps.z_rafter - 38.0}, {ps.x, yb, ps.z_purlin + 38.0});
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 8.0, 0.04);
            pin->name = fmt::format("purlin_pin_{}_{}", p, b);
            scene.add(pin, joint_group);
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two-bay pointed Gothic tree-fork portal nave (Thesis Appendix A Fig. A.3 B-C): four upright bifurcated Y-fork columns branch inward so both limbs of each natural spruce fork engage the inclined pointed arch rafters via pegged stopped-dovetail half-lap joints, meeting at pointed Gothic crowns and tied longitudinally by raw-wood purlins in eccentric conical cross-laps.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_fork_arch --parallel 6 && ./build/element_edge_raw_fork_arch && ../bash/publish-scene.sh --target element_edge_raw_fork_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
