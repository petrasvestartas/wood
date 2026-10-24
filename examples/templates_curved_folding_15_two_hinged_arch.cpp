#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 243, two-hinged arch of folded cylindrical surfaces: deep barrels rising from hinged feet as legs leaning outward, mirror-folded twice by 50 degrees into a tall polygonal arch with a flat crown, every segment a vault, 22 m span, the crown the plane of symmetry.
int main() {

    const double climb = 105.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 2400.0),
        along,
        Vector(0.0, -1.0, 0.0),
        40000.0,
        {{11000.0, -50.0 * Tolerance::TO_RADIANS, false}, {22000.0, -50.0 * Tolerance::TO_RADIANS, false}},
        Plane::from_point_normal(Point(11000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0))
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("two hinged arch: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 15 of the curved folding templates: Engel p. 243, the two-hinged arch, high and narrow. The sheet of four deep barrels, each 6 m wide and 2400 high, the capsules of Engel's end elevation, rises from the hinge at its foot as a leg leaning 15 degrees outward, is folded 50 degrees onto a segment at 55 degrees and 50 degrees again onto the crown segment at 5 degrees; the crown at mid-span is the plane of symmetry, the folded half mirrored in it, meeting itself along the plane's section of the barrels. Both folds are Engel's mirror folds: the next segment is the mirror image of the one before in the plane bisecting the fold, so every segment is a vault, as his end elevation shows the barrels rounded all the way up; the creases are the bisecting planes' sections, the ovals of his plan at the quarter points, the crown line is straight. Span 22 m, crown 20 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_15_two_hinged_arch --parallel 6 && ./build/templates_curved_folding_15_two_hinged_arch && ../bash/publish-scene.sh --target templates_curved_folding_15_two_hinged_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
