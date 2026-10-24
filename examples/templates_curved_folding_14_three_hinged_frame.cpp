#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// three-hinged frame of folded cylindrical surfaces: 6 m columns, a roof of a steep slope and a shoulder per side folded by reflection, and a notch at mid-span with the hinge in its bottom, the plane of symmetry; 36 m span.
int main() {

    const double climb = 90.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 400.0),
        along,
        Vector(0.0, -1.0, 0.0),
        30000.0,
        {{6000.0, -62.0 * Tolerance::TO_RADIANS, true}, {14400.0, -23.0 * Tolerance::TO_RADIANS, true}, {23400.0, -50.0 * Tolerance::TO_RADIANS, true}},
        Plane::from_point_normal(Point(17795.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("three hinged frame: {} lamellas, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.get_contacts().size(), fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 14 of the curved folding templates: the three-hinged frame. The sheet of four shallow barrels, 400 high, stands as a column 6 m high bulging outward, is reflected 62 degrees at the knee onto a roof rising 28 degrees, 23 degrees onto a shoulder of 5 degrees and 50 degrees down into the notch at mid-span, whose bottom, the hinge, is the plane of symmetry: three reflections, every crease the plane's oblique section of the barrels, the notch's two slopes between the shoulder creases the hatched ovals of the plan at mid-span. Every reflection turns the sheet over, so the columns and the shoulders are vaults and the slopes and the notch troughs, the shoulders' rounded tops the top of the end elevation. Span 36 m, eaves 6 m, shoulders 10.7 m, hinge 9.3 m. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_14_three_hinged_frame --parallel 6 && ./build/templates_curved_folding_14_three_hinged_frame && ./bash/publish-scene.sh --target templates_curved_folding_14_three_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
