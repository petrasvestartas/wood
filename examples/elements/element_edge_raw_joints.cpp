#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Top-to-side post and inclined brace joints and an end-to-end plain scarf splice on scanned spruce trunks.
int main() {

    WoodSession scene("element_edge_raw_joints");

    // 1. Top-to-side joints: a square mortise-and-tenon post and an inclined thrust strut on one raw carrier
    const std::shared_ptr<TreeNode> tee_group = scene.group_named("top_side");
    const std::shared_ptr<EdgeRawElement> carrier = EdgeRawElement::from_stock(
        Line::from_points({-640.0, 0.0, 620.0}, {640.0, 0.0, 620.0}),
        54.0,
        "raw_log_0",
        180.0,
        true,
        "tee_carrier"
    );
    carrier->cuts = {
        Plane::from_point_normal({-620.0, 0.0, 620.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({620.0, 0.0, 620.0}, {-1.0, 0.0, 0.0}),
    };
    const std::shared_ptr<EdgeRawElement> post = EdgeRawElement::from_stock(
        Line::from_points({-260.0, 0.0, 40.0}, {-260.0, 0.0, 620.0}),
        58.0,
        "raw_log_3",
        0.0,
        true,
        "tee_post"
    );
    post->cuts = {Plane::from_point_normal({-260.0, 0.0, 50.0}, {0.0, 0.0, 1.0})};

    const std::shared_ptr<EdgeRawElement> strut = EdgeRawElement::from_stock(
        Line::from_points({20.0, 0.0, 70.0}, {380.0, 0.0, 620.0}),
        55.0,
        "raw_log_2",
        40.0,
        true,
        "tee_strut"
    );
    strut->cuts = {
        Plane::from_point_normal({20.0, 0.0, 70.0}, Vector(360.0, 0.0, 550.0).normalized())
    };
    scene.add(carrier, tee_group);
    scene.add(post, tee_group);
    scene.add(strut, tee_group);

    const std::shared_ptr<JointBeamElement> mortise = JointBeamElement::from_library(
        "fully_shouldered_blind_mortise_tenon", *post, *carrier, {{"depth", 0.45}}
    );
    mortise->name = "tee_mortise";
    scene.add(mortise, tee_group);
    scene.add_interaction(mortise, post, mortise->interaction(0));
    scene.add_interaction(mortise, carrier, mortise->interaction(1));

    const std::shared_ptr<JointBeamElement> thrust =
        JointBeamElement::from_library("notched_oblique_thrust", *strut, *carrier);
    thrust->name = "tee_thrust";
    scene.add(thrust, tee_group);
    scene.add_interaction(thrust, strut, thrust->interaction(0));
    scene.add_interaction(thrust, carrier, thrust->interaction(1));

    // 2. End-to-end plain scarf splice locked by two inclined wooden pins (PhD Fig. 5.28 & Fig. A.7 B)
    const std::shared_ptr<TreeNode> splice_group = scene.group_named("scarf_splice");
    const Vector splice_shift(1260.0, -728.0, 0.0);
    const std::shared_ptr<EdgeRawElement> splice_left = EdgeRawElement::from_stock(
        Line::from_points(Point(-660.0, 0.0, 80.0) + splice_shift, Point(140.0, 0.0, 80.0) + splice_shift),
        58.0,
        "raw_log_2",
        0.0,
        true,
        "splice_left"
    );
    splice_left->cuts = {Plane::from_point_normal(Point(140.0, 0.0, 80.0) + splice_shift, {-1.0, 0.0, 0.0})};

    const std::shared_ptr<EdgeRawElement> splice_right = EdgeRawElement::from_stock(
        Line::from_points(Point(-140.0, 0.0, 80.0) + splice_shift, Point(680.0, 0.0, 80.0) + splice_shift),
        58.0,
        "raw_log_3",
        60.0,
        true,
        "splice_right"
    );
    splice_right->cuts = {Plane::from_point_normal(Point(-140.0, 0.0, 80.0) + splice_shift, {1.0, 0.0, 0.0})};

    scene.add(splice_left, splice_group);
    scene.add(splice_right, splice_group);

    const std::shared_ptr<JointBeamElement> scarf =
        JointBeamElement::from_library("half_lap_splice", *splice_left, *splice_right);
    scarf->name = "plain_scarf";
    scene.add(scarf, splice_group);
    scene.add_interaction(scarf, splice_left, scarf->interaction(0));
    scene.add_interaction(scarf, splice_right, scarf->interaction(1));

    for (size_t p = 0; p < 2; p++) {
        const double x = p == 0 ? -55.0 : 55.0;
        const Line pin_axis = Line::from_points(
            Point(x - 10.0, 0.0, 128.0) + splice_shift, Point(x + 10.0, 0.0, 32.0) + splice_shift
        );
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(pin_axis, 9.0, 0.05);
        pin->name = fmt::format("splice_pin_{}", p);
        scene.add(pin, splice_group);

        Line bore_axis = pin_axis;
        bore_axis.extend_equally(30.0);
        const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 9.0);
        bore->name = fmt::format("splice_bore_{}", p);
        bore->is_visible = false;
        bore->target_insertions = {{{0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}}};
        scene.add(bore, splice_group);
        scene.add_interaction(bore, splice_left, bore->interaction(0));
        scene.add_interaction(bore, splice_right, bore->interaction(0));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Top-to-side and end-to-end joinery on scanned spruce trunks: a blind mortise-and-tenon post and an inclined notched oblique thrust strut landing on one raw carrier (left), beside a colinear plain half-lap scarf splice locked by two inclined cylindrical pins (right).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_joints --parallel 6 && ./build/element_edge_raw_joints && ../bash/publish-scene.sh --target element_edge_raw_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
