#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// A row of plates lofted between two rails, as a bed row of the floor: one plate per segment.
int main() {

    std::vector<Point> near;
    std::vector<Point> far;

    for (size_t i = 0; i < 7; i++) {
        const double x = 500.0 * i;
        const double z = -400.0 * (1.0 - x / 3000.0) * (1.0 - x / 3000.0);
        near.push_back({x, 0.0, z});
        far.push_back({x, 600.0, z});
    }

    const Polyline bottom_near(near);
    const Polyline bottom_far(far);
    WoodSession scene("element_plate_session");

    for (const std::shared_ptr<Plate>& plate : wood_floor::plates_between({bottom_near, bottom_far}, {bottom_near.translated({0.0, 0.0, 40.0}), bottom_far.translated({0.0, 0.0, 40.0})}, "bed"))
        scene.add(plate);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A row of plates as a session, as a bed row of the floor: two bottom rails 600 apart following a parabola, 400 deep at the start and level at 3000, and the same two rails 40 higher as the top; wood_floor::plates_between lofts one plate per rail segment, six plates, each its bottom quad and its top quad, and the session holds them.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_plate_session --parallel 6 && ./build/element_plate_session && ../bash/publish-scene.sh --target element_plate_session

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
