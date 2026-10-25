#include "wood_session.h"
#include "src/templates/cylindrical_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// two-hinged A-frame of folded cylindrical surfaces.
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
        Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels =
        fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "a frame: {} lamellas, {} closed solids, {} contacts, development distortion {:.2e}, "
        "ruling twist {:.2e}\n",
        panels.size(),
        solids,
        fold.get_contacts().size(),
        fold.distortion(),
        fold.twist()
    );
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the cylindrical folding templates: the two-hinged A-frame. The sheet of four barrels, each 6 m wide and 1400 high, rises from the hinge at its foot leaning 20 degrees outward, is reflected 60 degrees at the knee into a leg at 50 degrees, and the apex at mid-span is the plane of symmetry: the leg and its mirror image meet there along the plane's section of the barrels, the straight ridge of the plan, the knee creases its pointed ovals near the feet; the feet are vaults and the legs troughs hanging between the cusps that run up to the apex, as in the end elevation. Span 25 m, apex 22.4 m. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_7_a_frame --parallel 6 && ./build/templates_cylindrical_folding_7_a_frame && ./bash/publish-scene.sh --target templates_cylindrical_folding_7_a_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
