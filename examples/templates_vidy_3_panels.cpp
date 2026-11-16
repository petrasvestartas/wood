#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Stage 3 — Mitered double-layer 45 mm 5-ply CLT panels (308 unique plate shapes with non-colliding corner chamfers) across the 11-bay Théâtre de Vidy pavilion.
int main() {

    const wood_vidy::VidyGuide guide(11);
    wood_vidy::VidyPavilion pavilion(guide, false);

    // Complete 2-bay double-layer section (all 56 inner and outer panels present, zero missing plates).
    const wood_vidy::VidyGuide two_bay_guide(2);
    const wood_vidy::VidyPavilion two_bay(two_bay_guide, false);
    const std::shared_ptr<TreeNode> two_bay_group = pavilion.add_group("two_bay_panels");
    for (size_t f = 0; f < two_bay.guide.facets().size(); f++) {
        pavilion.add(two_bay.inner_plates()[f]->clone(), two_bay_group);
        pavilion.add(two_bay.outer_plates()[f]->clone(), two_bay_group);
    }

    std::cout << fmt::format(
        "vidy double-layer panels: {} CLT plates ({} inner + {} outer, {:.0f} mm thick)\n",
        pavilion.all_plates().size(),
        pavilion.inner_plates().size(),
        pavilion.outer_plates().size(),
        guide.plate_thickness
    );
    pavilion.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Stage 3 offset mesh with all 308 mitered 45 mm CLT panels of the 11-bay Théâtre de Vidy pavilion (220 antiprismatic roof triangles and 88 V-pleated vertical wall quads) with angle-proportional corner chamfers preventing multi-valent corner collisions, plus a complete 56-panel two-bay double-layer module.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_3_panels --parallel 6 && ./build/templates_vidy_3_panels && ../bash/publish-scene.sh --target templates_vidy_3_panels

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
