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
    Vector v_ins; // Unit insertion direction across the crease.
    double le_tab = 0.0; // Single-layer bevel step length along v_ins.
    double le_offs = 0.0; // Cavity-spanning bevel offset length along v_ins.
    bool step_bot = false; // True when the bottom face steps further than the top face.
    int count = 0; // Pin count along the band.
    double pitch = 500.0; // Centre-to-centre pin spacing along the crease.
    double half_span = 0.0; // Minimum pocket or shelf half-length along the crease.
    double shift = 0.0; // Longitudinal shift of the pin centres from ref_pt.
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
    Vector v_ins_a; // Insertion direction from facet A across the crease.
    Vector v_ins_b; // Insertion direction from facet B across the crease.
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
// Frame vectors and mortise projection
// ═══════════════════════════════════════════════════════════════════════════

/// Unit vector in facet's plane perpendicular to directed edge (p_start -> p_end), pointing outward.
Vector compute_inplane_outward(const VidyFacet& facet, const Point& p_start, const Point& p_end) {

    Vector out = (p_end - p_start).normalized().cross(facet.normal).normalized();

    if ((Point::mid_point(p_start, p_end) - facet.centroid).dot(out) < 0.0)
        out = -out;

    return out;
}

/// In-plane crown insertion vector from the even-station base midpoint toward the odd-station apex.
Vector compute_crown_bisector(const VidyFacet& facet, const std::vector<Point>& verts) {

    const Point& p0 = verts[facet.vertices[0]];
    const Point& p1 = verts[facet.vertices[1]];
    const Point& p2 = verts[facet.vertices[2]];
    Point apex = p2;
    Point b0 = p0;
    Point b1 = p1;

    if (std::abs(p1[1] - p2[1]) < 1.0) {
        apex = p0;
        b0 = p1;
        b1 = p2;
    } else if (std::abs(p2[1] - p0[1]) < 1.0) {
        apex = p1;
        b0 = p2;
        b1 = p0;
    }

    const Vector raw = apex - Point::mid_point(b0, b1);

    return (raw - facet.normal * raw.dot(facet.normal)).normalized();
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
    const Point mi5 = mi4 + v_ins * (1.15 * le_tab);
    const Point pl5 = pl4 + v_ins * (1.15 * le_tab);
    const Point mi6 = (mi4 + v_ins * (1.65 * le_tab)) + nf * 20.0;
    const Point pl6 = (pl4 + v_ins * (1.65 * le_tab)) - nf * 20.0;
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
    const Point mi2 = mi1 + v_ins * (1.15 * le_tab);
    const Point pl2 = pl1 + v_ins * (1.15 * le_tab);
    const Point mi3 = (mi1 + v_ins * (1.65 * le_tab)) + nf * 20.0;
    const Point pl3 = (pl1 + v_ins * (1.65 * le_tab)) - nf * 20.0;
    out_shaft = {mi1, pl1};

    return {mi1, mi2, mi3, pl3, pl2, pl1};
}

/// Half-span offset from the band midpoint to the first pin centre.
double compute_first_pin_offset(int count, double pitch) {

    if (count % 2 == 0)
        return static_cast<double>(count / 2 - 1) * pitch + 0.5 * pitch;

    return static_cast<double>((count - 1) / 2) * pitch;
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
    const Point m_t0 =
        Line::from_points(band.ends[2], band.ends[3]).closest_point(band.ref_pt, false).second;
    const Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double f_off = compute_first_pin_offset(band.count, band.pitch);
    const double obliq = std::abs(band.v_ins.dot(nf)) * band.le_tab + 12.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(band.ends[0]), m_b0.distance(band.ends[1])),
        std::min(m_t0.distance(band.ends[2]), m_t0.distance(band.ends[3]))
    );
    const double sh_half = std::min(
        std::max(f_off + std::abs(band.shift) + 175.0, band.half_span),
        std::max(f_off + std::abs(band.shift) + 145.0, max_reach - obliq - 95.0)
    );
    const size_t opp_layer = 1 - same_layer;
    std::vector<Point> bot = {m_b0 - nf * (sh_half + obliq), pm_b - nf * sh_half};
    std::vector<Point> top = {m_t0 - nf * (sh_half + obliq), pm_t - nf * sh_half};
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
    bot.push_back(m_b0 + nf * (sh_half + obliq));
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * (sh_half + obliq));

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
    const Point m_t0 =
        Line::from_points(band.ends[2], band.ends[3]).closest_point(band.ref_pt, false).second;
    const Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double f_off = compute_first_pin_offset(band.count, band.pitch);
    const double obliq = std::abs(band.v_ins.dot(nf)) * band.le_tab + 12.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(band.ends[0]), m_b0.distance(band.ends[1])),
        std::min(m_t0.distance(band.ends[2]), m_t0.distance(band.ends[3]))
    );
    const double sh_half = std::min(
        std::max(f_off + std::abs(band.shift) + 130.0, band.half_span),
        std::max(f_off + std::abs(band.shift) + 100.0, max_reach - obliq - 95.0)
    );
    std::vector<Point> bot = {m_b0 - nf * (sh_half + obliq), pm_b - nf * sh_half};
    std::vector<Point> top = {m_t0 - nf * (sh_half + obliq), pm_t - nf * sh_half};
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
    bot.push_back(m_b0 + nf * (sh_half + obliq));
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * (sh_half + obliq));

    return {bot, top};
}

/// Paired bottom and top female Extend rabbet lip contours.
std::array<std::vector<Point>, 2> compute_extend_tab(const PinBand& band) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point pm_b =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const Point pm_t =
        Line::from_points(band.ends[2], band.ends[3]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const double ext_b = band.le_tab * (band.step_bot ? 2.0 : 1.0);
    const double ext_t = band.le_tab * (band.step_bot ? 1.0 : 2.0);
    const double obliq = std::abs(band.v_ins.dot(nf)) * (2.0 * band.le_tab) + 15.0;
    const double max_reach = std::min(
        std::min(pm_b.distance(band.ends[0]), pm_b.distance(band.ends[1])),
        std::min(pm_t.distance(band.ends[2]), pm_t.distance(band.ends[3]))
    );
    const double span = std::min(band.half_span, std::max(120.0, max_reach - obliq - 110.0));
    const std::vector<Point> bot = {
        pm_b - nf * (span + obliq),
        (pm_b - nf * span) + band.v_ins * ext_b,
        (pm_b + nf * span) + band.v_ins * ext_b,
        pm_b + nf * (span + obliq)
    };
    const std::vector<Point> top = {
        pm_t - nf * (span + obliq),
        (pm_t - nf * span) + band.v_ins * ext_t,
        (pm_t + nf * span) + band.v_ins * ext_t,
        pm_t + nf * (span + obliq)
    };

    return {bot, top};
}

/// Paired bottom and top Extend_NoPins cavity-bridging female eave contours.
std::array<std::vector<Point>, 2> compute_extend_no_pins(const PinBand& band) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const Point m_t0 =
        Line::from_points(band.ends[2], band.ends[3]).closest_point(band.ref_pt, false).second +
        nf * band.shift;
    const double ext_b = band.le_offs - (band.step_bot ? band.le_tab : 0.0);
    const double ext_t = band.le_offs - (band.step_bot ? 0.0 : band.le_tab);
    const double obliq = std::abs(band.v_ins.dot(nf)) * band.le_offs + 15.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(band.ends[0]), m_b0.distance(band.ends[1])),
        std::min(m_t0.distance(band.ends[2]), m_t0.distance(band.ends[3]))
    );
    const double span = std::min(band.half_span, std::max(120.0, max_reach - obliq - 95.0));
    const std::vector<Point> bot = {
        m_b0 - nf * (span + obliq),
        (m_b0 - nf * span) + band.v_ins * ext_b,
        (m_b0 + nf * span) + band.v_ins * ext_b,
        m_b0 + nf * (span + obliq)
    };
    const std::vector<Point> top = {
        m_t0 - nf * (span + obliq),
        (m_t0 - nf * span) + band.v_ins * ext_t,
        (m_t0 + nf * span) + band.v_ins * ext_t,
        m_t0 + nf * (span + obliq)
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
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second;
    const Point m_t0 =
        Line::from_points(band.ends[2], band.ends[3]).closest_point(band.ref_pt, false).second;
    const Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double f_off = compute_first_pin_offset(band.count, band.pitch);
    const double obliq = std::abs(band.v_ins.dot(nf)) * (band.le_offs + band.le_tab) + 20.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(band.ends[0]), m_b0.distance(band.ends[1])),
        std::min(m_t0.distance(band.ends[2]), m_t0.distance(band.ends[3]))
    );
    const double req_shelf = std::max(f_off + std::abs(band.shift) + 125.0, band.half_span);
    const double span = std::min(
        req_shelf, std::max(f_off + std::abs(band.shift) + 98.0, max_reach - obliq - 95.0)
    );
    const Point p3_b = (pm_b - nf * span) + band.v_ins * band.le_offs;
    const Point p4_b = (pm_b + nf * span) + band.v_ins * band.le_offs;
    const Point p3_t = (pm_t - nf * span) + band.v_ins * band.le_offs;
    const Point p4_t = (pm_t + nf * span) + band.v_ins * band.le_offs;
    std::vector<Point> bot = {m_b0 - nf * (span + obliq), p3_b};
    std::vector<Point> top = {m_t0 - nf * (span + obliq), p3_t};
    const Point ptc_b = (Point::mid_point(p3_b, p4_b) + nf * band.shift) - nf * f_off;
    const Point ptc_t = (Point::mid_point(p3_t, p4_t) + nf * band.shift) - nf * f_off;

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
    bot.push_back(m_b0 + nf * (span + obliq));
    top.push_back(p4_t);
    top.push_back(m_t0 + nf * (span + obliq));

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
        const double situ_pitch = std::min(pitch, 400.0);
        const PinBand dp_band = {
            cf.ends_b_in,
            cf.ref_b,
            cf.v_ins_b,
            cf.l_tab_b,
            cf.l_offs_b,
            false,
            2,
            situ_pitch,
            420.0,
            0.0
        };
        const PinBand sp_band = {
            cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, 0.0, false, 2, situ_pitch, 390.0, 0.0
        };
        const PinBand np_band = {
            cf.ends_a_in,
            cf.ref_b,
            cf.v_ins_a,
            cf.l_tab_a,
            cf.l_offs_a,
            false,
            0,
            situ_pitch,
            355.0,
            0.0
        };
        const PinBand ex_band = {
            cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, 0, situ_pitch, 315.0, 0.0
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

    const int n_dp = 12;
    const int n_sp = 12;
    const double f_dp = compute_first_pin_offset(n_dp, pitch);
    const double ext_half = compute_first_pin_offset(n_sp, pitch) + 240.0;
    const double ep_shelf = f_dp + 305.0;
    const PinBand ex_band = {
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, 0, pitch, ext_half, 0.0
    };
    const PinBand ep_band = {
        cf.ends_a_in,
        cf.ref_b,
        cf.v_ins_a,
        cf.l_tab_a,
        cf.l_offs_a,
        false,
        n_dp,
        pitch,
        ep_shelf,
        125.0
    };
    const PinBand sp_band = {
        cf.ends_b_out,
        cf.ref_t,
        cf.v_ins_b,
        cf.l_tab_b,
        0.0,
        false,
        n_sp,
        pitch,
        ext_half + 55.0,
        125.0
    };
    const PinBand dp_band = {
        cf.ends_b_in,
        cf.ref_b,
        cf.v_ins_b,
        cf.l_tab_b,
        cf.l_offs_b,
        false,
        n_dp,
        pitch,
        ep_shelf + 55.0,
        -125.0
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
    const int n_sp = std::min(
        14,
        std::clamp(
            2 * static_cast<int>(std::max(1000.0, cf.min_len - 1850.0) / (2.0 * pitch)), 10, 16
        )
    );
    const double f_sp = compute_first_pin_offset(n_sp, pitch);
    const double f_dp = compute_first_pin_offset(n_dp, pitch);
    const double ext_val = std::max(f_sp, f_dp) + 160.0;
    const double skew =
        std::abs(cf.v_ins_a.dot((cf.ends_a_out[1] - cf.ends_a_out[0]).normalized()));
    const double ep_shelf = f_dp + 180.0 + skew * cf.l_tab_b;
    const double ep_reach = ep_shelf + skew * (cf.l_offs_a + 2.0 * cf.l_tab_a) + 55.0;
    const PinBand dp_band = {
        cf.ends_b_out,
        cf.ref_t,
        cf.v_ins_b,
        cf.l_tab_b,
        cf.l_offs_b,
        true,
        n_dp,
        pitch,
        ep_reach,
        0.0
    };
    const PinBand ex_band = {
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, 0.0, false, 0, pitch, ext_val, 0.0
    };
    const PinBand ep_band = {
        cf.ends_a_out,
        cf.ref_t,
        cf.v_ins_a,
        cf.l_tab_a,
        cf.l_offs_a,
        true,
        n_dp,
        pitch,
        ep_shelf,
        0.0
    };
    const PinBand sp_band = {
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, 0.0, true, n_sp, pitch, ext_val + 65.0, 0.0
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
    const int n_dp = spec.kind == 2 ? 3 : (spec.kind == 6 ? 4 : (spec.kind == 3 ? 6 : 5));
    int n_sp = spec.kind == 2 ? 5 : (spec.kind == 6 ? 5 : (spec.kind == 3 ? 10 : 8));
    const double skew_a =
        std::abs(cf.v_ins_a.dot((cf.ends_a_in[1] - cf.ends_a_in[0]).normalized()));
    const bool is_crown = spec.kind == 3 || spec.kind == 4;
    const double sh_to_apex_b = is_crown ? (spec.kind == 3 ? 260.0 : 220.0) : 0.0;
    const double to_apex_a =
        std::abs(cf.ends_a_in[1][0]) < std::abs(cf.ends_a_in[0][0]) ? 1.0 : -1.0;
    const double sh_b = is_crown ? -sh_to_apex_b * to_apex_a : 0.0;
    const double sh_dp_a = is_crown ? -0.45 * skew_a * cf.l_offs_a * to_apex_a : 0.0;
    const double avail = std::min(
        0.5 * cf.min_len - sh_to_apex_b - 360.0,
        is_crown ? 0.5 * cf.min_len + sh_to_apex_b - 820.0 : 0.5 * cf.min_len - 260.0
    );

    for (int guard = 0; guard < 8 && n_sp > 3; guard++) {
        const double p_try = (n_dp % 2) == (n_sp % 2) ? 125.0 : 0.0;
        const double reach =
            compute_first_pin_offset(n_sp, pitch) + p_try + 160.0 + skew_a * (2.0 * cf.l_tab_a);

        if (reach <= avail)
            break;

        n_sp--;
    }

    const double p_shift = (n_dp % 2) == (n_sp % 2) ? 125.0 : 0.0;
    const double f_dp = compute_first_pin_offset(n_dp, pitch);
    const double f_max =
        std::max(compute_first_pin_offset(n_sp, pitch), f_dp) + sh_to_apex_b + p_shift;
    const double ext_half = f_max + 114.0 + skew_a * (2.0 * cf.l_tab_a);
    const double sp_pocket = std::max(
                                 ext_half + skew_a * (2.0 * cf.l_tab_a),
                                 f_dp + std::abs(sh_dp_a) + skew_a * cf.l_offs_a + 135.0
                             ) +
                             55.0;
    const double ep_shelf = f_dp + std::max(sh_to_apex_b + p_shift, std::abs(sh_dp_a)) + 180.0 +
                            skew_a * (cf.l_tab_a + cf.l_tab_b);
    const PinBand ex_band = {
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, 0, pitch, ext_half, 0.0
    };
    const PinBand dp_band = {
        cf.ends_a_in,
        cf.ref_b,
        cf.v_ins_a,
        cf.l_tab_a,
        cf.l_offs_a,
        false,
        n_dp,
        pitch,
        ep_shelf + skew_a * cf.l_offs_b + 55.0,
        sh_dp_a
    };
    const PinBand ep_band = {
        cf.ends_b_in,
        cf.ref_b,
        cf.v_ins_b,
        cf.l_tab_b,
        cf.l_offs_b,
        false,
        n_dp,
        pitch,
        ep_shelf,
        sh_b - p_shift
    };
    const PinBand sp_band = {
        cf.ends_b_out,
        cf.ref_t,
        cf.v_ins_b,
        cf.l_tab_b,
        0.0,
        false,
        n_sp,
        pitch,
        sp_pocket,
        sh_b + p_shift
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

/// Resolves the 3D miter endpoints, insertion vectors and bevel stepbacks of one Regular crease.
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
    cf.ref_b = Point::mid_point(cf.ends_a_in[0], cf.ends_a_in[1]);
    cf.ref_t = Point::mid_point(cf.ends_a_out[0], cf.ends_a_out[1]);
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
    cf.v_ins_a =
        (spec.kind == 3 || spec.kind == 4) ? compute_crown_bisector(fa, guide.vertices()) : perp_a;
    cf.v_ins_b = spec.kind == 1 ? Vector{0.0, 0.0, 1.0} : perp_b;
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
// Chamfered rings, features and border setbacks
// ═══════════════════════════════════════════════════════════════════════════

/// Edge snip along -u from apex keeping a roof corner within transverse station bounds [y_min, y_max].
double compute_station_snip(const Point& apex, const Vector& u, double y_min, double y_max) {

    double snip = 0.0;

    if (apex[1] < y_min - 1e-3 && u[1] < -1e-3)
        snip = std::max(snip, (y_min + 0.6 - apex[1]) / (-u[1]));

    if (apex[1] > y_max + 1e-3 && u[1] > 1e-3)
        snip = std::max(snip, (apex[1] - (y_max - 0.6)) / u[1]);

    return snip;
}

/// Closed plate ring at offset index hi with angle-proportional multi-valent corner chamfers.
Polyline compute_chamfered_ring(
    const VidyGuide& guide,
    const VidyFacet& facet,
    size_t hi,
    double factor,
    const std::vector<std::vector<Point>>& side_profiles
) {

    const size_t m = facet.vertices.size();
    std::vector<Point> pts;
    pts.reserve(64);
    const double half_total = 0.5 * static_cast<double>(guide.bays) * guide.bay_length;
    const double y_bay0 = -half_total + static_cast<double>(facet.bay) * guide.bay_length;
    const double y_min = facet.local_index <= 8 ? y_bay0 : y_bay0 + 0.5 * guide.bay_length;
    const double y_max =
        facet.local_index <= 8 ? y_bay0 + 0.5 * guide.bay_length : y_bay0 + guide.bay_length;

    for (size_t k = 0; k < m; k++) {
        const Point& apex = facet.miter_rings[hi][k];
        const Vector va = apex - side_profiles[(k + m - 1) % m].back();
        const Vector vb = apex - side_profiles[k].front();
        const double len_a = va.magnitude();
        const double len_b = vb.magnitude();
        const Vector ua = len_a > 1e-9 ? va / len_a : Vector{1.0, 0.0, 0.0};
        const Vector ub = len_b > 1e-9 ? vb / len_b : Vector{0.0, 1.0, 0.0};
        const double ang_deg = std::acos(std::clamp(ua.dot(ub), -1.0, 1.0)) * Tolerance::TO_DEGREES;
        const double base_c = (180.0 - ang_deg) * factor;
        const double ca = facet.role == VidyFacetRole::Roof
                              ? std::max(base_c, compute_station_snip(apex, ua, y_min, y_max))
                              : base_c;
        const double cb = facet.role == VidyFacetRole::Roof
                              ? std::max(base_c, compute_station_snip(apex, ub, y_min, y_max))
                              : base_c;
        pts.push_back(apex - ua * std::min(ca, 0.88 * len_a));
        pts.push_back(apex - ub * std::min(cb, 0.88 * len_b));
        pts.insert(pts.end(), side_profiles[k].begin(), side_profiles[k].end());
    }

    if (compute_newell(pts).dot(facet.normal) > 0.0)
        std::reverse(pts.begin(), pts.end());

    pts.push_back(pts.front());

    return Polyline(pts);
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

/// End clearance setback from neighbor_edge's side plane for a Vidy cavity border board.
double compute_border_setback(
    const VidyGuide& guide,
    const VidyFacet& facet,
    VidyEdgeKind border_kind,
    const VidyEdge& neighbor_edge,
    double board_depth_inward,
    double corner_scale
) {

    if (neighbor_edge.in_situ_eave)
        return facet.role == VidyFacetRole::Roof ? 165.0 : 110.0;

    const bool neighbor_vertical_border = neighbor_edge.kind == VidyEdgeKind::BaySeparation ||
                                          neighbor_edge.kind == VidyEdgeKind::GableEnd;

    if (border_kind == VidyEdgeKind::Ground && neighbor_vertical_border)
        return board_depth_inward + 2.5;

    if ((border_kind == VidyEdgeKind::BaySeparation || border_kind == VidyEdgeKind::GableEnd) &&
        neighbor_edge.kind == VidyEdgeKind::Ground)
        return std::max(14.0, 22.0 * corner_scale);

    if (facet.local_index == 6 || facet.local_index == 11)
        return guide.total_offset + 55.0;

    if (facet.role == VidyFacetRole::Roof)
        return std::max(65.0, 95.0 * corner_scale);

    return std::max(30.0, 45.0 * corner_scale);
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

/// Stock edge segments and optional jointed edge profiles + mortises across all facets.
std::pair<std::array<std::vector<std::vector<std::vector<Point>>>, 4>, JoineryTables>
compute_stock_and_joinery(
    const VidyGuide& guide, bool with_joints, double pin_pitch, size_t& out_seams
) {

    const std::vector<VidyFacet>& facets = guide.facets();
    const size_t fcount = facets.size();
    std::array<std::vector<std::vector<std::vector<Point>>>, 4> stock_polys;
    JoineryTables tables;
    tables.plate_slots[0].resize(fcount);
    tables.plate_slots[1].resize(fcount);

    for (size_t hi = 0; hi < 4; hi++) {
        stock_polys[hi].resize(fcount);
        tables.edge_polys[hi].resize(fcount);

        for (size_t f = 0; f < fcount; f++) {
            const size_t m = facets[f].vertices.size();
            stock_polys[hi][f].resize(m);
            tables.edge_polys[hi][f].resize(m);

            for (size_t k = 0; k < m; k++) {
                const Point& p0 = facets[f].miter_rings[hi][k];
                const Point& p1 = facets[f].miter_rings[hi][(k + 1) % m];
                const Vector nf = (p1 - p0).normalized();
                const double inset = std::min(460.0, 0.42 * p0.distance(p1));
                stock_polys[hi][f][k] = {p0 + nf * inset, p1 - nf * inset};
                tables.edge_polys[hi][f][k] = stock_polys[hi][f][k];
            }
        }
    }

    if (!with_joints)
        return {stock_polys, tables};

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
            out_seams++;
        }

    return {stock_polys, tables};
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

    // shell plates: inner and outer chamfered CLT plates with optional double-layer tenon joinery
    compute_shell_plates();

    // border boards: perpendicular cavity-closing boards on bay separations, gables and sills
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
// Shell and border plates
// ═══════════════════════════════════════════════════════════════════════════

void VidyPavilion::compute_shell_plates() {

    const std::vector<VidyFacet>& facets = guide.facets();
    const std::pair<std::array<std::vector<std::vector<std::vector<Point>>>, 4>, JoineryTables>
        built = compute_stock_and_joinery(guide, with_joints, pin_pitch, _total_jointed_seams);
    const std::array<std::vector<std::vector<std::vector<Point>>>, 4>& stock_polys = built.first;
    const JoineryTables& tables = built.second;
    _total_double_pins = tables.double_pins;
    _total_single_pins = tables.single_pins;
    const std::array<double, 4> factors = {
        0.40 * corner_scale, 0.75 * corner_scale, 1.08 * corner_scale, 1.25 * corner_scale
    };

    for (size_t f = 0; f < facets.size(); f++) {
        const VidyFacet& facet = facets[f];
        const std::string role_tag = facet_role_tag(facet.role);

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
            const Polyline s_bot =
                compute_chamfered_ring(guide, facet, h_b, factors[h_b], stock_polys[h_b][f]);
            const Polyline s_top =
                compute_chamfered_ring(guide, facet, h_t, factors[h_t], stock_polys[h_t][f]);
            const std::shared_ptr<FacePlanarElement> plate =
                std::make_shared<FacePlanarElement>(s_bot, s_top, plate_name);

            if (with_joints) {
                const Polyline j_bot = compute_chamfered_ring(
                    guide, facet, h_b, factors[h_b], tables.edge_polys[h_b][f]
                );
                const Polyline j_top = compute_chamfered_ring(
                    guide, facet, h_t, factors[h_t], tables.edge_polys[h_t][f]
                );
                attach_plate_features(*plate, facet, j_bot, j_top, tables.plate_slots[layer][f]);
                _total_mortise_slots += tables.plate_slots[layer][f].size();
            }

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
    const double outer_gap = 1.0;
    const double depth_inward = outer_gap + guide.plate_thickness;
    _facet_border_plates.assign(facets.size(), {});

    for (size_t f = 0; f < facets.size(); f++) {
        const VidyFacet& facet = facets[f];
        const Plane facet_plane = Plane::from_point_normal(facet.centroid, facet.normal);
        const std::string role_tag = facet_role_tag(facet.role);
        const size_t m = facet.vertices.size();

        for (size_t k = 0; k < m; k++) {
            const VidyEdge& edge = edges[static_cast<size_t>(facet.edge_ids[k])];

            if (edge.kind == VidyEdgeKind::Regular)
                continue;

            const VidyEdge& prev_edge = edges[static_cast<size_t>(facet.edge_ids[(k + m - 1) % m])];
            const VidyEdge& next_edge = edges[static_cast<size_t>(facet.edge_ids[(k + 1) % m])];
            CavityBorderSpec spec;
            spec.facet_plane = facet_plane;
            spec.boundary_plane = facet.side_planes[k];
            spec.start_plane = facet.side_planes[(k + m - 1) % m];
            spec.end_plane = facet.side_planes[(k + 1) % m];
            spec.inner_edge = {facet.miter_rings[1][k], facet.miter_rings[1][(k + 1) % m]};
            spec.outer_edge = {facet.miter_rings[2][k], facet.miter_rings[2][(k + 1) % m]};
            spec.inner_level = guide.plate_thickness + 0.35;
            spec.outer_level = guide.total_offset - guide.plate_thickness - 0.35;
            spec.thickness = guide.plate_thickness;
            spec.outer_gap = outer_gap;
            spec.start_setback = compute_border_setback(
                guide, facet, edge.kind, prev_edge, depth_inward, corner_scale
            );
            spec.end_setback = compute_border_setback(
                guide, facet, edge.kind, next_edge, depth_inward, corner_scale
            );

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
    const std::vector<VidyFacet>& facets = guide.facets();
    const std::shared_ptr<TreeNode> s1 = add_group("module_1_haunch_wings");

    for (const size_t loc : {size_t{5}, size_t{10}, size_t{7}, size_t{12}}) {
        const Vector delta = {
            (loc == 5 || loc == 10) ? -750.0 : 750.0, (loc == 5 || loc == 7) ? -580.0 : 580.0, 0.0
        };
        add(placed_copy(*_inner_plates[base + loc], delta), s1);
        add(placed_copy(*_outer_plates[base + loc], delta), s1);
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
        const VidyFacet& fc = facets[base + loc];
        Vector delta = {0.0, 0.0, 0.0};

        if (loc == 6 || loc == 11) {
            delta = -compute_crown_bisector(fc, guide.vertices()) * 1350.0;
        } else {
            const size_t mate = loc == 4 ? 5 : (loc == 9 ? 10 : (loc == 8 ? 7 : 12));
            std::vector<size_t> sh;

            for (const size_t va : fc.vertices)
                for (const size_t vb : facets[base + mate].vertices)
                    if (va == vb)
                        sh.push_back(va);

            const size_t sk = find_shared_side(fc, sh[0], sh[1]);
            delta = -compute_inplane_outward(
                        fc, fc.miter_rings[0][sk], fc.miter_rings[0][(sk + 1) % fc.vertices.size()]
                    ) *
                    1250.0;
        }

        add(placed_copy(*_inner_plates[base + loc], delta), s3);
    }
}

void VidyPavilion::add_cover_assembly_step(size_t b) {

    const size_t base = 14 * b;
    const std::vector<VidyFacet>& facets = guide.facets();
    const std::shared_ptr<TreeNode> s4 = add_group("module_4_outer_covers");

    for (size_t loc = 4; loc < 14; loc++) {
        add(placed_copy(*_inner_plates[base + loc]), s4);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board), s4);

        const bool is_cover =
            loc == 4 || loc == 6 || loc == 8 || loc == 9 || loc == 11 || loc == 13;
        const Vector delta = is_cover ? facets[base + loc].normal * 760.0 : Vector{0.0, 0.0, 0.0};
        add(placed_copy(*_outer_plates[base + loc], delta), s4);
    }
}

void VidyPavilion::add_wall_assembly_steps(size_t b, double roof_lift) {

    const size_t base = 14 * b;
    const std::vector<VidyFacet>& facets = guide.facets();
    const Vector lift = {0.0, 0.0, roof_lift};
    const std::shared_ptr<TreeNode> s5 = add_group("module_5_vertical_walls");

    for (size_t loc = 4; loc < 14; loc++) {
        add(placed_copy(*_inner_plates[base + loc], lift), s5);
        add(placed_copy(*_outer_plates[base + loc], lift), s5);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board, lift), s5);
    }

    for (size_t loc = 0; loc < 4; loc++) {
        const Vector slide = {0.0, (loc == 0 || loc == 2) ? -920.0 : 0.0, 0.0};
        const Vector out =
            (loc == 0 || loc == 2) ? facets[base + loc].normal * 640.0 : Vector{0.0, 0.0, 0.0};
        add(placed_copy(*_inner_plates[base + loc], slide), s5);

        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[base + loc])
            add(placed_copy(*board, slide), s5);

        add(placed_copy(*_outer_plates[base + loc], slide + out), s5);
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
