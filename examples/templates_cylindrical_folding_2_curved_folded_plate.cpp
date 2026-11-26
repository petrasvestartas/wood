#include "wood_session.h"
#include "src/templates/cylindrical_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_cylindrical_folding;

/// A curved folded plate.
int main() {

    ReflectionFold fold(ReflectionFoldGuide::curved_folded_plate());
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> panels =
        fold.get_elements_placed<FaceCurvedElement>("lamella");

    for (const std::shared_ptr<FaceCurvedElement>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "curved folded plate: {} lamellas, {} closed solids, {} contacts, development distortion "
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
Step 2 of the cylindrical folding templates: a curved folded plate. A wave corrugation, three sine waves of 3 m wavelength and 400 mm amplitude across a 9 m wide sheet, is swept along a polygonal arch of five segments of 3.62 m turning 30 degrees at each of four reverse folds, from 60 degrees up at one foot to 60 degrees down at the other, spanning 13.5 m and rising 4.9 m. Every reverse fold is a reflection of the sheet in the plane bisecting the kink, so every crease is that plane's section of the corrugation, a planar wave, and the sheet still unrolls flat. Every half wave of every segment is one lamella, a FaceCurvedElement: the exact cylinder between its two creases, 120 mm thick, mitred where it meets the next lamella across a crease, its caps the straight rulings at the waves' cusps and the sheet's edges. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_2_curved_folded_plate --parallel 6 && ./build/templates_cylindrical_folding_2_curved_folded_plate && ../bash/publish-scene.sh --target templates_cylindrical_folding_2_curved_folded_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
