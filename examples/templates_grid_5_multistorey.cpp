#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

int main() {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double GAP = 11000.0;

    WoodSession session("templates_grid_5_multistorey");
    double cursor = 0.0;

    // 1. Two-storey L-shaped five-bay frame with IN-LINE chamfered timber column heads (`[-beam_depth, 0]`)
    //    and flush CLT cassette decks notched at 45 degrees around every head!
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
            .deck_flush = true,
            .head_reach = 220.0,
            .profiles =
                {.column = profile_rectangle(240.0, 240.0),
                 .girder = profile_rectangle(240.0, 380.0)},
        };
        const wood_grid::Stack stack =
            wood_grid::Stack::from_footprint({l_ring}, {0.0, 3400.0, 6800.0}, pat, frm);
        session.graft(wood_grid::Grid(stack, "five_bay"), session.add_group("five_bay"));
        cursor += 12000.0 + GAP;
    }

    // 2. Shifted / cantilevered 5-storey timber boxes over a continuous plumb column grid (`Stack::shifted`).
    {
        const Polyline base_fp =
            Polyline::rectangle(Point(cursor + 3000.0, 3000.0, 0.0), X, Y, 18000.0, 12000.0);
        const wood_grid::Pattern shared_pat =
            wood_grid::Pattern::orthogonal(
                std::vector<double>(8, 3000.0), std::vector<double>(6, 3000.0)
            )
                .transformed(Xform::translation(cursor, 0.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::GirderPurlin,
            .node = wood_grid::NodeMode::Flush,
            .span = 0,
            .spacing = 1500.0,
            .deck = 160.0,
            .panel_width = 3000.0,
            .profiles =
                {.column = profile_rectangle(280.0, 280.0),
                 .girder = profile_rectangle(260.0, 480.0),
                 .purlin = profile_rectangle(180.0, 340.0)},
        };
        const wood_grid::Stack stack = wood_grid::Stack::shifted(
            {base_fp},
            {0.0, 3800.0, 7600.0, 11400.0, 15200.0, 19000.0},
            {{0.0, 0.0}, {3000.0, 0.0}, {3000.0, 3000.0}, {-3000.0, 3000.0}, {0.0, -3000.0}},
            shared_pat,
            frm
        );
        session.graft(wood_grid::Grid(stack, "shifted_floors"), session.add_group("shifted_floors"));
        cursor += 24000.0 + GAP;
    }

    // 3. Six-storey tapered timber tower (`Stack::tapered`) with inclined perimeter columns.
    {
        const Polyline base_fp =
            Polyline::rectangle(Point(cursor, 0.0, 0.0), X, Y, 21000.0, 21000.0);
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(21000.0, 7000.0), wood_grid::compute_bays(21000.0, 7000.0)
            )
                .transformed(Xform::translation(cursor, 0.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::GirderPurlin,
            .node = wood_grid::NodeMode::Flush,
            .span = 0,
            .spacing = 2200.0,
            .deck = 160.0,
            .panel_width = 2500.0,
            .profiles =
                {.column = profile_rectangle(300.0, 300.0),
                 .girder = profile_rectangle(260.0, 500.0),
                 .purlin = profile_rectangle(180.0, 340.0)},
        };
        const wood_grid::Stack stack = wood_grid::Stack::tapered(
            {base_fp},
            {0.0, 4000.0, 7800.0, 11600.0, 15400.0, 19200.0, 23000.0},
            pat,
            0.56,
            frm
        );
        session.graft(wood_grid::Grid(stack, "tapered_tower"), session.add_group("tapered_tower"));
        cursor += 21000.0 + GAP;
    }

    // 4. Six-storey twisting timber tower (`Stack::twisted`, 30 degrees total rotation) with helicoidal columns.
    {
        const Polyline base_fp =
            Polyline::rectangle(Point(cursor + 2000.0, 2000.0, 0.0), X, Y, 18000.0, 18000.0);
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(18000.0, 6000.0), wood_grid::compute_bays(18000.0, 6000.0)
            )
                .transformed(Xform::translation(cursor + 2000.0, 2000.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::PostAndBeam,
            .node = wood_grid::NodeMode::Flush,
            .span = 0,
            .deck = 160.0,
            .panel_width = 3000.0,
            .profiles =
                {.column = profile_round(320.0),
                 .girder = profile_rectangle(260.0, 480.0),
                 .beam = profile_rectangle(220.0, 420.0)},
        };
        const wood_grid::Stack stack = wood_grid::Stack::twisted(
            {base_fp},
            {0.0, 3800.0, 7600.0, 11400.0, 15200.0, 19000.0, 22800.0},
            pat,
            30.0,
            frm
        );
        session.graft(wood_grid::Grid(stack, "twisted_tower"), session.add_group("twisted_tower"));
        cursor += 24000.0 + GAP;
    }

    // 5. Five-storey podium + setback tower (`Stack::from_levels`) with continuous through-columns.
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
        session.graft(wood_grid::Grid(stack, "setback_tower"), session.add_group("setback_tower"));
        cursor += 24000.0 + GAP;
    }

    // 6. Three-storey hybrid timber building with pinwheel CLT shear core & corner X-braced facades.
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
        session.graft(
            wood_grid::Grid(stack, "core_and_facade"), session.add_group("core_and_facade")
        );
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Six multi-storey timber building topologies composed via Stack: a two-storey L-frame with in-line chamfered column heads and notched flush CLT cassette decks; five storeys of shifted cantilevered floor plates over a plumb column grid; a six-storey tapered tower with inclined perimeter columns; a six-storey 30-degree twisting tower with helicoidal round timber columns; a five-storey podium and setback tower with continuous through-columns; and a three-storey hybrid building around a pinwheel CLT core with corner X-braced perimeter facades.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_5_multistorey --parallel 6 && ./build/templates_grid_5_multistorey && ../bash/publish-scene.sh --target templates_grid_5_multistorey

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
