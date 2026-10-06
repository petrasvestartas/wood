#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A variable beam like the floor's outer rib: rectangles along a straight axis, their depth a parabola.
int main() {

    WoodSession scene("element_beam_variable");

    // seven rectangles 120 wide hanging from the axis, 730 deep at the start to 300 at the end
    const double length = 3000.0;
    std::vector<Polyline> sections;

    for (size_t i = 0; i < 7; i++) {
        const double x = length * i / 6.0;
        const double depth = 300.0 + 430.0 * (1.0 - x / length) * (1.0 - x / length);
        sections.push_back(Polyline({Point(x, -60.0, 0.0), Point(x, -60.0, -depth), Point(x, 60.0, -depth), Point(x, 60.0, 0.0)}).closed());
    }

    scene.add(std::make_shared<BeamVariable>(Line::from_points(Point(0.0, 0.0, 0.0), Point(length, 0.0, 0.0)), sections, "rib"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A variable beam like the floor's outer rib: a straight 3000 axis along its top and seven 120 wide rectangles hanging from it, 730 deep at the start and 300 at the end on a parabola, lofted from one to the next. The axis and the sections are its features.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_beam_variable --parallel 6 && ./build/element_beam_variable && ../bash/publish-scene.sh --target element_beam_variable

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
