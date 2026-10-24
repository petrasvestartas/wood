#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 241, composition of cylindrical surfaces crossing diagonally over a square grid: a groin vault on every bay of a 6 by 4 grid and one at twice the scale over the four middle bays.
int main() {

    CylinderVault vault(4, 6000.0, 3000.0, 0.0, CylinderVault::Kind::Groin, 0.0, 120.0, "vault_field", CylinderVault::field(6, 4, 6000.0));
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("vault field: {} panels, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 19 of the curved folding templates: Engel p. 241, the composition of cylindrical surfaces crossing diagonally over a square grid plan. A groin vault of two half barrels of 6 m span stands on every bay of a 6 by 4 grid of 6 m bays, and over the four bays in the middle one groin vault of 12 m span at twice the scale, standing up above the field with its four arches: the same CylinderVault placed 21 times, at every bay's centre and once at the middle at scale 2. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_19_vault_field --parallel 6 && ./build/templates_curved_folding_19_vault_field && ../bash/publish-scene.sh --target templates_curved_folding_19_vault_field

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
