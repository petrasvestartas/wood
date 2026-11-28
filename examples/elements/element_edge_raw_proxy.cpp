#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Three stages left to right: scanned RGB point clouds + axes, octagonal beam proxies, and attached raw trunks with identical interlocking joinery.
int main() {

    WoodSession scene("element_edge_raw_proxy");

    // Stage 0 (left): 3D laser-scanned RGB point clouds of all four raw trunks aligned onto the portal node axes via raw_xform
    {
        const std::shared_ptr<TreeNode> scan_group = scene.group_named("scan");
        const std::shared_ptr<EdgeRawElement> scan_girder = EdgeRawElement::from_stock(
            Line::from_points({-240.0, 0.0, 680.0}, {740.0, 0.0, 680.0}), 54.0, "raw_log_0", 180.0, true, "scan_girder"
        );
        const std::shared_ptr<EdgeRawElement> scan_post = EdgeRawElement::from_stock(
            Line::from_points({0.0, 0.0, 50.0}, {0.0, 0.0, 680.0}), 56.0, "raw_log_3", 45.0, true, "scan_post"
        );
        const std::shared_ptr<EdgeRawElement> scan_brace = EdgeRawElement::from_stock(
            Line::from_points({180.0, 0.0, 180.0}, {540.0, 0.0, 680.0}), 54.0, "raw_log_2", 20.0, true, "scan_brace"
        );
        const std::shared_ptr<EdgeRawElement> scan_rafter = EdgeRawElement::from_stock(
            Line::from_points({270.0, -320.0, 728.0}, {270.0, 320.0, 728.0}), 54.0, "raw_log_1", 90.0, true, "scan_rafter"
        );
        for (const std::shared_ptr<EdgeRawElement>& item : {scan_girder, scan_post, scan_brace, scan_rafter}) {
            if (item->has_raw_pointcloud()) {
                PointCloud cloud = item->placed_raw_pointcloud();
                cloud.name = item->name + "_cloud";
                scene.add_pointcloud(std::make_shared<PointCloud>(cloud), scan_group);
            }
            scene.add_line(std::make_shared<Line>(item->chord()), scan_group);
        }
    }

    // Stage 1 (middle): fast octagonal round-section beam proxies (use_raw_mesh = false)
    // Stage 2 (right): scanned raw spruce meshes attached onto the identical axes via attach_stock (raw_xform)
    for (int attached : {0, 1}) {
        const std::string tag = attached ? "raw" : "proxy";
        const std::shared_ptr<TreeNode> group = scene.group_named(tag);
        const Vector shift(attached ? 2360.0 : 1180.0, attached ? -1362.0 : -681.0, 0.0);

        const std::shared_ptr<EdgeRawElement> girder = EdgeRawElement::round(
            Line::from_points(Point(-240.0, 0.0, 680.0) + shift, Point(740.0, 0.0, 680.0) + shift),
            54.0,
            4.2,
            "girder_" + tag
        );
        const std::shared_ptr<EdgeRawElement> post = EdgeRawElement::round(
            Line::from_points(Point(0.0, 0.0, 50.0) + shift, Point(0.0, 0.0, 680.0) + shift),
            56.0,
            4.2,
            "post_" + tag
        );
        post->extension = 80.0;
        const std::shared_ptr<EdgeRawElement> brace = EdgeRawElement::round(
            Line::from_points(Point(180.0, 0.0, 180.0) + shift, Point(540.0, 0.0, 680.0) + shift),
            54.0,
            4.2,
            "brace_" + tag
        );
        brace->extension = 90.0;
        const std::shared_ptr<EdgeRawElement> rafter = EdgeRawElement::round(
            Line::from_points(Point(270.0, -320.0, 728.0) + shift, Point(270.0, 320.0, 728.0) + shift),
            54.0,
            4.2,
            "rafter_" + tag
        );

        if (attached) {
            girder->attach_stock("raw_log_0", 180.0, true);
            post->attach_stock("raw_log_3", 45.0, true);
            brace->attach_stock("raw_log_2", 20.0, true);
            rafter->attach_stock("raw_log_1", 90.0, true);
            girder->cuts = {
                Plane::from_point_normal(Point(-220.0, 0.0, 680.0) + shift, {1.0, 0.0, 0.0}),
                Plane::from_point_normal(Point(720.0, 0.0, 680.0) + shift, {-1.0, 0.0, 0.0}),
            };
            post->cuts = {Plane::from_point_normal(Point(0.0, 0.0, 60.0) + shift, {0.0, 0.0, 1.0})};
            brace->cuts = {
                Plane::from_point_normal(
                    Point(180.0, 0.0, 180.0) + shift, Vector(360.0, 0.0, 500.0).normalized()
                )
            };
            rafter->cuts = {
                Plane::from_point_normal(Point(270.0, -300.0, 728.0) + shift, {0.0, 1.0, 0.0}),
                Plane::from_point_normal(Point(270.0, 300.0, 728.0) + shift, {0.0, -1.0, 0.0}),
            };
        }

        scene.add(girder, group);
        scene.add(post, group);
        scene.add(brace, group);
        scene.add(rafter, group);

        const std::shared_ptr<JointBeamElement> bridle = JointBeamElement::from_library(
            "dovetail_t_bridle", *post, *girder, {{"slot", 0.48}, {"inside", 0.66}}
        );
        bridle->name = "bridle_" + tag;
        scene.add(bridle, group);
        scene.add_interaction(bridle, post, bridle->interaction(0));
        scene.add_interaction(bridle, girder, bridle->interaction(1));

        const std::shared_ptr<JointBeamElement> thrust = JointBeamElement::from_library(
            "notched_oblique_thrust_pegged_mortise_tenon", *brace, *girder
        );
        thrust->name = "thrust_" + tag;
        scene.add(thrust, group);
        scene.add_interaction(thrust, brace, thrust->interaction(0));
        scene.add_interaction(thrust, girder, thrust->interaction(1));

        Settings conical = scene.settings;
        conical.joint_parameters[10] = 0.68;
        conical.joint_parameters[11] = JointPlateElement::cross_half_lap_id(CrossDrills::one);
        const InteractionContactAxis cross_contact(
            Line::from_points(Point(270.0, 0.0, 728.0) + shift, Point(270.0, 0.0, 680.0) + shift),
            0.5,
            0.5,
            0,
            0,
            0,
            0
        );
        const std::shared_ptr<JointBeamElement> cross =
            JointBeamElement::from_contact(*rafter, *girder, cross_contact, 250.0, 0.91, 1, conical);
        cross->name = "conical_" + tag;
        cross->line_radius = 9.0;
        cross->target_insertions = {{{0.0, 0.0, -1.0}}, {{0.0, 0.0, 1.0}}};
        scene.add(cross, group);
        scene.add_interaction(cross, rafter, cross->interaction(0));
        scene.add_interaction(cross, girder, cross->interaction(1));

        const std::shared_ptr<JointPinElement> pin = std::make_shared<JointPinElement>(
            Line::from_points(Point(270.0, 0.0, 636.0) + shift, Point(270.0, 0.0, 772.0) + shift),
            9.0,
            0.05
        );
        pin->name = "pin_" + tag;
        scene.add(pin, group);
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Three stages of the raw-timber workflow side by side: co-registered RGB laser-scan point clouds aligned onto structural axes (left), a four-member portal node solved on fast octagonal round-section beam proxies (`use_raw_mesh = false`, middle), and the identical dovetail T-bridle, pegged oblique thrust mortise-and-tenon and conical cross-lap joints carved cleanly through the attached spruce trunks (`use_raw_mesh = true`, right).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_proxy --parallel 6 && ./build/element_edge_raw_proxy && ../bash/publish-scene.sh --target element_edge_raw_proxy

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
