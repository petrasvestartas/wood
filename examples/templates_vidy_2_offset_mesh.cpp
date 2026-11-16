#include "wood_session.h"
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

/// Stage 2 — Four-surface double-layer miter offset meshes (0, 45, 255 and 300 mm) across the 210 mm Vidy cavity.
int main() {

    WoodSession scene("templates_vidy_2_offset_mesh");

    const wood_vidy::VidyGuide full_guide(11);
    scene.add_mesh(full_guide.base_mesh(), scene.add_group("inner_surface"));
    scene.add_mesh(full_guide.outer_mesh(), scene.add_group("outer_surface"));

    const wood_vidy::VidyGuide bay_guide(2);
    const std::shared_ptr<TreeNode> bay_shells = scene.add_group("two_bay_shells");
    scene.add_mesh(bay_guide.base_mesh(), bay_shells);
    scene.add_mesh(bay_guide.inner_cavity_mesh(), bay_shells);
    scene.add_mesh(bay_guide.outer_cavity_mesh(), bay_shells);
    scene.add_mesh(bay_guide.outer_mesh(), bay_shells);

    std::cout << fmt::format(
        "vidy offset meshes: {} inner/outer facets across {:.0f} mm shell ({:.0f} mm cavity)\n",
        full_guide.facets().size(),
        full_guide.total_offset,
        full_guide.cavity_gap()
    );
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Stage 2 double-layer miter offset meshes of the Théâtre de Vidy pavilion: exact bisector offsets at h = 0, 45, 255 and 300 mm with flush z = 0 ground bases and vertical XZ bay-separation planes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_2_offset_mesh --parallel 6 && ./build/templates_vidy_2_offset_mesh && ../bash/publish-scene.sh --target templates_vidy_2_offset_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
