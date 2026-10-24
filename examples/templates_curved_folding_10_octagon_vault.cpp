#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 239, octagonal plan with segments sloping toward the centre: eight half barrels tilted down 15 degrees to the centre, a star of arches around a dish.
int main() {

    CylinderVault vault(8, 6000.0, 3000.0, -15.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("octagon vault: {} panels, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the curved folding templates: Engel p. 239, an octagonal plan with segments sloping toward the centre. Eight half barrels of 6 m span over a regular octagon, each with its generatrices tilted down 15 degrees from its side toward the centre, the arches standing on the sides and the eight segments sinking to the centre, meeting in the vertical planes through the corners. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_10_octagon_vault --parallel 6 && ./build/templates_curved_folding_10_octagon_vault && ../bash/publish-scene.sh --target templates_curved_folding_10_octagon_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
