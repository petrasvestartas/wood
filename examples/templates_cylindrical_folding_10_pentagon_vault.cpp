#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// The pentagon groin vault: five half barrels of 8 m span over a regular pentagon, level, the crown at the arches' top.
int main() {

    CylinderVault vault(5, 8000.0, 4000.0, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels =
        vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "pentagon vault: {} panels, {} closed solids, {} contacts, development distortion {:.2e}, "
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
Step 10 of the cylindrical folding templates: the pentagon groin vault. Five half barrels of 8 m span and 4 m radius over a regular pentagon, every one a level cylinder from its side to the centre, so the crown is at the height of the arches' top; neighbours meet on half ellipses in the vertical planes through the corners and the centre. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. Groups: plan, lamellas, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_10_pentagon_vault --parallel 6 && ./build/templates_cylindrical_folding_10_pentagon_vault && ./bash/publish-scene.sh --target templates_cylindrical_folding_10_pentagon_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
