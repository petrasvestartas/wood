#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Full 11-bay Théâtre de Vidy double-layer through-tenon joinery, cavity border boards and module assembly.
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

    const std::shared_ptr<TreeNode> corner_node = pavilion.add_group("eave_joint_node");
    const size_t mid_base = 14 * 5;

    for (const size_t loc : {size_t{0}, size_t{1}, size_t{4}, size_t{5}, size_t{9}}) {
        pavilion.add(pavilion.inner_plates()[mid_base + loc]->clone(), corner_node);
        pavilion.add(pavilion.outer_plates()[mid_base + loc]->clone(), corner_node);

        for (const std::shared_ptr<FacePlanarElement>& board :
             pavilion.border_plates_for_facet(mid_base + loc))
            pavilion.add(board->clone(), corner_node);
    }

    pavilion.add_module_assembly_groups(5, 1650.0);

    std::cout << fmt::format(
        "vidy full pavilion joinery: {} total plates ({} shell + {} border across {} bays), {} jointed seams, {} pins ({} Double_Pins + {} Single/Extend_Pins), {} mortise slots\n",
        pavilion.all_plates().size(),
        pavilion.shell_plates().size(),
        pavilion.border_plates().size(),
        full_guide.bays,
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
Stage 4 double-layer Théâtre de Vidy timber joinery across all 462 plates of the 11-bay pavilion (308 folded shell plates + 154 perpendicular cavity-closing border boards): stepped chamfered Double_Pins bridging the 210 mm cavity, interleaved Single_Pins, vertical +Z on-site eave joints, first-row verification arch, and six-step module assembly sequence.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_4_section_joints --parallel 6 && ./build/templates_vidy_4_section_joints && ../bash/publish-scene.sh --target templates_vidy_4_section_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
