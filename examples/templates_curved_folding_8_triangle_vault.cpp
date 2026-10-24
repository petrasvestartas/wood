#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 239, three cylindrical segments: three half barrels over a triangle, each from a side toward the centre, meeting on the lines from the corners.
int main() {

    CylinderVault vault(3, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Groin);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("triangle vault: {} panels, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the curved folding templates: Engel p. 239, a plan with three cylindrical segments. Three half barrels of 12 m span over an equilateral triangle, each with its generatrices running horizontally from its side toward the centre, meeting their neighbours in the vertical planes through the corners along planar curves; Engel draws them over a hexagon whose three other sides are the barrels' flanks, here the triangle alone. Every segment is one panel, a PlateCurved: its outer face the exact cylinder between the two vertical planes its generatrices run between, its meridians one rational arc each with a C0 knot where the generatrices change their plane (the groin's crease, two half ellipses meeting at the apex), 120 mm thick under the cylinder, its side faces the planar sections shared with the panel across the crease, its caps the first and the last generatrix drawn in to a millimetre from the corner or the apex; every panel is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_8_triangle_vault --parallel 6 && ./build/templates_curved_folding_8_triangle_vault && ../bash/publish-scene.sh --target templates_curved_folding_8_triangle_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
