#include "wood_session.h"
#include "wood_profile.h"

using namespace session_cpp;
using namespace wood_session;

/// Every section profile, each swept along a short beam.
int main() {

    WoodSession scene("element_profile");

    const std::vector<std::pair<std::string, std::vector<Polyline>>> profiles = {
        {"rectangle", profile_rectangle(200.0, 300.0)},
        {"round", profile_round(240.0)},
        {"w", profile_w(200.0, 300.0, 20.0, 12.0)},
        {"hss", profile_hss(200.0, 300.0, 12.0)},
        {"double", profile_double(80.0, 300.0, 40.0)},
        {"slab_band", profile_slab_band(500.0, 100.0)},
        {"t", profile_t(300.0, 300.0, 20.0, 40.0)},
    };

    for (size_t i = 0; i < profiles.size(); i++) {
        const double x = 700.0 * i;
        const Polyline axis({{x, 0.0, 0.0}, {x, 800.0, 0.0}});
        scene.add(std::make_shared<Beam>(axis, profiles[i].second, std::vector<Vector>{}, profiles[i].first));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The section profiles of wood_profile.h, each a Beam 800 long on its own axis: profile_rectangle(200, 300), profile_round(240), profile_w(200, 300, 20, 12), profile_hss(200, 300, 12), profile_double(80, 300, 40), profile_slab_band(500, 100) and profile_t(300, 300, 20, 40); loop 0 is the outline, further loops are holes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_profile --parallel 6 && ./build/element_profile && ../bash/publish-scene.sh --target element_profile

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
