#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Mitered double-layer 45 mm CLT panels (308 shell plates + 154 cavity border boards) of the Vidy pavilion.
int main() {

    const wood_vidy::VidyGuide guide(11);
    wood_vidy::VidyPavilion pavilion(guide, false);
    const wood_vidy::VidyGuide two_bay_guide(2);
    const wood_vidy::VidyPavilion two_bay(two_bay_guide, false);
    const std::shared_ptr<TreeNode> two_bay_group = pavilion.add_group("two_bay_panels");

    for (size_t f = 0; f < two_bay.guide.facets().size(); f++) {
        pavilion.add(two_bay.inner_plates()[f]->clone(), two_bay_group);
        pavilion.add(two_bay.outer_plates()[f]->clone(), two_bay_group);

        for (const std::shared_ptr<FacePlanarElement>& board : two_bay.border_plates_for_facet(f))
            pavilion.add(board->clone(), two_bay_group);
    }

    std::cout << fmt::format(
        "vidy double-layer panels: {} total plates ({} inner + {} outer + {} border boards, {:.0f} mm thick)\n",
        pavilion.all_plates().size(),
        pavilion.inner_plates().size(),
        pavilion.outer_plates().size(),
        pavilion.border_plates().size(),
        guide.plate_thickness
    );
    pavilion.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Stage 3 double-layer 45 mm CLT panels of the 11-bay Théâtre de Vidy pavilion: 308 mitered folded shell plates with angle-proportional corner chamfers plus 154 perpendicular 45 x 210 mm cavity-closing border boards along bay separations, gables and ground sills (462 plates total), together with an 84-plate two-bay module.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_3_panels --parallel 6 && ./build/templates_vidy_3_panels && ../bash/publish-scene.sh --target templates_vidy_3_panels

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
