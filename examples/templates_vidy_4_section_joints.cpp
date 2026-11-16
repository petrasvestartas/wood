#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Stage 4 — Double-layer Théâtre de Vidy linked-tenon and shadow-joint system (side_side_outplane_linked_tenons + side_side_outplane_linked_tenons_wall).
int main() {

    const wood_vidy::VidyGuide section_guide(1);
    wood_vidy::VidyPavilion section(section_guide, true, 0.0, 350.0);

    // Cutaway group hiding three outer haunch roof plates so the 210 mm cavity and cross-layer shadow tenons are exposed.
    const std::shared_ptr<TreeNode> cavity_view = section.add_group("cavity_cutaway");
    for (size_t f = 0; f < section.guide.facets().size(); f++) {
        section.add(section.inner_plates()[f]->clone(), cavity_view);
        const size_t loc = section.guide.facets()[f].local_index;
        if (loc != 4 && loc != 5 && loc != 6) {
            section.add(section.outer_plates()[f]->clone(), cavity_view);
        }
    }

    // Close-up 10-plate Vidy eave & haunch node: V-pleated left walls (0, 1) meeting three sloped roof triangles (4, 5, 9).
    const std::shared_ptr<TreeNode> corner_node = section.add_group("eave_joint_node");
    for (const size_t f : {size_t{0}, size_t{1}, size_t{4}, size_t{5}, size_t{9}}) {
        section.add(section.inner_plates()[f]->clone(), corner_node);
        section.add(section.outer_plates()[f]->clone(), corner_node);
    }

    std::cout << fmt::format(
        "vidy double-layer section joinery: {} plates, {} through-mortise slots\n",
        section.all_plates().size(),
        section.total_mortise_slots()
    );
    section.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Stage 4 double-layer Théâtre de Vidy timber joinery on a full-scale 28-plate bay arch section: native side_side_outplane_linked_tenons on roof/eave creases, side_side_outplane_linked_tenons_wall on V-pleated wall seams, and cross-layer Vidy shadow joints bridging the 210 mm hollow cavity.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_4_section_joints --parallel 6 && ./build/templates_vidy_4_section_joints && ../bash/publish-scene.sh --target templates_vidy_4_section_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
