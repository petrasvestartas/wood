#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Four bifurcated Y-fork columns joined at both stem (dovetail T-bridle) and branch (pegged oblique thrust tenon) into two girders under three conical cross-lap rafters.
int main() {

    WoodSession scene("element_edge_raw_session");

    const std::shared_ptr<TreeNode> forks_group = scene.group_named("forks");
    const std::shared_ptr<TreeNode> girders_group = scene.group_named("girders");
    const std::shared_ptr<TreeNode> rafters_group = scene.group_named("rafters");
    const std::shared_ptr<TreeNode> joints = scene.group_named("joints");

    constexpr double z_girder = 940.0;
    constexpr double z_rafter = 990.0;

    std::array<std::shared_ptr<EdgeRawElement>, 2> girders;
    for (size_t g = 0; g < 2; g++) {
        const double y = g == 0 ? 0.0 : 1150.0;
        girders[g] = EdgeRawElement::from_stock(
            Line::from_points({-200.0, y, z_girder}, {1650.0, y, z_girder}),
            54.0,
            g == 0 ? "raw_log_0" : "raw_log_1",
            g == 0 ? 0.0 : 90.0,
            true,
            fmt::format("girder_{}", g)
        );
        girders[g]->cuts = {
            Plane::from_point_normal({-180.0, y, z_girder}, {1.0, 0.0, 0.0}),
            Plane::from_point_normal({1630.0, y, z_girder}, {-1.0, 0.0, 0.0}),
        };
    }

    struct ColumnSpec {
        double x;
        double y;
        double roll;
        const char* stock;
        size_t girder_idx;
    };
    const std::array<ColumnSpec, 4> specs = {{
        {0.0, 0.0, -90.0, "raw_fork_0", 0},
        {1450.0, 0.0, 90.0, "raw_fork_2", 0},
        {0.0, 1150.0, -90.0, "raw_fork_2", 1},
        {1450.0, 1150.0, 90.0, "raw_fork_0", 1},
    }};

    std::array<std::shared_ptr<EdgeRawElement>, 4> fork_cols;
    for (size_t i = 0; i < specs.size(); i++) {
        fork_cols[i] = EdgeRawElement::from_stock(
            Line::from_points({specs[i].x, specs[i].y, 0.0}, {specs[i].x, specs[i].y, z_girder}),
            58.0,
            specs[i].stock,
            specs[i].roll,
            false,
            fmt::format("fork_{}", i)
        );
        fork_cols[i]->cuts = {
            Plane::from_point_normal({specs[i].x, specs[i].y, 40.0}, {0.0, 0.0, 1.0}),
        };
    }

    const std::array<double, 3> rafter_x = {165.0, 725.0, 1285.0};
    std::array<std::shared_ptr<EdgeRawElement>, 3> rafters;
    for (size_t r = 0; r < rafter_x.size(); r++) {
        rafters[r] = EdgeRawElement::from_stock(
            Line::from_points({rafter_x[r], -180.0, z_rafter}, {rafter_x[r], 1330.0, z_rafter}),
            54.0,
            r % 2 == 0 ? "raw_log_1" : "raw_log_0",
            45.0 * static_cast<double>(r),
            true,
            fmt::format("rafter_{}", r)
        );
        rafters[r]->cuts = {
            Plane::from_point_normal({rafter_x[r], -160.0, z_rafter}, {0.0, 1.0, 0.0}),
            Plane::from_point_normal({rafter_x[r], 1310.0, z_rafter}, {0.0, -1.0, 0.0}),
        };
    }

    // Add timbers in sequential collision-free assembly order: girder_0 -> fork_0,1 -> rafter_0..2 -> girder_1 -> fork_2,3
    scene.add(girders[0], girders_group);
    scene.add(fork_cols[0], forks_group);
    scene.add(fork_cols[1], forks_group);
    for (const std::shared_ptr<EdgeRawElement>& rafter : rafters)
        scene.add(rafter, rafters_group);
    scene.add(girders[1], girders_group);
    scene.add(fork_cols[2], forks_group);
    scene.add(fork_cols[3], forks_group);

    // Join EVERY fork's vertical stem (dovetail T-bridle) AND inclined branch (oblique thrust mortise-and-tenon + cross-pin) into its girder
    for (size_t i = 0; i < specs.size(); i++) {
        const std::shared_ptr<EdgeRawElement>& fork = fork_cols[i];
        const std::shared_ptr<EdgeRawElement>& girder = girders[specs[i].girder_idx];

        const std::shared_ptr<JointBeamElement> stem_bridle = JointBeamElement::from_library(
            "dovetail_t_bridle", *fork, *girder, {{"slot", 0.48}, {"inside", 0.66}}
        );
        stem_bridle->name = fmt::format("fork_stem_bridle_{}", i);
        scene.add(stem_bridle, joints);
        scene.add_interaction(stem_bridle, fork, stem_bridle->interaction(0));
        scene.add_interaction(stem_bridle, girder, stem_bridle->interaction(1));

        // Project the fork's placed secondary branch skeleton onto the girder's vertical plane and intersect with z = z_girder
        const std::vector<Point> br_pts = fork->placed_branch_axes().back().get_points();
        const Point b0(br_pts.front()[0], specs[i].y, br_pts.front()[2]);
        Point b_hit(br_pts.back()[0], specs[i].y, z_girder);
        for (size_t s = 0; s + 1 < br_pts.size(); s++) {
            const double z0 = br_pts[s][2];
            const double z1 = br_pts[s + 1][2];
            if ((z0 <= z_girder && z_girder <= z1) || (z1 <= z_girder && z_girder <= z0)) {
                const double t = std::abs(z1 - z0) > 1e-6 ? (z_girder - z0) / (z1 - z0) : 0.0;
                b_hit = Point(br_pts[s][0] + t * (br_pts[s + 1][0] - br_pts[s][0]), specs[i].y, z_girder);
                break;
            }
        }
        const std::shared_ptr<EdgeRawElement> branch =
            fork->branch_proxy(44.0, Line::from_points(b0, b_hit));
        const std::shared_ptr<JointBeamElement> branch_joint = JointBeamElement::from_library(
            "oblique_thrust_mortise_tenon",
            *branch,
            *girder,
            {{"depth", 0.42}, {"thickness", 0.34}}
        );
        branch_joint->name = fmt::format("fork_branch_tenon_{}", i);
        scene.add(branch_joint, joints);
        scene.add_interaction(branch_joint, fork, branch_joint->interaction(0));
        scene.add_interaction(branch_joint, girder, branch_joint->interaction(1));

        // Transverse hardwood locking pin through the girder and fork-branch tenon
        const Vector br_u = (b0 - b_hit).normalized();
        const Point pin_c = b_hit + br_u * 30.0;
        const Line pin_axis = Line::from_points(
            {pin_c[0], specs[i].y - 62.0, pin_c[2]}, {pin_c[0], specs[i].y + 62.0, pin_c[2]}
        );
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(pin_axis, 8.5, 0.05);
        pin->name = fmt::format("fork_branch_pin_{}", i);
        scene.add(pin, joints);

        Line bore_axis = pin_axis;
        bore_axis.extend_equally(28.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<EdgeRawElement>& member = m == 0 ? fork : girder;
            const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 8.5);
            bore->name = fmt::format("fork_branch_bore_{}_{}", i, m);
            bore->is_visible = false;
            scene.add(bore, joints);
            scene.add_interaction(bore, member, bore->interaction(0));
        }
    }

    // Six eccentric conical cross-lap joints + pins connecting the three rafters across both girders
    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.68;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    for (size_t r = 0; r < rafters.size(); r++) {
        for (size_t g = 0; g < 2; g++) {
            const double y = g == 0 ? 0.0 : 1150.0;
            const InteractionContactAxis contact(
                Line::from_points({rafter_x[r], y, z_rafter}, {rafter_x[r], y, z_girder}),
                0.5,
                0.5,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> lap = JointBeamElement::from_contact(
                *rafters[r], *girders[g], contact, 250.0, 0.91, 1, conical_settings
            );
            lap->name = fmt::format("roof_conical_{}_{}", r, g);
            lap->line_radius = 9.0;
            lap->target_insertions = {{{0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}}};
            scene.add(lap, joints);
            scene.add_interaction(lap, rafters[r], lap->interaction(0));
            scene.add_interaction(lap, girders[g], lap->interaction(1));

            const std::shared_ptr<JointPinElement> pin = std::make_shared<JointPinElement>(
                Line::from_points({rafter_x[r], y, z_girder - 40.0}, {rafter_x[r], y, z_rafter + 42.0}),
                9.0,
                0.05
            );
            pin->name = fmt::format("roof_pin_{}_{}", r, g);
            scene.add(pin, joints);
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A spatial pavilion of Rossiniere raw timber with fourteen interlocking joints: four bifurcated spruce Y-columns join two longitudinal log girders through flared dovetail T-bridles on every vertical stem and pinned oblique-thrust mortise-and-tenon joints on every inclined fork branch, spanned above by three raw rafters seated in six eccentric conical cross-laps with wooden pins.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_session --parallel 6 && ./build/element_edge_raw_session && ../bash/publish-scene.sh --target element_edge_raw_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
