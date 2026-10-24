#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// generatrices sloping toward the centre: the groin vault's segments tilted down 15 degrees toward the centre, a dish between four arches.
int main() {

    CylinderVault vault(4, 12000.0, 6000.0, -15.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("groin vault sloping: {} panels, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.get_contacts().size(), vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the curved folding templates: generatrices sloping toward the centre. The groin vault of step 5 with every segment's generatrices tilted down 15 degrees toward the centre: the arches over the sides, the vertical sections of the tilted cylinders, stand higher than the centre, where the four segments sink to a point. The creases stay in the vertical planes through the corners, since the four tilted cylinders are still mirror images in them. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat; the session's contact detection finds every shared face. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_6_groin_vault_sloping --parallel 6 && ./build/templates_curved_folding_6_groin_vault_sloping && ./bash/publish-scene.sh --target templates_curved_folding_6_groin_vault_sloping

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
