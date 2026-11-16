#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Stage 4 — Full 11-bay (308-plate) Théâtre de Vidy double-layer through-tenon joinery, first-row verification section, eave node, and 4-step erection sequence.
int main() {

    const wood_vidy::VidyGuide full_guide(11);
    wood_vidy::VidyPavilion pavilion(full_guide, true, 1.0, 500.0);

    // First-row verification section (Bay 0 = 28 jointed plates, matching vidy_one_section_with_joints.3dm).
    const std::shared_ptr<TreeNode> first_row = pavilion.add_group("first_row_section");
    for (size_t loc = 0; loc < 14; loc++) {
        pavilion.add(wood_vidy::VidyPavilion::copy_plate(*pavilion.inner_plates()[loc]), first_row);
        pavilion.add(wood_vidy::VidyPavilion::copy_plate(*pavilion.outer_plates()[loc]), first_row);
    }

    // Close-up eave & haunch node on Bay 5 (all 10 inner & outer plates; outer eave lifted to expose cavity spacers).
    const std::shared_ptr<TreeNode> corner_node = pavilion.add_group("eave_joint_node");
    const size_t mid_base = 14 * 5;
    for (const size_t loc : {size_t{0}, size_t{1}, size_t{4}, size_t{5}, size_t{9}}) {
        pavilion.add(
            wood_vidy::VidyPavilion::copy_plate(*pavilion.inner_plates()[mid_base + loc]),
            corner_node
        );
        const Vector lift =
            (loc == 4) ? (full_guide.facets()[mid_base + 4].normal * 680.0) : Vector(0.0, 0.0, 0.0);
        pavilion.add(
            wood_vidy::VidyPavilion::copy_plate(*pavilion.outer_plates()[mid_base + loc], lift),
            corner_node
        );
    }

    // Gradual 4-stage real-building erection sequence (1: vertical walls, 2: roof bottom, 3: roof top, 4: roof onto walls).
    pavilion.add_erection_sequence_groups(5);

    std::cout << fmt::format(
        "vidy full pavilion joinery: {} jointed CLT plates across {} bays, {} through-mortise slots\n",
        pavilion.all_plates().size(),
        full_guide.bays,
        pavilion.total_mortise_slots()
    );
    pavilion.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Stage 4 double-layer Théâtre de Vidy timber joinery across all 308 CLT plates of the 11-bay pavilion: stepped chamfered Double_Pins bridging the 210 mm cavity, interleaved Single_Pins, vertical +Z on-site eave joints, first-row verification arch, and four-stage gradual erection sequence.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_4_section_joints --parallel 6 && ./build/templates_vidy_4_section_joints && ../bash/publish-scene.sh --target templates_vidy_4_section_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
