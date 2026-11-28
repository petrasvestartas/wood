#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Six-log raw-wood reciprocal nexorade assembled rotationally via conical cross-lap joints and pins (PhD Fig. 5.60 C, Fig. 9.16 & Fig. A.9).
int main() {

    WoodSession scene("element_edge_raw_nexorade");

    const std::shared_ptr<TreeNode> spines_group = scene.group_named("spines");
    const std::shared_ptr<TreeNode> rungs_group = scene.group_named("rungs");
    const std::shared_ptr<TreeNode> joints_group = scene.group_named("joints");

    // Two longitudinal raw spruce trunks (D = 112 mm) spaced 580 mm apart, placed tip-to-butt
    const std::shared_ptr<EdgeRawElement> spine_0 = EdgeRawElement::from_stock(
        Line::from_points({-890.0, -290.0, 140.0}, {890.0, -290.0, 140.0}),
        58.0,
        "raw_log_0",
        0.0,
        true,
        "spine_0"
    );
    const std::shared_ptr<EdgeRawElement> spine_1 = EdgeRawElement::from_stock(
        Line::from_points({890.0, 290.0, 140.0}, {-890.0, 290.0, 140.0}),
        58.0,
        "raw_log_0",
        0.0,
        true,
        "spine_1"
    );
    scene.add(spine_0, spines_group);
    scene.add(spine_1, spines_group);

    // Four transverse raw logs alternating over/under around three cyclic reciprocal windows (dz = +-54 mm < 1/3 D cut)
    struct RungSpec {
        double x;
        const char* stock;
        double roll;
        bool over_front;
    };
    const std::array<RungSpec, 4> rung_specs = {{
        {-570.0, "raw_log_2", 15.0, true},
        {-190.0, "raw_log_3", 65.0, false},
        {190.0, "raw_log_2", 130.0, true},
        {570.0, "raw_log_3", 65.0, false},
    }};

    Settings conical_settings = scene.settings;
    conical_settings.joint_parameters[10] = 0.55;
    conical_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);

    for (size_t r = 0; r < rung_specs.size(); r++) {
        const double z_front = rung_specs[r].over_front ? 194.0 : 86.0;
        const double z_back = rung_specs[r].over_front ? 86.0 : 194.0;
        const double slope = (z_back - z_front) / 580.0;
        const double z0 = z_front - slope * 160.0;
        const double z1 = z_back + slope * 160.0;

        const std::shared_ptr<EdgeRawElement> rung = EdgeRawElement::from_stock(
            Line::from_points({rung_specs[r].x, -450.0, z0}, {rung_specs[r].x, 450.0, z1}),
            58.0,
            rung_specs[r].stock,
            rung_specs[r].roll,
            true,
            fmt::format("rung_{}", r)
        );
        scene.add(rung, rungs_group);

        // Rotational seating move about the rung midpoint (PhD Fig. 5.60 C):
        const Line pivot = Line::from_points({rung_specs[r].x, 0.0, 140.0}, {rung_specs[r].x + 1.0, 0.0, 140.0});
        const double seat_turn = rung_specs[r].over_front ? 30.0 : -30.0;
        const double total_tilt = 40.5 * Tolerance::TO_RADIANS;
        const Vector slide_in = rung_specs[r].over_front
                                    ? Vector(0.0, std::cos(total_tilt), -std::sin(total_tilt))
                                    : Vector(0.0, -std::cos(total_tilt), -std::sin(total_tilt));

        for (size_t s = 0; s < 2; s++) {
            const std::shared_ptr<EdgeRawElement>& spine = s == 0 ? spine_0 : spine_1;
            const double y = s == 0 ? -290.0 : 290.0;
            const double z_r = s == 0 ? z_front : z_back;
            const bool rung_over = z_r > 140.0;
            const std::shared_ptr<EdgeRawElement>& upper = rung_over ? rung : spine;
            const std::shared_ptr<EdgeRawElement>& lower = rung_over ? spine : rung;

            const InteractionContactAxis contact(
                Line::from_points({rung_specs[r].x, y, std::max(z_r, 140.0)}, {rung_specs[r].x, y, std::min(z_r, 140.0)}),
                0.5,
                0.5,
                0,
                0,
                0,
                0
            );
            const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_contact(
                *upper, *lower, contact, 235.0, 0.91, 1, conical_settings
            );
            joint->name = fmt::format("conical_cross_{}_{}", r, s);
            joint->line_radius = 8.5;

            const size_t rung_idx = rung_over ? 0 : 1;
            const size_t spine_idx = 1 - rung_idx;
            joint->target_insertions = {{}, {}};
            joint->target_legs = {{}, {}};
            joint->target_insertions[rung_idx] = {slide_in, Vector(0.0, 0.0, rung_over ? -1.0 : 1.0)};
            joint->target_legs[rung_idx] = {{JointMove::turn(pivot, seat_turn)}, {}};
            joint->target_insertions[spine_idx] = {Vector(0.0, 0.0, rung_over ? 1.0 : -1.0)};
            joint->target_legs[spine_idx] = {{}};

            scene.add(joint, joints_group);
            scene.add_interaction(joint, upper, joint->interaction(0));
            scene.add_interaction(joint, lower, joint->interaction(1));

            const std::shared_ptr<JointPinElement> pin = std::make_shared<JointPinElement>(
                Line::from_points({rung_specs[r].x, y, 62.0}, {rung_specs[r].x, y, 218.0}), 8.5, 0.05
            );
            pin->name = fmt::format("conical_pin_{}_{}", r, s);
            scene.add(pin, joints_group);
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A six-log raw-wood reciprocal nexorade reconstructing the prototype of Chapter 5.5 (Figure 5.60 B-C) and Appendix A.4 (Figure A.9): four transverse spruce logs weave alternately over and under two longitudinal trunks around three cyclic windows, joined at all eight eccentric crossings (dz = 54 mm) by conical cross-lap joints that allow collision-free rotational assembly before locking with cylindrical wooden pins.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_nexorade --parallel 6 && ./build/element_edge_raw_nexorade && ../bash/publish-scene.sh --target element_edge_raw_nexorade

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
