#include "wood_session.h"
#include "src/templates/vault/surface_vault.h"

using namespace session_cpp;
using namespace wood_session;

struct SurfaceCase {
    std::string name;
    wood_vault::SurfaceVaultGuide guide;
};

/// Six NURBS and multi-surface BRep NGon vaults with planarized side joints and planar caps.
int main() {

    WoodSession wood_session("vault_surface_subdivisions");

    const std::vector<SurfaceCase> cases = {
        {"hex_saddle",     wood_vault::SurfaceVaultGuide::hex_saddle_vault()},
        {"diamond_torus",  wood_vault::SurfaceVaultGuide::diamond_torus_vault()},
        {"sewn_groin",     wood_vault::SurfaceVaultGuide::sewn_groin_vault()},
        {"sewn_funnel",    wood_vault::SurfaceVaultGuide::sewn_funnel_vault()},
        {"courses_funnel", wood_vault::SurfaceVaultGuide::courses_funnel_vault()},
        {"chebyshev_dome", wood_vault::SurfaceVaultGuide::chebyshev_dome_vault()}
    };

    const double pitch_x = 9200.0;
    const double pitch_y = 9200.0;

    for (size_t i = 0; i < cases.size(); ++i) {
        const std::shared_ptr<TreeNode> group = wood_session.add_group(cases[i].name);
        const int col = static_cast<int>(i % 3);
        const int row = static_cast<int>(i / 3);
        const double offset_x = (static_cast<double>(col) - 1.0) * pitch_x;
        const double offset_y = (0.5 - static_cast<double>(row)) * pitch_y;
        const Xform shift = Xform::translation(offset_x, offset_y, 0.0);

        wood_vault::NgonSolidMetrics metrics;
        const std::vector<std::shared_ptr<Element>> blocks =
            wood_vault::surface_vault(cases[i].guide, &metrics);
        for (const std::shared_ptr<Element>& block : blocks) {
            block->place(shift);
            wood_session.add(block, group);
        }
        wood_session.assemblies.push_back(
            AssemblyModule::raise(cases[i].name, AssemblyModule::parts_of(blocks))
        );

        std::cout << fmt::format(
            "{}: {} blocks, side_planarity={:.2e} mm, cap_planarity={:.2e} mm\n",
            cases[i].name,
            blocks.size(),
            metrics.side_planarity,
            metrics.cap_planarity
        );
    }

    wood_session.compute_face_contacts(0);
    std::cout << fmt::format(
        "surface subdivisions: {} blocks, {} contacts\n",
        wood_session.objects.elements->size(),
        wood_session.graph.number_of_edges()
    );
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Six NURBS and sewn multi-surface BRep block vaults subdivided by SurfaceDivision (running-bond hexagons, diamonds, sewn groin seams, dual hexagons across multi-patch funnels, variable-count staggered brick courses, and 45-degree equal-edge Chebyshev compass nets) and extruded into planar-sided stereotomic blocks via conjugate-gradient side planarization and ProjectPairs cap projection.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_10_surface_subdivisions --parallel 6 && ./build/templates_vault_10_surface_subdivisions && ../bash/publish-scene.sh --target templates_vault_10_surface_subdivisions

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
