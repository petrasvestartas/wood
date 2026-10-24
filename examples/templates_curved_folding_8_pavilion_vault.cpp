#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// The pavilion vault: four cylinder segments on four walls, rising along the walls to the peak over the centre.
int main() {

    CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Cloister, 4000.0);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("pavilion vault: {} panels, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.get_contacts().size(), vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the curved folding templates: the pavilion vault. Four walls 4 m high on a 12 m square, on each a cylinder segment of 6 m radius along the wall rising to the peak over the centre; the segments meet on quarter ellipses in the diagonal planes. Every segment is one 120 mm PlateCurved: its outer face the exact cylinder, its inner face the cylinder 120 mm smaller, its side faces shared with its neighbours across the creases; every panel unfolds flat. The walls are flat 120 mm panels. Groups: plan, lamellas, walls, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_8_pavilion_vault --parallel 6 && ./build/templates_curved_folding_8_pavilion_vault && ./bash/publish-scene.sh --target templates_curved_folding_8_pavilion_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
