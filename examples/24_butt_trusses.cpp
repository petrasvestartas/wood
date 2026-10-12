#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════

/// The user's line, as drawn: red over the computed solids, in the input group.
static void draw_line(WoodSession& scene, const Polyline& line) {

    std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(line);
    drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
    drawn->width = 4.0;
    scene.add_polyline(drawn, scene.group_named("input"));
}

/// A member added with the line it was drawn on: a beam's axis.
static void add_drawn(WoodSession& scene, const std::shared_ptr<Beam>& member) {

    scene.add(member);
    draw_line(scene, member->axis);
}

/// A straight member of square section on the line between two nodes in the xz plane, added and drawn.
static std::shared_ptr<Beam> member(WoodSession& scene, double x0, double z0, double x1, double z1, double radius, const std::string& name) {

    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{x0, 0.0, z0}, {x1, 0.0, z1}}), radius, name);
    add_drawn(scene, beam);
    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Joining
// ═══════════════════════════════════════════════════════════════════════════

/// A butt of a pair, named by its configuration and its members, passed to both; none when they do not touch.
static void add_butt(WoodSession& scene, const std::shared_ptr<JointBeam>& butt, const std::shared_ptr<Beam>& a, const std::shared_ptr<Beam>& b) {

    if (!butt)
        return;
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    scene.add(butt);
    scene.add_interaction(butt, a, butt->interaction(0));
    scene.add_interaction(butt, b, butt->interaction(1));
}

/// A web member stopping on a chord it ends on.
static void stop(WoodSession& scene, const std::shared_ptr<Beam>& chord, const std::shared_ptr<Beam>& web) {

    add_butt(scene, JointBeam::butt_top_side(*chord, *web), chord, web);
}

/// Two members ending at the same node, both cut on the plane halfway.
static void miter(WoodSession& scene, const std::shared_ptr<Beam>& a, const std::shared_ptr<Beam>& b) {

    add_butt(scene, JointBeam::butt_side_side(*a, *b), a, b);
}

// ═══════════════════════════════════════════════════════════════════════════
// Pitched roofs
// ═══════════════════════════════════════════════════════════════════════════

/// The chords of a pitched roof truss: the bottom chord over the whole span and a top chord on each side up to the ridge.
struct Roof {
    double span;
    double rise;
    std::shared_ptr<Beam> bottom;
    std::array<std::shared_ptr<Beam>, 2> top;
};

/// A pitched roof's chords, butted at the heels on the bottom chord and mitered at the ridge.
static Roof add_roof(WoodSession& scene, double span, double rise) {

    Roof roof{span, rise, nullptr, {}};
    roof.bottom = member(scene, 0.0, 0.0, span, 0.0, 80.0, "bottom_chord");
    roof.top[0] = member(scene, 0.0, 0.0, 0.5 * span, rise, 80.0, "top_chord_0");
    roof.top[1] = member(scene, 0.5 * span, rise, span, 0.0, 80.0, "top_chord_1");

    // heels: each top chord butted on the bottom chord's end
    for (const std::shared_ptr<Beam>& top : roof.top)
        add_butt(scene, JointBeam::butt(*roof.bottom, *top), roof.bottom, top);

    // ridge: the two top chords mitered
    miter(scene, roof.top[0], roof.top[1]);
    return roof;
}

/// The height of a pitched roof's top chord at x.
static double roof_height(const Roof& roof, double x) {

    return roof.rise * (1.0 - std::abs(x - 0.5 * roof.span) / (0.5 * roof.span));
}

/// A web member stopping on the top chord its node at x lies on, at the ridge on both.
static void stop_on_top(WoodSession& scene, const Roof& roof, const std::shared_ptr<Beam>& web, double x) {

    if (x <= 0.5 * roof.span + 1e-6)
        stop(scene, roof.top[0], web);
    if (x >= 0.5 * roof.span - 1e-6)
        stop(scene, roof.top[1], web);
}

/// King post: the post from the bottom chord to the ridge, and a strut from its foot up to the middle of each top chord.
static void king_post() {

    WoodSession scene("butt_truss_king_post");
    const Roof roof = add_roof(scene, 8000.0, 2667.0);

    // king post: on the bottom chord, under both top chords
    const std::shared_ptr<Beam> post = member(scene, 4000.0, 0.0, 4000.0, 2667.0, 60.0, "king_post");
    stop(scene, roof.bottom, post);
    stop_on_top(scene, roof, post, 4000.0);

    // struts: from the post's foot to the middle of each top chord, mitered with the post
    for (int s = 0; s < 2; s++) {
        const double x = s == 0 ? 2000.0 : 6000.0;
        const std::shared_ptr<Beam> strut = member(scene, 4000.0, 0.0, x, roof_height(roof, x), 60.0, fmt::format("strut_{}", s));
        stop(scene, roof.bottom, strut);
        stop_on_top(scene, roof, strut, x);
        miter(scene, post, strut);
    }

    scene.pb_dump(pb_path("butt_truss_king_post"));
}

/// Pratt roof: six panels, a vertical at every inner panel point, diagonals falling towards the middle.
static void pratt() {

    WoodSession scene("butt_truss_pratt");
    const Roof roof = add_roof(scene, 12000.0, 3000.0);
    const int panels = 6;
    const double panel = roof.span / panels;

    // verticals: at every inner panel point, from the bottom chord to the top chord
    std::vector<std::shared_ptr<Beam>> verticals(panels + 1);
    for (int i = 1; i < panels; i++) {
        verticals[i] = member(scene, i * panel, 0.0, i * panel, roof_height(roof, i * panel), 60.0, fmt::format("vertical_{}", i));
        stop(scene, roof.bottom, verticals[i]);
        stop_on_top(scene, roof, verticals[i], i * panel);
    }

    // diagonals: from the top of vertical i down to the foot of the next one towards the middle, mitered with both
    for (int i = 1; i < panels / 2; i++) {
        for (int s = 0; s < 2; s++) {
            const int top = s == 0 ? i : panels - i;
            const int foot = s == 0 ? i + 1 : panels - i - 1;
            const std::shared_ptr<Beam> diagonal = member(scene, top * panel, roof_height(roof, top * panel), foot * panel, 0.0, 60.0, fmt::format("diagonal_{}_{}", i, s));
            stop(scene, roof.bottom, diagonal);
            stop_on_top(scene, roof, diagonal, top * panel);
            miter(scene, verticals[top], diagonal);
            miter(scene, verticals[foot], diagonal);
        }
    }

    scene.pb_dump(pb_path("butt_truss_pratt"));
}

/// Howe roof: six panels, a vertical at every inner panel point, diagonals rising towards the middle.
static void howe() {

    WoodSession scene("butt_truss_howe");
    const Roof roof = add_roof(scene, 12000.0, 3000.0);
    const int panels = 6;
    const double panel = roof.span / panels;

    // verticals: at every inner panel point, from the bottom chord to the top chord
    std::vector<std::shared_ptr<Beam>> verticals(panels + 1);
    for (int i = 1; i < panels; i++) {
        verticals[i] = member(scene, i * panel, 0.0, i * panel, roof_height(roof, i * panel), 60.0, fmt::format("vertical_{}", i));
        stop(scene, roof.bottom, verticals[i]);
        stop_on_top(scene, roof, verticals[i], i * panel);
    }

    // diagonals: from the foot of vertical i up to the top of the next one towards the middle, mitered with both
    for (int i = 1; i < panels / 2; i++) {
        for (int s = 0; s < 2; s++) {
            const int foot = s == 0 ? i : panels - i;
            const int top = s == 0 ? i + 1 : panels - i - 1;
            const std::shared_ptr<Beam> diagonal = member(scene, foot * panel, 0.0, top * panel, roof_height(roof, top * panel), 60.0, fmt::format("diagonal_{}_{}", i, s));
            stop(scene, roof.bottom, diagonal);
            stop_on_top(scene, roof, diagonal, top * panel);
            miter(scene, verticals[foot], diagonal);
            miter(scene, verticals[top], diagonal);
        }
    }

    scene.pb_dump(pb_path("butt_truss_howe"));
}

/// Fink: a W of four webs, each pair rising from a third point of the bottom chord to a quarter point and to the ridge.
static void fink() {

    WoodSession scene("butt_truss_fink");
    const Roof roof = add_roof(scene, 10000.0, 2500.0);

    // webs: per side from the third point to the quarter point and to the ridge, mitered at the third point
    std::array<std::shared_ptr<Beam>, 2> to_ridge;
    for (int s = 0; s < 2; s++) {
        const double third = s == 0 ? roof.span / 3.0 : 2.0 * roof.span / 3.0;
        const double quarter = s == 0 ? 0.25 * roof.span : 0.75 * roof.span;
        const std::shared_ptr<Beam> to_quarter = member(scene, third, 0.0, quarter, roof_height(roof, quarter), 60.0, fmt::format("web_quarter_{}", s));
        to_ridge[s] = member(scene, third, 0.0, 0.5 * roof.span, roof.rise, 60.0, fmt::format("web_ridge_{}", s));
        for (const std::shared_ptr<Beam>& web : {to_quarter, to_ridge[s]})
            stop(scene, roof.bottom, web);
        stop_on_top(scene, roof, to_quarter, quarter);
        stop_on_top(scene, roof, to_ridge[s], 0.5 * roof.span);
        miter(scene, to_quarter, to_ridge[s]);
    }

    // ridge: the two webs that reach it mitered
    miter(scene, to_ridge[0], to_ridge[1]);

    scene.pb_dump(pb_path("butt_truss_fink"));
}

/// Belgian: struts square to the top chord from its third points down to the bottom chord, and ties from their feet up to the next node.
static void belgian() {

    WoodSession scene("butt_truss_belgian");
    const Roof roof = add_roof(scene, 10000.0, 2500.0);
    const double slope = roof.rise / (0.5 * roof.span);

    // struts and ties per side: strut k from top node k square down to the bottom chord, tie k from its foot up to top node k + 1
    std::array<std::shared_ptr<Beam>, 2> last_ties;
    for (int s = 0; s < 2; s++) {
        std::shared_ptr<Beam> previous_tie = nullptr;
        for (int k = 1; k <= 2; k++) {
            const double node = k * roof.span / 6.0;
            const double foot = node + roof_height(roof, node) * slope;
            const double next = (k + 1) * roof.span / 6.0;
            const double sign_node = s == 0 ? node : roof.span - node;
            const double sign_foot = s == 0 ? foot : roof.span - foot;
            const double sign_next = s == 0 ? next : roof.span - next;
            const std::shared_ptr<Beam> strut = member(scene, sign_node, roof_height(roof, node), sign_foot, 0.0, 60.0, fmt::format("strut_{}_{}", k, s));
            const std::shared_ptr<Beam> tie = member(scene, sign_foot, 0.0, sign_next, roof_height(roof, next), 60.0, fmt::format("tie_{}_{}", k, s));
            stop(scene, roof.bottom, strut);
            stop_on_top(scene, roof, strut, sign_node);
            stop(scene, roof.bottom, tie);
            stop_on_top(scene, roof, tie, sign_next);
            miter(scene, strut, tie);
            if (previous_tie)
                miter(scene, previous_tie, strut);
            previous_tie = tie;
        }
        last_ties[s] = previous_tie;
    }

    // ridge: the two ties that reach it mitered
    miter(scene, last_ties[0], last_ties[1]);

    scene.pb_dump(pb_path("butt_truss_belgian"));
}

/// Scissors: bottom chords rising to a third of the rise at the middle, a king post, a vertical and a strut per side.
static void scissors() {

    WoodSession scene("butt_truss_scissors");
    const double span = 8000.0;
    const double rise = 3000.0;
    const double kink = rise / 3.0;

    // chords: per side a bottom chord from the heel to the middle and a top chord from the heel to the ridge
    std::array<std::shared_ptr<Beam>, 2> bottom;
    std::array<std::shared_ptr<Beam>, 2> top;
    bottom[0] = member(scene, 0.0, 0.0, 0.5 * span, kink, 80.0, "bottom_chord_0");
    bottom[1] = member(scene, 0.5 * span, kink, span, 0.0, 80.0, "bottom_chord_1");
    top[0] = member(scene, 0.0, 0.0, 0.5 * span, rise, 80.0, "top_chord_0");
    top[1] = member(scene, 0.5 * span, rise, span, 0.0, 80.0, "top_chord_1");

    // heels, kink and ridge: each top chord butted on its bottom chord, the bottom chords and the top chords mitered
    for (int s = 0; s < 2; s++)
        add_butt(scene, JointBeam::butt(*bottom[s], *top[s]), bottom[s], top[s]);
    miter(scene, bottom[0], bottom[1]);
    miter(scene, top[0], top[1]);

    // king post: from the kink to the ridge, on both bottom chords and under both top chords
    const std::shared_ptr<Beam> post = member(scene, 0.5 * span, kink, 0.5 * span, rise, 60.0, "king_post");
    for (int s = 0; s < 2; s++) {
        stop(scene, bottom[s], post);
        stop(scene, top[s], post);
    }

    // verticals and struts: per side a vertical at the quarter point and a strut from the kink up to its top, mitered at both ends
    for (int s = 0; s < 2; s++) {
        const double x = s == 0 ? 0.25 * span : 0.75 * span;
        const std::shared_ptr<Beam> vertical = member(scene, x, 0.5 * kink, x, 0.5 * rise, 60.0, fmt::format("vertical_{}", s));
        const std::shared_ptr<Beam> strut = member(scene, 0.5 * span, kink, x, 0.5 * rise, 60.0, fmt::format("strut_{}", s));
        stop(scene, bottom[s], vertical);
        stop(scene, top[s], vertical);
        stop(scene, bottom[0], strut);
        stop(scene, bottom[1], strut);
        stop(scene, top[s], strut);
        miter(scene, vertical, strut);
        miter(scene, post, strut);
    }

    scene.pb_dump(pb_path("butt_truss_scissors"));
}

// ═══════════════════════════════════════════════════════════════════════════
// Bridges
// ═══════════════════════════════════════════════════════════════════════════

/// Warren: parallel chords, eight panels of diagonals alternating up and down, with or without a vertical at every panel point.
static void warren(const std::string& name, bool verticals) {

    WoodSession scene(name);
    const double span = 12000.0;
    const double height = 1500.0;
    const int panels = 8;
    const double panel = span / panels;

    // chords: the bottom chord over the span, the top chord between the first and the last top node
    const std::shared_ptr<Beam> bottom = member(scene, 0.0, 0.0, span, 0.0, 80.0, "bottom_chord");
    const std::shared_ptr<Beam> top = member(scene, 0.5 * panel, height, span - 0.5 * panel, height, 80.0, "top_chord");

    // diagonals: up from bottom node i to top node i and down to bottom node i + 1, mitered at every node
    std::vector<std::shared_ptr<Beam>> up(panels);
    std::vector<std::shared_ptr<Beam>> down(panels);
    for (int i = 0; i < panels; i++) {
        up[i] = member(scene, i * panel, 0.0, (i + 0.5) * panel, height, 60.0, fmt::format("diagonal_up_{}", i));
        down[i] = member(scene, (i + 0.5) * panel, height, (i + 1) * panel, 0.0, 60.0, fmt::format("diagonal_down_{}", i));
        for (const std::shared_ptr<Beam>& diagonal : {up[i], down[i]}) {
            stop(scene, bottom, diagonal);
            stop(scene, top, diagonal);
        }
        if (!verticals) {
            miter(scene, up[i], down[i]);
            if (i > 0)
                miter(scene, down[i - 1], up[i]);
        }
    }

    // verticals: at every top node and every inner bottom node, between the chords, mitered with the diagonals at its node
    if (verticals) {
        for (int i = 0; i < panels; i++) {
            const std::shared_ptr<Beam> at_top = member(scene, (i + 0.5) * panel, 0.0, (i + 0.5) * panel, height, 60.0, fmt::format("vertical_top_{}", i));
            stop(scene, bottom, at_top);
            stop(scene, top, at_top);
            miter(scene, up[i], at_top);
            miter(scene, at_top, down[i]);
            if (i == 0)
                continue;
            const std::shared_ptr<Beam> at_bottom = member(scene, i * panel, 0.0, i * panel, height, 60.0, fmt::format("vertical_bottom_{}", i));
            stop(scene, bottom, at_bottom);
            stop(scene, top, at_bottom);
            miter(scene, down[i - 1], at_bottom);
            miter(scene, at_bottom, up[i]);
        }
    }

    scene.pb_dump(pb_path(name));
}

/// Pratt bridge: parallel chords, inclined end posts, six panels, a vertical at every panel point, diagonals falling towards the middle.
static void pratt_bridge() {

    WoodSession scene("butt_truss_pratt_bridge");
    const double span = 12000.0;
    const double height = 2000.0;
    const int panels = 6;
    const double panel = span / panels;

    // chords: the bottom chord over the span, the top chord between the first and the last inner panel point, an end post down to each heel
    const std::shared_ptr<Beam> bottom = member(scene, 0.0, 0.0, span, 0.0, 80.0, "bottom_chord");
    const std::shared_ptr<Beam> end_0 = member(scene, 0.0, 0.0, panel, height, 80.0, "end_post_0");
    const std::shared_ptr<Beam> top = member(scene, panel, height, span - panel, height, 80.0, "top_chord");
    const std::shared_ptr<Beam> end_1 = member(scene, span - panel, height, span, 0.0, 80.0, "end_post_1");
    add_butt(scene, JointBeam::butt(*bottom, *end_0), bottom, end_0);
    add_butt(scene, JointBeam::butt(*bottom, *end_1), bottom, end_1);
    miter(scene, end_0, top);
    miter(scene, top, end_1);

    // the top chord members a web ending at top node i stops on: the end post too at the first and the last
    const auto stop_on_top = [&](int i, const std::shared_ptr<Beam>& web) {
        stop(scene, top, web);
        if (i == 1)
            stop(scene, end_0, web);
        if (i == panels - 1)
            stop(scene, end_1, web);
    };

    // verticals: at every inner panel point, between the chords
    std::vector<std::shared_ptr<Beam>> verticals(panels + 1);
    for (int i = 1; i < panels; i++) {
        verticals[i] = member(scene, i * panel, 0.0, i * panel, height, 60.0, fmt::format("vertical_{}", i));
        stop(scene, bottom, verticals[i]);
        stop_on_top(i, verticals[i]);
    }

    // diagonals: from the top of vertical i down to the foot of the next one towards the middle, mitered with both
    for (int i = 1; i < panels / 2; i++) {
        for (int s = 0; s < 2; s++) {
            const int upper = s == 0 ? i : panels - i;
            const int foot = s == 0 ? i + 1 : panels - i - 1;
            const std::shared_ptr<Beam> diagonal = member(scene, upper * panel, height, foot * panel, 0.0, 60.0, fmt::format("diagonal_{}_{}", i, s));
            stop(scene, bottom, diagonal);
            stop_on_top(upper, diagonal);
            miter(scene, verticals[upper], diagonal);
            miter(scene, verticals[foot], diagonal);
        }
    }

    scene.pb_dump(pb_path("butt_truss_pratt_bridge"));
}

/// Parker: a Pratt bridge whose top chord is a polygon, one straight member per panel, highest at the middle.
static void parker() {

    WoodSession scene("butt_truss_parker");
    const double span = 12000.0;
    const int panels = 6;
    const double panel = span / panels;
    const std::array<double, 7> heights = {0.0, 1600.0, 2200.0, 2400.0, 2200.0, 1600.0, 0.0};

    // chords: the bottom chord over the span, the top chord one member per panel, butted at the heels and mitered at every kink
    const std::shared_ptr<Beam> bottom = member(scene, 0.0, 0.0, span, 0.0, 80.0, "bottom_chord");
    std::vector<std::shared_ptr<Beam>> top(panels);
    for (int k = 0; k < panels; k++)
        top[k] = member(scene, k * panel, heights[k], (k + 1) * panel, heights[k + 1], 80.0, fmt::format("top_chord_{}", k));
    add_butt(scene, JointBeam::butt(*bottom, *top[0]), bottom, top[0]);
    add_butt(scene, JointBeam::butt(*bottom, *top[panels - 1]), bottom, top[panels - 1]);
    for (int k = 0; k + 1 < panels; k++)
        miter(scene, top[k], top[k + 1]);

    // verticals: at every inner panel point, from the bottom chord to the kink, under both top chord members
    std::vector<std::shared_ptr<Beam>> verticals(panels + 1);
    for (int i = 1; i < panels; i++) {
        verticals[i] = member(scene, i * panel, 0.0, i * panel, heights[i], 60.0, fmt::format("vertical_{}", i));
        stop(scene, bottom, verticals[i]);
        stop(scene, top[i - 1], verticals[i]);
        stop(scene, top[i], verticals[i]);
    }

    // diagonals: from the top of vertical i down to the foot of the next one towards the middle, mitered with both
    for (int i = 1; i < panels / 2; i++) {
        for (int s = 0; s < 2; s++) {
            const int upper = s == 0 ? i : panels - i;
            const int foot = s == 0 ? i + 1 : panels - i - 1;
            const std::shared_ptr<Beam> diagonal = member(scene, upper * panel, heights[upper], foot * panel, 0.0, 60.0, fmt::format("diagonal_{}_{}", i, s));
            stop(scene, bottom, diagonal);
            stop(scene, top[upper - 1], diagonal);
            stop(scene, top[upper], diagonal);
            miter(scene, verticals[upper], diagonal);
            miter(scene, verticals[foot], diagonal);
        }
    }

    scene.pb_dump(pb_path("butt_truss_parker"));
}

/// K truss: parallel chords, inclined end posts, eight panels, a vertical at every panel point, and in every panel a K: two diagonals from
/// the middle of a vertical to the top and the foot of the next one towards the middle.
static void k_truss() {

    WoodSession scene("butt_truss_k_truss");
    const double span = 12000.0;
    const double height = 2400.0;
    const int panels = 8;
    const double panel = span / panels;

    // chords: the bottom chord over the span, the top chord between the first and the last inner panel point, an end post down to each heel
    const std::shared_ptr<Beam> bottom = member(scene, 0.0, 0.0, span, 0.0, 80.0, "bottom_chord");
    const std::shared_ptr<Beam> end_0 = member(scene, 0.0, 0.0, panel, height, 80.0, "end_post_0");
    const std::shared_ptr<Beam> top = member(scene, panel, height, span - panel, height, 80.0, "top_chord");
    const std::shared_ptr<Beam> end_1 = member(scene, span - panel, height, span, 0.0, 80.0, "end_post_1");
    add_butt(scene, JointBeam::butt(*bottom, *end_0), bottom, end_0);
    add_butt(scene, JointBeam::butt(*bottom, *end_1), bottom, end_1);
    miter(scene, end_0, top);
    miter(scene, top, end_1);

    // verticals: at every inner panel point, between the chords, the first and the last under the end post too
    std::vector<std::shared_ptr<Beam>> verticals(panels + 1);
    for (int i = 1; i < panels; i++) {
        verticals[i] = member(scene, i * panel, 0.0, i * panel, height, 60.0, fmt::format("vertical_{}", i));
        stop(scene, bottom, verticals[i]);
        stop(scene, top, verticals[i]);
        if (i == 1)
            stop(scene, end_0, verticals[i]);
        if (i == panels - 1)
            stop(scene, end_1, verticals[i]);
    }

    // Ks: from the middle of vertical i to the top and the foot of the next one towards the middle, on the vertical's side, mitered with each other and the next vertical
    for (int i = 1; i < panels / 2; i++) {
        for (int s = 0; s < 2; s++) {
            const int from = s == 0 ? i : panels - i;
            const int to = s == 0 ? i + 1 : panels - i - 1;
            const std::shared_ptr<Beam> upper = member(scene, from * panel, 0.5 * height, to * panel, height, 60.0, fmt::format("diagonal_top_{}_{}", i, s));
            const std::shared_ptr<Beam> lower = member(scene, from * panel, 0.5 * height, to * panel, 0.0, 60.0, fmt::format("diagonal_bottom_{}_{}", i, s));
            stop(scene, verticals[from], upper);
            stop(scene, verticals[from], lower);
            stop(scene, top, upper);
            stop(scene, bottom, lower);
            miter(scene, upper, lower);
            miter(scene, verticals[to], upper);
            miter(scene, verticals[to], lower);
        }
    }

    scene.pb_dump(pb_path("butt_truss_k_truss"));
}

// ═══════════════════════════════════════════════════════════════════════════
// Queen post
// ═══════════════════════════════════════════════════════════════════════════

/// Queen post: a trapezoid, the top chord inclined to the third points and level between them, a post under each kink.
static void queen_post() {

    WoodSession scene("butt_truss_queen_post");
    const double span = 9000.0;
    const double height = 2000.0;

    // chords: the bottom chord over the span, the top chord in three members, butted at the heels and mitered at the kinks
    const std::shared_ptr<Beam> bottom = member(scene, 0.0, 0.0, span, 0.0, 80.0, "bottom_chord");
    const std::array<std::shared_ptr<Beam>, 3> top = {
        member(scene, 0.0, 0.0, span / 3.0, height, 80.0, "top_chord_0"),
        member(scene, span / 3.0, height, 2.0 * span / 3.0, height, 80.0, "top_chord_1"),
        member(scene, 2.0 * span / 3.0, height, span, 0.0, 80.0, "top_chord_2"),
    };
    add_butt(scene, JointBeam::butt(*bottom, *top[0]), bottom, top[0]);
    add_butt(scene, JointBeam::butt(*bottom, *top[2]), bottom, top[2]);
    miter(scene, top[0], top[1]);
    miter(scene, top[1], top[2]);

    // queen posts: under each kink, on the bottom chord and under both top chord members
    for (int s = 0; s < 2; s++) {
        const double x = (s + 1) * span / 3.0;
        const std::shared_ptr<Beam> post = member(scene, x, 0.0, x, height, 60.0, fmt::format("queen_post_{}", s));
        stop(scene, bottom, post);
        stop(scene, top[s], post);
        stop(scene, top[s + 1], post);
    }

    scene.pb_dump(pb_path("butt_truss_queen_post"));
}

/// Classic timber trusses drawn as lines, every member a straight beam, chords running through, webs butted on them and mitered where they meet.
int main() {

    // each truss its own scene, written to data/output/pb/butt_truss_<name>.pb
    king_post();
    queen_post();
    pratt();
    howe();
    fink();
    belgian();
    scissors();
    warren("butt_truss_warren", false);
    warren("butt_truss_warren_verticals", true);
    pratt_bridge();
    parker();
    k_truss();
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Classic trusses from the HAER truss identification sheets, each drawn as the user draws it, every member a straight beam of square section on a line between two nodes (red) in the xz plane, chords 160 and webs 120 wide: king post, queen post, Pratt, Howe, Fink, Belgian and scissors roof trusses, and Warren, Warren with verticals, Pratt, Parker and K bridge trusses. Chords run through, a polygonal top chord one beam per straight segment; every web stops on each chord it ends on by JointBeam::butt_top_side, two webs ending at one node and two top chord members meeting at a kink are mitered by JointBeam::butt_side_side, and each top chord stands on the bottom chord's end at the heel by JointBeam::butt. Each truss is its own scene, data/output/pb/butt_truss_<name>.pb.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 24_butt_trusses --parallel 6 && ./build/24_butt_trusses && ../bash/publish-scene.sh --target 24_butt_trusses

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
