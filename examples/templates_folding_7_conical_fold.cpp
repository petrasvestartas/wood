#include "wood_session.h"
#include "src/templates/folding/conical_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Conical folding, a continuous fold profile with its upper edge cut by sloping planes (Engel, p. 224): four folds over a 14.4 m
/// span, their valleys level, their ridges 1500 high at mid-span and cut down to 600 at the ends.
int main() {

    ConicalFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("conical fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the folding templates: a surface with conical folding after Engel, Structure Systems, Folded plate structures (p. 224), a continuous fold profile with its upper edge cut by a sloping plane, read off the book's plan and both elevations. Four folds of plates 1500 across, their valleys level over a 14.4 m span and their ridges 1500 high, are cut by two planes sloping from 600 above the valleys at either end up to the ridges at mid-span: below the cut every flank keeps its plane, a pentagon from the valley line to its cut edge, which rises from the end to the ridge point at mid-span and falls again, and above it the cut is a thin triangle per fold and half span, from the flat top of the fold at the end to the ridge point, so the end profiles are trapezoids and the middle a sharp zigzag, the elongated X of every fold in Engel's plan: 16 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_7_conical_fold --parallel 6 && ./build/templates_folding_7_conical_fold && ../bash/publish-scene.sh --target templates_folding_7_conical_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
