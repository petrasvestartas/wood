#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 239, three cylindrical segments: three half barrels over a triangle, each from a side toward the centre, meeting on the lines from the corners.
int main() {

    CylinderVault vault(3, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Groin, 12);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("triangle vault: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the curved folding templates: Engel p. 239, a plan with three cylindrical segments. Three half barrels of 12 m span over an equilateral triangle, each with its generatrices running horizontally from its side toward the centre, meeting their neighbours in the vertical planes through the corners along planar curves; Engel draws them over a hexagon whose three other sides are the barrels' flanks, here the triangle alone. Every segment is cut along its generatrices into 12 lamellas, more where a generatrix changes the plane it ends on, each a PlateCurved: the exact cylinder strip between the two vertical planes its generatrices run between, 120 mm thick under the cylinder, its side faces the planar sections shared with the lamella across the crease, its caps the generatrices shared with the next lamella of the segment; every lamella is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>_<k>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_9_triangle_vault --parallel 6 && ./build/templates_curved_folding_9_triangle_vault && ../bash/publish-scene.sh --target templates_curved_folding_9_triangle_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
