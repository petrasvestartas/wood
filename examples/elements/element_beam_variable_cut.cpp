#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A variable beam cut at both ends by two plane elements through plane features.
int main() {

    WoodSession scene("element_beam_variable_cut");

    // the rib: seven rectangles 120 wide hanging from a straight axis, 730 deep at the start to 300 at the end
    const double length = 3000.0;
    std::vector<Polyline> sections;

    for (size_t i = 0; i < 7; i++) {
        const double x = length * i / 6.0;
        const double depth = 300.0 + 430.0 * (1.0 - x / length) * (1.0 - x / length);
        sections.push_back(Polyline({Point(x, -60.0, 0.0), Point(x, -60.0, -depth), Point(x, 60.0, -depth), Point(x, 60.0, 0.0)}).closed());
    }

    const std::shared_ptr<BeamVariable> rib = std::make_shared<BeamVariable>(Line::from_points(Point(0.0, 0.0, 0.0), Point(length, 0.0, 0.0)), sections, "rib");
    scene.add(rib);

    // two inclined planes, the rib keeping the side their normals point to
    const std::shared_ptr<CutPlane> start = std::make_shared<CutPlane>(Plane::from_point_normal(Point(300.0, 0.0, -400.0), Vector(1.0, 0.0, -0.4)), 900.0, "start");
    const std::shared_ptr<CutPlane> end = std::make_shared<CutPlane>(Plane::from_point_normal(Point(2700.0, 0.0, -200.0), Vector(-1.0, 0.0, -0.3)), 900.0, "end");
    scene.add(start);
    scene.add(end);
    scene.add_interaction(start, rib, start->feature());
    scene.add_interaction(end, rib, end->feature());

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The variable beam of element_beam_variable cut at both ends: two CutPlane elements, inclined planes drawn as 900 squares, each cutting the rib through its plane feature, add_interaction(plane, rib, plane->feature()), the rib keeping the side the normal points to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_beam_variable_cut --parallel 6 && ./build/element_beam_variable_cut && ../bash/publish-scene.sh --target element_beam_variable_cut

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
