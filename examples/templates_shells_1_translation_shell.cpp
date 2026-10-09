#include "wood_session.h"
#include "src/templates/shells/translation_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// The translation shell with its default cross section and profile: curves, sections, mesh and plates, each in its group.
int main() {

    TranslationShell shell;
    const std::vector<std::shared_ptr<Plate>> plates = shell.get_elements_numbered<Plate>("plate");
    std::cout << fmt::format("translation shell: {} plates\n", plates.size());
    shell.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The translation shell template: a cross section swept along a profile, one quad per two neighbouring sections, one mitred and chamfered plate per quad, each step in its group.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_shells_1_translation_shell --parallel 6 && ./build/templates_shells_1_translation_shell && ../bash/publish-scene.sh --target templates_shells_1_translation_shell

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
