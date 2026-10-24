#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 243, two-hinged arch of folded cylindrical surfaces: deep barrels rising from hinged feet as legs leaning outward, folded twice by 50 degrees into a tall polygonal arch with a flat crown, 27 m span, the crown the plane of symmetry.
int main() {

    const double climb = 105.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0),
        along,
        40000.0,
        ReflectionFold::turns(Point(0.0, 0.0, 0.0) + up * 1400.0, along, Vector(0.0, -1.0, 0.0), {7500.0, 16500.0}, {-50.0 * Tolerance::TO_RADIANS, -50.0 * Tolerance::TO_RADIANS}),
        Plane::from_point_normal(Point(13500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> lamellas = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& lamella : lamellas)
        solids += lamella->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("two hinged arch: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", lamellas.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 16 of the curved folding templates: Engel p. 243, the two-hinged arch, high and narrow. The sheet of four barrels, each 6 m wide and 1400 high, rises from the hinge at its foot as a leg leaning 15 degrees outward, is folded 50 degrees onto a segment at 55 degrees and 50 degrees again onto the crown segment at 5 degrees; the crown at mid-span is the plane of symmetry, the folded half mirrored in it, meeting itself along the plane's section of the barrels. The two reflections' creases are the ovals of Engel's plan at the quarter points, the crown line is straight. Span 27 m, crown 15.5 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, computed exactly ruling by ruling, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unfolded sheet in the group flat. Groups: sheet (the unfolded sheet with its creases), creases, lamellas (lamella_<fold>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_16_two_hinged_arch --parallel 6 && ./build/templates_curved_folding_16_two_hinged_arch && ../bash/publish-scene.sh --target templates_curved_folding_16_two_hinged_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
