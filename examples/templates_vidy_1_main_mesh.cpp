#include "wood_session.h"
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

/// Vidy base mesh: 127 vertices, 154 planar facets, 12 bay axes.
int main() {

    wood_vidy::VidyGuide guide(11);
    size_t reg_count = 0;
    size_t vcon_count = 0;
    double max_beta = 0.0;

    for (const wood_vidy::VidyEdge& edge : guide.edges()) {
        if (edge.kind == wood_vidy::VidyEdgeKind::Regular) {
            reg_count++;
            max_beta = std::max(max_beta, edge.tool_tilt_deg);
        } else if (edge.kind == wood_vidy::VidyEdgeKind::BaySeparation) {
            vcon_count++;
        }
    }

    std::cout << fmt::format(
        "vidy 1: {} verts, {} facets, {} folds (max {:.1f} deg), {} bay axes\n",
        guide.base_mesh().number_of_vertices(),
        guide.facets().size(),
        reg_count,
        max_beta,
        vcon_count
    );
    guide.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vidy 1: 28.7 x 19.1 x 10.3 m, 11 bays x 2609.6 mm, 127 vertices, 110 roof triangles + 44 vertical wall quads = 154 facets.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_1_main_mesh --parallel 6 && ./build/templates_vidy_1_main_mesh && ../bash/publish-scene.sh --target templates_vidy_1_main_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
