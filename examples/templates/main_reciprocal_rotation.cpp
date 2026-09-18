#include "wood_session.h"
#include "src/templates/reciprocal_rotation.h"

using namespace session_cpp;
using namespace wood_session;

/// Builds the reciprocal rotation dome and writes the dome mesh, one plate per interior beam and one per boundary beam to live.
int main() {

    const ReciprocalRotation shell;

    WoodSession wood_session("reciprocal_rotation");
    wood_session.add_mesh(std::make_shared<Mesh>(shell.dome_mesh));

    const std::vector<std::pair<size_t, size_t>> edges = shell.dome_mesh.edges();  // beams[i] sits on edges[i]
    size_t interior = 0;
    for (size_t beam = 0; beam < shell.beam_bottom.size(); beam++) {
        if (shell.dome_mesh.is_edge_on_boundary(edges[beam].first, edges[beam].second))
            continue;  // the rotated naked-edge beam: the straight boundary beam replaces it

        wood_session.add(std::make_shared<Plate>(shell.beam_bottom[beam], shell.beam_top[beam], "beam"));
        interior++;
    }

    for (size_t beam = 0; beam < shell.boundary_beam_bottom.size(); beam++)
        wood_session.add(std::make_shared<Plate>(shell.boundary_beam_bottom[beam], shell.boundary_beam_top[beam], "boundary_beam"));

    std::cout << fmt::format("reciprocal rotation: {} beams, {} boundary beams\n", interior, shell.boundary_beam_bottom.size());

    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reciprocal rotation template: a sinusoidal dome whose face edges become beams rotated about their midpoints, framed by straight boundary beams on the naked edges, one plate per beam, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/main_reciprocal_rotation && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target main_reciprocal_rotation

|||||||| WORKFLOW ||||||||
examples/templates/main_reciprocal_rotation.cpp
 |
 |-- ReciprocalRotation(nx, ny, W, D, h, angle, scale, beam_w, beam_h, extend_factor, cut_offset_factor)                       src/templates/reciprocal_rotation.h
 |    |-- make_dome -> dome_mesh; _build -> beams (meshes), beam_bottom / beam_top outlines, side0 / side1
 |    |-- naked_half_edges, mitre_plane, boundary_inner_plane -> boundary_beams, boundary_beam_bottom / boundary_beam_top, boundary_side0 / boundary_side1: a straight beam per naked edge, mitred at each boundary vertex; interior beams stop at its inner face
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 |-- add_to_tree(true, true, false, false)        one group per plate: the plate and its "outlines"
 '-- pb_dump(pb_path("live"))                    sync_geometry (Mesh::loft once per plate), Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
