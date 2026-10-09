#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A plate with four holes, each drilled by a hole element through a solid feature of drills.
int main() {

    WoodSession scene("element_plate_holes");

    const Polyline bottom = Polyline::rectangle(
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        600.0,
        400.0
    );
    const std::shared_ptr<Plate> plate = std::make_shared<Plate>(bottom, bottom.translated({0.0, 0.0, 40.0}), "plate");
    scene.add(plate);

    for (const Point& centre : std::vector<Point>{{100.0, 100.0, 0.0}, {500.0, 100.0, 0.0}, {500.0, 300.0, 0.0}, {100.0, 300.0, 0.0}}) {
        const Line axis = Line::from_points(centre + Vector(0.0, 0.0, 50.0), centre + Vector(0.0, 0.0, -10.0));
        const std::shared_ptr<Joint> hole = Joint::drill(axis, 15.0);
        hole->name = "hole";
        hole->is_visible = false;
        scene.add(hole);
        scene.add_interaction(hole, plate, std::make_shared<InteractionFeatureSolid>(std::vector<Line>{axis}, 15.0));
    }

    scene.compute_breps();
    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A plate with four holes: a 600 x 400 plate 40 thick from two polylines, and four hidden hole elements, Joint::drill along a vertical axis, each taking a 30 hole away through add_interaction(hole, plate, InteractionFeatureSolid(drills, radius)), a solid feature of drills alone; written as BReps so the holes are exact cylinders.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_plate_holes --parallel 6 && ./build/element_plate_holes && ../bash/publish-scene.sh --target element_plate_holes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
