#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A four-fork Y-column pavilion carrying two raw girders and three rafters joined by conical cross-laps.
int main() {

    WoodSession scene("element_edge_raw_session");

    const std::shared_ptr<TreeNode> forks = scene.group_named("forks");
    const std::shared_ptr<TreeNode> girders_group = scene.group_named("girders");
    const std::shared_ptr<TreeNode> rafters_group = scene.group_named("rafters");
    const std::shared_ptr<TreeNode> joints = scene.group_named("joints");

    struct ColumnSpec {
        double x;
        double y;
        double roll;
        const char* stock;
    };
    const std::array<ColumnSpec, 4> specs = {{
        {0.0, 0.0, -90.0, "raw_fork_0"},
        {1450.0, 0.0, 90.0, "raw_fork_2"},
        {0.0, 1150.0, -90.0, "raw_fork_2"},
        {1450.0, 1150.0, 90.0, "raw_fork_0"},
    }};
    for (size_t i = 0; i < specs.size(); i++) {
        const std::shared_ptr<EdgeRawElement> fork = EdgeRawElement::from_stock(
            Line::from_points({specs[i].x, specs[i].y, 0.0}, {specs[i].x, specs[i].y, 1160.0}),
            65.0,
            specs[i].stock,
            specs[i].roll,
            false,
            fmt::format("fork_{}", i)
        );
        fork->cuts = {
            Plane::from_point_normal({specs[i].x, specs[i].y, 40.0}, {0.0, 0.0, 1.0}),
            Plane::from_point_normal({specs[i].x, specs[i].y, 960.0}, {0.0, 0.0, -1.0}),
        };
        scene.add(fork, forks);
    }

    std::array<std::shared_ptr<EdgeRawElement>, 2> girders;
    for (size_t g = 0; g < 2; g++) {
        const double y = g == 0 ? 0.0 : 1150.0;
        girders[g] = EdgeRawElement::from_stock(
            Line::from_points({-200.0, y, 1015.0}, {1650.0, y, 1015.0}),
            55.0,
            g == 0 ? "raw_log_0" : "raw_log_1",
            g == 0 ? 0.0 : 90.0,
            true,
            fmt::format("girder_{}", g)
        );
        scene.add(girders[g], girders_group);
    }

    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.55;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    const std::array<double, 3> rafter_x = {220.0, 725.0, 1230.0};
    for (size_t r = 0; r < rafter_x.size(); r++) {
        const std::shared_ptr<EdgeRawElement> rafter = EdgeRawElement::from_stock(
            Line::from_points({rafter_x[r], -180.0, 1067.0}, {rafter_x[r], 1330.0, 1067.0}),
            55.0,
            r % 2 == 0 ? "raw_log_1" : "raw_log_0",
            45.0 * static_cast<double>(r),
            true,
            fmt::format("rafter_{}", r)
        );
        rafter->cuts = {
            Plane::from_point_normal({rafter_x[r], -160.0, 1067.0}, {0.0, 1.0, 0.0}),
            Plane::from_point_normal({rafter_x[r], 1310.0, 1067.0}, {0.0, -1.0, 0.0}),
        };
        scene.add(rafter, rafters_group);

        for (size_t g = 0; g < 2; g++) {
            const double y = g == 0 ? 0.0 : 1150.0;
            const InteractionContactAxis contact(
                Line::from_points({rafter_x[r], y, 1067.0}, {rafter_x[r], y, 1015.0}),
                0.5,
                0.5,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> lap = JointBeamElement::from_contact(
                *rafter, *girders[g], contact, 250.0, 0.91, 1, conical_settings
            );
            lap->name = fmt::format("roof_conical_{}_{}", r, g);
            lap->line_radius = 9.0;
            lap->target_insertions = {{{0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}}};
            scene.add(lap, joints);
            scene.add_interaction(lap, rafter, lap->interaction(0));
            scene.add_interaction(lap, girders[g], lap->interaction(1));

            const std::shared_ptr<JointPinElement> pin = std::make_shared<JointPinElement>(
                Line::from_points({rafter_x[r], y, 976.0}, {rafter_x[r], y, 1108.0}), 9.0, 0.05
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
A spatial pavilion of Rossiniere raw timber: four bifurcated spruce forks turned inward so both stem and branch cradle two longitudinal log girders, spanned across by three raw rafters seated in eccentric conical cross-laps and pinned onto the girders.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_session --parallel 6 && ./build/element_edge_raw_session && ../bash/publish-scene.sh --target element_edge_raw_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
