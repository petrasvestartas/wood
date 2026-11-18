#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Vidy 45 mm CLT panels: 308 shell plates + 154 full-corner border boards = 462 plates.
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
        "vidy 3: {} plates ({} inner + {} outer + {} borders, {:.0f} mm)\n",
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
Vidy 3: 308 x 45 mm shell plates + 154 x 45 x 210 mm full-corner border boards = 462 plates (84 in 2-bay module).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_3_panels --parallel 6 && ./build/templates_vidy_3_panels && ../bash/publish-scene.sh --target templates_vidy_3_panels

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
