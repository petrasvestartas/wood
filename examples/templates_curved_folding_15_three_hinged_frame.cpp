#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 242, three-hinged frame of folded cylindrical surfaces: 6 m columns, a roof of two slopes per side and a hinge at mid-span, every fold a reflection, 30 m span.
int main() {

    const double climb = 90.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0),
        along,
        44000.0,
        ReflectionFold::turns(Point(0.0, 0.0, 0.0) + up * 600.0, along, Vector(0.0, -1.0, 0.0), {6000.0, 13000.0, 22000.0, 31000.0, 38000.0}, {-62.0 * Tolerance::TO_RADIANS, -20.0 * Tolerance::TO_RADIANS, -16.0 * Tolerance::TO_RADIANS, -20.0 * Tolerance::TO_RADIANS, -62.0 * Tolerance::TO_RADIANS}),
        std::nullopt,
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("three hinged frame: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 15 of the curved folding templates: Engel p. 242, the three-hinged frame. The sheet of four shallow barrels stands as a column 6 m high, is folded 62 degrees at the knee onto a roof rising 28 degrees, 20 degrees onto a slope of 8 degrees, 16 degrees at the hinge in mid-span onto the falling slope, and symmetrically down into the second column: five reflections, every crease the bisecting plane's oblique section of the barrels, the hinge's the long ovals Engel draws at mid-span. Span 30 m, eaves 6 m, ridge 10.5 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, computed exactly ruling by ruling, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unfolded sheet in the group flat. Groups: sheet (the unfolded sheet with its creases), creases, lamellas (lamella_<fold>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_15_three_hinged_frame --parallel 6 && ./build/templates_curved_folding_15_three_hinged_frame && ../bash/publish-scene.sh --target templates_curved_folding_15_three_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
