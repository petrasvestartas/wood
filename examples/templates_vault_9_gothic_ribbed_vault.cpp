#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN_X = 10000.0;
const double SPAN_Y = 10000.0;
const double RISE = 1000.0;
const double RIB_WIDTH = 120.0;
const double RIB_HEIGHT = 180.0;
const int SUBDIVISIONS = 16;

struct GothicBayConfig {
    std::string name;
    wood_vault::GothicPattern pattern;
    wood_vault::ArchMethod surface;
};

int main() {

    WoodSession wood_session("vault_gothic_ribbed_vault");

    const std::vector<GothicBayConfig> bays = {
        {"quadripartite_wells",   wood_vault::GothicPattern::quadripartite,   wood_vault::ArchMethod::groin_cylinders},
        {"tierceron_exeter",      wood_vault::GothicPattern::tierceron,       wood_vault::ArchMethod::spherical_sail},
        {"star_stellar_burgos",   wood_vault::GothicPattern::star_stellar,    wood_vault::ArchMethod::cloister_cylinders},
        {"lierne_star_ely",       wood_vault::GothicPattern::lierne_star,     wood_vault::ArchMethod::spherical_sail},
        {"net_reticulated_parler",wood_vault::GothicPattern::net_reticulated, wood_vault::ArchMethod::groin_cylinders},
        {"fan_conoid_gloucester", wood_vault::GothicPattern::fan_conoid,      wood_vault::ArchMethod::spherical_sail}
    };

    const double pitch_x = SPAN_X + 1800.0;
    const double pitch_y = SPAN_Y + 1800.0;

    for (size_t i = 0; i < bays.size(); ++i) {
        const std::shared_ptr<TreeNode> group = wood_session.add_group(bays[i].name);
        const int col = static_cast<int>(i % 3);
        const int row = static_cast<int>(i / 3);
        const double offset_x = (static_cast<double>(col) - 1.0) * pitch_x;
        const double offset_y = (0.5 - static_cast<double>(row)) * pitch_y;
        const Xform shift = Xform::translation(offset_x, offset_y, 0.0);

        for (const std::shared_ptr<Element>& elem : wood_vault::gothic_ribbed_vault(
                 SPAN_X,
                 SPAN_Y,
                 RISE,
                 RIB_WIDTH,
                 RIB_HEIGHT,
                 bays[i].pattern,
                 bays[i].surface,
                 SUBDIVISIONS
             )) {
            if (auto rib = std::dynamic_pointer_cast<BeamCurved>(elem)) {
                rib->centreline.transform(shift);
                for (Plane& cut : rib->cuts)
                    cut.transform(shift);
                rib->invalidate_geometry();
            } else if (auto block = std::dynamic_pointer_cast<Block>(elem)) {
                for (Polyline& pl : block->loops)
                    pl.transform(shift);
                block->invalidate_geometry();
            }
            wood_session.add(elem, group);
        }

        if (std::getenv("DUMP_SINGLE_BAYS") != nullptr) {
            wood_vault::GothicRibbedVault single_bay(
                SPAN_X,
                SPAN_Y,
                RISE,
                RIB_WIDTH,
                RIB_HEIGHT,
                bays[i].pattern,
                bays[i].surface,
                SUBDIVISIONS,
                bays[i].name
            );
            single_bay.pb_dump(fmt::format("/tmp/{}.pb", bays[i].name));
        }
    }

    std::cout << fmt::format(
        "gothic ribbed vault: {} elements (curved glulam ribs + corner support blocks) across {} historical cathedral bays\n",
        wood_session.objects.elements->size(),
        bays.size()
    );
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Six symmetric shallow (1:10 rise-to-span, 10 m x 10 m span, 1 m rise) Gothic cathedral ribbed vault bays built from curved glulam beams (BeamCurved, 120 x 180 mm, 6 lamellae) with exact angle-bisector node miter cuts (rib->cuts), 4 polygonal corner support blocks (Block) where ribs terminate without intersecting, and analytical top vault surfaces (groin_cylinders, cloister_cylinders, spherical_sail) flush with the upper rib extrados for curved or segmented timber infill panels (Tracing the Past LiDAR survey, Buchanan, Webb & Peterson 2021; Wendland et al. 2024; Jobbik 2024):
1. quadripartite_wells    — Early Gothic quadripartite vault on intersected groin cylinders (Notre-Dame / Wells / Salisbury, 12 ribs + 4 blocks)
2. tierceron_exeter       — English Decorated tierceron palm-tree vault on a spherical sail (Lincoln / Exeter, 28 ribs + 4 blocks)
3. star_stellar_burgos    — 8-pointed stellar vault (Sterngewolbe / Boveda Estrellada) on cloister cylinders (36 ribs + 4 blocks)
4. lierne_star_ely        — Double-star tierceron + lierne vault on a spherical sail (Ely Choir / Wells Lady Chapel, 52 ribs + 4 blocks)
5. net_reticulated_parler — Late-Gothic reticulated diamond-lattice net vault (Netzgewolbe) on groin cylinders (Parler / Jobbik 2024, 44 ribs + 4 blocks)
6. fan_conoid_gloucester  — English Perpendicular radiating fan vault with 3 concentric rib tiers (Gloucester / King's College, 68 ribs + 4 blocks)

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vault_9_gothic_ribbed_vault --parallel 4 && ./build/templates_vault_9_gothic_ribbed_vault && ../bash/publish-scene.sh --target templates_vault_9_gothic_ribbed_vault

|||||||| WORKFLOW ||||||||
examples/templates_vault_9_gothic_ribbed_vault.cpp
 |
 |-- wood_vault::gothic_ribbed_vault(SPAN_X, SPAN_Y, RISE, RIB_WIDTH, RIB_HEIGHT, pattern, surface, ...)   src/templates/vault/vault.cpp
 |-- add(elem, group) for every corner support Block and mitered BeamCurved rib
 '-- pb_dump(pb_path("live"))                    data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
