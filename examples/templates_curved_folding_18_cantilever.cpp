#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 244, cantilevered beam on central supports: four shallow barrels as a roof 39 m long, reflected down in the middle into a V of two legs, cantilevering 14 m both ways.
int main() {

    const double climb = -3.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(std::sin(climb), 0.0, -std::cos(climb));
    const Point foot(0.0, 12000.0, 6100.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0),
        along,
        Vector(0.0, -1.0, 0.0),
        39000.0,
        {{14000.0, -72.0 * Tolerance::TO_RADIANS, true}, {19500.0, 150.0 * Tolerance::TO_RADIANS, true}, {25000.0, -72.0 * Tolerance::TO_RADIANS, true}},
        std::nullopt
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("cantilever: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 18 of the curved folding templates: Engel p. 244, the cantilevered beam on central supports. The sheet of four shallow barrels runs as a roof falling 3 degrees toward the middle, is reflected 72 degrees down into a leg at 75 degrees, 150 degrees at the ground into the rising leg and 72 degrees back into the roof rising 3 degrees to the other tip: a V of two legs under the middle of a roof cantilevering 14 m both ways, the legs' creases the pointed shapes of Engel's plan; the roof's two arms are a trough and a vault, every reflection turning the sheet over. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_18_cantilever --parallel 6 && ./build/templates_curved_folding_18_cantilever && ../bash/publish-scene.sh --target templates_curved_folding_18_cantilever

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
