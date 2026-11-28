#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Interlocking roundwood carpentry on scanned spruce trunks: wedged tusk tenon, pegged half-dovetail thrust brace, and Jupiter scarf with folding wedges.
int main() {

    WoodSession scene("element_edge_raw_joints");

    // 1. Top-to-side interlocking frame: horizontally wedged tusk tenon post + pegged angled half-dovetail sloped-shoulder brace
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
        56.0,
        "raw_log_3",
        0.0,
        true,
        "tee_post"
    );
    post->cuts = {Plane::from_point_normal({-260.0, 0.0, 50.0}, {0.0, 0.0, 1.0})};

    const std::shared_ptr<EdgeRawElement> strut = EdgeRawElement::from_stock(
        Line::from_points({490.0, 0.0, 70.0}, {130.0, 0.0, 620.0}),
        55.0,
        "raw_log_2",
        40.0,
        true,
        "tee_strut"
    );
    strut->cuts = {
        Plane::from_point_normal({490.0, 0.0, 70.0}, Vector(-360.0, 0.0, 550.0).normalized())
    };
    scene.add(carrier, tee_group);
    scene.add(post, tee_group);
    scene.add(strut, tee_group);

    const std::shared_ptr<JointBeamElement> tusk = JointBeamElement::from_library(
        "horizontally_wedged_tusk_tenon",
        *post,
        *carrier,
        {{"protrude", 0.36}, {"key_height", 0.16}, {"key_wide", 0.24}, {"key_length", 1.38}}
    );
    tusk->name = "tee_tusk_tenon";
    scene.add(tusk, tee_group);
    scene.add_interaction(tusk, post, tusk->interaction(0));
    scene.add_interaction(tusk, carrier, tusk->interaction(1));

    const std::shared_ptr<JointBeamElement> brace_joint = JointBeamElement::from_library(
        "pegged_angled_stopped_dovetail_half_lap",
        *strut,
        *carrier,
        {{"flare", 0.28}, {"stop", 0.75}}
    );
    brace_joint->name = "tee_dovetail_brace";
    scene.add(brace_joint, tee_group);
    scene.add_interaction(brace_joint, strut, brace_joint->interaction(0));
    scene.add_interaction(brace_joint, carrier, brace_joint->interaction(1));

    // 2. End-to-end cogged hooked scarf (Trait de Jupiter) locked by full-height opposing folding wedges and two pins
    const std::shared_ptr<TreeNode> splice_group = scene.group_named("jupiter_splice");
    const Vector splice_shift(1260.0, -728.0, 0.0);
    const std::shared_ptr<EdgeRawElement> splice_left = EdgeRawElement::from_stock(
        Line::from_points(Point(-680.0, 0.0, 80.0) + splice_shift, Point(135.0, 0.0, 80.0) + splice_shift),
        58.0,
        "raw_log_2",
        0.0,
        true,
        "splice_left"
    );
    splice_left->cuts = {
        Plane::from_point_normal(Point(-660.0, 0.0, 80.0) + splice_shift, {1.0, 0.0, 0.0}),
        Plane::from_point_normal(Point(135.0, 0.0, 80.0) + splice_shift, {-1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> splice_right = EdgeRawElement::from_stock(
        Line::from_points(Point(-135.0, 0.0, 80.0) + splice_shift, Point(680.0, 0.0, 80.0) + splice_shift),
        58.0,
        "raw_log_3",
        60.0,
        true,
        "splice_right"
    );
    splice_right->cuts = {
        Plane::from_point_normal(Point(-135.0, 0.0, 80.0) + splice_shift, {1.0, 0.0, 0.0}),
        Plane::from_point_normal(Point(660.0, 0.0, 80.0) + splice_shift, {-1.0, 0.0, 0.0}),
    };

    scene.add(splice_left, splice_group);
    scene.add(splice_right, splice_group);

    const std::shared_ptr<JointBeamElement> jupiter = JointBeamElement::from_library(
        "cogged_lap_folding_wedges_splice",
        *splice_right,
        *splice_left,
        {{"hook", 0.28}, {"gap", 0.16}, {"over", 26.0}}
    );
    jupiter->name = "jupiter_scarf";
    scene.add(jupiter, splice_group);
    scene.add_interaction(jupiter, splice_right, jupiter->interaction(0));
    scene.add_interaction(jupiter, splice_left, jupiter->interaction(1));

    for (size_t p = 0; p < 2; p++) {
        const double x = p == 0 ? -84.0 : 84.0;
        const Line pin_axis = Line::from_points(
            Point(x, 0.0, 28.0) + splice_shift, Point(x, 0.0, 132.0) + splice_shift
        );
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(pin_axis, 8.5, 0.05);
        pin->name = fmt::format("splice_pin_{}", p);
        scene.add(pin, splice_group);

        Line bore_axis = pin_axis;
        bore_axis.extend_equally(35.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<EdgeRawElement>& member = m == 0 ? splice_left : splice_right;
            const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 8.5);
            bore->name = fmt::format("splice_bore_{}_{}", p, m);
            bore->is_visible = false;
            scene.add(bore, splice_group);
            scene.add_interaction(bore, member, bore->interaction(0));
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Interlocking roundwood joinery on 3D-scanned spruce trunks: a horizontally wedged through tusk tenon post and an inclined pegged half-dovetail sloped-shoulder thrust strut landing on one raw carrier (left), beside a colinear Trait de Jupiter undersquinted hooked scarf locked by twin opposing folding wedges and two cylindrical pins (right).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_joints --parallel 6 && ./build/element_edge_raw_joints && ../bash/publish-scene.sh --target element_edge_raw_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
