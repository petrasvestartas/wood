#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// A truss from members of your own: two chords through their nodes and five webs, each with its half section; the template cuts the butts and miters.
int main() {

    // 1. members: the chords through their nodes and the webs between theirs, each with its half section
    const std::vector<wood_truss::TrussMember> chords = {
        {Polyline({{0.0, 0.0, 0.0}, {4000.0, 0.0, 0.0}, {8000.0, 0.0, 0.0}}), 80.0},
        {Polyline({{0.0, 0.0, 0.0}, {4000.0, 0.0, 2200.0}, {8000.0, 0.0, 0.0}}), 80.0},
    };
    const std::vector<wood_truss::TrussMember> webs = {
        {Polyline({{2000.0, 0.0, 1100.0}, {2000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{4000.0, 0.0, 2200.0}, {4000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{6000.0, 0.0, 1100.0}, {6000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{2000.0, 0.0, 1100.0}, {4000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{6000.0, 0.0, 1100.0}, {4000.0, 0.0, 0.0}}), 60.0},
    };

    // 2. the guide (160 x 160 mm chords, 120 x 120 mm webs) and the truss
    const wood_truss::TrussGuide guide(chords, webs);
    wood_truss::Truss truss(guide, "custom_pratt_truss");
    truss.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A truss from your own members: two chords and five webs given as polylines with their half sections, butted and mitered by the Truss template.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_4_custom_wireframe --parallel 6 && ./build/templates_truss_4_custom_wireframe && ../bash/publish-scene.sh --target templates_truss_4_custom_wireframe

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
