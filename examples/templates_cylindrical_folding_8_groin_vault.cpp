#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// The groin vault: two half barrels of 12 m span crossing over a square, every quarter one cylindrical panel.
int main() {

    CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels =
        vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "groin vault: {} panels, {} closed solids, {} contacts, development distortion {:.2e}, "
        "ruling twist {:.2e}\n",
        panels.size(),
        solids,
        vault.get_contacts().size(),
        vault.distortion(),
        vault.twist()
    );
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the cylindrical folding templates: the groin vault. Two half barrels of 12 m span and 6 m radius cross over a 12 m square; every quarter is a level cylinder from its side to the centre, and neighbours meet on half ellipses in the diagonal planes. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. Groups: plan, lamellas, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_8_groin_vault --parallel 6 && ./build/templates_cylindrical_folding_8_groin_vault && ./bash/publish-scene.sh --target templates_cylindrical_folding_8_groin_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
