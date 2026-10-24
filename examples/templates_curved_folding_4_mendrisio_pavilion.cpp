#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The Mendrisio pavilion: one shallow barrel 4.5 m wide, standing as a leaning end wall, reflected at the eave onto a short trough rising 10 degrees and at the ridge crease onto the level middle, mirrored at mid-length; 13 m long and 3.4 m high, two panels across the crown seam.
int main() {

    const Vector along(0.1719, 0.0, 0.9851);
    ReflectionFold fold(
        {Primitives::arc(Point(6000.0, -2250.0, 0.0), Point(6304.0, -1125.0, 0.0), Point(6400.0, 0.0, 0.0)), Primitives::arc(Point(6400.0, 0.0, 0.0), Point(6304.0, 1125.0, 0.0), Point(6000.0, 2250.0, 0.0))},
        along,
        Vector(0.0, -1.0, 0.0),
        12000.0,
        {{2923.0, 89.9 * Tolerance::TO_RADIANS, true}, {3623.0, 10.0 * Tolerance::TO_RADIANS, true}},
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        77.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("mendrisio pavilion: {} lamellas, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.get_contacts().size(), fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the curved folding templates: the Mendrisio pavilion of IBOIS, 13.0 m long at the top and 12.0 m along the floor, 4.5 m wide and 3.4 m high, 77 mm curved cross-laminated timber. One shallow barrel, an arc 4.5 m wide and 400 high split at its crown into two panels, stands on the ground as the end wall 2.9 m high, curved in plan and leaning out, is reflected 89.9 degrees at the eave onto the roof rising 10 degrees, which the reflection turns into a trough, and 700 mm further at the barrel's edges 10 degrees again at the ridge crease onto the level middle, a vault; the vertical plane at mid-length is the plane of symmetry. The eave crease is the short oval where the wall meets the roof; the ridge crease, which the trough's crown line reaches 4.5 m later than its edges, is the long oval reaching 5 m in from the end with its tip toward the middle, as in the pavilion's top view; the wall's foot is the arc on the ground, its ends and the roof's long edges straight rulings, and the two halves of the barrel meet on the straight crown seam. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_4_mendrisio_pavilion --parallel 6 && ./build/templates_curved_folding_4_mendrisio_pavilion && ./bash/publish-scene.sh --target templates_curved_folding_4_mendrisio_pavilion

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
