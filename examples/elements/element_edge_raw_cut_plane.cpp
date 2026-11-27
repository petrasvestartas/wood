#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A bifurcated tree fork trimmed across its butt, stem and branch by three cutting planes.
int main() {

    WoodSession scene("element_edge_raw_cut_plane");

    const std::shared_ptr<EdgeRawElement> fork = EdgeRawElement::from_stock(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 960.0}), 65.0, "raw_fork_1", 180.0, false, "fork"
    );
    scene.add(fork);

    const std::vector<Point> branch = fork->placed_branch_axes().back().get_points();
    const Vector along_branch = (branch.back() - branch[branch.size() - 2]).normalized();
    const Point on_branch = branch.back() - along_branch * 130.0;

    const std::array<Plane, 3> planes = {
        Plane::from_point_normal({0.0, 0.0, 70.0}, {0.0, 0.0, 1.0}),
        Plane::from_point_normal({0.0, 0.0, 880.0}, {0.0, 0.0, -1.0}),
        Plane::from_point_normal(on_branch, along_branch * -1.0),
    };
    for (size_t i = 0; i < planes.size(); i++) {
        const std::shared_ptr<JointCutPlaneElement> cut =
            std::make_shared<JointCutPlaneElement>(planes[i], 280.0, fmt::format("cut_{}", i));
        scene.add(cut);
        scene.add_interaction(cut, fork, cut->interaction(0));
    }

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        fork->name,
        fork->model_geometry_brep().face_count(),
        fork->model_geometry_brep().is_solid(),
        fork->model_geometry_brep().volume_exact(),
        fork->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A scanned spruce tree fork trimmed at its butt, upper stem and inclined branch end by three JointCutPlaneElement interactions.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_cut_plane --parallel 6 && ./build/element_edge_raw_cut_plane && ../bash/publish-scene.sh --target element_edge_raw_cut_plane

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
