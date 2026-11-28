#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace {

/// Orthonormal rigid frame placing a 3D-scanned Rossiniere trunk or fork into the triangular truss.
Xform stock_frame(const Point& origin, const Vector& col_x, const Vector& col_y) {

    const Vector x = col_x.normalized();
    const Vector y = (col_y - x * col_y.dot(x)).normalized();

    return Xform::frame_to_world(origin, x, y, x.cross(y));
}

} // namespace

/// Seven-member Rossiniere triangular tree-fork space truss: hooked splayed scarf splices with folding wedges & pins, plus pegged dovetail fork-branch joints.
int main() {

    WoodSession scene("element_edge_raw_truss");

    const std::shared_ptr<TreeNode> chords = scene.group_named("chords");
    const std::shared_ptr<TreeNode> joints = scene.group_named("joints");

    // Chord 1 (front-bottom): fork_0 spliced into log_0, fork_0's side branch triangulating into Chord 2
    const std::shared_ptr<EdgeRawElement> fork_0 = EdgeRawElement::from_registered_stock(
        "raw_fork_0",
        stock_frame({-77.5931, -195.8461, 81.3460}, {-0.0295057, 0.9991780, -0.0277991}, {-0.0073916, 0.0275924, 0.9995919}),
        60.0,
        Line::from_points({0.0, -177.0, 78.0}, {900.0, -177.0, 78.0}),
        "chord_1_fork_0"
    );
    fork_0->cuts = {
        Plane::from_point_normal({20.0, -177.0, 78.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({900.0, -177.0, 78.0}, {-1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> log_0 = EdgeRawElement::from_registered_stock(
        "raw_log_0",
        stock_frame({603.2371, -177.7976, 76.1424}, {-0.0008836, 0.0, 0.9999996}, {-0.0000466, -1.0, 0.0}),
        55.0,
        Line::from_points({680.0, -177.0, 78.0}, {2020.0, -177.0, 78.0}),
        "chord_1_log_0"
    );
    log_0->cuts = {
        Plane::from_point_normal({680.0, -177.0, 78.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({2000.0, -177.0, 78.0}, {-1.0, 0.0, 0.0}),
    };

    // Chord 2 (back-bottom): log_1 spliced into fork_1 on the straight butt stem before fork_1's bifurcation
    const std::shared_ptr<EdgeRawElement> log_1 = EdgeRawElement::from_registered_stock(
        "raw_log_1",
        stock_frame({-197.1748, 176.6692, 78.0}, {-0.0104344, -0.0001145, 0.9999456}, {0.0109748, -0.9999398, 0.0}),
        55.0,
        Line::from_points({0.0, 177.0, 78.0}, {1380.0, 177.0, 78.0}),
        "chord_2_log_1"
    );
    log_1->cuts = {
        Plane::from_point_normal({20.0, 177.0, 78.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({1380.0, 177.0, 78.0}, {-1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> fork_1 = EdgeRawElement::from_registered_stock(
        "raw_fork_1",
        stock_frame({1132.3712, 187.9836, 55.3259}, {0.0026706, -0.4563123, 0.8898157}, {-0.0230663, -0.8896102, -0.4561377}),
        58.0,
        Line::from_points({1140.0, 177.0, 78.0}, {2020.0, 177.0, 78.0}),
        "chord_2_fork_1"
    );
    fork_1->cuts = {
        Plane::from_point_normal({1140.0, 177.0, 78.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({2000.0, 177.0, 78.0}, {-1.0, 0.0, 0.0}),
    };

    // Chord 3 (apex): log_3 spliced into fork_2 and fork_2 into log_2, fork_2's branch descending back to Chord 1
    const std::shared_ptr<EdgeRawElement> log_3 = EdgeRawElement::from_registered_stock(
        "raw_log_3",
        stock_frame({-98.0504, -1.6456, 380.7735}, {-0.0088630, -0.0000042, 0.9999607}, {0.0004719, -0.9999999, 0.0}),
        62.0,
        Line::from_points({0.0, 0.0, 384.0}, {780.0, 0.0, 384.0}),
        "chord_3_log_3"
    );
    log_3->cuts = {
        Plane::from_point_normal({20.0, 0.0, 384.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({780.0, 0.0, 384.0}, {-1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> fork_2 = EdgeRawElement::from_registered_stock(
        "raw_fork_2",
        stock_frame({394.0044, 13.9106, 393.2541}, {-0.0065739, -0.4424505, -0.8967688}, {0.0233693, 0.8964753, -0.4424770}),
        64.0,
        Line::from_points({460.0, 0.0, 384.0}, {1540.0, 0.0, 384.0}),
        "chord_3_fork_2"
    );
    fork_2->cuts = {
        Plane::from_point_normal({460.0, 0.0, 384.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({1540.0, 0.0, 384.0}, {-1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> log_2 = EdgeRawElement::from_registered_stock(
        "raw_log_2",
        stock_frame({1153.9028, 2.5400, 375.6490}, {-0.0080613, -0.0000076, 0.9999675}, {0.0009410, -0.9999996, 0.0}),
        60.0,
        Line::from_points({1220.0, 0.0, 384.0}, {2020.0, 0.0, 384.0}),
        "chord_3_log_2"
    );
    log_2->cuts = {
        Plane::from_point_normal({1220.0, 0.0, 384.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({2000.0, 0.0, 384.0}, {-1.0, 0.0, 0.0}),
    };

    // Sequential tree assembly order from base `log_1` -> `fork_1` -> `log_2` -> `fork_2` -> `log_3` -> `log_0` -> closing `fork_0` (vertical +Z drop)
    for (const std::shared_ptr<EdgeRawElement>& member : {log_1, fork_1, log_2, fork_2, log_3, log_0, fork_0})
        scene.add(member, chords);

    // Four interlocking stop-splayed cogged scarf splices with central tension hooks + cylindrical pins (PhD Fig. 5.27 C & Fig. 5.29 C)
    // Ordered (moving, stationary) so `second` (`-p` insertion) lifts upward (+Z) in the exploded assembly diagram
    struct ScarfSpec {
        std::shared_ptr<EdgeRawElement> first;
        std::shared_ptr<EdgeRawElement> second;
        Point centre;
        double pin_dx;
    };
    const std::array<ScarfSpec, 4> splices = {{
        {log_0, fork_0, {790.0, -177.0, 78.0}, 62.0},
        {log_1, fork_1, {1260.0, 177.0, 78.0}, 68.0},
        {fork_2, log_3, {620.0, 0.0, 384.0}, 88.0},
        {log_2, fork_2, {1380.0, 0.0, 384.0}, 88.0},
    }};
    for (size_t i = 0; i < splices.size(); i++) {
        const std::shared_ptr<JointBeamElement> scarf = JointBeamElement::from_library(
            "stop_splayed_cogged_scarf",
            *splices[i].first,
            *splices[i].second,
            {{"cog", 0.30}}
        );
        scarf->name = fmt::format("chord_scarf_{}", i);
        scene.add(scarf, joints);
        scene.add_interaction(scarf, splices[i].first, scarf->interaction(0));
        scene.add_interaction(scarf, splices[i].second, scarf->interaction(1));

        for (size_t p = 0; p < 2; p++) {
            const double dx = p == 0 ? -splices[i].pin_dx : splices[i].pin_dx;
            const Line pin_axis = Line::from_points(
                splices[i].centre + Vector(dx, 0.0, -44.0), splices[i].centre + Vector(dx, 0.0, 44.0)
            );
            const std::shared_ptr<JointPinElement> pin =
                std::make_shared<JointPinElement>(pin_axis, 7.5, 0.05);
            pin->name = fmt::format("scarf_pin_{}_{}", i, p);
            scene.add(pin, joints);

            Line bore_axis = pin_axis;
            bore_axis.extend_equally(35.0);
            for (size_t m = 0; m < 2; m++) {
                const std::shared_ptr<EdgeRawElement>& member = m == 0 ? splices[i].first : splices[i].second;
                const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 7.5);
                bore->name = fmt::format("scarf_bore_{}_{}_{}", i, p, m);
                bore->is_visible = false;
                scene.add(bore, joints);
                scene.add_interaction(bore, member, bore->interaction(0));
            }
        }
    }

    // Three cyclic pegged stopped-dovetail fork-branch joints triangulating Chord 1 -> Chord 2 -> Chord 3 -> Chord 1 (PhD Fig. 5.45 & Arikake fork prototype)
    struct BranchWeb {
        std::shared_ptr<EdgeRawElement> fork;
        std::shared_ptr<EdgeRawElement> chord;
        Line axis;
    };
    const std::array<BranchWeb, 3> webs = {{
        {fork_0, log_1, Line::from_points({384.0, -177.0, 78.0}, {876.0, 177.0, 78.0})},
        {fork_1, log_2, Line::from_points({1544.0, 177.0, 78.0}, {1915.0, 0.0, 384.0})},
        {fork_2, log_0, Line::from_points({952.0, 0.0, 384.0}, {1389.0, -177.0, 78.0})},
    }};
    for (size_t i = 0; i < webs.size(); i++) {
        const std::shared_ptr<EdgeRawElement> branch = webs[i].fork->branch_proxy(46.0, webs[i].axis);
        const std::shared_ptr<JointBeamElement> web_joint = JointBeamElement::from_library(
            "pegged_angled_stopped_dovetail_half_lap",
            *branch,
            *webs[i].chord,
            {{"flare", 0.25}, {"stop", 0.72}}
        );
        web_joint->name = fmt::format("fork_dovetail_{}", i);
        scene.add(web_joint, joints);
        scene.add_interaction(web_joint, webs[i].fork, web_joint->interaction(0));
        scene.add_interaction(web_joint, webs[i].chord, web_joint->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The Rossiniere seven-member triangular space truss reconstructed from 3D-scanned spruce forks and logs with full interlocking joinery: four stop-splayed cogged scarf splices locked by twin opposing folding wedges and cylindrical pins along the three chords, and three cyclic pegged stopped-dovetail half-lap fork-branch joints (via EdgeRawElement::branch_proxy) triangulating the space frame.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_truss --parallel 6 && ./build/element_edge_raw_truss && ../bash/publish-scene.sh --target element_edge_raw_truss

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
