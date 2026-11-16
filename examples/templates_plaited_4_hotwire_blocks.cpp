#include "wood_session.h"
#include "src/templates/plaited/plaited.h"

using namespace session_cpp;
using namespace wood_session;

const double SHELL_SPAN = 4200.0;
const double THICKNESS = 200.0;
const double BEVEL = 0.46;
const double OPENING = 0.45;

/// Open hot-wire reciprocal block vaults (after Fallacara Stage 5 and Schwartz / HAL Robotics): flush-bounded trapezoidal bar blocks.
int main() {

    WoodSession scene("templates_plaited_4_hotwire_blocks");

    const Mesh abeille_shell =
        wood_plaited::flat_quad_mesh(SHELL_SPAN, SHELL_SPAN, 6, 6, 0.0, THICKNESS);
    const wood_plaited::PlaitedStereotomy abeille_open(
        abeille_shell,
        wood_plaited::PlaitedPattern::Abeille,
        THICKNESS,
        BEVEL,
        0.0,
        false,
        false,
        true,
        OPENING,
        false
    );
    scene.graft(abeille_open.get_branch("blocks"), scene.add_group("abeille_open"));

    Mesh sphera_shell = wood_plaited::spherical_cap_hex_mesh(5200.0, 1350.0, 3, true);
    sphera_shell.transform(Xform::translation(SHELL_SPAN + 2400.0, 0.0, 0.0));
    const wood_plaited::PlaitedStereotomy sphera_open(
        sphera_shell,
        wood_plaited::PlaitedPattern::Sphera,
        190.0,
        0.42,
        0.0,
        false,
        false,
        true,
        0.44,
        true
    );
    scene.graft(sphera_open.get_branch("blocks"), scene.add_group("sphera_open"));

    std::cout << fmt::format(
        "hot-wire reciprocal blocks: abeille_open {} blocks ({} contacts), sphera_open {} blocks ({} contacts)\n",
        abeille_open.blocks().size(),
        abeille_open.graph.number_of_edges(),
        sphera_open.blocks().size(),
        sphera_open.graph.number_of_edges()
    );
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Open-lattice robotic hot-wire reciprocal block stereotomy (opening > 0, after Fallacara's cover progression and Thibault Schwartz / HAL Robotics EPS vaults): trapezoidal-section bar blocks with flush perimeter edges and tight T-junctions, shown on a flat square-pinwheel Abeille bay and a doubly curved triangular-pinwheel Sphera dome.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_plaited_4_hotwire_blocks --parallel 6 && ./build/templates_plaited_4_hotwire_blocks && ../bash/publish-scene.sh --target templates_plaited_4_hotwire_blocks

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
