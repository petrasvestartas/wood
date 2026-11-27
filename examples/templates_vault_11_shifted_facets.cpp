#include "wood_session.h"
#include "src/templates/vault/surface_vault.h"

using namespace session_cpp;
using namespace wood_session;

struct ShiftedCase {
    std::string name;
    wood_vault::SurfaceVaultGuide guide;
};

/// Four NGon vaults whose top and bottom facets shift relative to each other while side joints stay planar.
int main() {

    WoodSession wood_session("vault_shifted_facets");

    const std::vector<ShiftedCase> cases = {
        {"shifted_hex",       wood_vault::SurfaceVaultGuide::shifted_hex_vault()},
        {"bevelled_hex_dome", wood_vault::SurfaceVaultGuide::bevelled_hex_dome()},
        {"shifted_groin",     wood_vault::SurfaceVaultGuide::shifted_diamond_groin()},
        {"shifted_chebyshev", wood_vault::SurfaceVaultGuide::shifted_chebyshev_dome()}
    };

    const double pitch_x = 8600.0;
    const double pitch_y = 8600.0;

    for (size_t i = 0; i < cases.size(); ++i) {
        const std::shared_ptr<TreeNode> group = wood_session.add_group(cases[i].name);
        const int col = static_cast<int>(i % 2);
        const int row = static_cast<int>(i / 2);
        const double offset_x = (static_cast<double>(col) - 0.5) * pitch_x;
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
        "shifted facets: {} blocks, {} contacts\n",
        wood_session.objects.elements->size(),
        wood_session.graph.number_of_edges()
    );
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four volumetric NGon block vaults whose top (extrados) and bottom (intrados) facets shift relative to each other — by tangential surface offset before side-quad planarization and by alternating edge-plane bevels — while maintaining exact planar side joints and planar top/bottom caps.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_11_shifted_facets --parallel 6 && ./build/templates_vault_11_shifted_facets && ../bash/publish-scene.sh --target templates_vault_11_shifted_facets

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
