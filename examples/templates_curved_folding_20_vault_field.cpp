#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_vault.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 241, composition of cylindrical surfaces crossing diagonally over a square grid: a groin vault on every bay of a 6 by 4 grid and one at twice the scale over the four middle bays.
int main() {

    CylinderVault vault(4, 6000.0, 3000.0, 0.0, CylinderVault::Kind::Groin, 8, 120.0, "vault_field", CylinderVault::field(6, 4, 6000.0));
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = vault.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("vault field: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, vault.distortion(), vault.twist());
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 20 of the curved folding templates: Engel p. 241, the composition of cylindrical surfaces crossing diagonally over a square grid plan. A groin vault of two half barrels of 6 m span stands on every bay of a 6 by 4 grid of 6 m bays, and over the four bays in the middle one groin vault of 12 m span at twice the scale, standing up above the field with its four arches: the same CylinderVault placed 21 times, at every bay's centre and once at the middle at scale 2. Every segment is cut along its generatrices into 8 lamellas, more where a generatrix changes the plane it ends on, each a PlateCurved: the exact cylinder strip between the two vertical planes its generatrices run between, 120 mm thick under the cylinder, its side faces the planar sections shared with the lamella across the crease, its caps the generatrices shared with the next lamella of the segment; every lamella is bent about one axis and cut from a flat panel, its development drawn in the group flat. Groups: plan (the segments' footprints), lamellas (lamella_<segment>_<k>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_20_vault_field --parallel 6 && ./build/templates_curved_folding_20_vault_field && ../bash/publish-scene.sh --target templates_curved_folding_20_vault_field

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
