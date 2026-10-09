#include "wood_session.h"
#include "src/templates/shells/lamella_gridshell.h"

using namespace session_cpp;
using namespace wood_session;

/// The lamella gridshell on its asymptotic curves, on the default 10 m saddle: surface, lamellas, crossings, stations, boards and studs, each in its group.
int main() {

    wood_gridshell::LamellaGridshell gridshell;
    gridshell.set_features_visible("section", false);
    gridshell.set_features_visible("axis", false);
    const size_t boards = gridshell.get_elements<Beam>().size();
    const size_t studs = gridshell.get_elements<Column>().size();
    std::cout << fmt::format("asymptotic gridshell: {} boards, {} studs\n", boards, studs);
    gridshell.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The lamella gridshell template on the asymptotic curves of a 10 m saddle: 9 lamellas per family, each two upright boards a gap apart, the first family a layer up the normal, the second a layer down, a hexagonal stud through both gaps at every crossing, its flats against the four boards.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_shells_2_gridshell --parallel 6 && ./build/templates_shells_2_gridshell && ../bash/publish-scene.sh --target templates_shells_2_gridshell

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
