#include "pch.h"
#include "src/templates/cavity_border.h"
#include "src/templates/vidy/vidy.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Joinery tables and frames
// ═══════════════════════════════════════════════════════════════════════════

/// Paired bottom and top four-corner hole loops of one rectangular through-mortise slot.
struct MortiseSlot {
    std::array<Point, 4> bottom; // Four corners on the receiving plate's bottom plane.
    std::array<Point, 4> top; // Matching four corners on the receiving plate's top plane.
};

/// Edge frame, insertion vector and layout parameters of one tenon or rabbet band.
struct PinBand {
    std::array<Point, 4> ends; // Bottom and top miter endpoints {b0, b1, t0, t1}.
    Point ref_pt; // Shared seam midpoint projected onto both edge lines.
    Vector v_ins; // Unit in-plane insertion direction across the crease.
    double le_tab = 0.0; // Single-layer bevel step length along v_ins.
    double le_offs = 0.0; // Cavity-spanning bevel offset length along v_ins.
    bool step_bot = false; // True when the bottom face steps back by le_tab.
    int count = 0; // Pin count (or mating pin count on rabbet shelves).
    double pitch = 500.0; // Centre-to-centre pin spacing along the crease.
    double pocket_extra = 0.0; // Extra pocket half-margin beyond 0.5 * count * pitch.
    double shift = 0.0; // Longitudinal shift of the band centre from ref_pt.
};

/// Intra-bay Regular crease descriptor pairing two local facet indices.
struct BayCreaseSpec {
    size_t loc_a; // Primary facet local index in 0..13.
    size_t loc_b; // Mating facet local index in 0..13.
    int kind; // Crease role: 0 wall, 1 situ, 2 knee_L, 3 crown_L, 4 crown_R, 5 valley, 6 knee_R.
};

/// Resolved 3D miter endpoints, insertion vectors and bevel stepbacks of one Regular crease.
struct CreaseFrame {
    std::array<Point, 4> ends_a_in; // Inner bottom and top miter endpoints on facet A.
    std::array<Point, 4> ends_a_out; // Outer bottom and top miter endpoints on facet A.
    std::array<Point, 4> ends_b_in; // Inner bottom and top miter endpoints on facet B.
    std::array<Point, 4> ends_b_out; // Outer bottom and top miter endpoints on facet B.
    Point ref_b; // Shared inner bottom reference midpoint.
    Point ref_t; // Shared outer bottom reference midpoint.
    Vector v_ins_a; // In-plane insertion direction of facet A across the crease.
    Vector v_ins_b; // In-plane insertion direction of facet B across the crease.
    double min_len = 0.0; // Shortest of the four miter edge segments.
    double l_tab_a = 0.0; // Single-layer bevel tab length along v_ins_a.
    double l_offs_a = 0.0; // Cavity-spanning bevel offset length along v_ins_a.
    double l_tab_b = 0.0; // Single-layer bevel tab length along v_ins_b.
    double l_offs_b = 0.0; // Cavity-spanning bevel offset length along v_ins_b.
    size_t fa_i = 0; // Global facet index A.
    size_t sa = 0; // Directed side index on facet A.
    size_t fb_i = 0; // Global facet index B.
    size_t sb = 0; // Directed side index on facet B.
};

/// Per-ring directed edge profiles, per-layer mortise slots and pin counters across all facets.
struct JoineryTables {
    std::array<std::vector<std::vector<std::vector<Point>>>, 4> edge_polys; // [ring][facet][side].
    std::array<std::vector<std::vector<MortiseSlot>>, 2> plate_slots; // [layer][facet].
    size_t double_pins = 0; // Stepped Double_Pin count.
    size_t single_pins = 0; // Single_Pin and Extend_Pin count.
};

// ═══════════════════════════════════════════════════════════════════════════
// Single-direction insertion vectors and mortise projection
// ═══════════════════════════════════════════════════════════════════════════

/// Unit vector in facet's plane perpendicular to directed edge (p_start -> p_end), pointing outward.
Vector compute_inplane_outward(const VidyFacet& facet, const Point& p_start, const Point& p_end) {

    Vector out = (p_end - p_start).normalized().cross(facet.normal).normalized();

    if ((Point::mid_point(p_start, p_end) - facet.centroid).dot(out) < 0.0)
        out = -out;

    return out;
}

/// Directed side index on facet sharing undirected vertices (u, v).
size_t find_shared_side(const VidyFacet& facet, size_t u, size_t v) {

    const size_t m = facet.vertices.size();

    for (size_t k = 0; k < m; k++) {
        const size_t a = facet.vertices[k];
        const size_t b = facet.vertices[(k + 1) % m];

        if ((a == u && b == v) || (a == v && b == u))
            return k;
    }

    return 0;
}

/// Directed side index on facet connecting facet_idx to neighbor_idx.
size_t find_neighbor_side(const VidyGuide& guide, size_t facet_idx, size_t neighbor_idx) {

    const VidyFacet& fa = guide.facets()[facet_idx];
    const VidyFacet& fb = guide.facets()[neighbor_idx];
    std::vector<size_t> shared;

    for (const size_t va : fa.vertices)
        for (const size_t vb : fb.vertices)
            if (va == vb)
                shared.push_back(va);

    if (shared.size() < 2)
        return 0;

    return find_shared_side(fa, shared[0], shared[1]);
}

/// Outward unit vector on facet's side shared with neighbor_idx.
Vector compute_neighbor_outward(const VidyGuide& guide, size_t facet_idx, size_t neighbor_idx) {

    const VidyFacet& f = guide.facets()[facet_idx];
    const size_t s = find_neighbor_side(guide, facet_idx, neighbor_idx);
    const size_t m = f.vertices.size();

    return compute_inplane_outward(f, f.miter_rings[0][s], f.miter_rings[0][(s + 1) % m]);
}

/// Single in-plane insertion vector of inner crown (6 or 11): normalized sum of its Regular orthogonals.
Vector compute_crown_inplane_vector(const VidyGuide& guide, const VidyFacet& crown) {

    const std::vector<VidyEdge>& edges = guide.edges();
    const size_t m = crown.vertices.size();
    Vector sum = {0.0, 0.0, 0.0};

    for (size_t k = 0; k < m; k++) {
        const VidyEdge& edge = edges[static_cast<size_t>(crown.edge_ids[k])];

        if (edge.kind == VidyEdgeKind::Regular)
            sum = sum + compute_inplane_outward(
                            crown, crown.miter_rings[0][k], crown.miter_rings[0][(k + 1) % m]
                        );
    }

    return sum.magnitude() > 1e-9 ? sum.normalized() : Vector{0.0, 1.0, 0.0};
}

/// Shared 3D tenon direction n_left x n_right on both haunches meeting crown_loc (6 or 11).
Vector compute_haunch_crown_vector(
    const VidyGuide& guide, size_t bay, size_t crown_loc, const Vector& perp_outward
) {

    const std::vector<VidyFacet>& facets = guide.facets();
    const size_t left_loc = crown_loc == 6 ? 5 : 10;
    const size_t right_loc = crown_loc == 6 ? 7 : 12;
    Vector dir = facets[14 * bay + left_loc].normal.cross(facets[14 * bay + right_loc].normal);

    if (dir.magnitude() < 1e-9)
        return perp_outward;

    dir = dir.normalized();

    if (dir.dot(perp_outward) < 0.0)
        dir = -dir;

    return dir;
}

/// Projects a four-arris tenon shaft along v_ins onto facet_fem with 0.5 mm corner clearance.
void append_projected_slot(
    const std::pair<Point, Point>& shaft_bot,
    const std::pair<Point, Point>& shaft_top,
    const Vector& v_ins,
    const VidyFacet& facet_fem,
    double h_bot,
    double h_top,
    std::vector<MortiseSlot>& out_slots
) {

    const Plane p_bot =
        Plane::from_point_normal(facet_fem.centroid + facet_fem.normal * h_bot, facet_fem.normal);
    const Plane p_top =
        Plane::from_point_normal(facet_fem.centroid + facet_fem.normal * h_top, facet_fem.normal);
    const std::array<Point, 4> arris = {
        shaft_bot.first, shaft_bot.second, shaft_top.second, shaft_top.first
    };
    MortiseSlot slot;

    for (size_t c = 0; c < 4; c++) {
        const Line ray = Line::from_point_and_vector(arris[c], v_ins);
        slot.bottom[c] = Intersection::line_plane(ray, p_bot, false).value_or(arris[c]);
        slot.top[c] = Intersection::line_plane(ray, p_top, false).value_or(arris[c]);
    }

    const Point cb =
        Point::centroid({slot.bottom[0], slot.bottom[1], slot.bottom[2], slot.bottom[3]});
    const Point ct = Point::centroid({slot.top[0], slot.top[1], slot.top[2], slot.top[3]});

    for (size_t c = 0; c < 4; c++) {
        slot.bottom[c] = slot.bottom[c] + (slot.bottom[c] - cb).normalized() * 0.5;
        slot.top[c] = slot.top[c] + (slot.top[c] - ct).normalized() * 0.5;
    }

    out_slots.push_back(slot);
}

// ═══════════════════════════════════════════════════════════════════════════
// Tenon and rabbet profiles
// ═══════════════════════════════════════════════════════════════════════════

/// Twelve-point stepped chamfered Double_Pin contour and its base and tip arris pairs.
std::vector<Point> compute_double_pin(
    const Point& pm,
    const Vector& nf,
    const Vector& v_ins,
    double le_tab,
    double le_offs,
    std::pair<Point, Point>& out_base,
    std::pair<Point, Point>& out_tip
) {

    const Point mi1 = pm - nf * 135.0;
    const Point pl1 = pm + nf * 135.0;
    const Point mi2 = mi1 + v_ins * (0.30 * le_offs);
    const Point pl2 = pl1 + v_ins * (0.30 * le_offs);
    const Point mi3 = (pm - nf * 125.0) + v_ins * le_offs;
    const Point pl3 = (pm + nf * 125.0) + v_ins * le_offs;
    const Point mi4 = mi3 + nf * 35.0;
    const Point pl4 = pl3 - nf * 35.0;
    const Point mi5 = mi4 + v_ins * (1.20 * le_tab);
    const Point pl5 = pl4 + v_ins * (1.20 * le_tab);
    const Point mi6 = (mi4 + v_ins * (2.00 * le_tab)) + nf * 20.0;
    const Point pl6 = (pl4 + v_ins * (2.00 * le_tab)) - nf * 20.0;
    out_base = {mi1, pl1};
    out_tip = {mi4, pl4};

    return {mi1, mi2, mi3, mi4, mi5, mi6, pl6, pl5, pl4, pl3, pl2, pl1};
}

/// Six-point chamfered Single_Pin contour and its shaft arris pair.
std::vector<Point> compute_single_pin(
    const Point& pm,
    const Vector& nf,
    const Vector& v_ins,
    double le_tab,
    std::pair<Point, Point>& out_shaft
) {

    const Point mi1 = pm - nf * 90.0;
    const Point pl1 = pm + nf * 90.0;
    const Point mi2 = mi1 + v_ins * (1.20 * le_tab);
    const Point pl2 = pl1 + v_ins * (1.20 * le_tab);
    const Point mi3 = (mi1 + v_ins * (2.00 * le_tab)) + nf * 20.0;
    const Point pl3 = (pl1 + v_ins * (2.00 * le_tab)) - nf * 20.0;
    out_shaft = {mi1, pl1};

    return {mi1, mi2, mi3, pl3, pl2, pl1};
}

/// Half-span offset from the band midpoint to the first pin centre.
double compute_first_pin_offset(int count, double pitch) {

    if (count % 2 == 0)
        return static_cast<double>(count / 2 - 1) * pitch + 0.5 * pitch;

    return static_cast<double>((count - 1) / 2) * pitch;
}

/// Reference point on top_ends aligned with m_b0 along a plane containing v_ins.
Point align_top_ref(
    const Point& m_b0,
    const Point& top_start,
    const Point& top_end,
    const Vector& nf,
    const Vector& v_ins
) {

    const Line top_line = Line::from_points(top_start, top_end);
    const Vector w_p = nf - v_ins * nf.dot(v_ins);

    if (w_p.magnitude() < 1e-6)
        return top_line.closest_point(m_b0, false).second;

    return Intersection::line_plane(top_line, Plane::from_point_normal(m_b0, w_p.normalized()), false)
        .value_or(top_line.closest_point(m_b0, false).second);
}

/// Paired bottom and top Multiple_Double_Pins contours plus base and tip mortise slots.
std::array<std::vector<Point>, 2> compute_double_pins(
    const PinBand& band,
    const VidyFacet& fem_facet,
    const std::array<double, 4>& heights,
    size_t same_layer,
    std::vector<MortiseSlot>& same_slots,
    std::vector<MortiseSlot>& opp_slots
) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double f_off = compute_first_pin_offset(band.count, band.pitch);
    const double sh_half = std::max(
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra,
        f_off + std::abs(band.shift) + 145.0
    );
    const size_t opp_layer = 1 - same_layer;
    std::vector<Point> bot = {m_b0 - nf * sh_half, pm_b - nf * sh_half};
    std::vector<Point> top = {m_t0 - nf * sh_half, pm_t - nf * sh_half};
    const Point ptc_b = (pm_b + nf * band.shift) - nf * f_off;
    const Point ptc_t = (pm_t + nf * band.shift) - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<Point, Point> base_b;
        std::pair<Point, Point> tip_b;
        std::pair<Point, Point> base_t;
        std::pair<Point, Point> tip_t;
        const std::vector<Point> pb = compute_double_pin(
            ptc_b + nf * step, nf, band.v_ins, band.le_tab, band.le_offs, base_b, tip_b
        );
        const std::vector<Point> pt = compute_double_pin(
            ptc_t + nf * step, nf, band.v_ins, band.le_tab, band.le_offs, base_t, tip_t
        );
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        append_projected_slot(
            base_b,
            base_t,
            band.v_ins,
            fem_facet,
            heights[2 * same_layer],
            heights[2 * same_layer + 1],
            same_slots
        );
        append_projected_slot(
            tip_b,
            tip_t,
            band.v_ins,
            fem_facet,
            heights[2 * opp_layer],
            heights[2 * opp_layer + 1],
            opp_slots
        );
    }

    bot.push_back(pm_b + nf * sh_half);
    bot.push_back(m_b0 + nf * sh_half);
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * sh_half);

    return {bot, top};
}

/// Paired bottom and top Multiple_Single_Pins contours plus mortise slots.
std::array<std::vector<Point>, 2> compute_single_pins(
    const PinBand& band,
    const VidyFacet& fem_facet,
    double h_bot,
    double h_top,
    std::vector<MortiseSlot>& slots
) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double f_off = compute_first_pin_offset(band.count, band.pitch);
    const double sh_half = std::max(
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra,
        f_off + std::abs(band.shift) + 100.0
    );
    std::vector<Point> bot = {m_b0 - nf * sh_half, pm_b - nf * sh_half};
    std::vector<Point> top = {m_t0 - nf * sh_half, pm_t - nf * sh_half};
    const Point ptc_b = (pm_b + nf * band.shift) - nf * f_off;
    const Point ptc_t = (pm_t + nf * band.shift) - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<Point, Point> sh_b;
        std::pair<Point, Point> sh_t;
        const std::vector<Point> pb =
            compute_single_pin(ptc_b + nf * step, nf, band.v_ins, band.le_tab, sh_b);
        const std::vector<Point> pt =
            compute_single_pin(ptc_t + nf * step, nf, band.v_ins, band.le_tab, sh_t);
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        append_projected_slot(sh_b, sh_t, band.v_ins, fem_facet, h_bot, h_top, slots);
    }

    bot.push_back(pm_b + nf * sh_half);
    bot.push_back(m_b0 + nf * sh_half);
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * sh_half);

    return {bot, top};
}

/// Paired bottom and top female Extend rabbet lip contours of span 0.5 * count * pitch.
std::array<std::vector<Point>, 2> compute_extend_tab(const PinBand& band) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const double span = 0.5 * static_cast<double>(band.count) * band.pitch - 0.5;
    const double ext_b = band.le_tab * (band.step_bot ? 2.0 : 1.0) - 0.5;
    const double ext_t = band.le_tab * (band.step_bot ? 1.0 : 2.0) - 0.5;
    const std::vector<Point> bot = {
        m_b0 - nf * span,
        (m_b0 - nf * span) + band.v_ins * ext_b,
        (m_b0 + nf * span) + band.v_ins * ext_b,
        m_b0 + nf * span
    };
    const std::vector<Point> top = {
        m_t0 - nf * span,
        (m_t0 - nf * span) + band.v_ins * ext_t,
        (m_t0 + nf * span) + band.v_ins * ext_t,
        m_t0 + nf * span
    };

    return {bot, top};
}

/// Paired bottom and top Extend_NoPins cavity-bridging female eave contours.
std::array<std::vector<Point>, 2> compute_extend_no_pins(const PinBand& band) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const double span = 0.5 * static_cast<double>(band.count) * band.pitch - 0.5;
    const double ext_b = band.le_offs - (band.step_bot ? band.le_tab : 0.0) - 0.5;
    const double ext_t = band.le_offs - (band.step_bot ? 0.0 : band.le_tab) - 0.5;
    const std::vector<Point> bot = {
        m_b0 - nf * span,
        (m_b0 - nf * span) + band.v_ins * ext_b,
        (m_b0 + nf * span) + band.v_ins * ext_b,
        m_b0 + nf * span
    };
    const std::vector<Point> top = {
        m_t0 - nf * span,
        (m_t0 - nf * span) + band.v_ins * ext_t,
        (m_t0 + nf * span) + band.v_ins * ext_t,
        m_t0 + nf * span
    };

    return {bot, top};
}

/// Paired bottom and top Extend_Pins cavity-bridging contours plus Single_Pin mortise slots.
std::array<std::vector<Point>, 2> compute_extend_pins(
    const PinBand& band,
    const VidyFacet& fem_facet,
    double h_bot,
    double h_top,
    std::vector<MortiseSlot>& slots
) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double span = 0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra;
    const double f_off = compute_first_pin_offset(band.count, band.pitch);
    const Point base_l_b = m_b0 - nf * span;
    const Point base_r_b = m_b0 + nf * span;
    const Point base_l_t = m_t0 - nf * span;
    const Point base_r_t = m_t0 + nf * span;
    const Point p3_b = (pm_b - nf * span) + band.v_ins * band.le_offs;
    const Point p4_b = (pm_b + nf * span) + band.v_ins * band.le_offs;
    const Point p3_t = (pm_t - nf * span) + band.v_ins * band.le_offs;
    const Point p4_t = (pm_t + nf * span) + band.v_ins * band.le_offs;
    std::vector<Point> bot = {base_l_b, p3_b};
    std::vector<Point> top = {base_l_t, p3_t};
    const Point ptc_b = Point::mid_point(p3_b, p4_b) - nf * f_off;
    const Point ptc_t = Point::mid_point(p3_t, p4_t) - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<Point, Point> sh_b;
        std::pair<Point, Point> sh_t;
        const std::vector<Point> pb =
            compute_single_pin(ptc_b + nf * step, nf, band.v_ins, band.le_tab, sh_b);
        const std::vector<Point> pt =
            compute_single_pin(ptc_t + nf * step, nf, band.v_ins, band.le_tab, sh_t);
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        append_projected_slot(sh_b, sh_t, band.v_ins, fem_facet, h_bot, h_top, slots);
    }

    bot.push_back(p4_b);
    bot.push_back(base_r_b);
    top.push_back(p4_t);
    top.push_back(base_r_t);

    return {bot, top};
}

// ═══════════════════════════════════════════════════════════════════════════
// Crease joinery dispatch
// ═══════════════════════════════════════════════════════════════════════════

/// Stores a bottom/top contour pair into the two ring slots of one plate layer.
void assign_layer_edge(
    std::array<std::vector<std::vector<std::vector<Point>>>, 4>& edge_polys,
    size_t layer,
    size_t facet_idx,
    size_t side_idx,
    const std::array<std::vector<Point>, 2>& pair
) {

    edge_polys[2 * layer][facet_idx][side_idx] = pair[0];
    edge_polys[2 * layer + 1][facet_idx][side_idx] = pair[1];
}

/// Emits wall V-pleat (kind 0) or on-site situ eave (kind 1) contours and mortise slots.
void compute_wall_or_situ_crease(
    const BayCreaseSpec& spec,
    const CreaseFrame& cf,
    double pitch,
    const VidyGuide& guide,
    JoineryTables& tables
) {

    const VidyFacet& fa = guide.facets()[cf.fa_i];
    const VidyFacet& fb = guide.facets()[cf.fb_i];
    const std::array<double, 4> heights = guide.layer_heights();

    if (spec.kind == 1) {
        const Vector to_even =
            std::abs(cf.ends_b_out[0][0]) < std::abs(cf.ends_b_out[1][0])
                ? (cf.ends_b_out[0] - cf.ends_b_out[1]).normalized()
                : (cf.ends_b_out[1] - cf.ends_b_out[0]).normalized();
        const Point situ_ref_b =
            Point::mid_point(cf.ends_b_in[0], cf.ends_b_in[1]) + to_even * 35.0;
        const Point situ_ref_t =
            Point::mid_point(cf.ends_b_out[0], cf.ends_b_out[1]) + to_even * 35.0;
        const PinBand dp_band = {
            cf.ends_b_in, situ_ref_b, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, false, 2, pitch, 80.0, 0.0
        };
        const PinBand sp_band = {
            cf.ends_b_out, situ_ref_t, cf.v_ins_b, cf.l_tab_b, 0.0, false, 2, pitch, 80.0, 0.0
        };
        const PinBand np_band = {
            cf.ends_a_in, situ_ref_b, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, false, 2, pitch, 0.0, 0.0
        };
        const PinBand ex_band = {
            cf.ends_a_out, situ_ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, 2, pitch, 0.0, 0.0
        };
        assign_layer_edge(
            tables.edge_polys,
            0,
            cf.fb_i,
            cf.sb,
            compute_double_pins(
                dp_band,
                fa,
                heights,
                0,
                tables.plate_slots[0][cf.fa_i],
                tables.plate_slots[1][cf.fa_i]
            )
        );
        assign_layer_edge(
            tables.edge_polys,
            1,
            cf.fb_i,
            cf.sb,
            compute_single_pins(sp_band, fa, heights[2], heights[3], tables.plate_slots[1][cf.fa_i])
        );
        assign_layer_edge(tables.edge_polys, 0, cf.fa_i, cf.sa, compute_extend_no_pins(np_band));
        assign_layer_edge(tables.edge_polys, 1, cf.fa_i, cf.sa, compute_extend_tab(ex_band));
        tables.double_pins += 2;
        tables.single_pins += 2;
        return;
    }

    const int n_dp = cf.min_len >= 7200.0 ? 11 : 10;
    const int n_sp = cf.min_len >= 7000.0 ? 12 : 10;
    const PinBand ex_band = {
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, n_sp, pitch, 0.0, 0.0
    };
    const PinBand ep_band = {
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, false, n_dp, pitch, 0.0, 0.0
    };
    const PinBand sp_band = {
        cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, 0.0, false, n_sp, pitch, 2.0, 0.0
    };
    const PinBand dp_band = {
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, false, n_dp, pitch, 2.0, 0.0
    };
    assign_layer_edge(tables.edge_polys, 1, cf.fa_i, cf.sa, compute_extend_tab(ex_band));
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fa_i,
        cf.sa,
        compute_extend_pins(ep_band, fb, heights[2], heights[3], tables.plate_slots[1][cf.fb_i])
    );
    assign_layer_edge(
        tables.edge_polys,
        1,
        cf.fb_i,
        cf.sb,
        compute_single_pins(sp_band, fa, heights[2], heights[3], tables.plate_slots[1][cf.fa_i])
    );
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fb_i,
        cf.sb,
        compute_double_pins(
            dp_band, fa, heights, 0, tables.plate_slots[0][cf.fa_i], tables.plate_slots[1][cf.fa_i]
        )
    );
    tables.double_pins += static_cast<size_t>(n_dp);
    tables.single_pins += static_cast<size_t>(n_dp + n_sp);
}

/// Emits negative-fold central valley (kind 5) contours and mortise slots.
void compute_valley_crease(
    const CreaseFrame& cf, double pitch, const VidyGuide& guide, JoineryTables& tables
) {

    const VidyFacet& fa = guide.facets()[cf.fa_i];
    const VidyFacet& fb = guide.facets()[cf.fb_i];
    const std::array<double, 4> heights = guide.layer_heights();
    const int n_dp = 11;
    const int n_sp = cf.min_len >= 10200.0 ? 16 : (cf.min_len >= 9200.0 ? 14 : 13);
    const PinBand dp_band = {
        cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, true, n_dp, pitch, 3.0, 0.0
    };
    const PinBand ex_band = {
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, 0.0, false, n_sp, pitch, 0.0, 0.0
    };
    const PinBand ep_band = {
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, true, n_dp, pitch, 0.0, 0.0
    };
    const PinBand sp_band = {
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, 0.0, true, n_sp, pitch, 2.0, 0.0
    };

    assign_layer_edge(
        tables.edge_polys,
        1,
        cf.fb_i,
        cf.sb,
        compute_double_pins(
            dp_band, fa, heights, 1, tables.plate_slots[1][cf.fa_i], tables.plate_slots[0][cf.fa_i]
        )
    );
    assign_layer_edge(tables.edge_polys, 0, cf.fb_i, cf.sb, compute_extend_tab(ex_band));
    assign_layer_edge(
        tables.edge_polys,
        1,
        cf.fa_i,
        cf.sa,
        compute_extend_pins(ep_band, fb, heights[0], heights[1], tables.plate_slots[0][cf.fb_i])
    );
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fa_i,
        cf.sa,
        compute_single_pins(sp_band, fb, heights[0], heights[1], tables.plate_slots[0][cf.fb_i])
    );
    tables.double_pins += static_cast<size_t>(n_dp);
    tables.single_pins += static_cast<size_t>(n_dp + n_sp);
}

/// Emits Flip_Insert knee and crown (kinds 2, 3, 4, 6) contours and mortise slots.
void compute_knee_or_crown_crease(
    const BayCreaseSpec& spec,
    const CreaseFrame& cf,
    double pitch,
    const VidyGuide& guide,
    JoineryTables& tables
) {

    const VidyFacet& fa = guide.facets()[cf.fa_i];
    const VidyFacet& fb = guide.facets()[cf.fb_i];
    const std::array<double, 4> heights = guide.layer_heights();
    const int n_dp =
        spec.kind == 2 ? 3
                       : (spec.kind == 6 ? 4
                                         : (spec.kind == 3 ? 6 : (cf.min_len >= 4900.0 ? 5 : 4)));
    const int n_sp =
        spec.kind == 2
            ? (cf.min_len >= 3950.0 ? 6 : 5)
            : (spec.kind == 6
                   ? (cf.min_len >= 4250.0 ? 7 : (cf.min_len >= 3950.0 ? 6 : 5))
                   : (spec.kind == 3
                          ? (cf.min_len >= 6950.0 ? 12 : 11)
                          : (cf.min_len >= 5150.0 ? 9 : (cf.min_len >= 4820.0 ? 8 : 7))));
    const Vector nf_a = (cf.ends_a_in[1] - cf.ends_a_in[0]).normalized();
    const Vector nf_b = (cf.ends_b_in[1] - cf.ends_b_in[0]).normalized();
    const double skew_a = std::abs(cf.v_ins_a.dot(nf_a));
    const double skew_b = std::abs(cf.v_ins_b.dot(nf_b));
    const double ep_extra = skew_a * cf.l_tab_a;
    const double dp_extra = ep_extra + skew_b * cf.l_offs_b + 3.0;
    const double sp_extra =
        std::max(skew_b * (2.0 * cf.l_tab_b), skew_a * (2.0 * cf.l_tab_a)) + 3.0;
    const PinBand ex_band = {
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, n_sp, pitch, 0.0, 0.0
    };
    const PinBand dp_band = {
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, false, n_dp, pitch, dp_extra, 0.0
    };
    const PinBand ep_band = {
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, false, n_dp, pitch, ep_extra, 0.0
    };
    const PinBand sp_band = {
        cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, 0.0, false, n_sp, pitch, sp_extra, 0.0
    };

    assign_layer_edge(tables.edge_polys, 1, cf.fa_i, cf.sa, compute_extend_tab(ex_band));
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fa_i,
        cf.sa,
        compute_double_pins(
            dp_band, fb, heights, 0, tables.plate_slots[0][cf.fb_i], tables.plate_slots[1][cf.fb_i]
        )
    );
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fb_i,
        cf.sb,
        compute_extend_pins(ep_band, fa, heights[2], heights[3], tables.plate_slots[1][cf.fa_i])
    );
    assign_layer_edge(
        tables.edge_polys,
        1,
        cf.fb_i,
        cf.sb,
        compute_single_pins(sp_band, fa, heights[2], heights[3], tables.plate_slots[1][cf.fa_i])
    );
    tables.double_pins += static_cast<size_t>(n_dp);
    tables.single_pins += static_cast<size_t>(n_dp + n_sp);
}

/// Resolves the 3D miter endpoints, single-direction insertion vectors and bevel stepbacks.
std::optional<CreaseFrame>
compute_crease_frame(const VidyGuide& guide, size_t bay, const BayCreaseSpec& spec) {

    const std::vector<VidyFacet>& facets = guide.facets();
    CreaseFrame cf;
    cf.fa_i = 14 * bay + spec.loc_a;
    cf.fb_i = 14 * bay + spec.loc_b;
    const VidyFacet& fa = facets[cf.fa_i];
    const VidyFacet& fb = facets[cf.fb_i];
    std::vector<size_t> shared;

    for (const size_t va : fa.vertices)
        for (const size_t vb : fb.vertices)
            if (va == vb)
                shared.push_back(va);

    if (shared.size() < 2)
        return std::nullopt;

    cf.sa = find_shared_side(fa, shared[0], shared[1]);
    cf.sb = find_shared_side(fb, shared[0], shared[1]);
    const size_t ma = fa.vertices.size();
    const size_t mb = fb.vertices.size();
    cf.ends_a_in = {
        fa.miter_rings[0][cf.sa],
        fa.miter_rings[0][(cf.sa + 1) % ma],
        fa.miter_rings[1][cf.sa],
        fa.miter_rings[1][(cf.sa + 1) % ma]
    };
    cf.ends_a_out = {
        fa.miter_rings[2][cf.sa],
        fa.miter_rings[2][(cf.sa + 1) % ma],
        fa.miter_rings[3][cf.sa],
        fa.miter_rings[3][(cf.sa + 1) % ma]
    };
    cf.ends_b_in = {
        fb.miter_rings[0][cf.sb],
        fb.miter_rings[0][(cf.sb + 1) % mb],
        fb.miter_rings[1][cf.sb],
        fb.miter_rings[1][(cf.sb + 1) % mb]
    };
    cf.ends_b_out = {
        fb.miter_rings[2][cf.sb],
        fb.miter_rings[2][(cf.sb + 1) % mb],
        fb.miter_rings[3][cf.sb],
        fb.miter_rings[3][(cf.sb + 1) % mb]
    };
    const Vector u_out = (cf.ends_a_out[1] - cf.ends_a_out[0]).normalized();
    const double tb0 = (cf.ends_b_out[0] - cf.ends_a_out[0]).dot(u_out);
    const double tb1 = (cf.ends_b_out[1] - cf.ends_a_out[0]).dot(u_out);
    const double lo_out = std::max(0.0, std::min(tb0, tb1));
    const double hi_out =
        std::min(cf.ends_a_out[0].distance(cf.ends_a_out[1]), std::max(tb0, tb1));
    cf.ref_b = Point::mid_point(cf.ends_a_in[0], cf.ends_a_in[1]);
    cf.ref_t = hi_out > lo_out ? (cf.ends_a_out[0] + u_out * (0.5 * (lo_out + hi_out)))
                               : Point::mid_point(cf.ends_a_out[0], cf.ends_a_out[1]);
    cf.min_len = std::min(
        {cf.ends_a_in[0].distance(cf.ends_a_in[1]),
         cf.ends_a_out[2].distance(cf.ends_a_out[3]),
         cf.ends_b_in[0].distance(cf.ends_b_in[1]),
         cf.ends_b_out[2].distance(cf.ends_b_out[3])}
    );

    const double phi = 0.5 * std::acos(std::clamp(fa.normal.dot(fb.normal), -1.0, 1.0));
    const double cos_dev = std::max(0.05, std::cos(std::abs(Tolerance::HALF_PI - 2.0 * phi)));
    const Vector perp_a = compute_inplane_outward(fa, cf.ends_a_in[0], cf.ends_a_in[1]);
    const Vector perp_b = compute_inplane_outward(fb, cf.ends_b_in[0], cf.ends_b_in[1]);
    Vector situ_a = fa.normal.cross(Vector{0.0, 1.0, 0.0});
    situ_a = situ_a.magnitude() > 1e-9 ? situ_a.normalized() : perp_a;

    if (situ_a.dot(perp_a) < 0.0)
        situ_a = -situ_a;

    const bool is_crown = spec.kind == 3 || spec.kind == 4;
    cf.v_ins_a =
        is_crown ? compute_crown_inplane_vector(guide, fa) : (spec.kind == 1 ? situ_a : perp_a);
    cf.v_ins_b = is_crown ? compute_haunch_crown_vector(guide, bay, spec.loc_a, perp_b)
                          : (spec.kind == 1 ? Vector{0.0, 0.0, 1.0} : perp_b);
    const double cos_a = std::max(0.05, std::abs(cf.v_ins_a.dot(perp_a)));
    const double cos_b = std::max(0.05, std::abs(cf.v_ins_b.dot(perp_b)));
    cf.l_tab_a = (guide.plate_thickness / cos_dev) / cos_a;
    cf.l_offs_a = ((guide.total_offset - guide.plate_thickness) / cos_dev) / cos_a;
    cf.l_tab_b = (guide.plate_thickness / cos_dev) / cos_b;
    cf.l_offs_b = ((guide.total_offset - guide.plate_thickness) / cos_dev) / cos_b;

    return cf;
}

/// Populates the contour profiles and mortise slots of one intra-bay Regular crease.
void compute_bay_crease_joinery(
    const VidyGuide& guide,
    size_t bay,
    const BayCreaseSpec& spec,
    double pitch,
    JoineryTables& tables
) {

    const std::optional<CreaseFrame> cf = compute_crease_frame(guide, bay, spec);

    if (!cf)
        return;

    if (spec.kind == 0 || spec.kind == 1)
        compute_wall_or_situ_crease(spec, *cf, pitch, guide, tables);
    else if (spec.kind == 5)
        compute_valley_crease(*cf, pitch, guide, tables);
    else
        compute_knee_or_crown_crease(spec, *cf, pitch, guide, tables);
}

// ═══════════════════════════════════════════════════════════════════════════
// Selective corner chamfers and plate features
// ═══════════════════════════════════════════════════════════════════════════

/// Outward dihedral bisector plane at an even-station arch-knee vertex ({4,6}, {6,8}, {9,11}, {11,13}).
std::optional<Plane>
find_arch_knee_bisector(const VidyGuide& guide, const VidyFacet& facet, size_t vertex_id) {

    const size_t loc = facet.local_index;
    const std::vector<VidyFacet>& facets = guide.facets();
    const size_t base = 14 * facet.bay;
    const std::array<std::pair<size_t, size_t>, 4> pairs = {{{4, 6}, {6, 8}, {9, 11}, {11, 13}}};

    for (const std::pair<size_t, size_t>& pr : pairs) {
        if (loc != pr.first && loc != pr.second)
            continue;

        const size_t other_loc = loc == pr.first ? pr.second : pr.first;
        const VidyFacet& other = facets[base + other_loc];

        for (const size_t ov : other.vertices)
            if (ov == vertex_id)
                return compute_cavity_bisector_plane(
                    guide.vertices()[vertex_id], facet.normal, other.normal
                );
    }

    return std::nullopt;
}

/// Incoming and outgoing edge snips separating apex from limit_plane by clearance.
std::pair<double, double> compute_plane_corner_snips(
    const Point& apex,
    const Vector& ua,
    const Vector& ub,
    const Plane& limit_plane,
    double clearance
) {

    const double raw = limit_plane.signed_distance(apex);
    const double ra = ua.dot(limit_plane.z_axis());
    const double rb = ub.dot(limit_plane.z_axis());

    if (raw <= 0.02 || std::max(ra, rb) < 0.15)
        return {0.0, 0.0};

    const double sd = raw + clearance;

    return {
        ra >= 0.12 ? std::min(520.0, sd / ra) : 0.0,
        rb >= 0.12 ? std::min(520.0, sd / rb) : 0.0
    };
}

/// Max edge snip along (-ua, -ub) separating apex from all non-adjacent facets sharing vertex_id.
std::pair<double, double> compute_saddle_snips(
    const VidyGuide& guide,
    const VidyFacet& facet,
    size_t vertex_id,
    const Point& apex,
    const Vector& ua,
    const Vector& ub
) {

    const std::vector<VidyFacet>& facets = guide.facets();
    const size_t base = 14 * facet.bay;
    double snip_a = 0.0;
    double snip_b = 0.0;

    for (size_t loc = 0; loc < 14; loc++) {
        if (loc == facet.local_index)
            continue;

        const VidyFacet& other = facets[base + loc];
        size_t shared = 0;
        bool has_v = false;

        for (const size_t ov : other.vertices) {
            if (ov == vertex_id)
                has_v = true;

            for (const size_t fv : facet.vertices)
                if (ov == fv)
                    shared++;
        }

        if (!has_v || shared != 1)
            continue;

        const std::optional<Plane> bis =
            compute_cavity_bisector_plane(guide.vertices()[vertex_id], facet.normal, other.normal);

        if (!bis)
            continue;

        const std::pair<double, double> ps =
            compute_plane_corner_snips(apex, ua, ub, *bis, 0.8);
        snip_a = std::max(snip_a, ps.first);
        snip_b = std::max(snip_b, ps.second);
    }

    const double half_total = 0.5 * static_cast<double>(guide.bays) * guide.bay_length;
    const double y_mid =
        -half_total + (static_cast<double>(facet.bay) + 0.5) * guide.bay_length;
    const size_t loc = facet.local_index;

    if ((loc == 4 || loc == 8 || loc == 9 || loc == 13) &&
        std::abs(guide.vertices()[vertex_id][1] - y_mid) < 1.0) {
        const Plane mid_plane = Plane::from_point_normal(
            {0.0, y_mid, 0.0}, {0.0, (loc == 4 || loc == 8) ? 1.0 : -1.0, 0.0}
        );
        const std::pair<double, double> mp =
            compute_plane_corner_snips(apex, ua, ub, mid_plane, 0.8);
        snip_a = std::max(snip_a, mp.first);
        snip_b = std::max(snip_b, mp.second);
    }

    return {snip_a, snip_b};
}

/// Edge snip along -u keeping a roof corner clear of bay planes and full-corner arch-knee border boards.
double compute_station_snip(
    const VidyGuide& guide, const VidyFacet& facet, size_t hi, const Point& apex, const Vector& u
) {

    const double half_total = 0.5 * static_cast<double>(guide.bays) * guide.bay_length;
    const double y_bay0 = -half_total + static_cast<double>(facet.bay) * guide.bay_length;
    const double y_bay1 = y_bay0 + guide.bay_length;
    const size_t loc = facet.local_index;
    const bool outer_haunch = hi >= 2 && (loc == 5 || loc == 7 || loc == 10 || loc == 12);
    const double y_min =
        outer_haunch ? (y_bay0 + guide.plate_thickness + 1.2) : (facet.bay > 0 ? y_bay0 + 0.6 : -1e9);
    const double y_max = outer_haunch ? (y_bay1 - guide.plate_thickness - 1.2)
                                      : (facet.bay + 1 < guide.bays ? y_bay1 - 0.6 : 1e9);
    double snip = 0.0;

    if (apex[1] < y_min && u[1] < -1e-3)
        snip = std::max(snip, (y_min - apex[1]) / (-u[1]));

    if (apex[1] > y_max && u[1] > 1e-3)
        snip = std::max(snip, (apex[1] - y_max) / u[1]);

    return snip;
}

/// Incoming and outgoing corner snip distances (0 when no chamfer is needed at this corner).
std::pair<double, double> compute_corner_snips(
    const VidyGuide& guide, const VidyFacet& facet, size_t k, size_t hi, double factor
) {

    if (factor <= 0.0)
        return {0.0, 0.0};

    const size_t m = facet.vertices.size();
    const Point& apex = facet.miter_rings[hi][k];
    const Point& prev_apex = facet.miter_rings[hi][(k + m - 1) % m];
    const Point& next_apex = facet.miter_rings[hi][(k + 1) % m];
    const Vector ua = (apex - prev_apex).normalized();
    const Vector ub = (apex - next_apex).normalized();
    const std::vector<VidyEdge>& edges = guide.edges();
    const VidyEdge& prev_edge = edges[static_cast<size_t>(facet.edge_ids[(k + m - 1) % m])];
    const VidyEdge& curr_edge = edges[static_cast<size_t>(facet.edge_ids[k])];
    const bool on_ground =
        prev_edge.kind == VidyEdgeKind::Ground || curr_edge.kind == VidyEdgeKind::Ground;
    const bool on_gable =
        prev_edge.kind == VidyEdgeKind::GableEnd || curr_edge.kind == VidyEdgeKind::GableEnd;
    const bool bay_eave =
        (prev_edge.in_situ_eave && curr_edge.kind == VidyEdgeKind::BaySeparation) ||
        (curr_edge.in_situ_eave && prev_edge.kind == VidyEdgeKind::BaySeparation);
    const bool inner_wall = hi <= 1 && facet.role != VidyFacetRole::Roof;

    if (on_ground || on_gable || bay_eave || inner_wall)
        return {0.0, 0.0};

    const size_t valence = guide.vertex_valences()[facet.vertices[k]];
    const double ang_deg = std::acos(std::clamp(ua.dot(ub), -1.0, 1.0)) * Tolerance::TO_DEGREES;
    const double base_c = valence >= 6 ? (180.0 - ang_deg) * (hi == 0 ? 0.18 : 0.52) * factor : 0.0;
    const std::pair<double, double> saddle =
        compute_saddle_snips(guide, facet, facet.vertices[k], apex, ua, ub);
    double snip_a = std::max(base_c, saddle.first * factor);
    double snip_b = std::max(base_c, saddle.second * factor);

    if (facet.role == VidyFacetRole::Roof) {
        snip_a = std::max(snip_a, compute_station_snip(guide, facet, hi, apex, ua) * factor);
        snip_b = std::max(snip_b, compute_station_snip(guide, facet, hi, apex, ub) * factor);
    }

    return {snip_a, snip_b};
}

/// Paired bottom and top closed plate rings with matching vertex counts and sharp unchamfered corners.
std::pair<Polyline, Polyline> compute_layer_rings(
    const VidyGuide& guide,
    const VidyFacet& facet,
    size_t h_bot,
    size_t h_top,
    double factor_bot,
    double factor_top,
    const std::vector<std::vector<Point>>& prof_bot,
    const std::vector<std::vector<Point>>& prof_top
) {

    const size_t m = facet.vertices.size();
    std::vector<Point> pts_b;
    std::vector<Point> pts_t;
    pts_b.reserve(64);
    pts_t.reserve(64);

    for (size_t k = 0; k < m; k++) {
        const Point& apex_b = facet.miter_rings[h_bot][k];
        const Point& apex_t = facet.miter_rings[h_top][k];
        const Point prev_b = prof_bot[(k + m - 1) % m].empty()
                                 ? facet.miter_rings[h_bot][(k + m - 1) % m]
                                 : prof_bot[(k + m - 1) % m].back();
        const Point next_b =
            prof_bot[k].empty() ? facet.miter_rings[h_bot][(k + 1) % m] : prof_bot[k].front();
        const Point prev_t = prof_top[(k + m - 1) % m].empty()
                                 ? facet.miter_rings[h_top][(k + m - 1) % m]
                                 : prof_top[(k + m - 1) % m].back();
        const Point next_t =
            prof_top[k].empty() ? facet.miter_rings[h_top][(k + 1) % m] : prof_top[k].front();
        const std::pair<double, double> sb =
            compute_corner_snips(guide, facet, k, h_bot, factor_bot);
        const std::pair<double, double> st =
            compute_corner_snips(guide, facet, k, h_top, factor_top);

        if (std::max({sb.first, sb.second, st.first, st.second}) < 0.5) {
            pts_b.push_back(apex_b);
            pts_t.push_back(apex_t);
        } else {
            const Vector uab = (apex_b - prev_b).normalized();
            const Vector ubb = (apex_b - next_b).normalized();
            const Vector uat = (apex_t - prev_t).normalized();
            const Vector ubt = (apex_t - next_t).normalized();
            const double cb1 = std::max(sb.first, 0.35 * st.first);
            const double cb2 = std::max(sb.second, 0.35 * st.second);
            const double ct1 = std::max(st.first, 0.35 * sb.first);
            const double ct2 = std::max(st.second, 0.35 * sb.second);
            const Point& raw_prev_b = facet.miter_rings[h_bot][(k + m - 1) % m];
            const Point& raw_next_b = facet.miter_rings[h_bot][(k + 1) % m];
            const Point& raw_prev_t = facet.miter_rings[h_top][(k + m - 1) % m];
            const Point& raw_next_t = facet.miter_rings[h_top][(k + 1) % m];
            const double lim_b1 = std::max(
                2.5, std::min(0.35 * apex_b.distance(raw_prev_b), 0.90 * apex_b.distance(prev_b))
            );
            const double lim_b2 = std::max(
                2.5, std::min(0.35 * apex_b.distance(raw_next_b), 0.90 * apex_b.distance(next_b))
            );
            const double lim_t1 = std::max(
                2.5, std::min(0.35 * apex_t.distance(raw_prev_t), 0.90 * apex_t.distance(prev_t))
            );
            const double lim_t2 = std::max(
                2.5, std::min(0.35 * apex_t.distance(raw_next_t), 0.90 * apex_t.distance(next_t))
            );
            pts_b.push_back(apex_b - uab * std::clamp(cb1, 2.0, lim_b1));
            pts_b.push_back(apex_b - ubb * std::clamp(cb2, 2.0, lim_b2));
            pts_t.push_back(apex_t - uat * std::clamp(ct1, 2.0, lim_t1));
            pts_t.push_back(apex_t - ubt * std::clamp(ct2, 2.0, lim_t2));
        }

        pts_b.insert(pts_b.end(), prof_bot[k].begin(), prof_bot[k].end());
        pts_t.insert(pts_t.end(), prof_top[k].begin(), prof_top[k].end());
    }

    if (compute_newell(pts_b).dot(facet.normal) > 0.0) {
        std::reverse(pts_b.begin(), pts_b.end());
        std::reverse(pts_t.begin(), pts_t.end());
    }

    pts_b.push_back(pts_b.front());
    pts_t.push_back(pts_t.front());

    return {Polyline(pts_b), Polyline(pts_t)};
}

/// Attaches jointed outer outlines and consistently wound through-mortise hole loops onto plate.
void attach_plate_features(
    FacePlanarElement& plate,
    const VidyFacet& facet,
    const Polyline& bot_outline,
    const Polyline& top_outline,
    const std::vector<MortiseSlot>& slots
) {

    plate.features.bottom = {bot_outline};
    plate.features.top = {top_outline};

    for (const MortiseSlot& slot : slots) {
        std::vector<Point> hb(slot.bottom.begin(), slot.bottom.end());
        std::vector<Point> ht(slot.top.begin(), slot.top.end());

        if (compute_newell(hb).dot(facet.normal) < 0.0) {
            std::reverse(hb.begin(), hb.end());
            std::reverse(ht.begin(), ht.end());
        }

        hb.push_back(hb.front());
        ht.push_back(ht.front());
        plate.features.bottom.push_back(Polyline(hb));
        plate.features.top.push_back(Polyline(ht));
    }

    plate.invalidate_geometry();
}

/// Outer boundary gap of a Vidy cavity border board (0.5 mm on BaySeparation, 0 on GableEnd/Ground).
double border_outer_gap(VidyEdgeKind kind) {
    return kind == VidyEdgeKind::BaySeparation ? 0.5 : 0.0;
}

/// CavityBorderSpec for non-Regular side k of facet, including mitered corners and arch-knee bisectors.
CavityBorderSpec
compute_facet_border_spec(const VidyGuide& guide, const VidyFacet& facet, size_t k) {

    const std::vector<VidyEdge>& edges = guide.edges();
    const size_t m = facet.vertices.size();
    const VidyEdge& edge = edges[static_cast<size_t>(facet.edge_ids[k])];
    const VidyEdge& prev_edge = edges[static_cast<size_t>(facet.edge_ids[(k + m - 1) % m])];
    const VidyEdge& next_edge = edges[static_cast<size_t>(facet.edge_ids[(k + 1) % m])];
    CavityBorderSpec spec;
    spec.facet_plane = Plane::from_point_normal(facet.centroid, facet.normal);
    spec.boundary_plane = facet.side_planes[k];
    spec.start_plane = facet.side_planes[(k + m - 1) % m];
    spec.end_plane = facet.side_planes[(k + 1) % m];
    spec.inner_level = guide.plate_thickness + 0.35;
    spec.outer_level = guide.total_offset - guide.plate_thickness - 0.35;
    spec.thickness = guide.plate_thickness;
    spec.outer_gap = border_outer_gap(edge.kind);
    spec.miter_start = prev_edge.kind != VidyEdgeKind::Regular;
    spec.start_neighbor_gap = border_outer_gap(prev_edge.kind);
    spec.start_neighbor_thickness = guide.plate_thickness;
    spec.miter_end = next_edge.kind != VidyEdgeKind::Regular;
    spec.end_neighbor_gap = border_outer_gap(next_edge.kind);
    spec.end_neighbor_thickness = guide.plate_thickness;
    spec.start_setback = 0.5;
    spec.end_setback = 0.5;
    const std::optional<Plane> start_bis =
        find_arch_knee_bisector(guide, facet, facet.vertices[k]);
    const std::optional<Plane> end_bis =
        find_arch_knee_bisector(guide, facet, facet.vertices[(k + 1) % m]);

    if (start_bis) {
        spec.start_plane = *start_bis;
        spec.miter_start = false;
    }

    if (end_bis) {
        spec.end_plane = *end_bis;
        spec.miter_end = false;
    }

    return spec;
}

/// Cloned plate with a fresh guid, optionally moved by delta.
std::shared_ptr<FacePlanarElement>
placed_copy(const FacePlanarElement& plate, const Vector& delta = {0.0, 0.0, 0.0}) {

    std::shared_ptr<FacePlanarElement> copy =
        std::dynamic_pointer_cast<FacePlanarElement>(plate.clone());

    if (!delta.is_zero())
        copy->place(Xform::translation(delta[0], delta[1], delta[2]));

    return copy;
}

/// Facet role tag used in element names.
std::string facet_role_tag(VidyFacetRole role) {

    if (role == VidyFacetRole::LeftWall)
        return "left_wall";

    if (role == VidyFacetRole::RightWall)
        return "right_wall";

    return "roof";
}

/// Optional jointed edge profiles and through-mortises across all Regular creases.
JoineryTables
compute_joinery_tables(const VidyGuide& guide, bool with_joints, double pin_pitch, size_t& seams) {

    const std::vector<VidyFacet>& facets = guide.facets();
    const size_t fcount = facets.size();
    JoineryTables tables;
    tables.plate_slots[0].resize(fcount);
    tables.plate_slots[1].resize(fcount);

    for (size_t hi = 0; hi < 4; hi++) {
        tables.edge_polys[hi].resize(fcount);

        for (size_t f = 0; f < fcount; f++)
            tables.edge_polys[hi][f].resize(facets[f].vertices.size());
    }

    if (!with_joints)
        return tables;

    const std::array<BayCreaseSpec, 16> specs = {
        {{0, 1, 0},
         {2, 3, 0},
         {4, 0, 1},
         {9, 1, 1},
         {8, 2, 1},
         {13, 3, 1},
         {4, 5, 2},
         {9, 10, 2},
         {8, 7, 6},
         {13, 12, 6},
         {6, 5, 3},
         {11, 10, 3},
         {6, 7, 4},
         {11, 12, 4},
         {5, 10, 5},
         {7, 12, 5}}
    };

    for (size_t b = 0; b < guide.bays; b++)
        for (const BayCreaseSpec& spec : specs) {
            compute_bay_crease_joinery(guide, b, spec, pin_pitch, tables);
            seams++;
        }

    return tables;
}

}

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

VidyPavilion::VidyPavilion(
    const VidyGuide& guide,
    bool with_joints,
    double corner_scale,
    double pin_pitch,
    const std::string& name
)
    : WoodSession(name), guide(guide), with_joints(with_joints),
      corner_scale(std::clamp(corner_scale, 0.0, 2.0)),
      pin_pitch(std::clamp(pin_pitch, 350.0, 650.0)) {

    // insertion vectors: one rigid-body assembly insertion direction per shell plate
    compute_insertion_vectors();

    // shell plates: inner and outer CLT plates with selective chamfers and Vidy joinery
    compute_shell_plates();

    // border boards: perpendicular cavity-closing boards extending to all corner planes
    compute_border_plates();

    // scene: inner and outer walls, inner and outer roof, and cavity border boards
    add_plates();
}

// ═══════════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════════

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::inner_plates() const {
    return _inner_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::outer_plates() const {
    return _outer_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::shell_plates() const {
    return _shell_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::border_plates() const {
    return _border_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>&
VidyPavilion::border_plates_for_facet(size_t facet_idx) const {
    return _facet_border_plates.at(facet_idx);
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::all_plates() const {
    return _all_plates;
}

Vector VidyPavilion::plate_insertion_vector(size_t facet_idx, size_t layer) const {
    return _insertion_vectors.at(layer).at(facet_idx);
}

size_t VidyPavilion::total_jointed_seams() const {
    return _total_jointed_seams;
}

size_t VidyPavilion::total_double_pins() const {
    return _total_double_pins;
}

size_t VidyPavilion::total_single_pins() const {
    return _total_single_pins;
}

size_t VidyPavilion::total_pins() const {
    return _total_double_pins + _total_single_pins;
}

size_t VidyPavilion::total_mortise_slots() const {
    return _total_mortise_slots;
}

// ═══════════════════════════════════════════════════════════════════════════
// Insertion vectors, shell plates and border boards
// ═══════════════════════════════════════════════════════════════════════════

void VidyPavilion::compute_insertion_vectors() {

    const std::vector<VidyFacet>& facets = guide.facets();
    _insertion_vectors[0].resize(facets.size(), {0.0, 0.0, 1.0});
    _insertion_vectors[1].resize(facets.size(), {0.0, 0.0, 1.0});

    for (size_t b = 0; b < guide.bays; b++) {
        const size_t base = 14 * b;

        for (const size_t w0 : {size_t{0}, size_t{2}}) {
            const Vector v0 = compute_neighbor_outward(guide, base + w0, base + w0 + 1);
            const Vector v1 = compute_neighbor_outward(guide, base + w0 + 1, base + w0);
            _insertion_vectors[0][base + w0] = v0;
            _insertion_vectors[0][base + w0 + 1] = v1;
            _insertion_vectors[1][base + w0 + 1] = -v0;
            _insertion_vectors[1][base + w0] = -v1;
        }

        for (const std::pair<size_t, size_t>& val :
             {std::pair<size_t, size_t>{5, 10}, std::pair<size_t, size_t>{7, 12}}) {
            const Vector va = compute_neighbor_outward(guide, base + val.first, base + val.second);
            const Vector vb = compute_neighbor_outward(guide, base + val.second, base + val.first);
            _insertion_vectors[0][base + val.first] = -vb;
            _insertion_vectors[0][base + val.second] = vb;
            _insertion_vectors[1][base + val.first] = va;
            _insertion_vectors[1][base + val.second] = vb;
        }

        for (const std::pair<size_t, size_t>& cr :
             {std::pair<size_t, size_t>{6, 5}, std::pair<size_t, size_t>{11, 10}}) {
            const Vector ph = compute_neighbor_outward(guide, base + cr.second, base + cr.first);
            _insertion_vectors[0][base + cr.first] =
                compute_crown_inplane_vector(guide, facets[base + cr.first]);
            _insertion_vectors[1][base + cr.first] =
                -compute_haunch_crown_vector(guide, b, cr.first, ph);
        }

        for (const std::pair<size_t, size_t>& kn :
             {std::pair<size_t, size_t>{4, 5},
              std::pair<size_t, size_t>{9, 10},
              std::pair<size_t, size_t>{8, 7},
              std::pair<size_t, size_t>{13, 12}}) {
            _insertion_vectors[0][base + kn.first] =
                compute_neighbor_outward(guide, base + kn.first, base + kn.second);
            _insertion_vectors[1][base + kn.first] =
                -compute_neighbor_outward(guide, base + kn.second, base + kn.first);
        }
    }
}

void VidyPavilion::compute_shell_plates() {

    const std::vector<VidyFacet>& facets = guide.facets();
    const JoineryTables tables =
        compute_joinery_tables(guide, with_joints, pin_pitch, _total_jointed_seams);
    _total_double_pins = tables.double_pins;
    _total_single_pins = tables.single_pins;
    const std::array<double, 4> factors = {
        corner_scale, corner_scale, corner_scale, corner_scale
    };

    for (size_t f = 0; f < facets.size(); f++) {
        const VidyFacet& facet = facets[f];
        const std::string role_tag = facet_role_tag(facet.role);
        const std::vector<std::vector<Point>> empty_sides(facet.vertices.size());

        for (size_t layer = 0; layer < 2; layer++) {
            const size_t h_b = 2 * layer;
            const size_t h_t = 2 * layer + 1;
            const std::string plate_name = fmt::format(
                "{}_{}_b{}_f{}",
                layer == 0 ? "inner" : "outer",
                role_tag,
                facet.bay,
                facet.local_index
            );
            const std::pair<Polyline, Polyline> stock = compute_layer_rings(
                guide, facet, h_b, h_t, factors[h_b], factors[h_t], empty_sides, empty_sides
            );
            const std::shared_ptr<FacePlanarElement> plate =
                std::make_shared<FacePlanarElement>(stock.first, stock.second, plate_name);

            if (with_joints) {
                const std::pair<Polyline, Polyline> jointed = compute_layer_rings(
                    guide,
                    facet,
                    h_b,
                    h_t,
                    factors[h_b],
                    factors[h_t],
                    tables.edge_polys[h_b][f],
                    tables.edge_polys[h_t][f]
                );
                attach_plate_features(
                    *plate, facet, jointed.first, jointed.second, tables.plate_slots[layer][f]
                );
                _total_mortise_slots += tables.plate_slots[layer][f].size();
            }

            plate->set_insertion_vectors({_insertion_vectors[layer][f]});

            if (layer == 0)
                _inner_plates.push_back(plate);
            else
                _outer_plates.push_back(plate);

            _shell_plates.push_back(plate);
            _all_plates.push_back(plate);
        }
    }
}

void VidyPavilion::compute_border_plates() {

    const std::vector<VidyFacet>& facets = guide.facets();
    const std::vector<VidyEdge>& edges = guide.edges();
    _facet_border_plates.assign(facets.size(), {});

    for (size_t f = 0; f < facets.size(); f++) {
        const VidyFacet& facet = facets[f];
        const std::string role_tag = facet_role_tag(facet.role);

        for (size_t k = 0; k < facet.vertices.size(); k++) {
            const VidyEdge& edge = edges[static_cast<size_t>(facet.edge_ids[k])];

            if (edge.kind == VidyEdgeKind::Regular)
                continue;

            const CavityBorderSpec spec = compute_facet_border_spec(guide, facet, k);
            const std::string kind_tag =
                edge.kind == VidyEdgeKind::BaySeparation
                    ? "bay"
                    : (edge.kind == VidyEdgeKind::GableEnd ? "gable" : "sill");
            const std::string board_name = fmt::format(
                "border_{}_{}_b{}_f{}_s{}", kind_tag, role_tag, facet.bay, facet.local_index, k
            );
            const std::shared_ptr<FacePlanarElement> board =
                compute_cavity_border_board(spec, board_name);

            if (board) {
                _border_plates.push_back(board);
                _facet_border_plates[f].push_back(board);
                _all_plates.push_back(board);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Module assembly and scene
// ═══════════════════════════════════════════════════════════════════════════

void VidyPavilion::add_module_assembly_groups(size_t bay_idx, double roof_lift) {

    const size_t b = std::min(bay_idx, guide.bays - 1);
    add_haunch_assembly_steps(b);
    add_cover_assembly_step(b);
    add_wall_assembly_steps(b, roof_lift);
}

void VidyPavilion::add_haunch_assembly_steps(size_t b) {

    const size_t base = 14 * b;
    const std::shared_ptr<TreeNode> s1 = add_group("module_1_haunch_wings");

    for (const std::pair<size_t, size_t>& wing :
         {std::pair<size_t, size_t>{5, 10}, std::pair<size_t, size_t>{7, 12}}) {
        const Vector spread = {wing.first == 5 ? -750.0 : 750.0, 0.0, 0.0};
        const Vector slide = -_insertion_vectors[1][base + wing.second] * 950.0;
        add(placed_copy(*_inner_plates[base + wing.first], spread), s1);
        add(placed_copy(*_outer_plates[base + wing.first], spread), s1);
        add(placed_copy(*_inner_plates[base + wing.second], spread + slide), s1);
        add(placed_copy(*_outer_plates[base + wing.second], spread + slide), s1);
    }

    const std::shared_ptr<TreeNode> s2 = add_group("module_2_haunches_joined");

    for (const size_t loc : {size_t{5}, size_t{10}, size_t{7}, size_t{12}}) {
        const Vector delta = {(loc == 5 || loc == 10) ? -520.0 : 520.0, 0.0, 0.0};
        add(placed_copy(*_inner_plates[base + loc], delta), s2);
        add(placed_copy(*_outer_plates[base + loc], delta), s2);
    }

    const std::shared_ptr<TreeNode> s3 = add_group("module_3_inner_crowns_eaves");

    for (const size_t loc : {size_t{5}, size_t{10}, size_t{7}, size_t{12}}) {
        add(placed_copy(*_inner_plates[base + loc]), s3);
        add(placed_copy(*_outer_plates[base + loc]), s3);
    }

    for (const size_t loc : {size_t{6}, size_t{11}, size_t{4}, size_t{9}, size_t{8}, size_t{13}}) {
        const double dist = (loc == 6 || loc == 11) ? 1350.0 : 1250.0;
        const Vector delta = -_insertion_vectors[0][base + loc] * dist;
        add(placed_copy(*_inner_plates[base + loc], delta), s3);
    }
}

void VidyPavilion::add_cover_assembly_step(size_t b) {

    const size_t base = 14 * b;
    const std::shared_ptr<TreeNode> s4 = add_group("module_4_outer_covers");

    for (size_t loc = 4; loc < 14; loc++) {
        add(placed_copy(*_inner_plates[base + loc]), s4);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board), s4);

        const bool is_cover =
            loc == 4 || loc == 6 || loc == 8 || loc == 9 || loc == 11 || loc == 13;
        const Vector delta =
            is_cover ? -_insertion_vectors[1][base + loc] * 950.0 : Vector{0.0, 0.0, 0.0};
        add(placed_copy(*_outer_plates[base + loc], delta), s4);
    }
}

void VidyPavilion::add_wall_assembly_steps(size_t b, double roof_lift) {

    const size_t base = 14 * b;
    const Vector lift = {0.0, 0.0, roof_lift};
    const std::shared_ptr<TreeNode> s5 = add_group("module_5_vertical_walls");

    for (size_t loc = 4; loc < 14; loc++) {
        add(placed_copy(*_inner_plates[base + loc], lift), s5);
        add(placed_copy(*_outer_plates[base + loc], lift), s5);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board, lift), s5);
    }

    for (const size_t w0 : {size_t{0}, size_t{2}}) {
        const size_t w1 = w0 + 1;
        const Vector slide1 = -_insertion_vectors[0][base + w1] * 850.0;
        const Vector slide0_out = -_insertion_vectors[1][base + w0] * 750.0;
        add(placed_copy(*_inner_plates[base + w0]), s5);
        add(placed_copy(*_outer_plates[base + w0], slide0_out), s5);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + w0])
            add(placed_copy(*board), s5);

        add(placed_copy(*_inner_plates[base + w1], slide1), s5);
        add(placed_copy(*_outer_plates[base + w1], slide1), s5);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + w1])
            add(placed_copy(*board, slide1), s5);
    }

    const std::shared_ptr<TreeNode> s6 = add_group("module_6_roof_onto_walls");

    for (size_t loc = 0; loc < 4; loc++) {
        add(placed_copy(*_inner_plates[base + loc]), s6);
        add(placed_copy(*_outer_plates[base + loc]), s6);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board), s6);
    }

    for (size_t loc = 4; loc < 14; loc++) {
        add(placed_copy(*_inner_plates[base + loc], lift), s6);
        add(placed_copy(*_outer_plates[base + loc], lift), s6);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board, lift), s6);
    }
}

void VidyPavilion::add_plates() {

    const std::shared_ptr<TreeNode> inner_walls = group_named("inner_walls");
    const std::shared_ptr<TreeNode> outer_walls = group_named("outer_walls");
    const std::shared_ptr<TreeNode> inner_roof = group_named("inner_roof");
    const std::shared_ptr<TreeNode> outer_roof = group_named("outer_roof");
    const std::shared_ptr<TreeNode> border_boards = group_named("border_boards");
    const std::vector<VidyFacet>& facets = guide.facets();

    for (size_t f = 0; f < facets.size(); f++) {
        const bool is_roof = facets[f].role == VidyFacetRole::Roof;
        add(_inner_plates[f], is_roof ? inner_roof : inner_walls);
        add(_outer_plates[f], is_roof ? outer_roof : outer_walls);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[f])
            add(board, border_boards);
    }
}

}
