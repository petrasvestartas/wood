#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

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
wood_grid::Stack make_stammhaus_structure(double x_origin, double length, double depth) {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double overhang = 900.0;
    const double cornice = 1850.0;

    const std::vector<double> base_u = wood_grid::compute_bays(length, 4500.0);
    const std::vector<double> base_v = wood_grid::compute_bays(depth, 5400.0);

    auto make_cantilever_pat = [&](double ext) {
        std::vector<double> cu = {ext};
        cu.insert(cu.end(), base_u.begin(), base_u.end());
        cu.push_back(ext);
        std::vector<double> cv = {ext};
        cv.insert(cv.end(), base_v.begin(), base_v.end());
        cv.push_back(ext);
        return wood_grid::Pattern::orthogonal(cu, cv)
            .transformed(Xform::translation(x_origin - ext, -ext, 0.0));
    };

    const Polyline upper_fp = Polyline::rectangle(
        Point(x_origin - overhang, -overhang, 0.0), X, Y, length + 2.0 * overhang, depth + 2.0 * overhang
    );
    const Polyline roof_fp = Polyline::rectangle(
        Point(x_origin - cornice, -cornice, 0.0), X, Y, length + 2.0 * cornice, depth + 2.0 * cornice
    );
    const wood_grid::Pattern upper_pat = make_cantilever_pat(overhang);
    const wood_grid::Pattern roof_pat = make_cantilever_pat(cornice);

    const wood_grid::Framing office_frm{
        .system = wood_grid::System::GirderPurlin,
        .node = wood_grid::NodeMode::Flush,
        .span = 1,
        .spacing = 1500.0,
        .deck = 180.0,
        .panel_width = 2250.0,
        .profiles =
            {.column = profile_rectangle(320.0, 320.0),
             .girder = profile_rectangle(260.0, 480.0),
             .beam = profile_rectangle(240.0, 440.0),
             .purlin = profile_rectangle(160.0, 320.0)},
    };
    wood_grid::Framing roof_frm = office_frm;
    roof_frm.deck = 520.0; // Deep projecting flat timber roof cornice crown!

    wood_grid::Stack stack = wood_grid::Stack::from_levels(
        0.0,
        {
            {4000.0, {upper_fp}, upper_pat, office_frm, {}},
            {7600.0, {upper_fp}, upper_pat, office_frm, {}},
            {11200.0, {upper_fp}, upper_pat, office_frm, {}},
            {14800.0, {upper_fp}, upper_pat, office_frm, {}},
            {18400.0, {roof_fp}, roof_pat, roof_frm, {}},
        }
    );

    // Restrict columns strictly to the recessed colonnade / interior structural grid (no posts on cantilevered edges!).
    for (wood_grid::LevelSpec& lvl : stack.levels) {
        wood_grid::Plan& pl = lvl.plan;
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
void add_inner_window_envelope(
    wood_grid::Stack& stack, double x_origin, double length, double depth
) {
    const double g_inset = 220.0;
    const std::array<std::pair<Point, Point>, 4> g_sides = {{
        {Point(x_origin + g_inset, g_inset, 0.0), Point(x_origin + length - g_inset, g_inset, 0.0)},
        {Point(x_origin + length - g_inset, g_inset, 0.0),
         Point(x_origin + length - g_inset, depth - g_inset, 0.0)},
        {Point(x_origin + length - g_inset, depth - g_inset, 0.0),
         Point(x_origin + g_inset, depth - g_inset, 0.0)},
        {Point(x_origin + g_inset, depth - g_inset, 0.0), Point(x_origin + g_inset, g_inset, 0.0)},
    }};

    for (size_t s = 0; s < g_sides.size(); ++s) {
        const Line g_line = Line::from_points(g_sides[s].first, g_sides[s].second);
        const wood_grid::Plan g_plan = wood_grid::Plan::facade(
            g_line, 3500.0, wood_grid::compute_bays(g_line.length(), 2250.0), {2750.0, 750.0}
        );
        stack.facades.push_back(wood_grid::LevelSpec{
            g_plan,
            wood_grid::Framing{
                .system = wood_grid::System::PostAndBeam,
                .span = 1,
                .post_height = 0.0,
                .profiles =
                    {.girder = profile_rectangle(110.0, 160.0),
                     .beam = profile_rectangle(100.0, 160.0)},
            },
            fmt::format("ground_glazing_{}", s),
        });
    }

    // Upper 4 office storeys: ribbon window mullions set near the outer loggia slab edge (`700 mm` out from colonnade).
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

    for (size_t s = 0; s < u_sides.size(); ++s) {
        for (int st = 0; st < 4; ++st) {
            const double z_base = 4180.0 + st * 3600.0;
            const Line u_line = Line::from_points(
                Point(u_sides[s].first[0], u_sides[s].first[1], z_base),
                Point(u_sides[s].second[0], u_sides[s].second[1], z_base)
            );
            const wood_grid::Plan w_plan = wood_grid::Plan::facade(
                u_line, 2960.0, wood_grid::compute_bays(u_line.length(), 1500.0), {760.0, 2200.0}
            );
            stack.facades.push_back(wood_grid::LevelSpec{
                w_plan,
                wood_grid::Framing{
                    .system = wood_grid::System::PostAndBeam,
                    .span = 1,
                    .post_height = 0.0,
                    .profiles =
                        {.girder = profile_rectangle(100.0, 150.0),
                         .beam = profile_rectangle(90.0, 150.0)},
                },
                fmt::format("window_wall_{}_{}", st + 2, s),
            });
        }
    }
}

/// Builds one two-layer developed timber facade (`Plan::facade` + `PurlinMode::Stacked`):
/// - Inner layer (`girder`, `family = 0`): `760 mm`-tall solid horizontal timber spandrel fascia bands at each floor!
/// - Outer layer (`purlin`, `family = 1`): continuous `95 x 320 mm` syncopated vertical glulam fins standing proud!
wood_grid::LevelSpec make_fin_screen_wall(
    const Point& a, const Point& b, int n_struct_bays, const std::string& name
) {
    std::vector<double> bays = modulated_fin_bays(n_struct_bays);
    const double raw_sum = 4500.0 * n_struct_bays;
    const double scale = a.distance(b) / raw_sum;
    for (double& w : bays)
        w *= scale;

    const wood_grid::Plan screen_plan =
        wood_grid::Plan::facade(Line::from_points(a, b), 14400.0, bays, {3600.0, 3600.0, 3600.0, 3600.0});

    const wood_grid::Framing screen_frm{
        .system = wood_grid::System::GirderPurlin,
        .node = wood_grid::NodeMode::Flush,
        .purlin = wood_grid::PurlinMode::Stacked,
        .span = 0,           // Family 0 = 760 mm tall horizontal spandrel bands (inner); Family 1 = continuous vertical fins (outer)!
        .post_height = 0.0,
        .spacing = 100000.0, // Keep exact syncopated fin bays without extra subdivision.
        .profiles =
            {.girder = profile_rectangle(760.0, 180.0),
             .purlin = profile_rectangle(95.0, 320.0)},
    };
    return wood_grid::LevelSpec{screen_plan, screen_frm, name};
}

void add_outer_fin_screens(
    wood_grid::Stack& stack, double x_origin, double length, double depth, int nx, int ny
) {
    // Cantilevered edge reaches `900 + 120 = 1020 mm`; two-layer screen is `180 + 320 = 500 mm` deep -> `out = 1540 mm` (`0.000 mm3` clash!).
    const double out = 1540.0;
    const double side_trim = 520.0;
    const double z0 = 3600.0; // Terminal fins extend +/-380 mm (`z = 3220 .. 18380`), tucking right under the `z = 18400` roof cornice!

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
    for (size_t i = 0; i < screens.size(); ++i) {
        const auto& [p0, p1, nb] = screens[i];
        stack.facades.push_back(make_fin_screen_wall(p0, p1, nb, fmt::format("fin_screen_{}", i)));
    }
}

} // namespace

int main() {
    const int NX = 8;
    const int NY = 2;
    const double LENGTH = NX * 4500.0; // 36.0 m (8-bay colonnade bar matching Blumer-Lehmann Stammhaus)
    const double DEPTH = NY * 5400.0;  // 10.8 m
    const double GAP = 12000.0;

    WoodSession session("templates_grid_4_developed_facade");
    double cursor = 0.0;

    // Stage 1: Primary 5-storey timber structure (recessed colonnade, 4 cantilevered slabs, projecting roof cornice).
    {
        const wood_grid::Stack s1 = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        session.graft(wood_grid::Grid(s1, "stage_1_colonnade"), session.add_group("stage_1_colonnade"));
        cursor += LENGTH + 4000.0 + GAP;
    }

    // Stage 2: Primary structure + recessed ground storefront & upper ribbon window mullions.
    {
        wood_grid::Stack s2 = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        add_inner_window_envelope(s2, cursor, LENGTH, DEPTH);
        session.graft(wood_grid::Grid(s2, "stage_2_spandrels"), session.add_group("stage_2_spandrels"));
        cursor += LENGTH + 4000.0 + GAP;
    }

    // Stage 3: Isolated two-layer brise-soleil screen (`760 mm` horizontal spandrel bands + syncopated vertical glulam fins).
    {
        const wood_grid::LevelSpec screen = make_fin_screen_wall(
            Point(cursor, 0.0, 3600.0), Point(cursor + LENGTH + 3080.0, 0.0, 3600.0), NX + 1, "fin_wall"
        );
        session.graft(
            wood_grid::Grid(screen.plan, screen.framing, "stage_3_fin_screen"),
            session.add_group("stage_3_fin_screen")
        );
        cursor += LENGTH + 4000.0 + GAP;
    }

    // Stage 4: Complete 5-storey Blumer-Lehmann-inspired Timber Headquarters.
    {
        wood_grid::Stack full = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        add_inner_window_envelope(full, cursor, LENGTH, DEPTH);
        add_outer_fin_screens(full, cursor, LENGTH, DEPTH, NX, NY);
        session.graft(
            wood_grid::Grid(full, "stammhaus_headquarters"),
            session.add_group("stammhaus_headquarters")
        );
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four-stage tectonic development of a five-storey timber headquarters inspired by the Blumer-Lehmann Stammhaus in Gossau: (1) primary glulam structure with an eight-bay recessed ground colonnade, four cantilevered upper CLT slabs, and a projecting flat timber roof cornice; (2) recessed ground storefront arcade and upper ribbon window mullions; (3) the isolated two-layer developed timber screen (`PurlinMode::Stacked` on `Plan::facade`) pairing 760 mm solid horizontal timber spandrel bands in the inner layer with continuous syncopated vertical glulam fins in the outer layer; and (4) the complete four-sided headquarters assembly.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_4_developed_facade --parallel 6 && ./build/templates_grid_4_developed_facade && ../bash/publish-scene.sh --target templates_grid_4_developed_facade

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
