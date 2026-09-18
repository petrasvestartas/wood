#include "wood_session.h"
#include "src/templates/chevron.h"

using namespace session_cpp;
using namespace wood_session;

/// Builds the chevron shell on its default surface and writes the mesh and its plates to live.
int main() {

    const Chevron shell;

    WoodSession wood_session("chevron");
    wood_session.add_mesh(std::make_shared<Mesh>(shell.mesh));
    for (const std::shared_ptr<Plate>& plate : shell.elements)
        wood_session.add(plate);

    std::cout << fmt::format("chevron: {} faces, {} plates\n", shell.mesh.number_of_faces(), shell.elements.size());

    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The chevron template: a NURBS surface folded into chevron strips, four plates per face, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/main_chevron && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target main_chevron

|||||||| WORKFLOW ||||||||
examples/templates/main_chevron.cpp
 |
 |-- Chevron(surface, u_divisions, v_division_dist, shift, scale, box_height, top_plate_inlet, plate_thickness, edge_rotation, edge_offset)                       src/templates/chevron.h
 |    |-- wood_chevron::chevron_mesh -> mesh; wood_chevron::chevron_plates -> 8 outlines per face -> 4 Plate(bottom, top) per face in `elements`,
 |    plus insertion_vectors, joints_per_face, three_valence, adjacency for the solver
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 |-- add_to_tree(true, true, false, false)        one group per plate: the plate and its "outlines"
 '-- pb_dump(pb_path("live"))                    sync_geometry (Mesh::loft once per plate), Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
