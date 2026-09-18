#include "wood_session.h"
#include "src/templates/reciprocal_move.h"

using namespace session_cpp;
using namespace wood_session;

/// Builds the reciprocal move dome and writes the dome mesh and one plate per beam to live.
int main() {

    const ReciprocalMove shell(12, 10, 12000.0, 10000.0, 3000.0, 50.0, 100.0, 200.0);

    WoodSession wood_session("reciprocal_move");
    wood_session.add_mesh(std::make_shared<Mesh>(shell.dome_mesh));
    for (size_t beam = 0; beam < shell.beam_bottom.size(); beam++)
        wood_session.add(std::make_shared<Plate>(shell.beam_bottom[beam], shell.beam_top[beam], "beam"));

    std::cout << fmt::format("reciprocal move: {} beams\n", shell.beam_bottom.size());

    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reciprocal move template: a sinusoidal dome whose face edges become beams shifted past each other, one plate per beam, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/main_reciprocal_move && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target main_reciprocal_move

|||||||| WORKFLOW ||||||||
examples/templates/main_reciprocal_move.cpp
 |
 |-- ReciprocalMove(nx, ny, W, D, h, angle, beam_w, beam_h, extend_factor, cut_offset_factor)                       src/templates/reciprocal_move.h
 |    |-- _make_dome -> dome_mesh; _build -> beams (meshes), beam_bottom / beam_top outlines, side0 / side1
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 |-- add_to_tree(true, true, false, false)        one group per plate: the plate and its "outlines"
 '-- pb_dump(pb_path("live"))                    sync_geometry (Mesh::loft once per plate), Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
