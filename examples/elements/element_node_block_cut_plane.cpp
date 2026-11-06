#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A block cut by a plane.
int main() {

    WoodSession scene("element_node_block_cut_plane");

    const std::shared_ptr<NodeBlockElement> block =
        NodeBlockElement::box({0.0, 0.0, 0.0}, 300.0, 300.0, 200.0, "block");
    scene.add(block);

    const std::shared_ptr<JointCutPlaneElement> cut = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal({150.0, 150.0, 120.0}, {-0.5, 0.0, -1.0}), 600.0, "cut"
    );
    scene.add(cut);
    scene.add_interaction(cut, block, cut->interaction(0));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        block->name,
        block->model_geometry_brep().face_count(),
        block->model_geometry_brep().is_solid(),
        block->model_geometry_brep().volume_exact(),
        block->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A block cut by a plane: a JointCutPlaneElement on the block, which keeps the side the normal points to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_node_block_cut_plane --parallel 6 && ./build/element_node_block_cut_plane && ../bash/publish-scene.sh --target element_node_block_cut_plane

|||||||| TIME ||||||||
0.78 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
