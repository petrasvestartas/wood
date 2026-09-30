#include "cutting_gallery.h"
using namespace cutting_gallery;

static std::shared_ptr<Plate> stock() {

    return Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 160, 35);
}

int main() {
    WoodSession scene("13_profile_cuts");
    const Polyline concave({{20, 20, -5}, {180, 20, -5}, {180, 65, -5}, {80, 65, -5},
                      {80, 140, -5}, {20, 140, -5}, {20, 20, -5}});
    cut_case(scene, "Concave L profile / intersection", stock(),
             std::make_shared<Joint>(concave, Vector(0, 0, 45)), 0);
    cut_case(scene, "Profile with a hole / intersection", stock(),
             std::make_shared<Joint>(std::vector<Polyline>{rectangle(15, 15, -5, 170, 130), rectangle(65, 55, -5, 70, 50)}, Vector(0, 0, 45)), 260);
    cut_case(scene, "Through slot / two separate pieces", stock(),
             std::make_shared<Joint>(std::vector<Polyline>{rectangle(85, -10, -5, 30, 180)}, Vector(0, 0, 45), SolidOperation::difference), 520);
    cut_case(scene, "Concave pocket / solid split", stock(),
             std::make_shared<Joint>(std::vector<Polyline>{concave.translated({0, 0, 25})}, Vector(0, 0, 30), SolidOperation::difference), 780);
    finish(scene, "13_profile_cuts");
}

/*
directory: cd /home/petras/code/code_cpp/wood_research/wood
run: buildslot ~/.local/bin/cmake --build build --target 13_profile_cuts --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/13_profile_cuts
cloudflare: ../bash/publish-scene.sh "$PWD/data/output/pb/13_profile_cuts.pb" --no-notify
view: https://petrasvestartas.github.io/session/
*/
