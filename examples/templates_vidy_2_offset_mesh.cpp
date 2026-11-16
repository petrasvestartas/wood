#include "wood_session.h"
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

/// Stage 2 — Four concentric mitered offset surfaces (h = 0, 45, 255, 300 mm) and 236 miter connection planes across the Théâtre de Vidy pavilion.
int main() {

    wood_vidy::VidyGuide guide(11);

    const wood_vidy::VidyGuide two_bay(2);
    const std::shared_ptr<TreeNode> shells_group = guide.add_group("two_bay_shells");
    guide.add_mesh(two_bay.base_mesh(), shells_group);
    guide.add_mesh(two_bay.inner_cavity_mesh(), shells_group);
    guide.add_mesh(two_bay.outer_cavity_mesh(), shells_group);
    guide.add_mesh(two_bay.outer_mesh(), shells_group);
    guide.add_mesh(two_bay.miter_planes_mesh(), shells_group);

    std::cout << fmt::format(
        "vidy offset shells: 4 concentric meshes ({} faces each) + {} miter connection planes, cavity = {:.0f} mm\n",
        guide.outer_mesh().number_of_faces(),
        guide.miter_planes_mesh().number_of_faces(),
        guide.cavity_gap()
    );
    guide.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Stage 2 double-layer miter offset surfaces and miter connection planes of the Théâtre de Vidy pavilion: four concentric shell meshes at h = {0, 45, 255, 300} mm together with all 236 angle-bisector and vertical XZ miter ribbons.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_2_offset_mesh --parallel 6 && ./build/templates_vidy_2_offset_mesh && ../bash/publish-scene.sh --target templates_vidy_2_offset_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
