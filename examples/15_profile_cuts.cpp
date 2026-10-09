#include "cutting_gallery.h"
using namespace cutting_gallery;

static std::shared_ptr<Plate> stock() {

    return Plate::from_rectangle(
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        200,
        160,
        35
    );
}

int main() {
    WoodSession scene("15_profile_cuts");
    const Polyline concave({{20, 20, -5}, {180, 20, -5}, {180, 65, -5}, {80, 65, -5},
                      {80, 140, -5}, {20, 140, -5}, {20, 20, -5}});
    const std::shared_ptr<Joint> l_profile =
             std::make_shared<Joint>(concave, Vector(0, 0, 45));
    cut_case(
        scene,
        "Concave L profile / intersection",
        stock(),
        l_profile,
        0
    );

    const Polyline outer = rectangle(
        15,
        15,
        -5,
        170,
        130
    );
    const Polyline hole = rectangle(
        65,
        55,
        -5,
        70,
        50
    );
    const std::shared_ptr<Joint> holed =
             std::make_shared<Joint>(std::vector<Polyline>{outer, hole}, Vector(0, 0, 45));
    cut_case(
        scene,
        "Profile with a hole / intersection",
        stock(),
        holed,
        260
    );

    const Polyline slot_outline = rectangle(
        85,
        -10,
        -5,
        30,
        180
    );
    const std::shared_ptr<Joint> slot =
             std::make_shared<Joint>(std::vector<Polyline>{slot_outline}, Vector(0, 0, 45), SolidOperation::subtract);
    cut_case(
        scene,
        "Through slot / two separate pieces",
        stock(),
        slot,
        520
    );

    const Polyline pocket_outline = concave.translated({0, 0, 25});
    const std::shared_ptr<Joint> pocket =
             std::make_shared<Joint>(std::vector<Polyline>{pocket_outline}, Vector(0, 0, 30), SolidOperation::subtract);
    cut_case(
        scene,
        "Concave pocket / solid split",
        stock(),
        pocket,
        780
    );
    finish(scene, "15_profile_cuts");
}

/*
directory: cd /home/petras/code/code_cpp/wood_research/wood
run: buildslot ~/.local/bin/cmake --build build --target 15_profile_cuts --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/15_profile_cuts
cloudflare: ../bash/publish-scene.sh "$PWD/data/output/pb/15_profile_cuts.pb" --no-notify
view: https://petrasvestartas.github.io/session/
*/
