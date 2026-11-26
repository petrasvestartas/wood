#include "wood_session.h"
#include "src/templates/developable_folding/curved_fold.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// A curved folded tent roof.
int main() {

    CurvedFold fold(CurvedFoldGuide::tent_roof());
    std::cout << fold << std::endl;
    std::cout << fmt::format(
        "curved fold: {} lamellas, {} flat pieces, {} contacts, development distortion {:.2e}\n",
        fold.get_elements_numbered<FaceCurvedElement>("lamella").size(),
        fold.get_elements_numbered<FaceCurvedElement>("flat").size(),
        fold.get_contacts().size(),
        fold.distortion()
    );
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the developable folding templates: a curved folded roof, a freestanding folded shell roof of 32 m diameter rising into a tower 26 m high with a round top of 7 m (CurvedFoldGuide::tent_roof). Twelve sectors, their valleys running down to the columns at 4000 and their ridges ending in a gable 4000 over the eaves, the folds following a trumpet curve, flat at the eaves and vertical at the tower. Every ridge and valley is a NURBS curve interpolated through fourteen rings, and every fold strip between two of them first a surface without thickness: a quad mesh of its rulings in strips, developed flat into the crease pattern in pattern. The thickness comes last: every strip becomes one lamella, a FaceCurvedElement of six exact NURBS patches, its outer face the strip, its inner face 120 mm in where the neighbouring strips offset by the thickness meet, its side faces shared with the neighbours as miters, and its flat piece for cutting in flat. With thickness 0 only the meridians, strips and pattern are drawn. Groups: meridians, strips, pattern, lamellas, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_1_curved_fold --parallel 6 && ./build/templates_developable_folding_1_curved_fold && ../bash/publish-scene.sh --target templates_developable_folding_1_curved_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
