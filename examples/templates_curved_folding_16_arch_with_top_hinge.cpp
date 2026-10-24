#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 243, arch with top hinge: the sheet of four barrels stands as legs rising as arches under the roof, is reflected at the eaves onto a steep roof and again onto a shoulder of troughs, mirror-folded to a level crown with the hinge in its middle, the plane of symmetry; 26 m span.
int main() {

    const double climb = 90.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(std::sin(climb), 0.0, -std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1600.0),
        along,
        Vector(0.0, -1.0, 0.0),
        30000.0,
        {{300.0, -15.0 * Tolerance::TO_RADIANS, true}, {18300.0, -40.0 * Tolerance::TO_RADIANS, true}, {25300.0, -35.0 * Tolerance::TO_RADIANS, false}},
        Plane::from_point_normal(Point(13000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0))
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("arch with top hinge: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 16 of the curved folding templates: Engel p. 243, the arch with a hinge in its crown. The sheet of four barrels, each 6 m wide and 1600 deep, stands on the ground as a vertical leg and is reflected 15 degrees at the eaves onto a roof rising 75 degrees; the reflection plane, through a station 300 mm up the ruling at the barrels' cusps, crosses the bulging legs 12 m higher in the middle of every barrel than at its cusps, so the crease is a tall arch and every leg a column rising under its trough to the crease, the roof's cusps coming down to the ground between the legs as the hatched notches of Engel's plan, and the roof hanging as troughs between the cusps, the wine glasses of his end elevation. The steep roof, a vault since the reflection turned the sheet over, is reflected again 40 degrees onto a shoulder at 35 degrees, a trough, and mirror-folded 35 degrees onto the level crown, keeping the trough, the cups of his end elevation; the hinge in the crown at mid-span is the plane of symmetry. Span 26 m, crown 21.7 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_16_arch_with_top_hinge --parallel 6 && ./build/templates_curved_folding_16_arch_with_top_hinge && ../bash/publish-scene.sh --target templates_curved_folding_16_arch_with_top_hinge

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
