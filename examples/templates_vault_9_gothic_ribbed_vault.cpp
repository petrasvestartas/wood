#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

const double SPAN_X = 10000.0;
const double SPAN_Y = 10000.0;
const double RISE = 1000.0;
const double RIB_WIDTH = 120.0;
const double RIB_HEIGHT = 360.0;
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
        {"tierceron_exeter",      wood_vault::GothicPattern::tierceron,       wood_vault::ArchMethod::groin_cylinders},
        {"star_stellar_burgos",   wood_vault::GothicPattern::star_stellar,    wood_vault::ArchMethod::groin_cylinders},
        {"lierne_star_ely",       wood_vault::GothicPattern::lierne_star,     wood_vault::ArchMethod::groin_cylinders},
        {"net_reticulated_parler",wood_vault::GothicPattern::net_reticulated, wood_vault::ArchMethod::groin_cylinders},
        {"fan_conoid_gloucester", wood_vault::GothicPattern::fan_conoid,      wood_vault::ArchMethod::groin_cylinders}
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
            elem->place(shift);
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
Six symmetric shallow (1:10 rise-to-span, 10 m x 10 m span, 1 m rise) Gothic cathedral ribbed vault bays built from single-curved planar glulam beams (BeamCurved, 120 x 360 mm, 6 lamellae) in vertical planes over straight 2D plan lines with exact angle-bisector node miter cuts (rib->cuts), 4 polygonal corner support blocks (Block) where ribs and web panels terminate cleanly, and simple cylindrical barrel web panels (BRep, K = 0 groin_cylinders) whose bottom and top edges lie directly above the supporting beam centerlines (Bern Cathedral / Berner Muenster; compas_tno; Tracing the Past LiDAR survey, Buchanan, Webb & Peterson 2021; Wendland et al. 2024; Jobbik 2024):
1. quadripartite_wells    — Classic quadripartite cross vault on intersecting barrel cylinders (Bern Cathedral / Wells / Salisbury, 4 blocks + 10 ribs + 8 cylindrical web panels = 22 elements)
2. tierceron_exeter       — 4-pointed starcut tierceron vault on intersecting barrel cylinders (Bern Cathedral / Lincoln / Exeter, 4 blocks + 18 ribs + 16 cylindrical web panels = 38 elements)
3. star_stellar_burgos    — 4-lozenge Greek-cross stellar vault (Sterngewoelbe / Boveda Estrellada) on intersecting barrel cylinders (Bern Cathedral, 4 blocks + 26 ribs + 20 cylindrical web panels = 50 elements)
4. lierne_star_ely        — Double-star tierceron-lierne vault on intersecting barrel cylinders (Bern Cathedral / Ely Choir / Wells Lady Chapel, 4 blocks + 34 ribs + 32 cylindrical web panels = 70 elements)
5. net_reticulated_parler — Reticulated diamond-lattice net vault (Netzgewoelbe) on intersecting barrel cylinders (Bern Cathedral Choir / Parler, 4 blocks + 34 ribs + 32 cylindrical web panels = 70 elements)
6. fan_conoid_gloucester  — Radiating fan vault with straight horizontal collar ribs along cylinder rulings (compas_tno / Gloucester, 4 blocks + 50 ribs + 48 cylindrical web panels = 102 elements)

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
