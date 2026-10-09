#include "wood_session.h"
#include "src/templates/shells/lamella_gridshell.h"

using namespace session_cpp;
using namespace wood_session;

/// The lamella gridshell on the iso-curves of a 6 m saddle, 5 lamellas per family.
int main() {

    const NurbsSurface saddle = wood_gridshell::LamellaGridshell::default_surface(6000.0);
    wood_gridshell::LamellaGridshell gridshell(saddle, 0, 5, 5);
    gridshell.set_features_visible("section", false);
    gridshell.set_features_visible("axis", false);
    const size_t boards = gridshell.get_elements<Beam>().size();
    const size_t studs = gridshell.get_elements<Column>().size();
    std::cout << fmt::format("iso gridshell: {} boards, {} studs\n", boards, studs);
    gridshell.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The lamella gridshell template on the u and v iso-curves of a 6 m saddle, 5 lamellas per family: the boards bend about their strong axis and do not unroll straight, unlike on the asymptotic curves.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_shells_3_gridshell_iso --parallel 6 && ./build/templates_shells_3_gridshell_iso && ../bash/publish-scene.sh --target templates_shells_3_gridshell_iso

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
