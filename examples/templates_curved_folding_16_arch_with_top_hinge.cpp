#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 243, arch with top hinge: a roof of four trough barrels mirror-folded twice by 30 degrees from a steep slope over a shoulder to a level crown with the hinge in its middle, standing 12 m above the ground on slender round posts under the troughs, the crown the plane of symmetry.
int main() {

    const double climb = 60.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(std::sin(climb), 0.0, -std::cos(climb));
    const Point foot(0.0, 12000.0, 12000.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 900.0),
        along,
        Vector(0.0, -1.0, 0.0),
        30000.0,
        {{7000.0, -30.0 * Tolerance::TO_RADIANS, false}, {14000.0, -30.0 * Tolerance::TO_RADIANS, false}},
        Plane::from_point_normal(Point(14500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        {{Point(1500.0, 9000.0, 0.0), 500.0}, {Point(1500.0, 3000.0, 0.0), 500.0}, {Point(1500.0, -3000.0, 0.0), 500.0}, {Point(1500.0, -9000.0, 0.0), 500.0}}
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
Step 16 of the curved folding templates: Engel p. 243, the arch with a hinge in its crown. The roof is a sheet of four barrels, each 6 m wide and 900 deep, hanging as troughs, that leaves the eaves 12 m above the ground rising 60 degrees, is folded 30 degrees onto a shoulder at 30 degrees and 30 degrees again onto the level crown, both Engel's mirror folds keeping the troughs; the hinge in the crown at mid-span is the plane of symmetry. The roof stands on slender round posts of 500 mm radius, one under the lowest line of every trough at the eaves, each a cylinder from the ground up to the sheet, two half shells 120 mm thick topped by the curves where their vertical generatrices meet the sheet, the stems of the wine glasses of Engel's end elevation; the roof creases are the ovals of his plan. Span 29 m, crown 21.6 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat. The posts are post_<k>_<half> in the group posts.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_16_arch_with_top_hinge --parallel 6 && ./build/templates_curved_folding_16_arch_with_top_hinge && ../bash/publish-scene.sh --target templates_curved_folding_16_arch_with_top_hinge

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
