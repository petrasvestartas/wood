#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<Line> LINES = {
    Line::from_points(Point(-2000.0, 5000.0, 0.0), Point(32000.0, 7000.0, 0.0)),
    Line::from_points(Point(-2000.0, 11000.0, 0.0), Point(32000.0, 12500.0, 0.0)),
    Line::from_points(Point(7000.0, -2000.0, 0.0), Point(5000.0, 21000.0, 0.0)),
    Line::from_points(Point(15000.0, -2000.0, 0.0), Point(16000.0, 21000.0, 0.0)),
    Line::from_points(Point(22000.0, -2000.0, 0.0), Point(24000.0, 21000.0, 0.0)),
}; // hand-drawn axes, no two parallel
const std::vector<Polyline> FOOTPRINT = {Polyline({Point(0.0, 0.0, 0.0), Point(28000.0, 1500.0, 0.0), Point(30000.0, 14000.0, 0.0), Point(15000.0, 19000.0, 0.0), Point(-1000.0, 12000.0, 0.0), Point(0.0, 0.0, 0.0)})};
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 7600.0};
const wood_grid::Framing FRAMING{.system = 1, .span = -1, .node = 0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_irregular");
    wood_grid::Building::from_footprint(FOOTPRINT, ELEVATIONS, wood_grid::Pattern::from_lines(LINES)).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The grid template on an irregular plan: five hand-drawn axes at odd angles clipped to a five-sided footprint over two storeys, every line a beam so the crossings become mitred nodes of four beams on heads shaped by their directions, edge beams on the footprint with the interior beams butting into them obliquely. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_irregular --parallel 4 && ./build/templates_grid_irregular && ../bash/publish-scene.sh --target templates_grid_irregular

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
