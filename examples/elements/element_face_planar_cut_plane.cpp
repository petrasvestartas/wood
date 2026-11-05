#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A planar face cut by a plane.
int main() {

    WoodSession scene("element_face_planar_cut_plane");

    const std::shared_ptr<FacePlanarElement> plate = FacePlanarElement::from_rectangle(
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 600.0, 400.0, 40.0, "plate"
    );
    scene.add(plate);

    const std::shared_ptr<JointCutPlaneElement> cut = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal({450.0, 200.0, 0.0}, {-1.0, -0.5, 0.0}), 500.0, "cut"
    );
    scene.add(cut);
    scene.add_interaction(cut, plate, cut->interaction(0));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        plate->name,
        plate->model_geometry_brep().face_count(),
        plate->model_geometry_brep().is_solid(),
        plate->model_geometry_brep().volume_exact(),
        plate->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A planar face cut by a plane: a JointCutPlaneElement on the plate, which keeps the side the normal points to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_face_planar_cut_plane --parallel 6 && ./build/element_face_planar_cut_plane && ../bash/publish-scene.sh --target element_face_planar_cut_plane

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
