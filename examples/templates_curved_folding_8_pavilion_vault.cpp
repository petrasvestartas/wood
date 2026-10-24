#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// right column: the pavilion vault on four walls, a cylinder segment per side with its generatrices along the wall, rising from the wall to the peak over the centre, the four meeting on the diagonals.
int main() {

    CylinderVault vault(4, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Cloister, 4000.0);
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
Step 8 of the curved folding templates: right column, generatrices in one plane: the pavilion (cloister) vault on walls, the two barrels kept where they are lower. On a 12 m square stand four walls 4 m high, and on every wall a cylinder segment whose generatrices run along the wall, its profile the quarter circle of 6 m radius from the wall's top up to the peak 6 m higher over the centre; the four segments are mirror images in the diagonal planes and meet in them along quarter ellipses, the creases rising from the corners to the peak, straight in plan and curved in elevation. The walls are flat panels of the same thickness under the segments' springing lines, in the group walls. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat; the session's contact detection finds every shared face. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_8_pavilion_vault --parallel 6 && ./build/templates_curved_folding_8_pavilion_vault && ./bash/publish-scene.sh --target templates_curved_folding_8_pavilion_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
