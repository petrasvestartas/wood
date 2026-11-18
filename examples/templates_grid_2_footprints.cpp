#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

struct FootprintCase {
    std::string name;
    wood_grid::Plan plan;
    wood_grid::Framing framing;
};

std::pair<double, double> plan_u_extent(const wood_grid::Plan& plan) {
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
         wood_grid::Plan::from_footprint(
             {Polyline({
                 Point(0.0, 0.0, 0.0),
                 Point(24000.0, 0.0, 0.0),
                 Point(24000.0, 7500.0, 0.0),
                 Point(16000.0, 7500.0, 0.0),
                 Point(16000.0, 15000.0, 0.0),
                 Point(0.0, 15000.0, 0.0),
                 Point(0.0, 0.0, 0.0),
             })},
             wood_grid::Pattern::orthogonal({8000.0, 8000.0, 8000.0}, {7500.0, 7500.0}),
             {},
             FLOOR
         ),
         wood_grid::Framing{
             .system = wood_grid::System::GirderPurlin,
             .node = wood_grid::NodeMode::Flush,
             .span = 0,
             .post_height = 4000.0,
             .spacing = 2500.0,
             .deck = 160.0,
             .panel_width = 2500.0,
         }},
        {"skewed",
         wood_grid::Plan::from_pattern(
             wood_grid::Pattern::orthogonal(
                 std::vector<double>(4, 6000.0), std::vector<double>(3, 6000.0), 30.0
             ),
             FLOOR
         ),
         wood_grid::Framing{
             .system = wood_grid::System::GirderPurlin,
             .node = wood_grid::NodeMode::Flush,
             .span = 0,
             .post_height = 4000.0,
             .spacing = 2000.0,
         }},
        {"radial",
         wood_grid::Plan::from_pattern(
             wood_grid::Pattern::radial({4000.0, 8000.0, 12000.0}, 12, true), FLOOR
         ),
         wood_grid::Framing{
             .system = wood_grid::System::PostAndBeam,
             .node = wood_grid::NodeMode::Flush,
             .span = 0,
             .post_height = 4000.0,
             .profiles =
                 {.column = profile_round(240.0),
                  .girder = profile_rectangle(240.0, 480.0),
                  .beam = profile_rectangle(240.0, 420.0)},
         }},
        {"triangular",
         wood_grid::Plan::from_pattern(wood_grid::Pattern::triangular(6000.0, 4, 3), FLOOR),
         wood_grid::Framing{
             .system = wood_grid::System::PostAndBeam,
             .node = wood_grid::NodeMode::Flush,
             .span = -1,
             .post_height = 4000.0,
             .profiles =
                 {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 440.0)},
         }},
        {"hexagonal",
         wood_grid::Plan::from_pattern(wood_grid::Pattern::hexagonal(4200.0, 3, 2), FLOOR),
         wood_grid::Framing{
             .system = wood_grid::System::PostAndBeam,
             .node = wood_grid::NodeMode::Flush,
             .span = -1,
             .post_height = 4000.0,
             .profiles =
                 {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 420.0)},
         }},
        {"irregular",
         wood_grid::Plan::from_footprint(
             {Polyline({
                 Point(0.0, 0.0, 0.0),
                 Point(26000.0, 1500.0, 0.0),
                 Point(28000.0, 13500.0, 0.0),
                 Point(14000.0, 18000.0, 0.0),
                 Point(-1000.0, 11500.0, 0.0),
                 Point(0.0, 0.0, 0.0),
             })},
             wood_grid::Pattern::from_lines({
                 Line::from_points(Point(-2000.0, 4800.0, 0.0), Point(30000.0, 6000.0, 0.0)),
                 Line::from_points(Point(-2000.0, 9000.0, 0.0), Point(30000.0, 10200.0, 0.0)),
                 Line::from_points(Point(7200.0, -2000.0, 0.0), Point(5800.0, 20000.0, 0.0)),
                 Line::from_points(Point(14800.0, -2000.0, 0.0), Point(15600.0, 20000.0, 0.0)),
                 Line::from_points(Point(21200.0, -2000.0, 0.0), Point(22000.0, 20000.0, 0.0)),
             }),
             {},
             FLOOR
         ),
         wood_grid::Framing{
             .system = wood_grid::System::PostAndBeam,
             .node = wood_grid::NodeMode::Flush,
             .span = 0,
             .post_height = 4000.0,
             .profiles =
                 {.column = profile_rectangle(240.0, 240.0),
                  .girder = profile_rectangle(240.0, 460.0)},
         }},
        {"courtyard",
         wood_grid::Plan::from_footprint(
             {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 32000.0, 24000.0),
              Polyline::rectangle(Point(8000.0, 8000.0, 0.0), X, Y, 16000.0, 8000.0)},
             wood_grid::Pattern::orthogonal(
                 wood_grid::compute_bays(32000.0, 8000.0), wood_grid::compute_bays(24000.0, 8000.0)
             ),
             {},
             FLOOR
         ),
         wood_grid::Framing{
             .system = wood_grid::System::GirderPurlin,
             .node = wood_grid::NodeMode::Through,
             .span = 0,
             .post_height = 4000.0,
             .spacing = 2666.7,
         }},
    };

    WoodSession session("templates_grid_2_footprints");
    double cursor = 0.0;
    for (const FootprintCase& item : cases) {
        const auto [lo, hi] = plan_u_extent(item.plan);
        const wood_grid::Plan placed = item.plan.transformed(Xform::translation(cursor - lo, 0.0, 0.0));
        const wood_grid::Grid grid(placed, item.framing, item.name);
        session.graft(grid, session.add_group(item.name));
        cursor += (hi - lo) + GAP;
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-floor planar grid patterns and footprints side by side: an L-plan with uneven bays, girders, purlins and CLT strips; a 30-degree skewed grid; a 12-sector concentric glulam ring pavilion around an open central oculus; an equilateral triangular diagrid with six-way mitered nodes; a honeycomb hexagonal floor on round timber columns and chamfered heads; a five-sided irregular footprint crossed by hand-drawn axes without triangulation diagonals; and a courtyard ring with continuous through-columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_2_footprints --parallel 6 && ./build/templates_grid_2_footprints && ../bash/publish-scene.sh --target templates_grid_2_footprints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
