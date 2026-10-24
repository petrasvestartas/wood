#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// The octagon groin vault: eight half barrels of 6 m span over a regular octagon, level, the crown at the arches' top.
int main() {

    CylinderVault vault(8, 6000.0, 3000.0, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("octagon vault: {} panels, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.get_contacts().size(), vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the curved folding templates: the octagon groin vault. Eight half barrels of 6 m span and 3 m radius over a regular octagon, every one a level cylinder from its side to the centre, so the crown is at the height of the arches' top; neighbours meet on half ellipses in the vertical planes through the corners and the centre. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. Groups: plan, lamellas, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_10_octagon_vault --parallel 6 && ./build/templates_curved_folding_10_octagon_vault && ./bash/publish-scene.sh --target templates_curved_folding_10_octagon_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
