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
                {.column = profile_rectangle(260.0, 260.0),
                 .girder = profile_rectangle(260.0, 480.0),
                 .purlin = profile_rectangle(160.0, 320.0)},
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

    // 3. Six-storey tapered timber tower (`Stack::tapered`) with seamless continuous leaning columns (`NodeMode::Through`).
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
            .node = wood_grid::NodeMode::Through,
            .span = 0,
            .spacing = 2200.0,
            .deck = 160.0,
            .panel_width = 2500.0,
            .profiles =
                {.column = profile_rectangle(300.0, 300.0),
                 .girder = profile_rectangle(300.0, 500.0),
                 .purlin = profile_rectangle(160.0, 320.0)},
        };
        const wood_grid::Stack stack = wood_grid::Stack::tapered(
            {base_fp},
            {0.0, 4000.0, 7800.0, 11600.0, 15400.0, 19200.0, 23000.0},
            pat,
            0.55,
            frm
        );
        session.graft(wood_grid::Grid(stack, "tapered_tower"), session.add_group("tapered_tower"));
        cursor += 21000.0 + GAP;
    }

    // 4. Seven-storey 42-degree twisting timber tower (`Stack::twisted`) with seamless helicoidal round columns.
    {
        const Polyline base_fp =
            Polyline::rectangle(Point(cursor + 3000.0, 3000.0, 0.0), X, Y, 18000.0, 18000.0);
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(18000.0, 6000.0), wood_grid::compute_bays(18000.0, 6000.0)
            )
                .transformed(Xform::translation(cursor + 3000.0, 3000.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::PostAndBeam,
            .node = wood_grid::NodeMode::Through,
            .span = 0,
            .deck = 160.0,
            .panel_width = 3000.0,
            .profiles =
                {.column = profile_round(320.0),
                 .girder = profile_rectangle(280.0, 460.0),
                 .beam = profile_rectangle(240.0, 420.0)},
        };
        const wood_grid::Stack stack = wood_grid::Stack::twisted(
            {base_fp},
            {0.0, 3600.0, 7200.0, 10800.0, 14400.0, 18000.0, 21600.0, 25200.0},
            pat,
            42.0,
            frm
        );
        session.graft(wood_grid::Grid(stack, "twisted_tower"), session.add_group("twisted_tower"));
        cursor += 25000.0 + GAP;
    }

    // 5. Six-storey cascading stepped-terrace building around an open central atrium (`Stack::from_levels`).
    {
        const Polyline atrium =
            Polyline::rectangle(Point(cursor + 10000.0, 10000.0, 0.0), X, Y, 10000.0, 10000.0);
        const Polyline tier1 =
            Polyline::rectangle(Point(cursor, 0.0, 0.0), X, Y, 30000.0, 30000.0);
        const Polyline tier2 =
            Polyline::rectangle(Point(cursor + 5000.0, 5000.0, 0.0), X, Y, 25000.0, 25000.0);
        const Polyline tier3 =
            Polyline::rectangle(Point(cursor + 5000.0, 5000.0, 0.0), X, Y, 20000.0, 20000.0);

        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(30000.0, 5000.0), wood_grid::compute_bays(30000.0, 5000.0)
            )
                .transformed(Xform::translation(cursor, 0.0, 0.0));

        const wood_grid::Framing frm{
            .system = wood_grid::System::GirderPurlin,
            .node = wood_grid::NodeMode::Flush,
            .span = 0,
            .spacing = 2500.0,
            .deck = 160.0,
            .panel_width = 2500.0,
        };
        const wood_grid::Stack stack = wood_grid::Stack::from_levels(
            0.0,
            {
                {4000.0, {tier1, atrium}, pat, frm, {}},
                {8000.0, {tier1, atrium}, pat, frm, {}},
                {11800.0, {tier2, atrium}, pat, frm, {}},
                {15600.0, {tier2, atrium}, pat, frm, {}},
                {19400.0, {tier3, atrium}, pat, frm, {}},
                {23200.0, {tier3, atrium}, pat, frm, {}},
            }
        );
        session.graft(wood_grid::Grid(stack, "setback_tower"), session.add_group("setback_tower"));
        cursor += 30000.0 + GAP;
    }

    // 6. Four-storey 16-sector cylindrical timber rotunda with concentric glulam rings & open central oculus.
    {
        const Point center(cursor + 14000.0, 14000.0, 0.0);
        const wood_grid::Pattern rad_pat =
            wood_grid::Pattern::radial({5000.0, 9500.0, 14000.0}, 16, true)
                .transformed(Xform::translation(center[0], center[1], 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::PostAndBeam,
            .node = wood_grid::NodeMode::Through,
            .span = 0,
            .deck = 160.0,
            .profiles =
                {.column = profile_round(260.0),
                 .girder = profile_rectangle(240.0, 480.0),
                 .beam = profile_rectangle(240.0, 420.0)},
        };
        const wood_grid::Stack stack = wood_grid::Stack::from_footprint(
            rad_pat.boundary, {0.0, 4000.0, 7800.0, 11600.0, 15400.0}, rad_pat, frm
        );
        session.graft(
            wood_grid::Grid(stack, "cylindrical_rotunda"), session.add_group("cylindrical_rotunda")
        );
        cursor += 28000.0 + GAP;
    }

    // 7. Four-storey chamfered pentagonal flatiron building with 45-degree diagonal facade cuts.
    {
        const Polyline flatiron({
            Point(cursor + 0.0, 0.0, 0.0),
            Point(cursor + 12000.0, 12000.0, 0.0),
            Point(cursor + 30000.0, 12000.0, 0.0),
            Point(cursor + 30000.0, 24000.0, 0.0),
            Point(cursor + 0.0, 24000.0, 0.0),
            Point(cursor + 0.0, 0.0, 0.0),
        });
        const wood_grid::Pattern pat =
            wood_grid::Pattern::orthogonal(
                wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(24000.0, 6000.0)
            )
                .transformed(Xform::translation(cursor, 0.0, 0.0));
        const wood_grid::Framing frm{
            .system = wood_grid::System::GirderPurlin,
            .node = wood_grid::NodeMode::Flush,
            .span = 0,
            .spacing = 2000.0,
            .deck = 160.0,
            .panel_width = 3000.0,
        };
        const wood_grid::Stack stack = wood_grid::Stack::from_footprint(
            {flatiron}, {0.0, 3800.0, 7600.0, 11400.0, 15200.0}, pat, frm
        );
        session.graft(
            wood_grid::Grid(stack, "oblique_flatiron"), session.add_group("oblique_flatiron")
        );
        cursor += 30000.0 + GAP;
    }

    // 8. Three-storey hybrid timber office with pinwheel CLT shear core walls and corner X-braced facades.
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
            .panel_width = 2000.0,
            .wall = 240.0,
        };
        wood_grid::Stack stack = wood_grid::Stack::from_footprint(
            {outer}, {0.0, 4000.0, 8000.0, 12000.0}, pat, floor_frm, {core}
        );
        stack.add_perimeter_facades(
            outer,
            0.0,
            12000.0,
            4000.0,
            {4000.0, 4000.0, 4000.0},
            wood_grid::Framing{
                .system = wood_grid::System::PostAndBeam,
                .brace = wood_grid::BraceMode::Cross,
                .span = 1,
                .post_height = 0.0,
                .profiles =
                    {.girder = profile_rectangle(160.0, 200.0),
                     .beam = profile_rectangle(140.0, 200.0),
                     .brace = profile_rectangle(140.0, 160.0)},
            }
        );
        session.graft(wood_grid::Grid(stack, "core_and_facade"), session.add_group("core_and_facade"));
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Eight multi-storey timber building topologies built with wood_grid::Stack: (1) a two-storey L-frame with in-line chamfered column heads and 45-degree notched flush CLT cassette decks; (2) five storeys of shifted cantilevered floor boxes over a plumb column grid; (3) a six-storey tapered tower with seamless continuous leaning columns; (4) a seven-storey 42-degree twisting tower with seamless helicoidal round timber columns; (5) a six-storey cascading stepped-terrace building around a central atrium; (6) a four-storey 16-sector cylindrical ring-and-radial rotunda around an open oculus; (7) a four-storey chamfered pentagonal flatiron building with diagonal facade cuts; and (8) a three-storey hybrid frame with pinwheel CLT core walls and corner X-braced facades.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_5_multistorey --parallel 6 && ./build/templates_grid_5_multistorey && ../bash/publish-scene.sh --target templates_grid_5_multistorey

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
