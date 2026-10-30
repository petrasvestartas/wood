#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// A Pratt roof truss from its guide, the design's overload by its parameters: the members and their butts made by the
/// template.
int main() {

    const wood_truss::TrussGuide guide(wood_truss::Pratt{.span = 12000.0, .rise = 3000.0, .panels = 6});
    wood_truss::Truss truss(guide);
    std::cout << truss << std::endl;
    truss.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A Pratt roof truss from TrussGuide(Pratt{...}), the design's overload by its parameters: the bottom chord over 12000, the top chord up to 3000 at the middle, six panels, a vertical at every inner panel point and the diagonals falling towards the middle, all drawn as lines (red). The Truss template makes a beam per chord run and per web, mitres the top chord at the ridge, butts the heels, stops every web top_side on the chords and mitres the webs that share a node.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_1_pratt --parallel 6 && ./build/templates_truss_1_pratt && ../bash/publish-scene.sh --target templates_truss_1_pratt

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
