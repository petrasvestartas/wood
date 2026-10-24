#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel p. 244, two-hinged A-frame of folded cylindrical surfaces: deep barrels rising from splayed feet, reflected at the knee into two steep legs of troughs to the apex 22 m high, 25 m span, the apex the plane of symmetry.
int main() {

    const double climb = 110.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0),
        along,
        Vector(0.0, -1.0, 0.0),
        40000.0,
        {{5600.0, -60.0 * Tolerance::TO_RADIANS, true}},
        Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0))
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("a frame: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 17 of the curved folding templates: Engel p. 244, the two-hinged A-frame. The sheet of four barrels, each 6 m wide and 1400 high, rises from the hinge at its foot leaning 20 degrees outward, is reflected 60 degrees at the knee into a leg at 50 degrees, and the apex at mid-span is the plane of symmetry: the leg and its mirror image meet there along the plane's section of the barrels, the straight ridge of Engel's plan, the knee creases its pointed ovals near the feet; the feet are vaults and the legs troughs hanging between the cusps that run up to the apex, as in his end elevation. Span 25 m, apex 22.4 m. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_17_a_frame --parallel 6 && ./build/templates_curved_folding_17_a_frame && ../bash/publish-scene.sh --target templates_curved_folding_17_a_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
