#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 238, generatrices in one plane: the groin vault, two half barrels of 12 m span crossing over a square, meeting on the diagonals.
int main() {

    CylinderVault vault(4, 12000.0, 6000.0, 0.0, CylinderVault::Kind::Groin, 12);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("groin vault: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the curved folding templates: Engel's structure systems through interpenetration of cylindrical surfaces (Structure Systems p. 238), generatrices in one plane. Two equal half barrels of 12 m span and 6 m radius cross over a 12 m square, each kept where it is higher: the groin vault. Each of the four segments is a cylinder whose generatrices run horizontally from its side toward the centre, and two neighbouring segments are each other's mirror image in the vertical plane through their shared corner and the centre, so they meet in that plane along a planar curve, a half ellipse straight in plan and curved in elevation, the same planar curved crease the reflection fold makes. Every segment is cut along its generatrices into 12 lamellas, more where a generatrix changes the plane it ends on, each a PlateCurved: the exact cylinder strip between the two vertical planes its generatrices run between, 120 mm thick under the cylinder, its side faces the planar sections shared with the lamella across the crease, its caps the generatrices shared with the next lamella of the segment; every lamella is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>_<k>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_4_groin_vault --parallel 6 && ./build/templates_curved_folding_4_groin_vault && ../bash/publish-scene.sh --target templates_curved_folding_4_groin_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
