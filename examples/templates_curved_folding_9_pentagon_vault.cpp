#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// pentagonal plan with segments rising toward the centre: five half barrels tilted up 20 degrees to a point above the centre.
int main() {

    CylinderVault vault(5, 8000.0, 4000.0, 20.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("pentagon vault: {} panels, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.get_contacts().size(), vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the curved folding templates: a pentagonal plan with segments rising toward the centre. Five half barrels of 8 m span over a regular pentagon, each with its generatrices tilted up 20 degrees from its side toward the centre, meeting their neighbours in the vertical planes through the corners and rising to a point above the centre. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat; the session's contact detection finds every shared face. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_9_pentagon_vault --parallel 6 && ./build/templates_curved_folding_9_pentagon_vault && ./bash/publish-scene.sh --target templates_curved_folding_9_pentagon_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
