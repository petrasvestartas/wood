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

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The translation shell template: a cross section swept along a profile, one chamfered plate per strip, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_translation_shell --parallel 4 && ./build/templates_translation_shell && ../bash/publish-scene.sh --target templates_translation_shell

|||||||| WORKFLOW ||||||||
examples/templates_translation_shell.cpp
 |
 |-- TranslationShell(cross_section, profile, thickness, chamfer, chamfer_angle)                       src/templates/translation_shell.h
 |    |-- sweep(cross_section, profile) -> mesh; chamfer_mask, chamfer_apply -> one Plate(bottom, top) per strip in `elements`
 |
 |-- WoodSession, add_mesh(mesh), add(plate)      src/joinery_solver/wood_session.cpp -> Session::add_element
 '-- pb_dump(pb_path("live"))                    Mesh::loft once per stale plate, Session::pb_dump
                                                 -> data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
