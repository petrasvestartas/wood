#include "wood_session.h"
#include "src/templates/translation_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// Builds the translation shell with its default cross section and profile and writes the mesh and its plates to live.
int main() {

    const TranslationShell shell;

    WoodSession wood_session("translation_shell");
    wood_session.add_mesh(std::make_shared<Mesh>(shell.mesh));
    for (const std::shared_ptr<Plate>& plate : shell.elements)
        wood_session.add(plate);

    std::cout << fmt::format("translation shell: {} plates\n", shell.elements.size());

    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The translation shell template: a cross section swept along a profile, one chamfered plate per strip, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/main_translation_shell && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target main_translation_shell

|||||||| WORKFLOW ||||||||
examples/templates/main_translation_shell.cpp
 |
 |-- TranslationShell(cross_section, profile, thickness, chamfer, chamfer_angle)                       src/templates/translation_shell.h
 |    |-- sweep(cross_section, profile) -> mesh; chamfer_mask, chamfer_apply -> one Plate(bottom, top) per strip in `elements`
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 |-- add_to_tree(true, true, false, false)        one group per plate: the plate and its "outlines"
 '-- pb_dump(pb_path("live"))                    sync_geometry (Mesh::loft once per plate), Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
