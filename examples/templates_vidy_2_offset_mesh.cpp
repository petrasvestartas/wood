#include "wood_session.h"
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

/// Vidy miter offset shells at h = {0, 45, 255, 300} mm and 236 miter planes.
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
        "vidy 2: 4 shells x {} faces, {} miter planes, {:.0f} mm cavity\n",
        guide.outer_mesh().number_of_faces(),
        guide.miter_planes_mesh().number_of_faces(),
        guide.cavity_gap()
    );
    guide.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vidy 2: 4 offset shells at h = {0, 45, 255, 300} mm (210 mm cavity), 236 miter planes (176 fold, 50 bay XZ, 10 gable XZ).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_2_offset_mesh --parallel 6 && ./build/templates_vidy_2_offset_mesh && ../bash/publish-scene.sh --target templates_vidy_2_offset_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
