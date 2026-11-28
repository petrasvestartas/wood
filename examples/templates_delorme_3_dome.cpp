#include "src/templates/delorme/delorme.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_delorme;

/// Hemispherical Delorme cupola with compression oculus curb and staggered chordal latitude liernes, after the Halle au Ble (Paris, 1783).
int main() {

    const DelormeGuide guide =
        DelormeGuide::dome(6400.0, 1280.0, 12, 2, 1100.0, 210.0, 32.0);
    DelormeSystem scene(guide, "templates_delorme_3_dome");

    std::cout << fmt::format(
        "Halle au Ble cupola: {} meridian planks, {} curb segments, {} chordal liernes, {} clavettes\n",
        scene.planks().size(),
        scene.curbs().size(),
        scene.liernes().size(),
        scene.clavettes().size()
    );
    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Hemispherical small-timber dome after Legrand, Molinos and master carpenter Andre-Jacob Roubo's 1782-1783 Halle au Ble cupola in Paris: twelve radial two-ply Delorme meridian ribs rise from a level circular base to a segmented compression oculus ring, braced circumferentially by alternating staggered rings of straight chordal liernes and twin tapered clavettes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_delorme_3_dome --parallel 6 && ./build/templates_delorme_3_dome && ../bash/publish-scene.sh --target templates_delorme_3_dome

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
