#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The IBOIS curved folded plate: a smooth wave section of alternating arcs, three crests, swept and reflected at the kinks of their polyline, a short rise, a long descending slope and a steep leg to the ground.
int main() {

    const double climb = 25.0 * Tolerance::TO_RADIANS;
    const Vector along(std::cos(climb), 0.0, std::sin(climb));
    const Vector up(-std::sin(climb), 0.0, std::cos(climb));
    const Point foot(0.0, 12000.0, 0.0);
    ReflectionFold fold(
        ReflectionFold::meander(foot, Vector(0.0, -1.0, 0.0), up, 6, 1200.0, 450.0),
        along,
        Vector(0.0, -1.0, 0.0),
        17000.0,
        {{3000.0, -45.0 * Tolerance::TO_RADIANS, true}, {12000.0, -40.0 * Tolerance::TO_RADIANS, true}},
        std::nullopt,
        120.0
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("curved folded plate: {} lamellas, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.get_contacts().size(), fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the curved folding templates: the IBOIS curved folded plate, a smooth wave section folded along a polyline, the curved counterpart of a zigzag folded plate. The section is a meander of six circular arcs, each 1200 wide and 450 deep, bulging up and down in turn and meeting tangent to each other, three crests across 7.2 m; it is swept along a polyline, a short rise at 25 degrees for 3 m, a reflection of 45 degrees at the ridge onto a slope falling 20 degrees for 9 m, and a reflection of 40 degrees onto a leg falling 60 degrees for 5 m to the ground. Every crease is the reflection plane's section of the wave, every half wave of every segment one lamella. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_3_curved_folded_plate --parallel 6 && ./build/templates_curved_folding_3_curved_folded_plate && ./bash/publish-scene.sh --target templates_curved_folding_3_curved_folded_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
