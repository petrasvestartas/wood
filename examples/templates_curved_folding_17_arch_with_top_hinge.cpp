#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 243, arch with top hinge: slender 14 m columns, deep roof barrels folded twice by 40 degrees to the hinge in the crown, 26 m span, the crown the plane of symmetry.
int main() {

    const double climb = 90.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0),
        along,
        40000.0,
        ReflectionFold::turns(Point(0.0, 0.0, 0.0) + up * 1400.0, along, Vector(0.0, -1.0, 0.0), {14000.0, 22000.0}, {-40.0 * Tolerance::TO_RADIANS, -40.0 * Tolerance::TO_RADIANS}),
        Plane::from_point_normal(Point(13000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("arch with top hinge: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 17 of the curved folding templates: Engel p. 243, the arch with a hinge in its crown. The sheet of four barrels stands as a column 14 m high, is folded 40 degrees onto a roof rising 50 degrees and 40 degrees again onto the crown segment at 10 degrees; the hinge in the crown at mid-span is the plane of symmetry. The columns are the upright barrels, the roof creases the ovals of Engel's plan. Span 26 m, crown 21.5 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, computed exactly ruling by ruling, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unfolded sheet in the group flat. Groups: sheet (the unfolded sheet with its creases), creases, lamellas (lamella_<fold>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_17_arch_with_top_hinge --parallel 6 && ./build/templates_curved_folding_17_arch_with_top_hinge && ../bash/publish-scene.sh --target templates_curved_folding_17_arch_with_top_hinge

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
