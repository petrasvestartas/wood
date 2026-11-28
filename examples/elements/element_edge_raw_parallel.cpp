#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

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
    constexpr double over = 6.0;

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
    auto left_pocket = [&](double z) {
        return Polyline({
            {x_centre - head_half - fit, y_seam - half_len - fit, z},
            {x_centre + head_half + fit, y_seam - half_len - fit, z},
            {x_centre + waist_half + fit, y_seam + 2.0, z},
            {x_centre - waist_half - fit, y_seam + 2.0, z},
            {x_centre - head_half - fit, y_seam - half_len - fit, z},
        });
    };
    auto right_pocket = [&](double z) {
        return Polyline({
            {x_centre - waist_half - fit, y_seam - 2.0, z},
            {x_centre + waist_half + fit, y_seam - 2.0, z},
            {x_centre + head_half + fit, y_seam + half_len + fit, z},
            {x_centre - head_half - fit, y_seam + half_len + fit, z},
            {x_centre - waist_half - fit, y_seam - 2.0, z},
        });
    };

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
    left_step.removal = Mesh::loft({left_pocket(z_top - depth - fit)}, {left_pocket(z_top + over)}, true);
    JointCutStep right_step;
    right_step.label = "B2";
    right_step.target = 1;
    right_step.removal = Mesh::loft({right_pocket(z_top - depth - fit)}, {right_pocket(z_top + over)}, true);
    joint->steps = {left_step, right_step};
    return joint;
}

} // namespace

/// Four parallel raw spruce trunks flattened on three faces and locked side-to-side by flared butterfly keys and dowels (PhD Fig. 5.37 A1-A3).
int main() {

    WoodSession scene("element_edge_raw_parallel");

    const std::shared_ptr<TreeNode> logs_group = scene.group_named("logs");
    const std::shared_ptr<TreeNode> keys_group = scene.group_named("butterfly_keys");
    const std::shared_ptr<TreeNode> pins_group = scene.group_named("pins");

    constexpr size_t count = 4;
    constexpr double pitch = 104.0;
    constexpr double z_axis = 70.0;
    constexpr double z_top = 112.0;

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
        // Trim ends plumb and surface the top face flat while keeping natural bark underneath and on the outer flanks (PhD Fig. 5.37 A1)
        logs[i]->cuts = {
            Plane::from_point_normal({-610.0, y, z_axis}, {1.0, 0.0, 0.0}),
            Plane::from_point_normal({610.0, y, z_axis}, {-1.0, 0.0, 0.0}),
            Plane::from_point_normal({0.0, y, z_top}, {0.0, 0.0, -1.0}),
        };
        if (i > 0)
            logs[i]->cuts.push_back(Plane::from_point_normal({0.0, y - 0.5 * pitch, z_axis}, {0.0, 1.0, 0.0}));
        if (i + 1 < count)
            logs[i]->cuts.push_back(Plane::from_point_normal({0.0, y + 0.5 * pitch, z_axis}, {0.0, -1.0, 0.0}));

        scene.add(logs[i], logs_group);
    }

    const std::array<double, 3> pin_stations = {-460.0, 0.0, 460.0};

    for (size_t i = 0; i + 1 < count; i++) {
        const double y_seam = (static_cast<double>(i) - 1.0) * pitch;
        const std::array<double, 2> key_stations =
            i % 2 == 0 ? std::array<double, 2>{-290.0, 290.0} : std::array<double, 2>{-165.0, 165.0};

        for (size_t k = 0; k < key_stations.size(); k++) {
            const std::shared_ptr<JointBeamElement> key = dovetail_butterfly_key(
                *logs[i], *logs[i + 1], key_stations[k], y_seam, z_top, fmt::format("butterfly_{}_{}", i, k)
            );
            scene.add(key, keys_group);
            scene.add_interaction(key, logs[i], key->interaction(0));
            scene.add_interaction(key, logs[i + 1], key->interaction(1));
        }

        for (size_t p = 0; p < pin_stations.size(); p++) {
            const double x = pin_stations[p];
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

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Side-to-side raw-wood slab joinery from Chapter 5.5 (Figure 5.37 A1-A3): four scanned spruce trunks are surfaced flat on top and along their longitudinal contact seams while preserving natural round bark underneath and on the outer edges, then locked across every seam by inlaid flared double-dovetail butterfly keys and transverse cylindrical wooden dowels.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_parallel --parallel 6 && ./build/element_edge_raw_parallel && ../bash/publish-scene.sh --target element_edge_raw_parallel

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
