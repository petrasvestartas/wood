#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// two-hinged frame of folded cylindrical surfaces: four shallow barrels standing as 6 m columns, reflected at the knee onto a roof of troughs rising 14 degrees to the ridge at mid-span, 40 m span, the ridge the plane of symmetry.
int main() {

    const double climb = 90.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0),
        along,
        Vector(0.0, -1.0, 0.0),
        30000.0,
        {{6000.0, -76.0 * Tolerance::TO_RADIANS, true}},
        Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("two hinged frame: {} lamellas, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.get_contacts().size(), fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 13 of the curved folding templates: the linear structure systems of folded cylindrical surfaces, the two-hinged frame, a low wide hall. A sheet of four barrels side by side, each 6 m wide and 600 high, stands as a column 6 m high bulging inward and is folded 76 degrees at the knee onto a roof rising 14 degrees by the reflection, which turns the sheet over: the roof barrels hang as troughs between the cusps while the columns are vaults, the end elevation shows, and the knee crease is the reflection plane's oblique section of the barrels, the beaks over the eaves of the perspective. The ridge at mid-span, 20 m from the columns and 11 m high, is the plane of symmetry: the folded half is mirrored in the vertical plane through the ridge and meets itself along the plane's section of the barrels, the straight ridge of the plan. Span 40 m, eaves 6 m, ridge 11 m. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_13_two_hinged_frame --parallel 6 && ./build/templates_curved_folding_13_two_hinged_frame && ./bash/publish-scene.sh --target templates_curved_folding_13_two_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
