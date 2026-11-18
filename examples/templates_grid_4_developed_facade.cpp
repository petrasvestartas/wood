#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

namespace {

/// Syncopated vertical glulam fin rhythm inspired by the Blumer-Lehmann Stammhaus (Gossau):
/// five modulated sub-bays summing to each 4500 mm structural bay.
std::vector<double> modulated_fin_bays(int structural_bays) {
    const std::array<double, 5> rhythm = {950.0, 680.0, 1240.0, 880.0, 750.0};
    std::vector<double> out;
    out.reserve(structural_bays * rhythm.size());
    for (int i = 0; i < structural_bays; ++i)
        for (double w : rhythm)
            out.push_back(w);
    return out;
}

/// Builds the 5-storey timber primary frame: recessed ground colonnade (`z = 0..4000`),
/// four cantilevered upper office floors (`900 mm` overhang), and a wide projecting flat timber roof cornice.
Stack make_stammhaus_structure(double x_origin, double length, double depth) {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double overhang = 900.0;
    const double cornice = 1850.0;

    const std::vector<double> base_u = compute_bays(length, 4500.0);
    const std::vector<double> base_v = compute_bays(depth, 5400.0);

    auto make_cantilever_pat = [&](double ext) {
        std::vector<double> cu = {ext};
        cu.insert(cu.end(), base_u.begin(), base_u.end());
        cu.push_back(ext);
        std::vector<double> cv = {ext};
        cv.insert(cv.end(), base_v.begin(), base_v.end());
        cv.push_back(ext);
        return Pattern::orthogonal(cu, cv).transformed(Xform::translation(x_origin - ext, -ext, 0.0));
    };

    const Polyline upper_fp = Polyline::rectangle(
        Point(x_origin - overhang, -overhang, 0.0), X, Y, length + 2.0 * overhang, depth + 2.0 * overhang
    );
    const Polyline roof_fp = Polyline::rectangle(
        Point(x_origin - cornice, -cornice, 0.0), X, Y, length + 2.0 * cornice, depth + 2.0 * cornice
    );
    const Pattern upper_pat = make_cantilever_pat(overhang);
    const Pattern roof_pat = make_cantilever_pat(cornice);

    const Framing office_frm{
        .system = System::GirderPurlin,
        .node = Node::Flush,
        .span = 1,
        .spacing = 1500.0,
        .deck = 180.0,
        .panel = 2250.0,
        .profiles =
            {.column = profile_rectangle(320.0, 320.0),
             .girder = profile_rectangle(260.0, 480.0),
             .beam = profile_rectangle(240.0, 440.0),
             .purlin = profile_rectangle(160.0, 320.0)},
    };
    Framing roof_frm = office_frm;
    roof_frm.deck = 520.0;

    auto floor_at = [&](double z, const Polyline& fp, const Pattern& pat, const Framing& frm) {
        return Stack::Level{
            Plan::from_footprint({fp}, pat, {}, Plane(Point(0.0, 0.0, z), X, Y)), frm
        };
    };

    Stack stack = Stack::from_levels({
        floor_at(4000.0, upper_fp, upper_pat, office_frm),
        floor_at(7600.0, upper_fp, upper_pat, office_frm),
        floor_at(11200.0, upper_fp, upper_pat, office_frm),
        floor_at(14800.0, upper_fp, upper_pat, office_frm),
        floor_at(18400.0, roof_fp, roof_pat, roof_frm),
    });

    // Restrict columns strictly to the recessed colonnade / interior structural grid.
    for (Stack::Level& lvl : stack.levels) {
        Plan& pl = lvl.plan;
        for (size_t v = 0; v < pl.nodes.size(); ++v) {
            const Point w = pl.to_world(pl.nodes[v][0], pl.nodes[v][1], 0.0);
            if (w[0] < x_origin - 10.0 || w[0] > x_origin + length + 10.0 ||
                w[1] < -10.0 || w[1] > depth + 10.0) {
                pl.standing[v] = false;
            }
        }
    }
    return stack;
}

/// Appends the recessed ground-floor storefront glazing and upper-storey ribbon window mullions.
void add_inner_window_envelope(Stack& stack, double x_origin, double length, double depth) {
    const double g_inset = 220.0;
    const std::array<std::pair<Point, Point>, 4> g_sides = {{
        {Point(x_origin + g_inset, g_inset, 0.0), Point(x_origin + length - g_inset, g_inset, 0.0)},
        {Point(x_origin + length - g_inset, g_inset, 0.0),
         Point(x_origin + length - g_inset, depth - g_inset, 0.0)},
        {Point(x_origin + length - g_inset, depth - g_inset, 0.0),
         Point(x_origin + g_inset, depth - g_inset, 0.0)},
        {Point(x_origin + g_inset, depth - g_inset, 0.0), Point(x_origin + g_inset, g_inset, 0.0)},
    }};

    for (const auto& [a, b] : g_sides) {
        const Line g_line = Line::from_points(a, b);
        const Plan g_plan =
            Plan::facade(g_line, 3500.0, compute_bays(g_line.length(), 2250.0), {2750.0, 750.0});
        stack.facades.push_back(Stack::Level{
            g_plan,
            Framing{
                .system = System::PostAndBeam,
                .span = 1,
                .height = 0.0,
                .profiles =
                    {.girder = profile_rectangle(110.0, 160.0),
                     .beam = profile_rectangle(100.0, 160.0)},
            },
        });
    }

    const double u_out = 680.0;
    const std::array<std::pair<Point, Point>, 4> u_sides = {{
        {Point(x_origin - u_out + 180.0, -u_out, 0.0),
         Point(x_origin + length + u_out - 180.0, -u_out, 0.0)},
        {Point(x_origin + length + u_out, -u_out + 180.0, 0.0),
         Point(x_origin + length + u_out, depth + u_out - 180.0, 0.0)},
        {Point(x_origin + length + u_out - 180.0, depth + u_out, 0.0),
         Point(x_origin - u_out + 180.0, depth + u_out, 0.0)},
        {Point(x_origin - u_out, depth + u_out - 180.0, 0.0),
         Point(x_origin - u_out, -u_out + 180.0, 0.0)},
    }};

    for (const auto& [a, b] : u_sides) {
        for (int st = 0; st < 4; ++st) {
            const double z_base = 4180.0 + st * 3600.0;
            const Line u_line =
                Line::from_points(Point(a[0], a[1], z_base), Point(b[0], b[1], z_base));
            const Plan w_plan = Plan::facade(
                u_line, 2960.0, compute_bays(u_line.length(), 1500.0), {760.0, 2200.0}
            );
            stack.facades.push_back(Stack::Level{
                w_plan,
                Framing{
                    .system = System::PostAndBeam,
                    .span = 1,
                    .height = 0.0,
                    .profiles =
                        {.girder = profile_rectangle(100.0, 150.0),
                         .beam = profile_rectangle(90.0, 150.0)},
                },
            });
        }
    }
}

/// Builds one two-layer developed timber facade (`Plan::facade` + `System::StackedPurlin`):
/// - Inner layer (`girder`, `family = 0`): `760 mm`-tall solid horizontal timber spandrel fascia bands!
/// - Outer layer (`purlin`, `family = 1`): continuous `95 x 320 mm` syncopated vertical glulam fins!
Stack::Level make_fin_screen_wall(const Point& a, const Point& b, int n_struct_bays) {
    std::vector<double> bays = modulated_fin_bays(n_struct_bays);
    const double scale = a.distance(b) / (4500.0 * n_struct_bays);
    for (double& w : bays)
        w *= scale;

    const Plan screen_plan = Plan::facade(
        Line::from_points(a, b), 14400.0, bays, {3600.0, 3600.0, 3600.0, 3600.0}
    );
    const Framing screen_frm{
        .system = System::StackedPurlin,
        .node = Node::Flush,
        .span = 0,
        .height = 0.0,
        .spacing = 100000.0,
        .profiles =
            {.girder = profile_rectangle(760.0, 180.0), .purlin = profile_rectangle(95.0, 320.0)},
    };
    return Stack::Level{screen_plan, screen_frm};
}

void add_outer_fin_screens(
    Stack& stack, double x_origin, double length, double depth, int nx, int ny
) {
    const double out = 1540.0;
    const double side_trim = 520.0;
    const double z0 = 3600.0;

    const std::array<std::tuple<Point, Point, int>, 4> screens = {{
        {Point(x_origin - out, -out, z0), Point(x_origin + length + out, -out, z0), nx + 1},
        {Point(x_origin + length + out, -out + side_trim, z0),
         Point(x_origin + length + out, depth + out - side_trim, z0),
         ny + 1},
        {Point(x_origin + length + out, depth + out, z0),
         Point(x_origin - out, depth + out, z0),
         nx + 1},
        {Point(x_origin - out, depth + out - side_trim, z0),
         Point(x_origin - out, -out + side_trim, z0),
         ny + 1},
    }};
    for (const auto& [p0, p1, nb] : screens)
        stack.facades.push_back(make_fin_screen_wall(p0, p1, nb));
}

} // namespace

int main() {
    const int NX = 8;
    const int NY = 2;
    const double LENGTH = NX * 4500.0;
    const double DEPTH = NY * 5400.0;
    const double GAP = 12000.0;

    WoodSession session("templates_grid_4_developed_facade");
    double cursor = 0.0;

    // Stage 1: Primary 5-storey timber structure (recessed colonnade, 4 cantilevered slabs, roof cornice).
    {
        const Stack s1 = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        session.graft(Grid(s1, "stage_1_colonnade"), session.add_group("stage_1_colonnade"));
        cursor += LENGTH + 4000.0 + GAP;
    }

    // Stage 2: Primary structure + recessed ground storefront & upper ribbon window mullions.
    {
        Stack s2 = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        add_inner_window_envelope(s2, cursor, LENGTH, DEPTH);
        session.graft(Grid(s2, "stage_2_spandrels"), session.add_group("stage_2_spandrels"));
        cursor += LENGTH + 4000.0 + GAP;
    }

    // Stage 3: Isolated two-layer brise-soleil screen (`760 mm` spandrel bands + vertical glulam fins).
    {
        const Stack::Level screen = make_fin_screen_wall(
            Point(cursor, 0.0, 3600.0), Point(cursor + LENGTH + 3080.0, 0.0, 3600.0), NX + 1
        );
        session.graft(
            Grid(screen.plan, screen.framing, "stage_3_fin_screen"),
            session.add_group("stage_3_fin_screen")
        );
        cursor += LENGTH + 4000.0 + GAP;
    }

    // Stage 4: Complete 5-storey Blumer-Lehmann-inspired Timber Headquarters.
    {
        Stack full = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        add_inner_window_envelope(full, cursor, LENGTH, DEPTH);
        add_outer_fin_screens(full, cursor, LENGTH, DEPTH, NX, NY);
        session.graft(Grid(full, "stammhaus_headquarters"), session.add_group("stammhaus_headquarters"));
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four-stage tectonic development of a five-storey timber headquarters inspired by the Blumer-Lehmann Stammhaus in Gossau: (1) primary glulam structure with an eight-bay recessed ground colonnade, four cantilevered upper CLT slabs, and a projecting flat timber roof cornice; (2) recessed ground storefront arcade and upper ribbon window mullions; (3) the isolated two-layer developed timber screen (`System::StackedPurlin` on `Plan::facade`) pairing 760 mm solid horizontal timber spandrel bands in the inner layer with continuous syncopated vertical glulam fins in the outer layer; and (4) the complete four-sided headquarters assembly.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_4_developed_facade --parallel 6 && ./build/templates_grid_4_developed_facade && ../bash/publish-scene.sh --target templates_grid_4_developed_facade

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
