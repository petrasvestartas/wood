#include "wood_session.h"
#include "src/templates/cylindrical_folding/reflection_fold.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_cylindrical_folding;

/// two-hinged arch of folded cylindrical surfaces.
int main() {

    ReflectionFold fold(ReflectionFoldGuide::two_hinged_arch());
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> panels =
        fold.get_elements_placed<FaceCurvedElement>("lamella");

    for (const std::shared_ptr<FaceCurvedElement>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "two hinged arch: {} lamellas, {} closed solids, {} contacts, development distortion "
        "{:.2e}, ruling twist {:.2e}\n",
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
Step 5 of the cylindrical folding templates: the two-hinged arch, high and narrow. The sheet of four deep barrels, each 6 m wide and 2400 high, the capsules of the end elevation, rises from the hinge at its foot as a leg leaning 15 degrees outward, is folded 50 degrees onto a segment at 55 degrees and 50 degrees again onto the crown segment at 5 degrees; the crown at mid-span is the plane of symmetry, the folded half mirrored in it, meeting itself along the plane's section of the barrels. Both folds are the mirror folds: the next segment is the mirror image of the one before in the plane bisecting the fold, so every segment is a vault, as the end elevation shows the barrels rounded all the way up; the creases are the bisecting planes' sections, the ovals of the plan at the quarter points, the crown line is straight. Span 22 m, crown 20 m. Every part of the sheet between two creases and two curves of the profile is a lamella, a FaceCurvedElement: the exact cylinder between its two creases, the thickness on the one physical underside the material keeps through every fold, its side faces the mitre it shares with the next plate across the crease, the crease plane's section at a mirror fold and the exact crossing of the two offset cylinders at a reflection, its caps the rulings at the profile curves' ends, mitred between neighbouring curves; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat; the session's contact detection finds every shared face. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_5_two_hinged_arch --parallel 6 && ./build/templates_cylindrical_folding_5_two_hinged_arch && ../bash/publish-scene.sh --target templates_cylindrical_folding_5_two_hinged_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
