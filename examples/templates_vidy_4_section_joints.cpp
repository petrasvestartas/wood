#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Vidy 11-bay joinery: 462 plates, 176 seams, single-axis through-tenons and 6-step module assembly.
int main() {

    const wood_vidy::VidyGuide full_guide(11);
    wood_vidy::VidyPavilion pavilion(full_guide, true, 1.0, 500.0);
    const std::shared_ptr<TreeNode> first_row = pavilion.add_group("first_row_section");

    for (size_t loc = 0; loc < 14; loc++) {
        pavilion.add(pavilion.inner_plates()[loc]->clone(), first_row);
        pavilion.add(pavilion.outer_plates()[loc]->clone(), first_row);

        for (const std::shared_ptr<FacePlanarElement>& board :
             pavilion.border_plates_for_facet(loc))
            pavilion.add(board->clone(), first_row);
    }

    pavilion.add_module_assembly_groups(5, 1650.0);

    std::cout << fmt::format(
        "vidy 4: {} plates ({} shell + {} border), {} seams, {} pins ({} double + {} single), {} slots\n",
        pavilion.all_plates().size(),
        pavilion.shell_plates().size(),
        pavilion.border_plates().size(),
        pavilion.total_jointed_seams(),
        pavilion.total_pins(),
        pavilion.total_double_pins(),
        pavilion.total_single_pins(),
        pavilion.total_mortise_slots()
    );
    pavilion.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vidy 4: 462 plates, 176 seams, 500 mm pitch, Double_Pin 270/180/140 mm, Single_Pin 180/140 mm, +Z eave tenons, 6-step assembly.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_4_section_joints --parallel 6 && ./build/templates_vidy_4_section_joints && ../bash/publish-scene.sh --target templates_vidy_4_section_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
