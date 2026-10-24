#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 238, right column: a cylinder segment per corner, its generatrices across the corner's diagonal, the four meeting on the lines from the sides' middles to the centre.
int main() {

    CylinderVault vault(4, 12000.0, 9000.0, 0.0, CylinderVault::Kind::Diagonal, 12);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("diagonal vault: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the curved folding templates: Engel p. 238, right column, generatrices in one plane across the diagonals. Over a 12 m square a cylinder segment per corner: its generatrices run horizontally across the corner's diagonal, its profile is the arc of 9 m radius from the corner on the ground up to the crown 3.9 m above the centre, and it covers the kite between the two neighbouring sides' middles. Two neighbouring segments are mirror images in the vertical plane from a side's middle to the centre, so they meet there in a planar curve; the sides of the square are cut by the segments as pointed arches rising from the corners to the sides' middles. Every segment is cut along its generatrices into 12 lamellas, more where a generatrix changes the plane it ends on, each a PlateCurved: the exact cylinder strip between the two vertical planes its generatrices run between, 120 mm thick under the cylinder, its side faces the planar sections shared with the lamella across the crease, its caps the generatrices shared with the next lamella of the segment; every lamella is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>_<k>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_7_diagonal_vault --parallel 6 && ./build/templates_curved_folding_7_diagonal_vault && ../bash/publish-scene.sh --target templates_curved_folding_7_diagonal_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
