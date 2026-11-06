#include "cutting_gallery.h"
using namespace cutting_gallery;

static std::shared_ptr<FacePlanarElement> stock() {

    return FacePlanarElement::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 160, 35);
}

int main() {
    WoodSession scene("10_profile_cuts");
    const Polyline concave(
        {{20, 20, -5},
         {180, 20, -5},
         {180, 65, -5},
         {80, 65, -5},
         {80, 140, -5},
         {20, 140, -5},
         {20, 20, -5}}
    );
    const std::shared_ptr<JointElement> l_profile = std::make_shared<JointElement>(concave, Vector(0, 0, 45));
    cut_case(scene, "Concave L profile / intersection", stock(), l_profile, 0);

    const Polyline outer = rectangle(15, 15, -5, 170, 130);
    const Polyline hole = rectangle(65, 55, -5, 70, 50);
    const std::shared_ptr<JointElement> holed =
        std::make_shared<JointElement>(std::vector<Polyline>{outer, hole}, Vector(0, 0, 45));
    cut_case(scene, "Profile with a hole / intersection", stock(), holed, 260);

    const Polyline slot_outline = rectangle(85, -10, -5, 30, 180);
    const std::shared_ptr<JointElement> slot = std::make_shared<JointElement>(
        std::vector<Polyline>{slot_outline}, Vector(0, 0, 45), SolidOperation::subtract
    );
    cut_case(scene, "Through slot / two separate pieces", stock(), slot, 520);

    const Polyline pocket_outline = concave.translated({0, 0, 25});
    const std::shared_ptr<JointElement> pocket = std::make_shared<JointElement>(
        std::vector<Polyline>{pocket_outline}, Vector(0, 0, 30), SolidOperation::subtract
    );
    cut_case(scene, "Concave pocket / solid split", stock(), pocket, 780);
    finish(scene, "10_profile_cuts");
}

/*
|||||||| DESCRIPTION ||||||||
A plate cut by profile outlines pushed through it: a convex profile, a through slot that leaves two pieces and a concave pocket that splits the solid.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 10_profile_cuts --parallel 6 && ./build/10_profile_cuts && ../bash/publish-scene.sh --target 10_profile_cuts

|||||||| TIME ||||||||
0.89 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
