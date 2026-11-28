#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

/// Interlocking castellated longitudinal shear-cog seam milled between adjacent parallel raw logs (Rossiniere prototype & PhD Fig. 5.36).
std::shared_ptr<JointBeamElement> castellated_longitudinal_seam(
    const Element& left_log,
    const Element& right_log,
    double y_seam,
    double z_top,
    const std::string& name
) {
    constexpr double cog_half = 11.0;
    constexpr double clear_x = 0.35;
    constexpr double z_low = -40.0;
    constexpr double z_high = 30.0;
    constexpr double y_reach = 36.0;

    auto box_z = [&](double x0, double x1, double y0, double y1) {
        const Polyline low({{x0, y0, z_low}, {x1, y0, z_low}, {x1, y1, z_low}, {x0, y1, z_low}, {x0, y0, z_low}});
        const Polyline high({
            {x0, y0, z_top + z_high},
            {x1, y0, z_top + z_high},
            {x1, y1, z_top + z_high},
            {x0, y1, z_top + z_high},
            {x0, y0, z_top + z_high},
        });
        return Mesh::loft({low}, {high}, true);
    };

    // Five longitudinal zones: left_log projects two cogs on [-345,-115] and [115,345]; right_log projects three complementary cogs
    const std::vector<Mesh> cut_left = {
        box_z(-660.0, -345.0 + clear_x, y_seam - cog_half, y_seam + y_reach),
        box_z(-115.0 - clear_x, 115.0 + clear_x, y_seam - cog_half, y_seam + y_reach),
        box_z(345.0 - clear_x, 660.0, y_seam - cog_half, y_seam + y_reach),
    };
    const std::vector<Mesh> cut_right = {
        box_z(-345.0 - clear_x, -115.0 + clear_x, y_seam - y_reach, y_seam + cog_half),
        box_z(115.0 - clear_x, 345.0 + clear_x, y_seam - y_reach, y_seam + cog_half),
    };

    const std::shared_ptr<JointBeamElement> joint = std::make_shared<JointBeamElement>();
    joint->name = name;
    joint->is_visible = false;
    joint->targets = {left_log.guid(), right_log.guid()};
    joint->target_insertions = {{{0.0, 1.0, 0.0}}, {{0.0, -1.0, 0.0}}};

    JointCutStep step_a;
    step_a.label = "A2";
    step_a.target = 0;
    for (const Mesh& m : cut_left)
        step_a.removal = step_a.removal.number_of_faces() == 0 ? m : solid_boolean(step_a.removal, m, SolidOperation::add);

    JointCutStep step_b;
    step_b.label = "B2";
    step_b.target = 1;
    for (const Mesh& m : cut_right)
        step_b.removal = step_b.removal.number_of_faces() == 0 ? m : solid_boolean(step_b.removal, m, SolidOperation::add);

    joint->steps = {step_a, step_b};
    return joint;
}

/// A monolithic flared double-dovetail butterfly key inlaid flush into the flattened top face `z_top` across seam `y_seam` (PhD Fig. 5.37 A2-A3).
std::shared_ptr<JointBeamElement> dovetail_butterfly_key(
    const Element& left_log,
    const Element& right_log,
    double x_centre,
    double y_seam,
    double z_top,
    const std::string& name
) {
    constexpr double half_len = 38.0;
    constexpr double waist_half = 12.0;
    constexpr double head_half = 26.0;
    constexpr double depth = 26.0;
    constexpr double over = 14.0;
    constexpr double fit = 0.6;

    auto rect_ring = [&](double z) {
        return Polyline({
            {x_centre - head_half, y_seam - half_len, z},
            {x_centre + head_half, y_seam - half_len, z},
            {x_centre + head_half, y_seam + half_len, z},
            {x_centre - head_half, y_seam + half_len, z},
            {x_centre - head_half, y_seam - half_len, z},
        });
    };
    auto right_notch = [&](double z) {
        return Polyline({
            {x_centre + waist_half, y_seam, z},
            {x_centre + head_half, y_seam - half_len, z},
            {x_centre + head_half + 12.0, y_seam, z},
            {x_centre + head_half, y_seam + half_len, z},
            {x_centre + waist_half, y_seam, z},
        });
    };
    auto left_notch = [&](double z) {
        return Polyline({
            {x_centre - waist_half, y_seam, z},
            {x_centre - head_half, y_seam + half_len, z},
            {x_centre - head_half - 12.0, y_seam, z},
            {x_centre - head_half, y_seam - half_len, z},
            {x_centre - waist_half, y_seam, z},
        });
    };
    // Full hourglass socket carved out of both logs across the castellated seam (`y_seam +- 11 mm`)
    auto full_socket = [&](double z) {
        return Polyline({
            {x_centre - head_half - fit, y_seam - half_len - fit, z},
            {x_centre + head_half + fit, y_seam - half_len - fit, z},
            {x_centre + head_half + fit, y_seam + half_len + fit, z},
            {x_centre - head_half - fit, y_seam + half_len + fit, z},
            {x_centre - head_half - fit, y_seam - half_len - fit, z},
        });
    };
    auto socket_right_keep = [&](double z) {
        return Polyline({
            {x_centre + waist_half + fit, y_seam, z},
            {x_centre + head_half + fit, y_seam - half_len - fit, z},
            {x_centre + head_half + 14.0, y_seam, z},
            {x_centre + head_half + fit, y_seam + half_len + fit, z},
            {x_centre + waist_half + fit, y_seam, z},
        });
    };
    auto socket_left_keep = [&](double z) {
        return Polyline({
            {x_centre - waist_half - fit, y_seam, z},
            {x_centre - head_half - fit, y_seam + half_len + fit, z},
            {x_centre - head_half - 14.0, y_seam, z},
            {x_centre - head_half - fit, y_seam - half_len - fit, z},
            {x_centre - waist_half - fit, y_seam, z},
        });
    };

    const Mesh hourglass_pocket = solid_difference(
        Mesh::loft({full_socket(z_top - depth - fit)}, {full_socket(z_top + over)}, true),
        {
            Mesh::loft({socket_right_keep(z_top - depth - over)}, {socket_right_keep(z_top + 2.0 * over)}, true),
            Mesh::loft({socket_left_keep(z_top - depth - over)}, {socket_left_keep(z_top + 2.0 * over)}, true),
        }
    );

    const std::shared_ptr<JointBeamElement> joint = std::make_shared<JointBeamElement>();
    joint->name = name;
    joint->is_visible = false;
    joint->targets = {left_log.guid(), right_log.guid()};
    joint->target_insertions = {{{0.0, 1.0, 0.0}}, {{0.0, -1.0, 0.0}}, {{0.0, 0.0, -1.0}}};
    joint->parts = {{rect_ring(z_top - depth), rect_ring(z_top)}};
    joint->solid_features = {
        InteractionFeatureSolid(
            Mesh::loft({right_notch(z_top - depth - over)}, {right_notch(z_top + over)}, true),
            SolidOperation::subtract
        ),
        InteractionFeatureSolid(
            Mesh::loft({left_notch(z_top - depth - over)}, {left_notch(z_top + over)}, true),
            SolidOperation::subtract
        ),
    };

    JointCutStep left_step;
    left_step.label = "A2";
    left_step.target = 0;
    left_step.removal = hourglass_pocket;
    JointCutStep right_step;
    right_step.label = "B2";
    right_step.target = 1;
    right_step.removal = hourglass_pocket;
    joint->steps = {left_step, right_step};
    return joint;
}

} // namespace

/// Four parallel raw spruce trunks interlocked by castellated longitudinal shear-cog seams, butterfly keys, and dowels (PhD Ch. 5.5 & Rossiniere).
int main() {

    WoodSession scene("element_edge_raw_parallel");

    const std::shared_ptr<TreeNode> logs_group = scene.group_named("logs");
    const std::shared_ptr<TreeNode> seams_group = scene.group_named("castellated_seams");
    const std::shared_ptr<TreeNode> keys_group = scene.group_named("butterfly_keys");
    const std::shared_ptr<TreeNode> pins_group = scene.group_named("pins");

    constexpr size_t count = 4;
    constexpr double pitch = 98.0;
    constexpr double cog_half = 11.0;
    constexpr double z_axis = 70.0;
    constexpr double z_top = 110.0;

    const std::array<const char*, count> stocks = {"raw_log_2", "raw_log_3", "raw_log_0", "raw_log_1"};
    std::array<std::shared_ptr<EdgeRawElement>, count> logs;

    for (size_t i = 0; i < count; i++) {
        const double y = (static_cast<double>(i) - 1.5) * pitch;
        logs[i] = EdgeRawElement::from_stock(
            Line::from_points({-640.0, y, z_axis}, {640.0, y, z_axis}),
            60.0,
            stocks[i],
            45.0 * static_cast<double>(i),
            true,
            fmt::format("slab_log_{}", i)
        );
        // Surface top deck flat and trim inner cheeks at `y_seam +- cog_half` so adjacent logs overlap by 22 mm for castellated cogs
        logs[i]->cuts = {
            Plane::from_point_normal({-610.0, y, z_axis}, {1.0, 0.0, 0.0}),
            Plane::from_point_normal({610.0, y, z_axis}, {-1.0, 0.0, 0.0}),
            Plane::from_point_normal({0.0, y, z_top}, {0.0, 0.0, -1.0}),
        };
        if (i > 0)
            logs[i]->cuts.push_back(
                Plane::from_point_normal({0.0, y - 0.5 * pitch - cog_half, z_axis}, {0.0, 1.0, 0.0})
            );
        if (i + 1 < count)
            logs[i]->cuts.push_back(
                Plane::from_point_normal({0.0, y + 0.5 * pitch + cog_half, z_axis}, {0.0, -1.0, 0.0})
            );

        scene.add(logs[i], logs_group);
    }

    const std::array<double, 3> pin_stations = {-475.0, 0.0, 475.0};

    for (size_t i = 0; i + 1 < count; i++) {
        const double y_seam = (static_cast<double>(i) - 1.0) * pitch;

        const std::shared_ptr<JointBeamElement> seam = castellated_longitudinal_seam(
            *logs[i], *logs[i + 1], y_seam, z_top, fmt::format("seam_cogs_{}", i)
        );
        scene.add(seam, seams_group);
        scene.add_interaction(seam, logs[i], seam->interaction(0));
        scene.add_interaction(seam, logs[i + 1], seam->interaction(1));

        const std::array<double, 2> key_stations =
            i % 2 == 0 ? std::array<double, 2>{-230.0, 230.0} : std::array<double, 2>{-465.0, 465.0};
        for (size_t k = 0; k < key_stations.size(); k++) {
            const std::shared_ptr<JointBeamElement> key = dovetail_butterfly_key(
                *logs[i], *logs[i + 1], key_stations[k], y_seam, z_top, fmt::format("butterfly_{}_{}", i, k)
            );
            scene.add(key, keys_group);
            scene.add_interaction(key, logs[i], key->interaction(0));
            scene.add_interaction(key, logs[i + 1], key->interaction(1));
        }

        const std::array<double, 2> seam_pins =
            i % 2 == 0 ? std::array<double, 2>{-475.0, 475.0} : std::array<double, 2>{-230.0, 230.0};
        for (size_t p = 0; p < seam_pins.size(); p++) {
            const double x = seam_pins[p];
            const Line pin_axis = Line::from_points({x, y_seam - 42.0, 62.0}, {x, y_seam + 42.0, 62.0});
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 8.0, 0.05);
            pin->name = fmt::format("seam_pin_{}_{}", i, p);
            scene.add(pin, pins_group);

            Line bore_axis = pin_axis;
            bore_axis.extend_equally(18.0);
            for (size_t m = 0; m < 2; m++) {
                const std::shared_ptr<EdgeRawElement>& member = m == 0 ? logs[i] : logs[i + 1];
                const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 8.0);
                bore->name = fmt::format("seam_bore_{}_{}_{}", i, p, m);
                bore->is_visible = false;
                scene.add(bore, pins_group);
                scene.add_interaction(bore, member, bore->interaction(0));
            }
        }
    }
    (void)pin_stations;

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Side-to-side raw-wood slab joinery combining the Rossiniere stave prototype and Chapter 5.5 (Figures 5.36-5.37): four scanned spruce trunks are surfaced flat on top while keeping natural bark underneath and on the outer flanks, interlocked along every longitudinal seam by five alternating 22 mm castellated shear cogs, transverse cylindrical wooden dowels, and inlaid flared double-dovetail butterfly keys.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_parallel --parallel 6 && ./build/element_edge_raw_parallel && ../bash/publish-scene.sh --target element_edge_raw_parallel

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
