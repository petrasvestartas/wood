#include "wood_session.h"
#include "src/templates/folding/diamond_mesh.h"

using namespace session_cpp;
using namespace wood_session;

const int U_DIVISIONS = 8;
const int V_DIVISIONS = 4;
const double THICKNESS = 40.0;
const double CHAMFER = 10.0;
const double CHAMFER_ANGLE = 180.0; // corners sharper than this are chamfered, 180 every corner

int main() {

    const DiamondMesh shell(DiamondMesh::default_surface(), U_DIVISIONS, V_DIVISIONS, THICKNESS, CHAMFER, CHAMFER_ANGLE);

    WoodSession wood_session("diamond_mesh");
    wood_session.add_mesh(std::make_shared<Mesh>(shell.mesh));
    for (const std::shared_ptr<Plate>& plate : shell.elements)
        wood_session.add(plate);

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The diamond mesh template on its default arch surface: the NURBS surface split into a rhombus pattern of triangles, one chamfered plate per triangle, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_diamond_mesh --parallel 4 && ./build/templates_diamond_mesh && ../bash/publish-scene.sh --target templates_diamond_mesh

|||||||| WORKFLOW ||||||||
examples/templates_diamond_mesh.cpp
 |
 |-- DiamondMesh(surface, u_div, v_div, thickness, chamfer, chamfer_angle)                       src/templates/folding/diamond_mesh.h
 |    |-- triangle pairs on the surface -> mesh; Mesh::miter_contours, chamfer_mask, chamfer_apply -> one Plate(bottom, top) per triangle in `elements`
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 '-- pb_dump(pb_path("live"))                    Mesh::loft once per stale plate, Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
