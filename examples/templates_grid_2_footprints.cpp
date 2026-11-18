#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

struct FootprintCase {
    std::string name;
    Plan plan;
    Framing framing;
};

std::pair<double, double> plan_u_extent(const Plan& plan) {
    double lo = std::numeric_limits<double>::max();
    double hi = -lo;
    for (const Point& p : plan.nodes) {
        const Point w = plan.to_world(p[0], p[1], 0.0);
        lo = std::min(lo, w[0]);
        hi = std::max(hi, w[0]);
    }
    return {lo, hi};
}

int main() {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const Plane FLOOR = Plane(Point(0.0, 0.0, 4000.0), X, Y);
    const double GAP = 8000.0;

    const std::vector<FootprintCase> cases = {
        {"orthogonal",
         Plan::from_footprint(
             {Polyline({
                 Point(0.0, 0.0, 0.0),
                 Point(24000.0, 0.0, 0.0),
                 Point(24000.0, 7500.0, 0.0),
                 Point(16000.0, 7500.0, 0.0),
                 Point(16000.0, 15000.0, 0.0),
                 Point(0.0, 15000.0, 0.0),
                 Point(0.0, 0.0, 0.0),
             })},
             Pattern::orthogonal({8000.0, 8000.0, 8000.0}, {7500.0, 7500.0}),
             {},
             FLOOR
         ),
         Framing{
             .system = System::GirderPurlin,
             .node = Node::Flush,
             .span = 0,
             .height = 4000.0,
             .spacing = 2500.0,
             .deck = 160.0,
             .panel = 2500.0,
         }},
        {"skewed",
         Plan::from_pattern(
             Pattern::orthogonal(std::vector<double>(4, 6000.0), std::vector<double>(3, 6000.0), 30.0),
             FLOOR
         ),
         Framing{
             .system = System::GirderPurlin,
             .node = Node::Flush,
             .span = 0,
             .height = 4000.0,
             .spacing = 2000.0,
         }},
        {"radial",
         Plan::from_pattern(Pattern::radial({4000.0, 8000.0, 12000.0}, 12, true), FLOOR),
         Framing{
             .system = System::PostAndBeam,
             .node = Node::Flush,
             .span = 0,
             .height = 4000.0,
             .profiles =
                 {.column = profile_round(240.0),
                  .girder = profile_rectangle(240.0, 480.0),
                  .beam = profile_rectangle(240.0, 420.0)},
         }},
        {"triangular",
         Plan::from_pattern(Pattern::triangular(6000.0, 4, 3), FLOOR),
         Framing{
             .system = System::PostAndBeam,
             .node = Node::Flush,
             .span = -1,
             .height = 4000.0,
             .profiles =
                 {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 440.0)},
         }},
        {"hexagonal",
         Plan::from_pattern(Pattern::hexagonal(4200.0, 3, 2), FLOOR),
         Framing{
             .system = System::PostAndBeam,
             .node = Node::Flush,
             .span = -1,
             .height = 4000.0,
             .profiles =
                 {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 420.0)},
         }},
        {"irregular",
         Plan::from_footprint(
             {Polyline({
                 Point(0.0, 0.0, 0.0),
                 Point(26000.0, 1500.0, 0.0),
                 Point(28000.0, 13500.0, 0.0),
                 Point(14000.0, 18000.0, 0.0),
                 Point(-1000.0, 11500.0, 0.0),
                 Point(0.0, 0.0, 0.0),
             })},
             Pattern::from_lines({
                 Line::from_points(Point(-2000.0, 4800.0, 0.0), Point(30000.0, 6000.0, 0.0)),
                 Line::from_points(Point(-2000.0, 9000.0, 0.0), Point(30000.0, 10200.0, 0.0)),
                 Line::from_points(Point(7200.0, -2000.0, 0.0), Point(5800.0, 20000.0, 0.0)),
                 Line::from_points(Point(14800.0, -2000.0, 0.0), Point(15600.0, 20000.0, 0.0)),
                 Line::from_points(Point(21200.0, -2000.0, 0.0), Point(22000.0, 20000.0, 0.0)),
             }),
             {},
             FLOOR
         ),
         Framing{
             .system = System::PostAndBeam,
             .node = Node::Flush,
             .span = 0,
             .height = 4000.0,
             .profiles =
                 {.column = profile_rectangle(240.0, 240.0),
                  .girder = profile_rectangle(240.0, 460.0)},
         }},
        {"courtyard",
         Plan::from_footprint(
             {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 32000.0, 24000.0),
              Polyline::rectangle(Point(8000.0, 8000.0, 0.0), X, Y, 16000.0, 8000.0)},
             Pattern::orthogonal(compute_bays(32000.0, 8000.0), compute_bays(24000.0, 8000.0)),
             {},
             FLOOR
         ),
         Framing{
             .system = System::GirderPurlin,
             .node = Node::Through,
             .span = 0,
             .height = 4000.0,
             .spacing = 2666.7,
         }},
    };

    WoodSession session("templates_grid_2_footprints");
    double cursor = 0.0;
    for (const FootprintCase& item : cases) {
        const auto [lo, hi] = plan_u_extent(item.plan);
        const Plan placed = item.plan.transformed(Xform::translation(cursor - lo, 0.0, 0.0));
        const Grid grid(placed, item.framing, item.name);
        session.graft(grid, session.add_group(item.name));
        cursor += (hi - lo) + GAP;
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-floor planar grid patterns and footprints side by side: an L-plan with uneven bays, girders, purlins and CLT strips; a 30-degree skewed grid with beveled T-section perimeter girders; a 12-sector concentric glulam ring pavilion around an open central oculus with square-ended radial girders and T-butted ring beams; an equilateral triangular diagrid with six-way mitered nodes; a honeycomb hexagonal floor with three-way Y-miters on round timber columns; a five-sided irregular footprint with continuous girders and T-butted cross-beams; and a courtyard ring with continuous through-columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_2_footprints --parallel 6 && ./build/templates_grid_2_footprints && ../bash/publish-scene.sh --target templates_grid_2_footprints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
