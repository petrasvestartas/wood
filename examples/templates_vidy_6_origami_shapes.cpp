#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"
#include "src/templates/folding/diagonal_fold.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

/// Double-layer CLT origami shells with selective chamfers, corner borders and stepped tenons.
int main() {

    const wood_vidy::OrigamiShell miura(
        wood_vidy::OrigamiShell::miura_arch_mesh(6, 3, 12000.0, 9600.0, 4400.0, 560.0, 420.0),
        40.0,
        240.0,
        true,
        true,
        420.0,
        1.0,
        "miura_arch"
    );
    const wood_vidy::OrigamiShell yoshimura(
        wood_vidy::OrigamiShell::yoshimura_arch_mesh(3, 5, 12000.0, 2800.0, 4600.0, 540.0),
        40.0,
        240.0,
        true,
        true,
        400.0,
        1.0,
        "yoshimura_vault"
    );
    const CounterFold cf_base(
        CounterFold::elevated_centre(3, 1600.0, 1500.0, 12800.0, 1500.0),
        Folding::ridge_to_ridge,
        40.0
    );
    const wood_vidy::OrigamiShell counter_fold(
        cf_base.mesh(), 40.0, 240.0, true, true, 420.0, 1.0, "counter_fold"
    );
    const DiagonalFold df_base(14400.0, {2800.0, 0.0, 5600.0, 0.5, 3400.0}, 40.0);
    const wood_vidy::OrigamiShell cross_fold(
        df_base.mesh(), 40.0, 240.0, true, true, 400.0, 1.0, "cross_fold"
    );

    WoodSession scene("vidy_origami_shapes");
    miura.copy_plates_to(scene, "miura_arch");
    yoshimura.copy_plates_to(scene, "yoshimura_vault");
    counter_fold.copy_plates_to(scene, "counter_fold");
    cross_fold.copy_plates_to(scene, "cross_fold");

    std::cout << fmt::format(
        "vidy 6 origami: miura={}/{}p, yoshimura={}/{}p, counter={}/{}p, cross={}/{}p\n",
        miura.all_plates().size(),
        miura.total_pins(),
        yoshimura.all_plates().size(),
        yoshimura.total_pins(),
        counter_fold.all_plates().size(),
        counter_fold.total_pins(),
        cross_fold.all_plates().size(),
        cross_fold.total_pins()
    );
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vidy 6 origami: Miura-ori quad arch, Yoshimura diamond vault, CounterFold span and DiagonalFold dome with double-layer 40/240 mm CLT, full-corner borders and stepped tenons.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_6_origami_shapes --parallel 6 && ./build/templates_vidy_6_origami_shapes && ../bash/publish-scene.sh --target templates_vidy_6_origami_shapes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
