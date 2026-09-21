#include "wood_session.h"
#include "src/templates/reflex_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Builds the reflex fold with its default cross section and profile and writes the mesh and its plates to live.
int main() {

    const ReflexFold shell;

    WoodSession wood_session("reflex_fold");
    wood_session.add_mesh(std::make_shared<Mesh>(shell.mesh));
    for (const std::shared_ptr<Plate>& plate : shell.elements)
        wood_session.add(plate);

    std::cout << fmt::format("reflex fold: {} plates\n", shell.elements.size());

    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reflex fold template: a folded cross section along a profile, one plate per fold, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_reflex_fold --parallel 4 && ./build/templates_reflex_fold && ../bash/publish-scene.sh --target templates_reflex_fold

|||||||| WORKFLOW ||||||||
examples/templates_reflex_fold.cpp
 |
 |-- ReflexFold(cross_section, profile, thickness, chamfer_bot, chamfer_top, chamfer_angle)                       src/templates/reflex_fold.h
 |    |-- reflex_fold(cross_section, profile) -> mesh; chamfer_mask, chamfer_apply -> one Plate(bottom, top) per fold in `elements`
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 |-- add_to_tree(true, true, false, false)        one group per plate: the plate and its "outlines"
 '-- pb_dump(pb_path("live"))                    sync_geometry (Mesh::loft once per plate), Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
