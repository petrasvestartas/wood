#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// the IBOIS reflection principle: a vertical strip with a circular arc section, cut by the plane R and the part beyond it mirrored in R, bending the strip by 120 degrees; two lamellas meeting on one mitred crease, the plane's section of the arc.
int main() {

    ReflectionFold fold(
        {Primitives::arc(Point(500.0, -866.0, 3000.0), Point(1000.0, 0.0, 3000.0), Point(500.0, 866.0, 3000.0))},
        Vector(0.0, 0.0, -1.0),
        Vector(0.0, -1.0, 0.0),
        3500.0,
        {{1500.0, 60.0 * Tolerance::TO_RADIANS}}
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("reflection: {} lamellas, {} closed solids, {} contacts, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.get_contacts().size(), fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the curved folding templates: the reflection principle. A vertical strip with a circular arc section e(t), 120 degrees of a circle of 1 m radius, runs down from s0 and is cut at s1 by the plane R inclined at (pi - phi) / 2 to its axis; the part beyond R is mirrored in R, so the strip bends by phi = 120 degrees toward s2r, and the crease a(t) is R's section of the arc, a planar curve. Two lamellas 120 mm thick meet on the crease along one mitre, the exact curve where their two offset cylinders cross. Every part of the sheet between two creases and two curves of the profile is a lamella, a PlateCurved: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_2_reflection --parallel 6 && ./build/templates_curved_folding_2_reflection && ./bash/publish-scene.sh --target templates_curved_folding_2_reflection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
