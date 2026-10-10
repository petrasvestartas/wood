#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The plates of annen_box_pair joined by points and lines, as the plugin's dots and lines set them: no sidecar, each point names its face's joint type, each line its face's insertion vector.
int main() {

    // the plates of the dataset and its settings, without its sidecars
    WoodSession scene = WoodSession::obj_load("annen_box_pair");
    scene.settings = config::load_yaml("annen_box_pair");
    scene.name = "element_plate_assign_joints";

    // a point at the centre of every side face that is joined: 10 the side-to-side joints, 20 the top-to-side ones
    const std::vector<Point> points = {
        {-632.383, -853.908, -197.922}, {-79.806, -853.908, 185.917}, {-99.086, 475.200, 78.237}, {194.781, -408.746, -622.197},
        {202.789, 221.365, 532.068}, {256.424, 48.863, -452.615}, {324.694, 83.236, -484.489}, {48.696, -873.908, -326.382},
        {578.235, -185.452, -2.611}, {656.267, -151.400, -42.119}, {674.422, -526.549, -238.183}, {77.030, -39.981, 52.933},
        {842.265, -82.062, -560.511},
    };
    const std::vector<int> types = {20, 20, 20, 20, 20, 20, 20, 10, 20, 20, 20, 10, 0};
    scene.assign_joint_types_by_points(points, types, 1.0);

    // a line from the centre of every face a top-to-side tenon slides into, along the way it slides
    const Vector way(-5.835732, 19.397964, 13.677616);
    const Vector across(27.603175, 25.122242, -9.293505);
    scene.assign_insertion_vectors_by_lines({
        Line::from_points({-632.383, -853.908, -197.922}, Point(-632.383, -853.908, -197.922) + way),
        Line::from_points({-79.806, -853.908, 185.917}, Point(-79.806, -853.908, 185.917) + way),
        Line::from_points({194.781, -408.746, -622.197}, Point(194.781, -408.746, -622.197) + way),
        Line::from_points({674.422, -526.549, -238.183}, Point(674.422, -526.549, -238.183) + way),
        Line::from_points({256.424, 48.863, -452.615}, Point(256.424, 48.863, -452.615) - way),
        Line::from_points({578.235, -185.452, -2.611}, Point(578.235, -185.452, -2.611) - way),
        Line::from_points({-99.086, 475.200, 78.237}, Point(-99.086, 475.200, 78.237) + across),
        Line::from_points({202.789, 221.365, 532.068}, Point(202.789, 221.365, 532.068) + across),
        Line::from_points({324.694, 83.236, -484.489}, Point(324.694, 83.236, -484.489) + across),
        Line::from_points({656.267, -151.400, -42.119}, Point(656.267, -151.400, -42.119) + across),
    }, 1.0);

    // the joints the types and vectors choose, cut into the plates
    const std::vector<InteractionFeaturePlate> joints = scene.compute_features();
    std::cout << scene << " " << joints.size() << " joints" << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The plates of annen_box_pair, eight plates of two Annen boxes, read from the dataset's obj with its settings and none of its sidecars; WoodSession::assign_joint_types_by_points puts a joint type on every face a point lies on (each snaps to the side face whose middle line is nearest within 1 mm: 10 the side-to-side joints, 20 the top-to-side ones), and WoodSession::assign_insertion_vectors_by_lines puts each line's direction on the face its start lies on, the way the top-to-side tenons slide in; compute_features then joins the plates with the 13 joints the sidecars annen_box_pair_joints_types.txt and annen_box_pair_insertion_vectors.txt give (tests/plate_assignment.cpp proves the same on every sidecar dataset).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_plate_assign_joints --parallel 6 && ./build/element_plate_assign_joints && ../bash/publish-scene.sh --target element_plate_assign_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
