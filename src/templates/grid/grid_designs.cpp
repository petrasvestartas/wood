#include "pch.h"
#include "src/templates/grid/grid_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_grid {

namespace {

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);

constexpr double OFFICE_LENGTH = 36000.0; // Eight bays of 4500 along the colonnade office.
constexpr double OFFICE_DEPTH = 10800.0; // Two bays of 5400 across it.

/// A fin screen from a to b, 14400 tall in four storeys: bays of five fins, the rhythm stretched to the length.
Plan fin_screen_between(const Point& a, const Point& b, int bays) {

    const std::array<double, 5> rhythm = {950.0, 680.0, 1240.0, 880.0, 750.0};
    std::vector<double> fins;
    for (int i = 0; i < bays; ++i)
        for (double width : rhythm)
            fins.push_back(width);
    const double scale = a.distance(b) / (4500.0 * bays);
    for (double& width : fins)
        width *= scale;

    return Plan::facade(Line::from_points(a, b), 14400.0, fins, {3600.0, 3600.0, 3600.0, 3600.0});
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Profiles
// ═══════════════════════════════════════════════════════════════════════════

Profiles Profiles::heavy() {
    return Profiles{
        .column = profile_rectangle(280.0, 280.0),
        .girder = profile_rectangle(280.0, 600.0),
        .purlin = profile_rectangle(220.0, 440.0),
    };
}

Profiles Profiles::mullions() {
    return Profiles{
        .girder = profile_rectangle(160.0, 280.0),
        .beam = profile_rectangle(140.0, 280.0),
        .brace = profile_rectangle(140.0, 180.0),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Framing of bays
// ═══════════════════════════════════════════════════════════════════════════

Framing Framing::l_shaped_floor() {
    return Framing{
        .system = 1,
        .node = 0,
        .span = 0,
        .height = 3600.0,
        .deck = 160.0,
        .profiles = {
            .column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 440.0)
        },
    };
}

Framing Framing::slab_on_heads() {
    return Framing{
        .system = 0,
        .node = 2,
        .height = 4200.0,
        .deck = 240.0,
        .panel = 3000.0,
        .head = 280.0,
        .reach = 240.0,
        .profiles = Profiles::heavy(),
    };
}

Framing Framing::beams_on_heads() {
    return Framing{
        .system = 1,
        .node = 2,
        .span = -1,
        .height = 4200.0,
        .reach = 220.0,
        .profiles = {
            .column = profile_rectangle(260.0, 260.0), .girder = profile_rectangle(260.0, 460.0)
        },
    };
}

Framing Framing::girders_and_purlins() {
    return Framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .height = 4200.0,
        .spacing = 2000.0,
        .deck = 140.0,
        .panel = 2000.0,
        .profiles = Profiles::heavy(),
    };
}

Framing Framing::through_columns() {
    return Framing{
        .system = 2,
        .node = 1,
        .span = 0,
        .height = 4200.0,
        .spacing = 2000.0,
        .profiles = Profiles::heavy(),
    };
}

Framing Framing::stacked_purlins() {
    return Framing{
        .system = 3,
        .node = 0,
        .span = 0,
        .height = 4200.0,
        .spacing = 2000.0,
        .profiles = Profiles::heavy(),
    };
}

Framing Framing::round_columns() {
    return Framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .height = 4200.0,
        .spacing = 2000.0,
        .deck = 140.0,
        .profiles = {
            .column = profile_round(280.0),
            .girder = profile_rectangle(280.0, 600.0),
            .purlin = profile_rectangle(220.0, 440.0)
        },
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Framing of floors
// ═══════════════════════════════════════════════════════════════════════════

Framing Framing::large_l_shaped_floor() {
    return Framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .height = 4000.0,
        .spacing = 2500.0,
        .deck = 160.0,
        .panel = 2500.0,
    };
}

Framing Framing::skewed_floor() {
    return Framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .height = 4000.0,
        .spacing = 2000.0,
    };
}

Framing Framing::ring_floor() {
    return Framing{
        .system = 1,
        .node = 0,
        .span = 0,
        .height = 4000.0,
        .profiles = {
            .column = profile_round(240.0),
            .girder = profile_rectangle(240.0, 480.0),
            .beam = profile_rectangle(240.0, 420.0)
        },
    };
}

Framing Framing::triangular_floor() {
    return Framing{
        .system = 1,
        .node = 0,
        .span = -1,
        .height = 4000.0,
        .profiles = {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 440.0)},
    };
}

Framing Framing::hexagonal_floor() {
    return Framing{
        .system = 1,
        .node = 0,
        .span = -1,
        .height = 4000.0,
        .profiles = {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 420.0)},
    };
}

Framing Framing::irregular_floor() {
    return Framing{
        .system = 1,
        .node = 0,
        .span = 0,
        .height = 4000.0,
        .profiles = {
            .column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 460.0)
        },
    };
}

Framing Framing::courtyard_floor() {
    return Framing{
        .system = 2,
        .node = 1,
        .span = 0,
        .height = 4000.0,
        .spacing = 2666.7,
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Framing of facades
// ═══════════════════════════════════════════════════════════════════════════

Framing Framing::curtain_wall() {
    return Framing{
        .system = 1,
        .span = 1,
        .height = 0.0,
        .profiles = Profiles::mullions(),
    };
}

Framing Framing::punched_wall() {
    return Framing{
        .system = 1,
        .span = 1,
        .height = 0.0,
        .deck = 120.0,
        .deck_flush = true,
        .profiles = Profiles::mullions(),
    };
}

Framing Framing::braced_wall() {

    // outer bays: those whose middle is in the first or the last bay of 4500
    const Plan plan = Plan::braced_wall();
    std::vector<size_t> outer;
    for (size_t cell = 0; cell < plan.cells.size(); ++cell) {
        std::vector<Point> points;
        for (size_t node : plan.cells[cell].outer)
            points.push_back(plan.nodes[node]);
        const Point middle = Point::centroid(points);
        if (middle[0] < 4500.0 || middle[0] > 13500.0)
            outer.push_back(cell);
    }

    return Framing{
        .system = 1,
        .brace = 2,
        .braced_bays = outer,
        .span = 1,
        .height = 0.0,
        .profiles = {
            .girder = profile_rectangle(240.0, 360.0),
            .beam = profile_rectangle(220.0, 360.0),
            .brace = profile_rectangle(160.0, 240.0)
        },
    };
}

Framing Framing::gable_wall() {
    return Framing{
        .system = 1,
        .brace = 3,
        .span = 1,
        .height = 0.0,
        .profiles = Profiles::mullions(),
    };
}

Framing Framing::fin_screen() {
    return Framing{
        .system = 3,
        .node = 0,
        .span = 0,
        .height = 0.0,
        .spacing = 100000.0,
        .profiles = {
            .girder = profile_rectangle(760.0, 180.0), .purlin = profile_rectangle(95.0, 320.0)
        },
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Plans of bays and floors
// ═══════════════════════════════════════════════════════════════════════════

Plan Plan::l_shaped_floor() {

    const Polyline outline({
        Point(0.0, 0.0, 0.0),
        Point(12000.0, 0.0, 0.0),
        Point(12000.0, 4000.0, 0.0),
        Point(8000.0, 4000.0, 0.0),
        Point(8000.0, 8000.0, 0.0),
        Point(0.0, 8000.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });

    return Plan::from_footprint(
        {outline},
        Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}),
        {},
        Plane(Point(0.0, 0.0, 3600.0), X, Y)
    );
}

Plan Plan::square_bay() {
    return Plan::from_pattern(
        Pattern::orthogonal({6000.0}, {6000.0}), Plane(Point(0.0, 0.0, 4200.0), X, Y)
    );
}

Plan Plan::double_bay() {
    return Plan::from_pattern(
        Pattern::orthogonal({4500.0, 4500.0}, {6000.0}), Plane(Point(0.0, 0.0, 4200.0), X, Y)
    );
}

Plan Plan::large_l_shaped_floor() {

    const Polyline outline({
        Point(0.0, 0.0, 0.0),
        Point(24000.0, 0.0, 0.0),
        Point(24000.0, 7500.0, 0.0),
        Point(16000.0, 7500.0, 0.0),
        Point(16000.0, 15000.0, 0.0),
        Point(0.0, 15000.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });

    return Plan::from_footprint(
        {outline},
        Pattern::orthogonal({8000.0, 8000.0, 8000.0}, {7500.0, 7500.0}),
        {},
        Plane(Point(0.0, 0.0, 4000.0), X, Y)
    );
}

Plan Plan::skewed_floor() {
    return Plan::from_pattern(
        Pattern::orthogonal(std::vector<double>(4, 6000.0), std::vector<double>(3, 6000.0), 30.0),
        Plane(Point(0.0, 0.0, 4000.0), X, Y)
    );
}

Plan Plan::ring_floor() {
    return Plan::from_pattern(
        Pattern::radial({4000.0, 8000.0, 12000.0}, 12, true), Plane(Point(0.0, 0.0, 4000.0), X, Y)
    );
}

Plan Plan::triangular_floor() {
    return Plan::from_pattern(
        Pattern::triangular(6000.0, 4, 3), Plane(Point(0.0, 0.0, 4000.0), X, Y)
    );
}

Plan Plan::hexagonal_floor() {
    return Plan::from_pattern(
        Pattern::hexagonal(4200.0, 3, 2), Plane(Point(0.0, 0.0, 4000.0), X, Y)
    );
}

Plan Plan::irregular_floor() {

    const Polyline outline({
        Point(0.0, 0.0, 0.0),
        Point(26000.0, 1500.0, 0.0),
        Point(28000.0, 13500.0, 0.0),
        Point(14000.0, 18000.0, 0.0),
        Point(-1000.0, 11500.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });
    const Pattern axes = Pattern::from_lines({
        Line::from_points(Point(-2000.0, 4800.0, 0.0), Point(30000.0, 6000.0, 0.0)),
        Line::from_points(Point(-2000.0, 9000.0, 0.0), Point(30000.0, 10200.0, 0.0)),
        Line::from_points(Point(7200.0, -2000.0, 0.0), Point(5800.0, 20000.0, 0.0)),
        Line::from_points(Point(14800.0, -2000.0, 0.0), Point(15600.0, 20000.0, 0.0)),
        Line::from_points(Point(21200.0, -2000.0, 0.0), Point(22000.0, 20000.0, 0.0)),
    });

    return Plan::from_footprint({outline}, axes, {}, Plane(Point(0.0, 0.0, 4000.0), X, Y));
}

Plan Plan::courtyard_floor() {
    return Plan::from_footprint(
        {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 32000.0, 24000.0),
         Polyline::rectangle(Point(8000.0, 8000.0, 0.0), X, Y, 16000.0, 8000.0)},
        Pattern::orthogonal(compute_bays(32000.0, 8000.0), compute_bays(24000.0, 8000.0)),
        {},
        Plane(Point(0.0, 0.0, 4000.0), X, Y)
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// Plans of facades
// ═══════════════════════════════════════════════════════════════════════════

Plan Plan::curtain_wall() {
    return Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(16000.0, 0.0, 0.0)),
        10800.0,
        {4000.0, 4000.0, 4000.0, 4000.0},
        {3600.0, 3600.0, 3600.0}
    );
}

Plan Plan::punched_wall() {

    const std::vector<Polyline> openings = {
        Polyline::rectangle(Point(3000.0, 0.0, 0.0), X, Y, 3000.0, 3200.0),
        Polyline::rectangle(Point(9000.0, 900.0, 0.0), X, Y, 3000.0, 2300.0),
        Polyline::rectangle(Point(3000.0, 4500.0, 0.0), X, Y, 3000.0, 2300.0),
        Polyline::rectangle(Point(9000.0, 4500.0, 0.0), X, Y, 3000.0, 2300.0),
    };

    return Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(15000.0, 0.0, 0.0)),
        8000.0,
        {3000.0, 3000.0, 3000.0, 3000.0, 3000.0},
        {900.0, 2300.0, 1300.0, 2300.0, 1200.0},
        openings
    );
}

Plan Plan::braced_wall() {
    return Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(18000.0, 0.0, 0.0)),
        11400.0,
        {4500.0, 4500.0, 4500.0, 4500.0},
        {3800.0, 3800.0, 3800.0}
    );
}

Plan Plan::gable_wall() {

    const Polyline outline({
        Point(0.0, 0.0, 0.0),
        Point(16000.0, 0.0, 0.0),
        Point(16000.0, 6400.0, 0.0),
        Point(8000.0, 10800.0, 0.0),
        Point(0.0, 6400.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });

    return Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(16000.0, 0.0, 0.0)),
        10800.0,
        {3200.0, 3200.0, 3200.0, 3200.0, 3200.0},
        {3200.0, 3200.0, 2200.0, 2200.0},
        {},
        outline
    );
}

Plan Plan::fin_screen() {
    return fin_screen_between(
        Point(0.0, 0.0, 3600.0), Point(OFFICE_LENGTH + 3080.0, 0.0, 3600.0), 9
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// Stacks
// ═══════════════════════════════════════════════════════════════════════════

Stack Stack::l_shaped_two_storey(double x) {

    const Polyline outline({
        Point(x, 0.0, 0.0),
        Point(x + 12000.0, 0.0, 0.0),
        Point(x + 12000.0, 3000.0, 0.0),
        Point(x + 8000.0, 3000.0, 0.0),
        Point(x + 8000.0, 6000.0, 0.0),
        Point(x, 6000.0, 0.0),
        Point(x, 0.0, 0.0),
    });
    const Pattern pattern = Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {3000.0, 3000.0})
                                .transformed(Xform::translation(x, 0.0, 0.0));
    const Framing framing{
        .system = 1,
        .node = 2,
        .span = -1,
        .deck = 180.0,
        .deck_flush = true,
        .reach = 220.0,
        .profiles = {
            .column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 380.0)
        },
    };

    return Stack::from_footprint({outline}, {0.0, 3400.0, 6800.0}, pattern, framing);
}

Stack Stack::shifted_floors(double x) {

    const Polyline outline =
        Polyline::rectangle(Point(x + 3000.0, 3000.0, 0.0), X, Y, 18000.0, 12000.0);
    const Pattern pattern =
        Pattern::orthogonal(std::vector<double>(8, 3000.0), std::vector<double>(6, 3000.0))
            .transformed(Xform::translation(x, 0.0, 0.0));
    const Framing framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .spacing = 1500.0,
        .deck = 160.0,
        .panel = 3000.0,
        .profiles = {
            .column = profile_rectangle(260.0, 260.0),
            .girder = profile_rectangle(260.0, 480.0),
            .purlin = profile_rectangle(160.0, 320.0)
        },
    };
    const auto floor = [&](double z, double dx, double dy) {
        const Polyline shifted = outline.transformed(Xform::translation(dx, dy, 0.0));
        return Level{
            Plan::from_footprint({shifted}, pattern, {}, Plane(Point(0.0, 0.0, z), X, Y)), framing
        };
    };

    return Stack::from_levels({
        floor(3800.0, 0.0, 0.0),
        floor(7600.0, 3000.0, 0.0),
        floor(11400.0, 3000.0, 3000.0),
        floor(15200.0, -3000.0, 3000.0),
        floor(19000.0, 0.0, -3000.0),
    });
}

Stack Stack::tapered_tower(double x) {

    const Point pivot(x + 10500.0, 10500.0, 0.0);
    const Pattern pattern =
        Pattern::orthogonal(compute_bays(21000.0, 7000.0), compute_bays(21000.0, 7000.0))
            .transformed(Xform::translation(x, 0.0, 0.0));
    const Framing framing{
        .system = 2,
        .node = 1,
        .span = 0,
        .spacing = 2200.0,
        .deck = 160.0,
        .panel = 2500.0,
        .profiles = {
            .column = profile_rectangle(300.0, 300.0),
            .girder = profile_rectangle(300.0, 500.0),
            .purlin = profile_rectangle(160.0, 320.0)
        },
    };

    return Stack::morph(
        {Polyline::rectangle(Point(x, 0.0, 0.0), X, Y, 21000.0, 21000.0)},
        {0.0, 4000.0, 7800.0, 11600.0, 15400.0, 19200.0, 23000.0},
        pattern,
        [pivot](double t) {
            const double scale = 1.0 - 0.45 * t;
            return Xform::scale_non_uniform(pivot, scale, scale, 1.0);
        },
        framing
    );
}

Stack Stack::twisted_tower(double x) {

    const Point pivot(x + 12000.0, 12000.0, 0.0);
    const Pattern pattern =
        Pattern::orthogonal(compute_bays(18000.0, 6000.0), compute_bays(18000.0, 6000.0))
            .transformed(Xform::translation(x + 3000.0, 3000.0, 0.0));
    const Framing framing{
        .system = 1,
        .node = 1,
        .span = 0,
        .deck = 160.0,
        .panel = 3000.0,
        .profiles = {
            .column = profile_round(320.0),
            .girder = profile_rectangle(280.0, 460.0),
            .beam = profile_rectangle(240.0, 420.0)
        },
    };

    return Stack::morph(
        {Polyline::rectangle(Point(x + 3000.0, 3000.0, 0.0), X, Y, 18000.0, 18000.0)},
        {0.0, 3600.0, 7200.0, 10800.0, 14400.0, 18000.0, 21600.0, 25200.0},
        pattern,
        [pivot](double t) {
            return Xform::translation(pivot[0], pivot[1], 0.0) * Xform::rotation_z(42.0 * t, true) *
                   Xform::translation(-pivot[0], -pivot[1], 0.0);
        },
        framing
    );
}

Stack Stack::setback_tower(double x) {

    const Polyline atrium =
        Polyline::rectangle(Point(x + 10000.0, 10000.0, 0.0), X, Y, 10000.0, 10000.0);
    const Polyline lower = Polyline::rectangle(Point(x, 0.0, 0.0), X, Y, 30000.0, 30000.0);
    const Polyline middle =
        Polyline::rectangle(Point(x + 5000.0, 5000.0, 0.0), X, Y, 25000.0, 25000.0);
    const Polyline upper =
        Polyline::rectangle(Point(x + 5000.0, 5000.0, 0.0), X, Y, 20000.0, 20000.0);
    const Pattern pattern =
        Pattern::orthogonal(compute_bays(30000.0, 5000.0), compute_bays(30000.0, 5000.0))
            .transformed(Xform::translation(x, 0.0, 0.0));
    const Framing framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .spacing = 2500.0,
        .deck = 160.0,
        .panel = 2500.0,
    };
    const auto terrace = [&](double z, const Polyline& tier) {
        return Level{
            Plan::from_footprint({tier, atrium}, pattern, {}, Plane(Point(0.0, 0.0, z), X, Y)),
            framing
        };
    };

    return Stack::from_levels({
        terrace(4000.0, lower),
        terrace(8000.0, lower),
        terrace(11800.0, middle),
        terrace(15600.0, middle),
        terrace(19400.0, upper),
        terrace(23200.0, upper),
    });
}

Stack Stack::rotunda(double x) {

    const Pattern pattern = Pattern::radial({5000.0, 9500.0, 14000.0}, 16, true)
                                .transformed(Xform::translation(x + 14000.0, 14000.0, 0.0));
    const Framing framing{
        .system = 1,
        .node = 1,
        .span = 0,
        .deck = 160.0,
        .profiles = {
            .column = profile_round(260.0),
            .girder = profile_rectangle(240.0, 480.0),
            .beam = profile_rectangle(240.0, 420.0)
        },
    };

    return Stack::from_footprint(
        pattern.boundary, {0.0, 4000.0, 7800.0, 11600.0, 15400.0}, pattern, framing
    );
}

Stack Stack::flatiron(double x) {

    const Polyline outline({
        Point(x, 0.0, 0.0),
        Point(x + 12000.0, 12000.0, 0.0),
        Point(x + 30000.0, 12000.0, 0.0),
        Point(x + 30000.0, 24000.0, 0.0),
        Point(x, 24000.0, 0.0),
        Point(x, 0.0, 0.0),
    });
    const Pattern pattern =
        Pattern::orthogonal(compute_bays(30000.0, 6000.0), compute_bays(24000.0, 6000.0))
            .transformed(Xform::translation(x, 0.0, 0.0));
    const Framing framing{
        .system = 2,
        .node = 0,
        .span = 0,
        .spacing = 2000.0,
        .deck = 160.0,
        .panel = 3000.0,
    };

    return Stack::from_footprint(
        {outline}, {0.0, 3800.0, 7600.0, 11400.0, 15200.0}, pattern, framing
    );
}

Stack Stack::core_and_facades(double x) {

    const Pattern pattern =
        Pattern::orthogonal(compute_bays(24000.0, 8000.0), compute_bays(18000.0, 6000.0))
            .transformed(Xform::translation(x, 0.0, 0.0));
    const Framing floors{
        .system = 2,
        .node = 0,
        .span = 0,
        .spacing = 2000.0,
        .deck = 160.0,
        .panel = 2000.0,
        .wall = 240.0,
    };
    const Framing facades{
        .system = 1,
        .brace = 2,
        .span = 1,
        .profiles = {
            .girder = profile_rectangle(160.0, 200.0),
            .beam = profile_rectangle(140.0, 200.0),
            .brace = profile_rectangle(140.0, 160.0)
        },
    };

    Stack stack = Stack::from_footprint(
        {Polyline::rectangle(Point(x, 0.0, 0.0), X, Y, 24000.0, 18000.0)},
        {0.0, 4000.0, 8000.0, 12000.0},
        pattern,
        floors,
        {Polyline::rectangle(Point(x + 8000.0, 6000.0, 0.0), X, Y, 8000.0, 6000.0)}
    );
    stack.add_facades(4000.0, facades);

    return stack;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stacks of the colonnade office
// ═══════════════════════════════════════════════════════════════════════════

Stack Stack::colonnade_office(double x) {

    const double length = OFFICE_LENGTH;
    const double depth = OFFICE_DEPTH;
    const double overhang = 900.0;
    const double cornice = 1850.0;
    const std::vector<double> u_bays = compute_bays(length, 4500.0);
    const std::vector<double> v_bays = compute_bays(depth, 5400.0);

    // patterns: the colonnade's bays with a cantilever bay of ext on every side
    const auto cantilevered = [&](double ext) {
        std::vector<double> u = {ext};
        u.insert(u.end(), u_bays.begin(), u_bays.end());
        u.push_back(ext);
        std::vector<double> v = {ext};
        v.insert(v.end(), v_bays.begin(), v_bays.end());
        v.push_back(ext);
        return Pattern::orthogonal(u, v).transformed(Xform::translation(x - ext, -ext, 0.0));
    };
    const Polyline upper = Polyline::rectangle(
        Point(x - overhang, -overhang, 0.0), X, Y, length + 2.0 * overhang, depth + 2.0 * overhang
    );
    const Polyline roof = Polyline::rectangle(
        Point(x - cornice, -cornice, 0.0), X, Y, length + 2.0 * cornice, depth + 2.0 * cornice
    );
    const Pattern upper_pattern = cantilevered(overhang);
    const Pattern roof_pattern = cantilevered(cornice);

    // framings: girders with purlins under a 180 deck, the roof's deck 520
    const Framing office{
        .system = 2,
        .node = 0,
        .span = 1,
        .spacing = 1500.0,
        .deck = 180.0,
        .panel = 2250.0,
        .profiles = {
            .column = profile_rectangle(320.0, 320.0),
            .girder = profile_rectangle(260.0, 480.0),
            .beam = profile_rectangle(240.0, 440.0),
            .purlin = profile_rectangle(160.0, 320.0)
        },
    };
    Framing top = office;
    top.deck = 520.0;

    // floors: four office floors and the roof
    const auto floor =
        [&](double z, const Polyline& outline, const Pattern& pattern, const Framing& framing) {
            return Level{
                Plan::from_footprint({outline}, pattern, {}, Plane(Point(0.0, 0.0, z), X, Y)),
                framing
            };
        };
    Stack stack = Stack::from_levels({
        floor(4000.0, upper, upper_pattern, office),
        floor(7600.0, upper, upper_pattern, office),
        floor(11200.0, upper, upper_pattern, office),
        floor(14800.0, upper, upper_pattern, office),
        floor(18400.0, roof, roof_pattern, top),
    });

    // columns: only within the colonnade
    for (Level& level : stack.levels) {
        Plan& plan = level.plan;
        for (size_t v = 0; v < plan.nodes.size(); ++v) {
            const Point world = plan.to_world(plan.nodes[v][0], plan.nodes[v][1], 0.0);
            if (world[0] < x - 10.0 || world[0] > x + length + 10.0 || world[1] < -10.0 ||
                world[1] > depth + 10.0)
                plan.standing[v] = false;
        }
    }

    return stack;
}

Stack Stack::glazed_office(double x) {

    Stack stack = Stack::colonnade_office(x);
    const double length = OFFICE_LENGTH;
    const double depth = OFFICE_DEPTH;

    // storefront: four walls set 220 into the colonnade, 3500 tall
    const double inset = 220.0;
    const std::array<std::pair<Point, Point>, 4> store_sides = {{
        {Point(x + inset, inset, 0.0), Point(x + length - inset, inset, 0.0)},
        {Point(x + length - inset, inset, 0.0), Point(x + length - inset, depth - inset, 0.0)},
        {Point(x + length - inset, depth - inset, 0.0), Point(x + inset, depth - inset, 0.0)},
        {Point(x + inset, depth - inset, 0.0), Point(x + inset, inset, 0.0)},
    }};
    const Framing store{
        .system = 1,
        .span = 1,
        .height = 0.0,
        .profiles = {
            .girder = profile_rectangle(110.0, 160.0), .beam = profile_rectangle(100.0, 160.0)
        },
    };
    for (const std::pair<Point, Point>& side : store_sides) {
        const Line line = Line::from_points(side.first, side.second);
        const Plan wall =
            Plan::facade(line, 3500.0, compute_bays(line.length(), 2250.0), {2750.0, 750.0});
        stack.facades.push_back(Level{wall, store});
    }

    // ribbon windows: four walls 680 out from the colonnade on every upper storey
    const double out = 680.0;
    const std::array<std::pair<Point, Point>, 4> ribbon_sides = {{
        {Point(x - out + 180.0, -out, 0.0), Point(x + length + out - 180.0, -out, 0.0)},
        {Point(x + length + out, -out + 180.0, 0.0),
         Point(x + length + out, depth + out - 180.0, 0.0)},
        {Point(x + length + out - 180.0, depth + out, 0.0),
         Point(x - out + 180.0, depth + out, 0.0)},
        {Point(x - out, depth + out - 180.0, 0.0), Point(x - out, -out + 180.0, 0.0)},
    }};
    const Framing ribbon{
        .system = 1,
        .span = 1,
        .height = 0.0,
        .profiles = {
            .girder = profile_rectangle(100.0, 150.0), .beam = profile_rectangle(90.0, 150.0)
        },
    };
    for (const std::pair<Point, Point>& side : ribbon_sides) {
        for (int storey = 0; storey < 4; ++storey) {
            const double z = 4180.0 + storey * 3600.0;
            const Line line = Line::from_points(
                Point(side.first[0], side.first[1], z), Point(side.second[0], side.second[1], z)
            );
            const Plan wall =
                Plan::facade(line, 2960.0, compute_bays(line.length(), 1500.0), {760.0, 2200.0});
            stack.facades.push_back(Level{wall, ribbon});
        }
    }

    return stack;
}

Stack Stack::screened_office(double x) {

    Stack stack = Stack::glazed_office(x);
    const double length = OFFICE_LENGTH;
    const double depth = OFFICE_DEPTH;

    // fin screens: 1540 out from the colonnade, the short sides trimmed 520 at each end
    const double out = 1540.0;
    const double trim = 520.0;
    const double z = 3600.0;
    const std::array<std::tuple<Point, Point, int>, 4> screens = {{
        {Point(x - out, -out, z), Point(x + length + out, -out, z), 9},
        {Point(x + length + out, -out + trim, z),
         Point(x + length + out, depth + out - trim, z),
         3},
        {Point(x + length + out, depth + out, z), Point(x - out, depth + out, z), 9},
        {Point(x - out, depth + out - trim, z), Point(x - out, -out + trim, z), 3},
    }};
    for (const std::tuple<Point, Point, int>& screen : screens) {
        const Plan wall =
            fin_screen_between(std::get<0>(screen), std::get<1>(screen), std::get<2>(screen));
        stack.facades.push_back(Level{wall, Framing::fin_screen()});
    }

    return stack;
}

} // namespace wood_grid
