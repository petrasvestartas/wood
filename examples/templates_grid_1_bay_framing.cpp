#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

struct Case {
    std::string name;
    Plan plan;
    Framing framing;
};

int main() {
    const double BAY = 6000.0;
    const double GAP = 3500.0;
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);

    // 5-bay L-shaped single floor plan, plus bay 0 isolated via `l_plan.bay(0)`.
    const Polyline l_footprint({
        Point(0.0, 0.0, 0.0),
        Point(12000.0, 0.0, 0.0),
        Point(12000.0, 4000.0, 0.0),
        Point(8000.0, 4000.0, 0.0),
        Point(8000.0, 8000.0, 0.0),
        Point(0.0, 8000.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });
    const Plan l_plan = Plan::from_footprint(
        {l_footprint},
        Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}),
        {},
        Plane(Point(0.0, 0.0, 3600.0), X, Y)
    );
    const Plan one_bay = Plan::from_pattern(
        Pattern::orthogonal({BAY}, {BAY}), Plane(Point(0.0, 0.0, 4200.0), X, Y)
    );

    const Profiles glulam{
        .column = profile_rectangle(280.0, 280.0),
        .girder = profile_rectangle(280.0, 600.0),
        .purlin = profile_rectangle(220.0, 440.0),
    };

    const std::vector<std::pair<Case, double>> cases = {
        {{"floor_l_plan",
          l_plan,
          Framing{
              .system = System::PostAndBeam,
              .node = Node::Flush,
              .span = 0,
              .height = 3600.0,
              .deck = 160.0,
              .profiles =
                  {.column = profile_rectangle(240.0, 240.0),
                   .girder = profile_rectangle(240.0, 440.0)},
          }},
         12000.0},
        {{"isolated_bay",
          l_plan.bay(0),
          Framing{
              .system = System::PostAndBeam,
              .node = Node::Flush,
              .span = 0,
              .height = 3600.0,
              .deck = 160.0,
              .profiles =
                  {.column = profile_rectangle(240.0, 240.0),
                   .girder = profile_rectangle(240.0, 440.0)},
          }},
         4000.0},
        {{"slab_head",
          one_bay,
          Framing{
              .system = System::Slab,
              .node = Node::Head,
              .height = 4200.0,
              .deck = 240.0,
              .panel = 3000.0,
              .head = 280.0,
              .reach = 240.0,
              .profiles = glulam,
          }},
         BAY},
        {{"beam_head",
          Plan::from_pattern(
              Pattern::orthogonal({4500.0, 4500.0}, {6000.0}),
              Plane(Point(0.0, 0.0, 4200.0), X, Y)
          ),
          Framing{
              .system = System::PostAndBeam,
              .node = Node::Head,
              .span = -1,
              .height = 4200.0,
              .reach = 220.0,
              .profiles =
                  {.column = profile_rectangle(260.0, 260.0),
                   .girder = profile_rectangle(260.0, 460.0)},
          }},
         9000.0},
        {{"node_flush",
          one_bay,
          Framing{
              .system = System::GirderPurlin,
              .node = Node::Flush,
              .span = 0,
              .height = 4200.0,
              .spacing = 2000.0,
              .deck = 140.0,
              .panel = 2000.0,
              .profiles = glulam,
          }},
         BAY},
        {{"node_through",
          one_bay,
          Framing{
              .system = System::GirderPurlin,
              .node = Node::Through,
              .span = 0,
              .height = 4200.0,
              .spacing = 2000.0,
              .profiles = glulam,
          }},
         BAY},
        {{"purlin_stacked",
          one_bay,
          Framing{
              .system = System::StackedPurlin,
              .node = Node::Flush,
              .span = 0,
              .height = 4200.0,
              .spacing = 2000.0,
              .profiles = glulam,
          }},
         BAY},
        {{"round_columns",
          one_bay,
          Framing{
              .system = System::GirderPurlin,
              .node = Node::Flush,
              .span = 0,
              .height = 4200.0,
              .spacing = 2000.0,
              .deck = 140.0,
              .profiles =
                  {.column = profile_round(280.0),
                   .girder = profile_rectangle(280.0, 600.0),
                   .purlin = profile_rectangle(220.0, 440.0)},
          }},
         BAY},
    };

    WoodSession session("templates_grid_1_bay_framing");
    double offset = 0.0;
    for (const auto& [item, width] : cases) {
        const Plan shifted = item.plan.transformed(Xform::translation(offset, 0.0, 0.0));
        const Grid grid(shifted, item.framing, item.name);
        session.graft(grid, session.add_group(item.name));
        offset += width + GAP;
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-floor bay isolation and pure-timber framing modes side by side: a five-bay L floor beside bay 0 isolated via Plan::bay(0); a point-supported CLT slab on columns with a single chamfered timber head; post-and-beam with in-line chamfered timber heads; girder-and-purlin with flush columns and CLT strips; continuous through-columns with beams butting to column faces; continuous purlins stacked on lowered girders; and glulam framing on round timber columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_1_bay_framing --parallel 6 && ./build/templates_grid_1_bay_framing && ../bash/publish-scene.sh --target templates_grid_1_bay_framing

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
