#include "src/templates/delorme/delorme.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_delorme;

/// Semicircular three-ply barrel vault beside a two-ply pointed comble a l'imperiale (carene de navire) roof.
int main() {

    WoodSession scene("templates_delorme_2_barrel_vault");

    const DelormeGuide barrel_guide =
        DelormeGuide::barrel(4800.0, 2400.0, 3200.0, 4, 3, 1200.0, 220.0, 32.0);
    const DelormeSystem barrel(barrel_guide, "semicircular_barrel");
    barrel.append_to(scene, Vector(0.0, 0.0, 0.0), "semicircular");

    const DelormeGuide pointed_guide =
        DelormeGuide::pointed_imperiale(4600.0, 3000.0, 3200.0, 4, 2, 1150.0, 220.0, 34.0);
    const DelormeSystem pointed(pointed_guide, "pointed_imperiale");
    pointed.append_to(scene, Vector(6200.0, 0.0, 0.0), "pointed_imperiale");

    std::cout << fmt::format(
        "Barrel: {} planks (3-ply) | Pointed imperiale: {} planks (2-ply)\n",
        barrel.planks().size(),
        pointed.planks().size()
    );
    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Philibert de l'Orme's two canonical long-span roof profiles from Book I of Nouvelles inventions (1561): a four-rib three-ply semicircular barrel vault with one-third running-bond plank stagger (left), and a two-ply pointed comble a l'imperiale ship-hull roof rising from horizontal wall springings to a pointed ridge miter (right), both tied longitudinally without iron nails by mortised liernes and twin tapered clavettes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_delorme_2_barrel_vault --parallel 6 && ./build/templates_delorme_2_barrel_vault && ../bash/publish-scene.sh --target templates_delorme_2_barrel_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
