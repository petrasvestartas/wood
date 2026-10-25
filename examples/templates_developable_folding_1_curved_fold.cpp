#include "wood_session.h"
#include "src/templates/developable_folding/curved_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// A curved folded tent roof.
int main() {

    CurvedFold fold;
    std::cout << fold << std::endl;

    for (const std::shared_ptr<PlateCurved>& lamella :
         fold.get_elements_numbered<PlateCurved>("lamella")) {
        const BRep& solid = lamella->element_geometry_brep();
        std::cout << fmt::format(
            "{}: {} faces, solid {}, volume {:.3f} m3\n",
            lamella->name,
            solid.face_count(),
            solid.is_solid(),
            lamella->element_geometry_mesh().volume() * 1e-9
        );
    }

    std::cout << fmt::format(
        "curved fold: {} sectors, {} lamellas, {} contacts\n",
        fold.sectors,
        2 * fold.sectors,
        fold.get_contacts().size()
    );
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the developable folding templates: a curved folded roof, a freestanding folded shell roof of 32 m diameter rising into a tower 26 m high with a round top of 7 m. Twelve sectors, their valleys running down to the columns at 4000 and their ridges ending in a gable 4000 over the eaves, the folds following a trumpet curve, flat at the eaves and vertical at the tower, their depth shrinking with the radius into the tower, the top cut level at 26 m. Every ridge and valley is a NURBS curve interpolated through fourteen rings, each the same star scaled, and every fold strip between two of them one lamella, a PlateCurved: a closed curved solid of six exact NURBS patches, its outer face the ruled surface between the two meridians, its inner face 120 mm in between the inner meridians, where the neighbouring strips offset by the thickness meet, its side faces along the folds shared with the neighbours as miters, and a cap at the eaves and at the top: 24 curved solids, no facets. Each step is a group: meridians, lamellas, the lamellas named lamella_<j>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_1_curved_fold --parallel 6 && ./build/templates_developable_folding_1_curved_fold && ./bash/publish-scene.sh --target templates_developable_folding_1_curved_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
