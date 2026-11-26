#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"
#include "src/templates/folding/prismatic_fold.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_folding;

/// Long deep double-layer CLT folded-plate halls with staggered Vidy tenons and corner borders.
int main() {

    const PrismaticFold pf_base(PrismaticFoldGuide::vidy_hall(), "prismatic_base");
    const wood_vidy::OrigamiShell prismatic_fold(
        pf_base.mesh(), 45.0, 300.0, true, true, 500.0, 1.0, "prismatic_fold"
    );

    const CounterFold cf_base(CounterFoldGuide::vidy_hall());
    const wood_vidy::OrigamiShell counter_fold(
        cf_base.mesh(), 45.0, 300.0, true, true, 500.0, 1.0, "counter_fold"
    );

    WoodSession scene("vidy_origami_shapes");
    prismatic_fold.copy_plates_to(scene, "prismatic_fold");
    counter_fold.copy_plates_to(scene, "counter_fold");

    std::cout << fmt::format(
        "vidy 6 origami: prismatic={}/{}p/{}s (skew={:.1f}deg, unjoined={}), "
        "counter={}/{}p/{}s (skew={:.1f}deg, unjoined={})\n",
        prismatic_fold.all_plates().size(),
        prismatic_fold.total_pins(),
        prismatic_fold.total_mortise_slots(),
        prismatic_fold.max_insertion_skew_deg(),
        prismatic_fold.unjoined_interior_creases(),
        counter_fold.all_plates().size(),
        counter_fold.total_pins(),
        counter_fold.total_mortise_slots(),
        counter_fold.max_insertion_skew_deg(),
        counter_fold.unjoined_interior_creases()
    );
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vidy 6 origami: long deep PrismaticFold and CounterFold halls (14.0 x 13.2 m, 45/300 mm CLT, 500 mm staggered pitch, 180/270 mm full-size tenons, 0 unjoined creases, |theta| <= 18 deg).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_6_origami_shapes --parallel 6 && ./build/templates_vidy_6_origami_shapes && ../bash/publish-scene.sh --target templates_vidy_6_origami_shapes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
