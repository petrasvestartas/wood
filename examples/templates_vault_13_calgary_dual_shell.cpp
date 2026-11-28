#include "wood_session.h"
#include "src/templates/vault/tile_pattern.h"

using namespace laika_cpp;
using namespace wood_session;

struct CalgaryCase {
    std::string name;
    wood_vault::CalgaryVaultGuide guide;
};

/// Three materialisations of the Calgary 2025 four-pier asymmetric thrust shell (variable-thickness flat-top blocks, chamfered trivalent corners, and interlocking bevelled blocks).
int main() {

    WoodSession wood_session("vault_calgary_dual_shell");

    const std::vector<CalgaryCase> cases = {
        {"calgary_blocks",    wood_vault::CalgaryVaultGuide::variable_thickness_blocks()},
        {"calgary_chamfered", wood_vault::CalgaryVaultGuide::chamfered_blocks()},
        {"calgary_bevelled",  wood_vault::CalgaryVaultGuide::interlocking_bevel_blocks()}
    };

    const double pitch_x = 12200.0;

    for (size_t i = 0; i < cases.size(); ++i) {
        const std::shared_ptr<TreeNode> group = wood_session.add_group(cases[i].name);
        const double offset_x = (static_cast<double>(i) - 1.0) * pitch_x;
        const Xform shift = Xform::translation(offset_x, 0.0, 0.0);

        wood_vault::CalgaryDualMetrics metrics;
        const std::vector<std::shared_ptr<Element>> elements =
            wood_vault::calgary_vault(cases[i].guide, &metrics);
        for (const std::shared_ptr<Element>& elem : elements) {
            elem->place(shift);
            wood_session.add(elem, group);
        }
        wood_session.assemblies.push_back(
            AssemblyModule::raise(cases[i].name, AssemblyModule::parts_of(elements))
        );

        std::cout << fmt::format(
            "{}: blocks={}, supports={}, chamfers={}, thickness=[{:.1f}, {:.1f}] mm, side_planarity={:.2e} mm, top_planarity={:.2e} mm\n",
            cases[i].name,
            elements.size(),
            metrics.support_blocks,
            metrics.chamfer_cuts,
            metrics.min_thickness,
            metrics.max_thickness,
            metrics.side_planarity,
            metrics.top_cap_planarity
        );
    }

    wood_session.compute_face_contacts(0);
    std::cout << fmt::format(
        "calgary dual shell: {} elements, {} contacts\n",
        wood_session.objects.elements->size(),
        wood_session.graph.number_of_edges()
    );
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Three stereotomic dual-shell materialisations of the 9.5 m x 11.2 m x 3.0 m four-pier asymmetric Calgary 2025 thrust network shell (BRG-teaching/Calgary2025): boundary-preserving isotropic triangle remeshing, boundary-inclusive trivalent dualization with NURBS-smoothed arch borders, Delaunay variable thickness interpolation (300 mm lower piers, 200 mm upper hillside piers, 150 mm arch midspans, 100 mm crown), flat-top stepped extrados blocks with flush intrados, chamfered trivalent corners, and alternating interlocking side-plane bevels.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_13_calgary_dual_shell --parallel 6 && ./build/templates_vault_13_calgary_dual_shell && ../bash/publish-scene.sh --target templates_vault_13_calgary_dual_shell

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
