#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Mitani's curved folding by reflection: a sheet of two shallow barrels, 20 m along its rulings, folded at three planes into a hump, every crease the plane's section of the barrels and every part between two creases a curved plate that unfolds flat.
int main() {

    const Vector along(0.0, 1.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(Point(-3000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), 2, 3000.0, 400.0),
        along,
        Vector(1.0, 0.0, 0.0),
        20000.0,
        {{4000.0, 35.0 * Tolerance::TO_RADIANS}, {10000.0, -70.0 * Tolerance::TO_RADIANS}, {16000.0, 35.0 * Tolerance::TO_RADIANS}}
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("reflection fold: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the curved folding templates: Mitani and Igarashi's planar curved folding by reflection (Pacific Graphics 2011). A developable sheet, two shallow circular barrels 3000 wide and 400 high side by side swept 20 m along y, is cut by a plane and the part beyond it is mirrored in the plane; the crease is the plane's section of the sheet, a planar curve, and the mirrored part keeps its development, so the sheet still unfolds flat. Three planes containing the x direction turn the sheet up 35 degrees, down 70 and up 35 again into a hump; each acts on the sheet as it lies after the folds before it. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_2_reflection_fold --parallel 6 && ./build/templates_curved_folding_2_reflection_fold && ../bash/publish-scene.sh --target templates_curved_folding_2_reflection_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
