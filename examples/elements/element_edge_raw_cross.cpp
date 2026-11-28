#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Six-cut extended and nine-cut sliced conical cross-lap joints on eccentric raw spruce trunks (PhD Ch. 5.5 & Ch. 9.4).
int main() {

    WoodSession scene("element_edge_raw_cross");

    // 1. Extended six-cut conical cross-lap with central pin at 60 degrees (PhD Fig. 5.55 B, Fig. 5.63 I & Fig. 9.16)
    // Lower trunk aligned across screen so its flared conical saddle and side cheeks open directly towards the viewer
    const std::shared_ptr<TreeNode> extended_group = scene.group_named("extended_conical");
    const std::shared_ptr<EdgeRawElement> ext_lower = EdgeRawElement::from_stock(
        Line::from_points({-346.0, 200.0, 62.0}, {346.0, -200.0, 62.0}), 64.0, "raw_log_0", 0.0, true, "ext_lower"
    );
    ext_lower->cuts = {
        Plane::from_point_normal({-338.0, 195.0, 62.0}, Vector(346.0, -200.0, 0.0).normalized()),
        Plane::from_point_normal({338.0, -195.0, 62.0}, Vector(-346.0, 200.0, 0.0).normalized()),
    };
    const std::shared_ptr<EdgeRawElement> ext_upper = EdgeRawElement::from_stock(
        Line::from_points({-346.0, -200.0, 118.0}, {346.0, 200.0, 118.0}), 64.0, "raw_log_2", 15.0, true, "ext_upper"
    );
    ext_upper->cuts = {
        Plane::from_point_normal({-338.0, -195.0, 118.0}, Vector(346.0, 200.0, 0.0).normalized()),
        Plane::from_point_normal({338.0, 195.0, 118.0}, Vector(-346.0, -200.0, 0.0).normalized()),
    };
    scene.add(ext_lower, extended_group);
    scene.add(ext_upper, extended_group);

    Settings ext_settings = scene.settings;
    ext_settings.joint_parameters[10] = 0.72;
    ext_settings.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);
    const InteractionContactAxis ext_contact(
        Line::from_points({0.0, 0.0, 62.0}, {0.0, 0.0, 118.0}), 0.5, 0.5, 0, 0, 0, 0
    );
    const std::shared_ptr<JointBeamElement> ext_joint = JointBeamElement::from_contact(
        *ext_lower, *ext_upper, ext_contact, 320.0, 0.91, 1, ext_settings
    );
    ext_joint->name = "extended_conical_joint";
    ext_joint->line_radius = 10.0;
    ext_joint->target_insertions = {{{0.0, 0.0, 1.0}}, {{0.0, 0.0, -1.0}}};
    scene.add(ext_joint, extended_group);
    scene.add_interaction(ext_joint, ext_lower, ext_joint->interaction(0));
    scene.add_interaction(ext_joint, ext_upper, ext_joint->interaction(1));

    const std::shared_ptr<JointPinElement> ext_pin = std::make_shared<JointPinElement>(
        Line::from_points({0.0, 0.0, 14.0}, {0.0, 0.0, 166.0}), 10.0, 0.05
    );
    ext_pin->name = "ext_pin";
    scene.add(ext_pin, extended_group);

    // 2. Nine-cut sliced conical cross-lap at 60 degrees with two diagonal pins (PhD Fig. 5.55 C & Fig. 5.57 B)
    const std::shared_ptr<TreeNode> sliced_group = scene.group_named("sliced_conical");
    const Vector shift(960.0, -554.0, 0.0);
    const std::shared_ptr<EdgeRawElement> slc_lower = EdgeRawElement::from_stock(
        Line::from_points(Point(-346.0, 200.0, 62.0) + shift, Point(346.0, -200.0, 62.0) + shift),
        64.0,
        "raw_log_2",
        95.0,
        true,
        "slc_lower"
    );
    slc_lower->cuts = {
        Plane::from_point_normal(Point(-338.0, 195.0, 62.0) + shift, Vector(346.0, -200.0, 0.0).normalized()),
        Plane::from_point_normal(Point(338.0, -195.0, 62.0) + shift, Vector(-346.0, 200.0, 0.0).normalized()),
    };
    const std::shared_ptr<EdgeRawElement> slc_upper = EdgeRawElement::from_stock(
        Line::from_points(Point(-346.0, -200.0, 118.0) + shift, Point(346.0, 200.0, 118.0) + shift),
        64.0,
        "raw_log_0",
        180.0,
        true,
        "slc_upper"
    );
    slc_upper->cuts = {
        Plane::from_point_normal(Point(-338.0, -195.0, 118.0) + shift, Vector(346.0, 200.0, 0.0).normalized()),
        Plane::from_point_normal(Point(338.0, 195.0, 118.0) + shift, Vector(-346.0, -200.0, 0.0).normalized()),
    };
    scene.add(slc_lower, sliced_group);
    scene.add(slc_upper, sliced_group);

    Settings slc_settings = scene.settings;
    slc_settings.joint_parameters[10] = 0.72;
    slc_settings.joint_parameters[11] = JointPlateElement::library_id("cross_sliced_half_lap");
    const InteractionContactAxis slc_contact(
        Line::from_points(Point(0.0, 0.0, 62.0) + shift, Point(0.0, 0.0, 118.0) + shift),
        0.5,
        0.5,
        0,
        0,
        0,
        0
    );
    const std::shared_ptr<JointBeamElement> slc_joint = JointBeamElement::from_contact(
        *slc_lower, *slc_upper, slc_contact, 320.0, 0.91, 1, slc_settings
    );
    slc_joint->name = "sliced_conical_joint";
    slc_joint->target_insertions = {{{0.0, 0.0, 1.0}}, {{0.0, 0.0, -1.0}}};
    scene.add(slc_joint, sliced_group);
    scene.add_interaction(slc_joint, slc_lower, slc_joint->interaction(0));
    scene.add_interaction(slc_joint, slc_upper, slc_joint->interaction(1));

    for (int side : {-1, 1}) {
        const Line diag_axis = Line::from_points(
            Point(22.0 * side, -20.0 * side, 16.0) + shift,
            Point(-22.0 * side, 20.0 * side, 164.0) + shift
        );
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(diag_axis, 8.0, 0.05);
        pin->name = side < 0 ? "slc_pin_0" : "slc_pin_1";
        scene.add(pin, sliced_group);

        Line bore_axis = diag_axis;
        bore_axis.extend_equally(35.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<EdgeRawElement>& member = m == 0 ? slc_lower : slc_upper;
            const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 8.0);
            bore->name = fmt::format("slc_bore_{}_{}", side < 0 ? 0 : 1, m);
            bore->is_visible = false;
            scene.add(bore, sliced_group);
            scene.add_interaction(bore, member, bore->interaction(0));
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The two raw-wood conical cross-lap topologies from Chapter 5.5 and Chapter 9.4 of the PhD thesis side by side at 60 degrees on eccentric spruce trunks (dz = 52 mm on D = 120 mm, preserving over two-thirds of each timber section): the six-cut extended conical cross-lap with a central cylindrical pin (left), and the nine-cut sliced conical cross-lap locked by two diagonal pins (right), with conical side cheeks swept outward clear of the irregular bark.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_cross --parallel 6 && ./build/element_edge_raw_cross && ../bash/publish-scene.sh --target element_edge_raw_cross

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
