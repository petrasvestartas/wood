#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 239, pentagonal plan with segments rising toward the centre: five half barrels tilted up 20 degrees to a point above the centre.
int main() {

    CylinderVault vault(5, 8000.0, 4000.0, 20.0 * Tolerance::TO_RADIANS, CylinderVault::Kind::Groin, 10);
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("pentagon vault: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the curved folding templates: Engel p. 239, a pentagonal plan with segments rising toward the centre. Five half barrels of 8 m span over a regular pentagon, each with its generatrices tilted up 20 degrees from its side toward the centre, meeting their neighbours in the vertical planes through the corners and rising to a point above the centre. Every segment is cut along its generatrices into 10 lamellas, more where a generatrix changes the plane it ends on, each a PlateCurved: the exact cylinder strip between the two vertical planes its generatrices run between, 120 mm thick under the cylinder, its side faces the planar sections shared with the lamella across the crease, its caps the generatrices shared with the next lamella of the segment; every lamella is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>_<k>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_10_pentagon_vault --parallel 6 && ./build/templates_curved_folding_10_pentagon_vault && ../bash/publish-scene.sh --target templates_curved_folding_10_pentagon_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
