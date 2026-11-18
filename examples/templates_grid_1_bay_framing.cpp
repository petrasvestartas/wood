#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

struct Case {
    std::string name;
    wood_grid::Plan plan;
    wood_grid::Framing framing;
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
    const Plane floor_at_3600 = Plane(Point(0.0, 0.0, 3600.0), X, Y);
    const wood_grid::Plan l_plan = wood_grid::Plan::from_footprint(
        {l_footprint},
        wood_grid::Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}),
        {},
        floor_at_3600
    );
    const wood_grid::Plan one_bay = wood_grid::Plan::from_pattern(
        wood_grid::Pattern::orthogonal({BAY}, {BAY}),
        Plane(Point(0.0, 0.0, 4200.0), X, Y)
    );

    const wood_grid::Profiles glulam{
        .column = profile_rectangle(280.0, 280.0),
        .girder = profile_rectangle(280.0, 600.0),
        .purlin = profile_rectangle(220.0, 440.0),
    };

    const std::vector<std::pair<Case, double>> cases = {
        {{"floor_l_plan",
          l_plan,
          wood_grid::Framing{
              .system = wood_grid::System::PostAndBeam,
              .node = wood_grid::NodeMode::Flush,
              .span = -1,
              .post_height = 3600.0,
              .deck = 160.0,
              .profiles =
                  {.column = profile_rectangle(240.0, 240.0),
                   .girder = profile_rectangle(240.0, 440.0)},
          }},
         12000.0},
        {{"isolated_bay",
          l_plan.bay(0),
          wood_grid::Framing{
              .system = wood_grid::System::PostAndBeam,
              .node = wood_grid::NodeMode::Flush,
              .span = -1,
              .post_height = 3600.0,
              .deck = 160.0,
              .profiles =
                  {.column = profile_rectangle(240.0, 240.0),
                   .girder = profile_rectangle(240.0, 440.0)},
          }},
         4000.0},
        {{"slab_head",
          one_bay,
          wood_grid::Framing{
              .system = wood_grid::System::Slab,
              .node = wood_grid::NodeMode::Head,
              .post_height = 4200.0,
              .deck = 240.0,
              .panel_width = 3000.0,
              .head_height = 280.0,
              .head_reach = 240.0,
              .profiles = glulam,
          }},
         BAY},
        {{"beam_head",
          one_bay,
          wood_grid::Framing{
              .system = wood_grid::System::PostAndBeam,
              .node = wood_grid::NodeMode::Head,
              .span = -1,
              .post_height = 4200.0,
              .head_height = 240.0,
              .head_reach = 180.0,
              .profiles = glulam,
          }},
         BAY},
        {{"node_flush",
          one_bay,
          wood_grid::Framing{
              .system = wood_grid::System::GirderPurlin,
              .node = wood_grid::NodeMode::Flush,
              .purlin = wood_grid::PurlinMode::Flush,
              .span = 0,
              .post_height = 4200.0,
              .spacing = 2000.0,
              .deck = 140.0,
              .panel_width = 2000.0,
              .profiles = glulam,
          }},
         BAY},
        {{"node_through",
          one_bay,
          wood_grid::Framing{
              .system = wood_grid::System::GirderPurlin,
              .node = wood_grid::NodeMode::Through,
              .purlin = wood_grid::PurlinMode::Flush,
              .span = 0,
              .post_height = 4200.0,
              .spacing = 2000.0,
              .profiles = glulam,
          }},
         BAY},
        {{"purlin_stacked",
          one_bay,
          wood_grid::Framing{
              .system = wood_grid::System::GirderPurlin,
              .node = wood_grid::NodeMode::Flush,
              .purlin = wood_grid::PurlinMode::Stacked,
              .span = 0,
              .post_height = 4200.0,
              .spacing = 2000.0,
              .profiles = glulam,
          }},
         BAY},
        {{"round_columns",
          one_bay,
          wood_grid::Framing{
              .system = wood_grid::System::GirderPurlin,
              .node = wood_grid::NodeMode::Flush,
              .purlin = wood_grid::PurlinMode::Flush,
              .span = 0,
              .post_height = 4200.0,
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
        const wood_grid::Plan shifted = item.plan.transformed(Xform::translation(offset, 0.0, 0.0));
        const wood_grid::Grid grid(shifted, item.framing, item.name);
        session.graft(grid, session.add_group(item.name));
        offset += width + GAP;
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-floor bay isolation and pure-timber framing modes side by side: a five-bay L floor beside bay 0 isolated via Plan::bay(0); a point-supported CLT slab on columns with a single chamfered timber head; post-and-beam resting on chamfered timber heads; girder-and-purlin with flush columns and CLT strips; continuous through-columns with beams butting to column faces; continuous purlins stacked on lowered girders; and glulam framing on round timber columns.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_1_bay_framing --parallel 6 && ./build/templates_grid_1_bay_framing && ../bash/publish-scene.sh --target templates_grid_1_bay_framing

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
