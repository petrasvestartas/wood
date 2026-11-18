#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

/// Modulated vertical fin bay spacing inspired by Blumer-Lehmann Stammhaus (Gossau):
/// four syncopated sub-bays summing to each 4800 mm structural bay.
std::vector<double> modulated_fin_bays(int structural_bays) {
    const std::array<double, 4> rhythm = {1150.0, 850.0, 1450.0, 1350.0};
    std::vector<double> out;
    out.reserve(structural_bays * rhythm.size());
    for (int i = 0; i < structural_bays; ++i)
        for (double w : rhythm)
            out.push_back(w);
    return out;
}

/// Builds the 5-storey recessed-colonnade + cantilevered-loggia primary timber frame at `x_origin`.
wood_grid::Stack make_stammhaus_structure(double x_origin, double length, double depth) {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double overhang = 850.0;
    const double cornice = 1250.0;

    const Polyline ground_fp =
        Polyline::rectangle(Point(x_origin, 0.0, 0.0), X, Y, length, depth);
    const Polyline upper_fp = Polyline::rectangle(
        Point(x_origin - overhang, -overhang, 0.0), X, Y, length + 2.0 * overhang, depth + 2.0 * overhang
    );
    const Polyline roof_fp = Polyline::rectangle(
        Point(x_origin - cornice, -cornice, 0.0), X, Y, length + 2.0 * cornice, depth + 2.0 * cornice
    );

    const std::vector<double> base_u = wood_grid::compute_bays(length, 4800.0);
    const std::vector<double> base_v = wood_grid::compute_bays(depth, 4800.0);
    const wood_grid::Pattern ground_pat =
        wood_grid::Pattern::orthogonal(base_u, base_v)
            .transformed(Xform::translation(x_origin, 0.0, 0.0));

    auto with_cantilever = [&](double ext) {
        std::vector<double> cu = {ext};
        cu.insert(cu.end(), base_u.begin(), base_u.end());
        cu.push_back(ext);
        std::vector<double> cv = {ext};
        cv.insert(cv.end(), base_v.begin(), base_v.end());
        cv.push_back(ext);
        return wood_grid::Pattern::orthogonal(cu, cv)
            .transformed(Xform::translation(x_origin - ext, -ext, 0.0));
    };
    const wood_grid::Pattern upper_pat = with_cantilever(overhang);
    const wood_grid::Pattern roof_pat = with_cantilever(cornice);

    const wood_grid::Framing ground_frm{
        .system = wood_grid::System::GirderPurlin,
        .node = wood_grid::NodeMode::Flush,
        .span = 1,
        .spacing = 1600.0,
        .deck = 180.0,
        .panel_width = 2400.0,
        .profiles =
            {.column = profile_rectangle(320.0, 320.0),
             .girder = profile_rectangle(280.0, 560.0),
             .purlin = profile_rectangle(180.0, 360.0)},
    };
    wood_grid::Framing office_frm = ground_frm;
    wood_grid::Framing roof_frm = ground_frm;
    roof_frm.deck = 240.0;

    wood_grid::Stack stack = wood_grid::Stack::from_levels(
        0.0,
        {
            {4400.0, {ground_fp}, ground_pat, ground_frm, {}},
            {7900.0, {upper_fp}, upper_pat, office_frm, {}},
            {11400.0, {upper_fp}, upper_pat, office_frm, {}},
            {14900.0, {upper_fp}, upper_pat, office_frm, {}},
            {18400.0, {roof_fp}, roof_pat, roof_frm, {}},
        }
    );

    // Keep columns strictly on the primary interior/colonnade structural grid (cantilevered edges have no posts!).
    for (size_t li = 1; li < stack.levels.size(); ++li) {
        wood_grid::Plan& pl = stack.levels[li].plan;
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

/// Appends the recessed inner window/spandrel envelope onto `stack`.
void add_inner_spandrel_envelope(
    wood_grid::Stack& stack, double x_origin, double length, double depth
) {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double inset = 220.0;

    // Recessed ground-floor storefront arcade (`z = 0 .. 3840`).
    const std::array<std::pair<Point, Point>, 4> sides = {{
        {Point(x_origin + inset, inset, 0.0), Point(x_origin + length - inset, inset, 0.0)},
        {Point(x_origin + length - inset, inset, 0.0),
         Point(x_origin + length - inset, depth - inset, 0.0)},
        {Point(x_origin + length - inset, depth - inset, 0.0),
         Point(x_origin + inset, depth - inset, 0.0)},
        {Point(x_origin + inset, depth - inset, 0.0), Point(x_origin + inset, inset, 0.0)},
    }};

    for (size_t s = 0; s < sides.size(); ++s) {
        const Line g_line = Line::from_points(sides[s].first, sides[s].second);
        const wood_grid::Plan g_plan = wood_grid::Plan::facade(
            g_line, 3840.0, wood_grid::compute_bays(g_line.length(), 2400.0), {3000.0, 840.0}
        );
        stack.facades.push_back(wood_grid::LevelSpec{
            g_plan,
            wood_grid::Framing{
                .system = wood_grid::System::PostAndBeam,
                .span = 1,
                .post_height = 0.0,
                .profiles =
                    {.girder = profile_rectangle(120.0, 180.0),
                     .beam = profile_rectangle(100.0, 180.0)},
            },
            fmt::format("ground_glazing_{}", s),
        });

        // Upper 4 office storeys: window bands (`2360 mm`) over flush CLT timber spandrel bands (`960 mm`).
        for (int st = 0; st < 4; ++st) {
            const double z_base = 4400.0 + 180.0 + st * 3500.0;
            const Line u_line = Line::from_points(
                Point(sides[s].first[0], sides[s].first[1], z_base),
                Point(sides[s].second[0], sides[s].second[1], z_base)
            );
            const std::vector<double> bays = wood_grid::compute_bays(u_line.length(), 1600.0);
            std::vector<Polyline> window_voids;
            double u_cur = 0.0;
            for (double bw : bays) {
                window_voids.push_back(
                    Polyline::rectangle(Point(u_cur + 60.0, 920.0, 0.0), X, Y, bw - 120.0, 1960.0)
                );
                u_cur += bw;
            }
            const wood_grid::Plan sp_plan =
                wood_grid::Plan::facade(u_line, 2940.0, bays, {900.0, 2040.0}, window_voids);
            stack.facades.push_back(wood_grid::LevelSpec{
                sp_plan,
                wood_grid::Framing{
                    .system = wood_grid::System::PostAndBeam,
                    .span = 1,
                    .post_height = 0.0,
                    .deck = 100.0,
                    .deck_flush = true,
                    .profiles =
                        {.girder = profile_rectangle(110.0, 160.0),
                         .beam = profile_rectangle(100.0, 160.0)},
                },
                fmt::format("spandrel_{}_{}", st + 2, s),
            });
        }
    }
}

/// Builds one two-layer modulated vertical timber fin screen (`PurlinMode::Stacked` on a vertical Plane):
/// continuous vertical glulam fins stand proud in front of horizontal loggia spandrel ledgers!
wood_grid::LevelSpec make_fin_screen_wall(
    const Point& a, const Point& b, int n_struct_bays, const std::string& name
) {
    std::vector<double> bays = modulated_fin_bays(n_struct_bays);
    const double raw_sum = 4800.0 * n_struct_bays;
    const double scale = a.distance(b) / raw_sum;
    for (double& w : bays)
        w *= scale;

    const wood_grid::Plan screen_plan =
        wood_grid::Plan::facade(Line::from_points(a, b), 13720.0, bays, {3430.0, 3430.0, 3430.0, 3430.0});

    const wood_grid::Framing screen_frm{
        .system = wood_grid::System::GirderPurlin,
        .node = wood_grid::NodeMode::Flush,
        .purlin = wood_grid::PurlinMode::Stacked,
        .span = 0,          // Family 0 = horizontal ledgers (inner layer); Family 1 = continuous vertical fins (outer layer)!
        .post_height = 0.0,
        .spacing = 100000.0, // Use exact modulated fin bays without extra subdivision.
        .profiles =
            {.girder = profile_rectangle(220.0, 160.0),
             .purlin = profile_rectangle(110.0, 320.0)},
    };
    return wood_grid::LevelSpec{screen_plan, screen_frm, name};
}

void add_outer_fin_screens(
    wood_grid::Stack& stack, double x_origin, double length, double depth, int nx, int ny
) {
    const double out = 850.0 + 320.0 + 10.0; // Proud of the cantilevered slab edge (`0.000 mm3` clash!)
    const double z0 = 4580.0;
    const std::array<std::tuple<Point, Point, int>, 4> screens = {{
        {Point(x_origin - 850.0, -out, z0), Point(x_origin + length + 850.0, -out, z0), nx},
        {Point(x_origin + length + out, -850.0, z0),
         Point(x_origin + length + out, depth + 850.0, z0),
         ny},
        {Point(x_origin + length + 850.0, depth + out, z0),
         Point(x_origin - 850.0, depth + out, z0),
         nx},
        {Point(x_origin - out, depth + 850.0, z0), Point(x_origin - out, -850.0, z0), ny},
    }};
    for (size_t i = 0; i < screens.size(); ++i) {
        const auto& [p0, p1, nb] = screens[i];
        stack.facades.push_back(make_fin_screen_wall(p0, p1, nb, fmt::format("fin_screen_{}", i)));
    }
}

} // namespace

int main() {
    const int NX = 6;
    const int NY = 3;
    const double LENGTH = NX * 4800.0; // 28.8 m
    const double DEPTH = NY * 4800.0;  // 14.4 m
    const double GAP = 10000.0;

    WoodSession session("templates_grid_4_developed_facade");
    double cursor = 0.0;

    // Stage 1: Primary 5-storey frame with recessed ground colonnade, cantilevered upper slabs & roof cornice.
    {
        const wood_grid::Stack s1 = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        session.graft(wood_grid::Grid(s1, "stage_1_colonnade"), session.add_group("stage_1_colonnade"));
        cursor += LENGTH + 3000.0 + GAP;
    }

    // Stage 2: Primary frame + recessed ground glazing & upper punched timber spandrel bands.
    {
        wood_grid::Stack s2 = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        add_inner_spandrel_envelope(s2, cursor, LENGTH, DEPTH);
        session.graft(wood_grid::Grid(s2, "stage_2_spandrels"), session.add_group("stage_2_spandrels"));
        cursor += LENGTH + 3000.0 + GAP;
    }

    // Stage 3: Isolated two-layer developed vertical timber fin screen (`PurlinMode::Stacked` on vertical Plane).
    {
        const wood_grid::LevelSpec screen = make_fin_screen_wall(
            Point(cursor, 0.0, 4580.0), Point(cursor + LENGTH + 1700.0, 0.0, 4580.0), NX, "fin_wall"
        );
        session.graft(
            wood_grid::Grid(screen.plan, screen.framing, "stage_3_fin_screen"),
            session.add_group("stage_3_fin_screen")
        );
        cursor += LENGTH + 3000.0 + GAP;
    }

    // Stage 4: Complete 5-storey Blumer-Lehmann-inspired Timber Headquarters.
    {
        wood_grid::Stack full = make_stammhaus_structure(cursor, LENGTH, DEPTH);
        add_inner_spandrel_envelope(full, cursor, LENGTH, DEPTH);
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
Step-by-step development of a five-storey timber headquarters facade inspired by Blumer-Lehmann Stammhaus (Gossau): Stage 1 builds the recessed ground-floor glulam colonnade, cantilevered upper CLT floor plates and projecting roof cornice; Stage 2 adds the recessed ground storefront and upper-floor punched timber spandrel bands; Stage 3 isolates the two-layer brise-soleil screen where modulated vertical glulam fins ({1150, 850, 1450, 1350} mm rhythm) run continuously in front of horizontal loggia ledgers via PurlinMode::Stacked on a vertical Plane; Stage 4 assembles the complete four-sided headquarters.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_4_developed_facade --parallel 6 && ./build/templates_grid_4_developed_facade && ../bash/publish-scene.sh --target templates_grid_4_developed_facade

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
