#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

int main() {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double GAP = 10000.0;

    WoodSession session("templates_grid_4_multistorey");
    double cursor = 0.0;

    // 1. Classic 2-storey 5-bay L-shaped post-and-beam building with chamfered timber heads & CLT decks.
    {
        const Polyline l_ring({
            Point(cursor + 0.0, 0.0, 0.0),
            Point(cursor + 12000.0, 0.0, 0.0),
            Point(cursor + 12000.0, 3000.0, 0.0),
            Point(cursor + 8000.0, 3000.0, 0.0),
            Point(cursor + 8000.0, 6000.0, 0.0),
            Point(cursor + 0.0, 6000.0, 0.0),
            Point(cursor + 0.0, 0.0, 0.0),
        });
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {3000.0, 3000.0})
                .transformed(Xform::translation(cursor, 0.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::PostAndBeam,
            .node = wood_grid::NodeMode::Head,
            .span = -1,
            .deck = 180.0,
            .head_height = 200.0,
            .head_reach = 150.0,
            .profiles =
                {.column = profile_rectangle(220.0, 220.0),
                 .girder = profile_rectangle(220.0, 360.0)},
        };
        const wood_grid::Stack stack =
            wood_grid::Stack::from_footprint({l_ring}, {0.0, 3200.0, 6400.0}, pat, frm);
        const wood_grid::Grid bldg(stack, "five_bay");
        session.graft(bldg, session.add_group("five_bay"));
        cursor += 12000.0 + GAP;
    }

    // 2. 5-storey podium + setback tower (`Stack::from_levels`) with continuous columns.
    {
        const Polyline podium =
            Polyline::rectangle(Point(cursor, 0.0, 0.0), X, Y, 24000.0, 18000.0);
        const Polyline tower =
            Polyline::rectangle(Point(cursor + 6000.0, 6000.0, 0.0), X, Y, 12000.0, 12000.0);
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(24000.0, 6000.0), wood_grid::compute_bays(18000.0, 6000.0)
            )
                .transformed(Xform::translation(cursor, 0.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::GirderPurlin,
            .node = wood_grid::NodeMode::Through,
            .span = 0,
            .spacing = 2000.0,
            .deck = 160.0,
            .panel_width = 2000.0,
        };
        const wood_grid::Stack stack = wood_grid::Stack::from_levels(
            0.0,
            {
                {4000.0, {podium}, pat, frm, {}},
                {8000.0, {podium}, pat, frm, {}},
                {11600.0, {tower}, pat, frm, {}},
                {15200.0, {tower}, pat, frm, {}},
                {18800.0, {tower}, pat, frm, {}},
            }
        );
        const wood_grid::Grid bldg(stack, "setback_tower");
        session.graft(bldg, session.add_group("setback_tower"));
        cursor += 24000.0 + GAP;
    }

    // 3. 3-storey hybrid timber building with pinwheel CLT core shear walls & braced perimeter facades.
    {
        const Polyline outer =
            Polyline::rectangle(Point(cursor, 0.0, 0.0), X, Y, 24000.0, 18000.0);
        const Polyline core =
            Polyline::rectangle(Point(cursor + 8000.0, 6000.0, 0.0), X, Y, 8000.0, 6000.0);
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(24000.0, 8000.0), wood_grid::compute_bays(18000.0, 6000.0)
            )
                .transformed(Xform::translation(cursor, 0.0, 0.0));
        const wood_grid::Framing floor_frm{
            .system = wood_grid::System::GirderPurlin,
            .node = wood_grid::NodeMode::Flush,
            .span = 0,
            .spacing = 2000.0,
            .deck = 160.0,
            .panel_width = 2667.0,
            .wall = 240.0,
        };
        wood_grid::Stack stack = wood_grid::Stack::from_footprint(
            {outer}, {0.0, 3800.0, 7600.0, 11400.0}, pat, floor_frm, {core}
        );
        stack.add_perimeter_facades(
            outer,
            0.0,
            11400.0,
            3800.0,
            {3800.0, 3800.0, 3800.0},
            wood_grid::Framing{
                .system = wood_grid::System::PostAndBeam,
                .brace = wood_grid::BraceMode::Cross,
                .span = 1,
                .post_height = 0.0,
                .profiles =
                    {.girder = profile_rectangle(140.0, 200.0),
                     .beam = profile_rectangle(120.0, 200.0),
                     .brace = profile_rectangle(120.0, 160.0)},
            }
        );
        const wood_grid::Grid bldg(stack, "core_and_facade");
        session.graft(bldg, session.add_group("core_and_facade"));
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Multi-storey timber buildings composed from single-floor and facade Plans via Stack: a two-storey L-shaped five-bay frame with chamfered timber heads and CLT decks; a five-storey podium and setback tower with continuous through-columns and notched CLT floor plates; and a three-storey timber frame around a pinwheel CLT shear core wrapped with braced perimeter timber facades.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_4_multistorey --parallel 6 && ./build/templates_grid_4_multistorey && ../bash/publish-scene.sh --target templates_grid_4_multistorey

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
