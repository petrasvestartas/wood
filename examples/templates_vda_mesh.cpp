#include "wood_session.h"
#include "src/templates/vda_mesh.h"

using namespace session_cpp;
using namespace wood_session;

const double FACE_THICKNESS = 40.0;
const std::vector<double> FACE_POSITIONS = {0.0}; // one plate per face on the mesh, more values stack plates above it
const std::vector<int> EDGE_DIVISIONS = {2}; // connector pairs per interior edge
const double CONNECTOR_WIDTH = 300.0;
const double CONNECTOR_HEIGHT = 300.0;
const double CONNECTOR_THICKNESS = 40.0;

int main() {

    const Mesh mesh = VdaMesh::default_mesh();
    const VdaMesh vda(mesh, FACE_THICKNESS, FACE_POSITIONS, EDGE_DIVISIONS, {}, {}, CONNECTOR_WIDTH, CONNECTOR_HEIGHT, CONNECTOR_THICKNESS);

    WoodSession wood_session("vda_mesh");
    wood_session.add_mesh(std::make_shared<Mesh>(mesh));

    for (const std::vector<Polyline>& outlines : vda.f_polylines)
        for (size_t i = 0; i + 1 < outlines.size(); i += 2)
            wood_session.add(std::make_shared<Plate>(outlines[i], outlines[i + 1]));

    for (const std::vector<Polyline>& outlines : vda.e_polylines)
        for (size_t i = 0; i + 1 < outlines.size(); i += 2)
            if (!outlines[i].is_empty())
                wood_session.add(std::make_shared<Plate>(outlines[i], outlines[i + 1], "connector"));

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The vda mesh template on its default hexagonal mesh: one plate per face cut back by the bisector planes of its edges, and two rectangular connector plates across every interior edge, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vda_mesh --parallel 4 && ./build/templates_vda_mesh && ../bash/publish-scene.sh --target templates_vda_mesh

|||||||| WORKFLOW ||||||||
examples/templates_vda_mesh.cpp
 |
 |-- VdaMesh(mesh, face_thickness, face_positions, edge_divisions, edge_division_len, insertion_lines, width, height, thickness)   src/templates/vda_mesh.h
 |    |-- face planes, face-edge planes, bisector planes -> f_polylines, a bottom and top outline per face per position
 |    '-- edge planes per division -> e_polylines, two connector rectangles per interior edge division
 |
 |-- WoodSession, add_mesh(mesh), add(Plate(bottom, top))      src/joinery_solver/wood_session.cpp -> Session::add_element
 '-- pb_dump(pb_path("live"))                                 Mesh::loft once per stale plate, Session::pb_dump
                                                              -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
