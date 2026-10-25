#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_intersection.h"

using namespace session_cpp;
using namespace wood_session;

/// Parallel cylinders intersected in sequence: a and b, then with c, then with d, side by side.
int main() {

    WoodSession stages("cylinder_intersection");
    const std::vector<CylinderIntersection::Cylinder> cylinders = {
        {-1500.0, 0.0, 3000.0},
        {1500.0, 0.0, 3000.0},
        {0.0, 1800.0, 3000.0},
        {0.0, -1800.0, 3000.0},
    };

    for (size_t count = 2; count <= cylinders.size(); count++) {
        const std::vector<CylinderIntersection::Cylinder> first(cylinders.begin(), cylinders.begin() + static_cast<long>(count));
        const Point origin(0.0, 8000.0 * static_cast<double>(count - 2), 0.0);
        CylinderIntersection stage(
            first,
            12000.0,
            60.0,
            origin,
            fmt::format("intersection_{}", count)
        );
        const size_t plates = stage.get_elements_placed<PlateCurved>("plate").size();
        std::cout << fmt::format("{} cylinders: {} plates, {} contacts, development distortion {:.2e}\n", count, plates, stage.get_contacts().size(), stage.distortion());
        stages.graft(stage, stages.add_group(fmt::format("stage_{}", count)));
    }

    stages.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 34 of the curved folding templates: the intersection of parallel cylinders in sequence. Four cylinders of 3 m radius along x, 12 m long, their axes 1.5 m either side across and 1.8 m above and below: a and b meet in a lens of two cylinder plates, (a and b) and c cuts its top, ((a and b) and c) and d its bottom. Every surviving arc of a circle is one 60 mm plate, offset inside, and two neighbours meet on the plane through their outer and inner creases, straight lines along the axis. The three stages stand side by side. Groups: stage_<cylinders>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_34_cylinder_intersection --parallel 6 && ./build/templates_curved_folding_34_cylinder_intersection && ../bash/publish-scene.sh --target templates_curved_folding_34_cylinder_intersection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
