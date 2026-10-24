#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 238, generatrices in one plane: the groin vault, two half barrels of 12 m span crossing over a square, meeting on the diagonals, every quarter one curved panel.
int main() {

    CylinderVault vault(4, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("groin vault: {} panels, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the curved folding templates: Engel's structure systems through interpenetration of cylindrical surfaces (Structure Systems p. 238), generatrices in one plane. Two equal half barrels of 12 m span and 6 m radius cross over a 12 m square, each kept where it is higher: the groin vault. Each of the four segments is a cylinder whose generatrices run horizontally from its side toward the centre, and two neighbouring segments are each other's mirror image in the vertical plane through their shared corner and the centre, so they meet in that plane along a planar curve, a half ellipse straight in plan and curved in elevation, the same planar curved crease the reflection fold makes. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_4_groin_vault --parallel 6 && ./build/templates_curved_folding_4_groin_vault && ../bash/publish-scene.sh --target templates_curved_folding_4_groin_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
