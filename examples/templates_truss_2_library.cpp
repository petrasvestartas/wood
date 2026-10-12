#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every truss of the TrussGuide library built by the Truss template, each its own scene truss_<name>.
int main() {

    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = {
        {"king_post", wood_truss::TrussGuide::king_post()},
        {"queen_post", wood_truss::TrussGuide::queen_post()},
        {"pratt", wood_truss::TrussGuide::pratt()},
        {"howe", wood_truss::TrussGuide::howe()},
        {"fink", wood_truss::TrussGuide::fink()},
        {"belgian", wood_truss::TrussGuide::belgian()},
        {"scissors", wood_truss::TrussGuide::scissors()},
        {"warren", wood_truss::TrussGuide::warren()},
        {"warren_verticals", wood_truss::TrussGuide::warren_verticals()},
        {"pratt_bridge", wood_truss::TrussGuide::pratt_bridge()},
        {"parker", wood_truss::TrussGuide::parker()},
        {"k_truss", wood_truss::TrussGuide::k_truss()},
        {"cambered_verticals", wood_truss::TrussGuide::cambered_verticals()},
        {"funicular_arch", wood_truss::TrussGuide::funicular_arch()},
        {"lens", wood_truss::TrussGuide::lens()},
        {"triangulated_arch", wood_truss::TrussGuide::triangulated_arch()},
        {"sloped", wood_truss::TrussGuide::sloped()},
        {"pratt_flat", wood_truss::TrussGuide::pratt_flat()},
        {"flat_peaked", wood_truss::TrussGuide::flat_peaked()},
        {"bowstring", wood_truss::TrussGuide::bowstring()},
        {"pitched_posts", wood_truss::TrussGuide::pitched_posts()},
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        wood_truss::Truss truss(guide.second, "truss_" + guide.first);
        truss.pb_dump(pb_path("truss_" + guide.first));
    }
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The TrussGuide library, every guide built by the Truss template, each its own scene data/output/pb/truss_<name>.pb: the king post, queen post, Pratt, Howe, Fink, Belgian and scissors roof trusses, the Warren, Warren with verticals, Pratt, Parker and K bridge trusses, and the graphic-statics forms: cambered verticals, funicular arch, lens, triangulated arch, sloped, flat Pratt, flat peaked, bowstring and pitched on end posts. Each guide is only lines, chords as polylines through their nodes, webs and end posts as lines (red); the template makes a beam per chord run, web and post, and every butt from how the lines meet at the nodes: runs mitered at kinks, heels by JointBeam::butt, web and post ends top_side on the runs, a chord running on over a lone end, and the webs at a node mitered with their neighbours.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_2_library --parallel 6 && ./build/templates_truss_2_library && ../bash/publish-scene.sh --target templates_truss_2_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
