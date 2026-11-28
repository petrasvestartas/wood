#include "src/templates/delorme/delorme.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_delorme;

/// Philibert de l'Orme (1561) three-rib small-timber arch bay reconstructing the woodcut of Nouvelles inventions pour bien bastir et a petits fraiz.
int main() {

    DelormeParams params;
    params.kind = DelormeKind::SemicircularBarrel;
    params.span = 3600.0;
    params.rise = 1650.0;
    params.length = 1800.0;
    params.n_ribs = 3;
    params.n_plies = 2;
    params.plank_length = 1120.0;
    params.rib_depth = 220.0;
    params.plank_thickness = 34.0;
    params.stub_tenon_length = 28.0;
    params.lierne_spacing = 880.0;
    params.lierne_overhang = 150.0;
    params.clavette_length = 156.0;
    params.cheville_radius = 8.0;

    DelormeSystem scene(DelormeGuide(params), "templates_delorme_1_arch_detail");

    std::cout << fmt::format(
        "Delorme 1561 bay: {} planks, {} liernes, {} clavettes, {} chevilles\n",
        scene.planks().size(),
        scene.liernes().size(),
        scene.clavettes().size(),
        scene.chevilles().size()
    );
    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Philibert de l'Orme's 1561 charpente a petits bois reconstructed directly from Nouvelles inventions pour bien bastir et a petits fraiz: three two-ply arch ribs assembled from short edge-standing flat planks in running-bond (quinconce) stagger with central stub-tenon butt joints (tenon et entaille), wooden alignment pegs (chevilles), continuous transverse binding liernes threading aligned rectangular mortises, and twin opposing tapered wedges (clavettes) driven tightly against both outer cheeks of every rib.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_delorme_1_arch_detail --parallel 6 && ./build/templates_delorme_1_arch_detail && ../bash/publish-scene.sh --target templates_delorme_1_arch_detail

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
