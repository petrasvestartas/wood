#include "pch.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

/// Paired bottom and top 4-corner hole loops of one rectangular through-mortise slot.
struct MortiseSlot {
    std::array<Point, 4> bottom; // Four corners on the receiving plate's bottom plane.
    std::array<Point, 4> top; // Matching four corners on the receiving plate's top plane.
};

/// Projects ref orthogonally onto the infinite line through start and end.
Point correct_midpoint(const Point& start, const Point& end, const Point& ref) {

    const Vector dir = (end - start).normalized();
    return start + dir * (ref - start).dot(dir);
}

/// Intersects line (origin + t * dir) with plane; falls back to origin when nearly parallel.
Point intersect_line_plane(const Point& origin, const Vector& dir, const Plane& plane) {

    const double denom = dir.dot(plane.z_axis());
    if (std::abs(denom) < 1e-9) {
        return origin;
    }
    const double t = (plane.origin() - origin).dot(plane.z_axis()) / denom;
    return origin + dir * t;
}

/// Unit vector in facet's plane perpendicular to directed edge (p_start -> p_end), pointing out of facet.
Vector inplane_outward_perp(const VidyFacet& facet, const Point& p_start, const Point& p_end) {

    const Vector edge_dir = (p_end - p_start).normalized();
    Vector out = edge_dir.cross(facet.normal).normalized();
    const Point mid = Point::mid_point(p_start, p_end);
    if ((mid - facet.centroid).dot(out) < 0.0) {
        out = -out;
    }
    return out;
}

/// Single in-plane crown insertion vector from the even-station base edge toward the odd-station apex.
Vector crown_bisector_vector(const VidyFacet& facet, const std::vector<Point>& verts) {

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
    Vector raw = apex - Point::mid_point(b0, b1);
    raw = raw - facet.normal * raw.dot(facet.normal);
    return raw.normalized();
}

/// Newell area vector of an open or closed polygon ring.
Vector ring_newell(const std::vector<Point>& pts) {

    Vector n(0.0, 0.0, 0.0);
    if (pts.size() < 3) {
        return n;
    }
    const size_t count =
        (pts.front().distance(pts.back()) < 1e-6 && pts.size() > 3) ? (pts.size() - 1) : pts.size();
    for (size_t i = 0; i < count; i++) {
        const Point& cur = pts[i];
        const Point& nxt = pts[(i + 1) % count];
        n = n + Vector(
            (cur[1] - nxt[1]) * (cur[2] + nxt[2]),
            (cur[2] - nxt[2]) * (cur[0] + nxt[0]),
            (cur[0] - nxt[0]) * (cur[1] + nxt[1])
        );
    }
    return n;
}

/// Projects a 4-arris tenon shaft along v_ins onto the bottom and top planes of receiving facet_fem.
void append_projected_slot(
    const std::pair<Point, Point>& shaft_bot,
    const std::pair<Point, Point>& shaft_top,
    const Vector& v_ins,
    const VidyFacet& facet_fem,
    double h_bot,
    double h_top,
    std::vector<MortiseSlot>& out_slots
) {

    const Plane p_bot = Plane::from_point_normal(
        facet_fem.centroid + facet_fem.normal * h_bot, facet_fem.normal
    );
    const Plane p_top = Plane::from_point_normal(
        facet_fem.centroid + facet_fem.normal * h_top, facet_fem.normal
    );
    const std::array<Point, 4> arris = {
        shaft_bot.first, shaft_bot.second, shaft_top.second, shaft_top.first
    };
    MortiseSlot slot;
    for (size_t c = 0; c < 4; c++) {
        slot.bottom[c] = intersect_line_plane(arris[c], v_ins, p_bot);
        slot.top[c] = intersect_line_plane(arris[c], v_ins, p_top);
    }
    out_slots.push_back(slot);
}

/// Twelve-point stepped chamfered Vidy Double_Pin contour (270 mm base -> 35 mm step -> 180 mm tip -> 140 mm chamfer).
std::vector<Point> make_double_pin_12(
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

/// Six-point chamfered Vidy Single_Pin contour (180 mm shaft -> 140 mm chamfered tip).
std::vector<Point> make_single_pin_6(
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

/// Half-span offset to the first pin centre for n_pins spaced at pitch.
double first_pin_offset(int n_pins, double pitch) {

    if (n_pins % 2 == 0) {
        return (static_cast<double>(n_pins / 2 - 1) * pitch) + 0.5 * pitch;
    }
    return static_cast<double>((n_pins - 1) / 2) * pitch;
}

/// Emits paired bottom/top Multiple_Double_Pins contours and appends base + tip mortises onto fem_facet.
void emit_multiple_double_pins(
    const std::array<Point, 4>& ends,
    const Point& ref_pt,
    const Vector& v_ins,
    double le_tab,
    double le_offs,
    bool step_bot,
    int n_dp,
    double pitch,
    double shift_val,
    const VidyFacet& fem_facet,
    const std::array<double, 4>& heights,
    size_t same_layer,
    size_t opp_layer,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top,
    std::vector<MortiseSlot>& out_same_slots,
    std::vector<MortiseSlot>& out_opp_slots
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt) + nf * shift_val;
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt) + nf * shift_val;
    const Point pm_b = m_b0 - v_ins * (step_bot ? le_tab : 0.0);
    const Point pm_t = m_t0 - v_ins * (step_bot ? 0.0 : le_tab);
    const double f_off = first_pin_offset(n_dp, pitch);
    const double sh_half = f_off + 135.0 + 40.0;
    const double obliq = std::abs(v_ins.dot(nf)) * le_tab + 12.0;

    out_bot = {m_b0 - nf * (sh_half + obliq), pm_b - nf * sh_half};
    out_top = {m_t0 - nf * (sh_half + obliq), pm_t - nf * sh_half};
    const Point ptc_b = pm_b - nf * f_off;
    const Point ptc_t = pm_t - nf * f_off;

    for (int i = 0; i < n_dp; i++) {
        const double step = pitch * static_cast<double>(i);
        std::pair<Point, Point> base_b;
        std::pair<Point, Point> tip_b;
        std::pair<Point, Point> base_t;
        std::pair<Point, Point> tip_t;
        const std::vector<Point> pb =
            make_double_pin_12(ptc_b + nf * step, nf, v_ins, le_tab, le_offs, base_b, tip_b);
        const std::vector<Point> pt =
            make_double_pin_12(ptc_t + nf * step, nf, v_ins, le_tab, le_offs, base_t, tip_t);
        out_bot.insert(out_bot.end(), pb.begin(), pb.end());
        out_top.insert(out_top.end(), pt.begin(), pt.end());
        append_projected_slot(
            base_b, base_t, v_ins, fem_facet, heights[2 * same_layer], heights[2 * same_layer + 1], out_same_slots
        );
        append_projected_slot(
            tip_b, tip_t, v_ins, fem_facet, heights[2 * opp_layer], heights[2 * opp_layer + 1], out_opp_slots
        );
    }
    out_bot.push_back(pm_b + nf * sh_half);
    out_bot.push_back(m_b0 + nf * (sh_half + obliq));
    out_top.push_back(pm_t + nf * sh_half);
    out_top.push_back(m_t0 + nf * (sh_half + obliq));
}

/// Emits paired bottom/top Multiple_Single_Pins contours and appends mortises onto fem_facet.
void emit_multiple_single_pins(
    const std::array<Point, 4>& ends,
    const Point& ref_pt,
    const Vector& v_ins,
    double le_tab,
    bool step_bot,
    int n_sp,
    double pitch,
    double shift_val,
    const VidyFacet& fem_facet,
    double h_bot,
    double h_top,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top,
    std::vector<MortiseSlot>& out_slots
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt) + nf * shift_val;
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt) + nf * shift_val;
    const Point pm_b = m_b0 - v_ins * (step_bot ? le_tab : 0.0);
    const Point pm_t = m_t0 - v_ins * (step_bot ? 0.0 : le_tab);
    const double f_off = first_pin_offset(n_sp, pitch);
    const double sh_half = f_off + 90.0 + 40.0;
    const double obliq = std::abs(v_ins.dot(nf)) * le_tab + 12.0;

    out_bot = {m_b0 - nf * (sh_half + obliq), pm_b - nf * sh_half};
    out_top = {m_t0 - nf * (sh_half + obliq), pm_t - nf * sh_half};
    const Point ptc_b = pm_b - nf * f_off;
    const Point ptc_t = pm_t - nf * f_off;

    for (int i = 0; i < n_sp; i++) {
        const double step = pitch * static_cast<double>(i);
        std::pair<Point, Point> sh_b;
        std::pair<Point, Point> sh_t;
        const std::vector<Point> pb = make_single_pin_6(ptc_b + nf * step, nf, v_ins, le_tab, sh_b);
        const std::vector<Point> pt = make_single_pin_6(ptc_t + nf * step, nf, v_ins, le_tab, sh_t);
        out_bot.insert(out_bot.end(), pb.begin(), pb.end());
        out_top.insert(out_top.end(), pt.begin(), pt.end());
        append_projected_slot(sh_b, sh_t, v_ins, fem_facet, h_bot, h_top, out_slots);
    }
    out_bot.push_back(pm_b + nf * sh_half);
    out_bot.push_back(m_b0 + nf * (sh_half + obliq));
    out_top.push_back(pm_t + nf * sh_half);
    out_top.push_back(m_t0 + nf * (sh_half + obliq));
}

/// Emits paired bottom/top female Extend rabbet lip contours (pure female cover edge).
void emit_extend_tab(
    const std::array<Point, 4>& ends,
    const Point& ref_pt,
    const Vector& v_ins,
    double le_tab,
    bool step_bot,
    double half_span,
    double shift_val,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point pm_b = correct_midpoint(ends[0], ends[1], ref_pt) + nf * shift_val;
    const Point pm_t = correct_midpoint(ends[2], ends[3], ref_pt) + nf * shift_val;
    const double ext_b = le_tab * (step_bot ? 2.0 : 1.0);
    const double ext_t = le_tab * (step_bot ? 1.0 : 2.0);
    const double obliq = std::abs(v_ins.dot(nf)) * (2.0 * le_tab) + 15.0;

    out_bot = {
        pm_b - nf * (half_span + obliq),
        (pm_b - nf * half_span) + v_ins * ext_b,
        (pm_b + nf * half_span) + v_ins * ext_b,
        pm_b + nf * (half_span + obliq)
    };
    out_top = {
        pm_t - nf * (half_span + obliq),
        (pm_t - nf * half_span) + v_ins * ext_t,
        (pm_t + nf * half_span) + v_ins * ext_t,
        pm_t + nf * (half_span + obliq)
    };
}

/// Emits paired bottom/top Extend_NoPins cavity-bridging female eave contours (situ inner roof edge).
void emit_extend_no_pins(
    const std::array<Point, 4>& ends,
    const Point& ref_pt,
    const Vector& v_ins,
    double le_tab,
    double le_offs,
    bool step_bot,
    double half_span,
    double shift_val,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt) + nf * shift_val;
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt) + nf * shift_val;
    const double ext_b = le_offs - (step_bot ? le_tab : 0.0);
    const double ext_t = le_offs - (step_bot ? 0.0 : le_tab);
    const double obliq = std::abs(v_ins.dot(nf)) * le_offs + 15.0;

    out_bot = {
        m_b0 - nf * (half_span + obliq),
        (m_b0 - nf * half_span) + v_ins * ext_b,
        (m_b0 + nf * half_span) + v_ins * ext_b,
        m_b0 + nf * (half_span + obliq)
    };
    out_top = {
        m_t0 - nf * (half_span + obliq),
        (m_t0 - nf * half_span) + v_ins * ext_t,
        (m_t0 + nf * half_span) + v_ins * ext_t,
        m_t0 + nf * (half_span + obliq)
    };
}

/// Emits paired bottom/top Extend_Pins cavity-bridging contours with Single_Pin tips into fem_facet.
void emit_extend_pins(
    const std::array<Point, 4>& ends,
    const Point& ref_pt,
    const Vector& v_ins,
    double le_tab,
    double le_offs,
    bool step_bot,
    int n_dp,
    double pitch,
    double shift_val,
    const VidyFacet& fem_facet,
    double h_bot,
    double h_top,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top,
    std::vector<MortiseSlot>& out_slots
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt) + nf * shift_val;
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt) + nf * shift_val;
    const Point pm_b = m_b0 - v_ins * (step_bot ? le_tab : 0.0);
    const Point pm_t = m_t0 - v_ins * (step_bot ? 0.0 : le_tab);
    const double f_off = first_pin_offset(n_dp, pitch);
    const double half_span = f_off + 90.0 + 45.0;
    const double obliq = std::abs(v_ins.dot(nf)) * (le_offs + le_tab) + 20.0;

    const Point p3_b = (pm_b - nf * half_span) + v_ins * le_offs;
    const Point p4_b = (pm_b + nf * half_span) + v_ins * le_offs;
    const Point p3_t = (pm_t - nf * half_span) + v_ins * le_offs;
    const Point p4_t = (pm_t + nf * half_span) + v_ins * le_offs;

    out_bot = {m_b0 - nf * (half_span + obliq), p3_b};
    out_top = {m_t0 - nf * (half_span + obliq), p3_t};
    const Point ptc_b = Point::mid_point(p3_b, p4_b) - nf * f_off;
    const Point ptc_t = Point::mid_point(p3_t, p4_t) - nf * f_off;

    for (int i = 0; i < n_dp; i++) {
        const double step = pitch * static_cast<double>(i);
        std::pair<Point, Point> sh_b;
        std::pair<Point, Point> sh_t;
        const std::vector<Point> pb = make_single_pin_6(ptc_b + nf * step, nf, v_ins, le_tab, sh_b);
        const std::vector<Point> pt = make_single_pin_6(ptc_t + nf * step, nf, v_ins, le_tab, sh_t);
        out_bot.insert(out_bot.end(), pb.begin(), pb.end());
        out_top.insert(out_top.end(), pt.begin(), pt.end());
        append_projected_slot(sh_b, sh_t, v_ins, fem_facet, h_bot, h_top, out_slots);
    }
    out_bot.push_back(p4_b);
    out_bot.push_back(m_b0 + nf * (half_span + obliq));
    out_top.push_back(p4_t);
    out_top.push_back(m_t0 + nf * (half_span + obliq));
}

/// Directed side index on facet sharing undirected vertices (u, v).
size_t find_shared_side(const VidyFacet& facet, size_t u, size_t v) {

    const size_t m = facet.vertices.size();
    for (size_t k = 0; k < m; k++) {
        const size_t a = facet.vertices[k];
        const size_t b = facet.vertices[(k + 1) % m];
        if ((a == u && b == v) || (a == v && b == u)) {
            return k;
        }
    }
    return 0;
}

/// Connects directed edge profiles around facet at ring hi with Robeller's angle-proportional corner chamfers.
Polyline assemble_chamfered_ring(
    const VidyFacet& facet,
    size_t hi,
    double factor,
    const std::vector<std::vector<Point>>& side_profiles
) {

    const size_t m = facet.vertices.size();
    std::vector<Point> pts;
    pts.reserve(64);

    for (size_t k = 0; k < m; k++) {
        const Point& apex = facet.miter_rings[hi][k];
        const Point& prev_end = side_profiles[(k + m - 1) % m].back();
        const Point& curr_start = side_profiles[k].front();
        const Vector va = apex - prev_end;
        const Vector vb = apex - curr_start;
        const double len_a = va.magnitude();
        const double len_b = vb.magnitude();
        const Vector ua = len_a > 1e-9 ? (va * (1.0 / len_a)) : Vector(1.0, 0.0, 0.0);
        const Vector ub = len_b > 1e-9 ? (vb * (1.0 / len_b)) : Vector(0.0, 1.0, 0.0);
        const double ang_deg =
            std::acos(std::clamp(ua.dot(ub), -1.0, 1.0)) * 180.0 / Tolerance::PI;
        const double chamfer = std::min({(180.0 - ang_deg) * factor, 0.45 * len_a, 0.45 * len_b});
        pts.push_back(apex - ua * chamfer);
        pts.push_back(apex - ub * chamfer);
        pts.insert(pts.end(), side_profiles[k].begin(), side_profiles[k].end());
    }

    if (ring_newell(pts).dot(facet.normal) > 0.0) {
        std::reverse(pts.begin(), pts.end());
    }
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

    plate.features.bottom.clear();
    plate.features.top.clear();
    plate.features.bottom.push_back(bot_outline);
    plate.features.top.push_back(top_outline);

    for (const MortiseSlot& slot : slots) {
        std::vector<Point> hb(slot.bottom.begin(), slot.bottom.end());
        std::vector<Point> ht(slot.top.begin(), slot.top.end());
        if (ring_newell(hb).dot(facet.normal) < 0.0) {
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

/// Intra-bay crease descriptor matching Robeller's VidyComponent.cs segment roles.
struct BayCreaseSpec {
    size_t loc_a; // Female-outer / Extend_Pins segment local index in 0..13.
    size_t loc_b; // Double_Pin / Single_Pin sender segment local index in 0..13.
    int kind; // 0 = wall_pleat, 1 = situ_eave, 2 = knee, 3 = crown_L, 4 = crown_R, 5 = valley.
};

/// Populates all 8 contour profiles and mortise slots for one intra-bay Regular crease.
void build_bay_crease_joinery(
    const VidyGuide& guide,
    size_t bay,
    const BayCreaseSpec& spec,
    double pitch,
    std::array<std::vector<std::vector<std::vector<Point>>>, 4>& edge_polys,
    std::array<std::vector<std::vector<MortiseSlot>>, 2>& plate_slots
) {

    const std::vector<VidyFacet>& facets = guide.facets();
    const std::array<double, 4> heights = guide.layer_heights();
    const size_t fa_i = 14 * bay + spec.loc_a;
    const size_t fb_i = 14 * bay + spec.loc_b;
    const VidyFacet& fa = facets[fa_i];
    const VidyFacet& fb = facets[fb_i];

    std::vector<size_t> shared;
    for (const size_t va : fa.vertices) {
        for (const size_t vb : fb.vertices) {
            if (va == vb) {
                shared.push_back(va);
            }
        }
    }
    if (shared.size() < 2) {
        return;
    }

    const size_t sa = find_shared_side(fa, shared[0], shared[1]);
    const size_t sb = find_shared_side(fb, shared[0], shared[1]);
    const size_t ma = fa.vertices.size();
    const size_t mb = fb.vertices.size();

    const std::array<Point, 4> ends_a_in = {
        fa.miter_rings[0][sa], fa.miter_rings[0][(sa + 1) % ma],
        fa.miter_rings[1][sa], fa.miter_rings[1][(sa + 1) % ma]
    };
    const std::array<Point, 4> ends_a_out = {
        fa.miter_rings[2][sa], fa.miter_rings[2][(sa + 1) % ma],
        fa.miter_rings[3][sa], fa.miter_rings[3][(sa + 1) % ma]
    };
    const std::array<Point, 4> ends_b_in = {
        fb.miter_rings[0][sb], fb.miter_rings[0][(sb + 1) % mb],
        fb.miter_rings[1][sb], fb.miter_rings[1][(sb + 1) % mb]
    };
    const std::array<Point, 4> ends_b_out = {
        fb.miter_rings[2][sb], fb.miter_rings[2][(sb + 1) % mb],
        fb.miter_rings[3][sb], fb.miter_rings[3][(sb + 1) % mb]
    };

    const Point ref_b = Point::mid_point(ends_a_in[0], ends_a_in[1]);
    const Point ref_t = Point::mid_point(ends_a_out[0], ends_a_out[1]);
    const double min_len = std::min({
        ends_a_in[0].distance(ends_a_in[1]),
        ends_a_out[2].distance(ends_a_out[3]),
        ends_b_in[0].distance(ends_b_in[1]),
        ends_b_out[2].distance(ends_b_out[3])
    });

    const double phi = 0.5 * std::acos(std::clamp(fa.normal.dot(fb.normal), -1.0, 1.0));
    const double cos_dev = std::max(0.35, std::cos(std::abs(0.5 * Tolerance::PI - 2.0 * phi)));
    const double l_tab_perp = guide.plate_thickness / cos_dev;
    const double l_offs_perp = (guide.total_offset - guide.plate_thickness) / cos_dev;

    const Vector perp_a = inplane_outward_perp(fa, ends_a_in[0], ends_a_in[1]);
    const Vector perp_b = inplane_outward_perp(fb, ends_b_in[0], ends_b_in[1]);
    Vector v_ins_a = perp_a;
    Vector v_ins_b = perp_b;
    if (spec.kind == 1) {
        v_ins_b = Vector(0.0, 0.0, 1.0);
    } else if (spec.kind == 3 || spec.kind == 4) {
        v_ins_a = crown_bisector_vector(fa, guide.vertices());
    }

    const double cos_a = std::max(0.45, std::abs(v_ins_a.dot(perp_a)));
    const double cos_b = std::max(0.45, std::abs(v_ins_b.dot(perp_b)));
    const double l_tab_a = l_tab_perp / cos_a;
    const double l_offs_a = l_offs_perp / cos_a;
    const double l_tab_b = l_tab_perp / cos_b;
    const double l_offs_b = l_offs_perp / cos_b;

    if (spec.kind == 1) {
        emit_multiple_double_pins(
            ends_b_in, ref_b, v_ins_b, l_tab_b, l_offs_b, false, 2, pitch, 0.0, fa, heights, 0, 1,
            edge_polys[0][fb_i][sb], edge_polys[1][fb_i][sb], plate_slots[0][fa_i], plate_slots[1][fa_i]
        );
        emit_multiple_single_pins(
            ends_b_out, ref_t, v_ins_b, l_tab_b, false, 2, pitch, 0.0, fa, heights[2], heights[3],
            edge_polys[2][fb_i][sb], edge_polys[3][fb_i][sb], plate_slots[1][fa_i]
        );
        emit_extend_no_pins(
            ends_a_in, ref_b, v_ins_a, l_tab_a, l_offs_a, false, 440.0, 0.0,
            edge_polys[0][fa_i][sa], edge_polys[1][fa_i][sa]
        );
        emit_extend_tab(
            ends_a_out, ref_t, v_ins_a, l_tab_a, true, 440.0, 0.0,
            edge_polys[2][fa_i][sa], edge_polys[3][fa_i][sa]
        );
        return;
    }

    if (spec.kind == 5) {
        const int n_pins = std::clamp(static_cast<int>(std::max(600.0, min_len - 2800.0) / pitch), 2, 11);
        emit_multiple_double_pins(
            ends_b_out, ref_t, v_ins_b, l_tab_b, l_offs_b, true, n_pins, pitch, 0.0, fa, heights, 1, 0,
            edge_polys[2][fb_i][sb], edge_polys[3][fb_i][sb], plate_slots[1][fa_i], plate_slots[0][fa_i]
        );
        emit_extend_tab(
            ends_b_in, ref_b, v_ins_b, l_tab_b, false, 0.5 * n_pins * pitch + 90.0, 0.0,
            edge_polys[0][fb_i][sb], edge_polys[1][fb_i][sb]
        );
        emit_extend_pins(
            ends_a_out, ref_t, v_ins_a, l_tab_a, l_offs_a, true, n_pins, pitch, 0.0, fb, heights[0], heights[1],
            edge_polys[2][fa_i][sa], edge_polys[3][fa_i][sa], plate_slots[0][fb_i]
        );
        emit_multiple_single_pins(
            ends_a_in, ref_b, v_ins_a, l_tab_a, true, n_pins, pitch, 0.0, fb, heights[0], heights[1],
            edge_polys[0][fa_i][sa], edge_polys[1][fa_i][sa], plate_slots[0][fb_i]
        );
        return;
    }

    int n_sp = 2;
    int n_dp = 2;
    if (spec.kind == 0) {
        n_sp = std::clamp(static_cast<int>(std::max(1000.0, min_len - 1100.0) / pitch), 2, 12);
        n_dp = std::max(2, n_sp - 1);
    } else if (spec.kind == 2) {
        n_sp = std::clamp(static_cast<int>(std::max(800.0, min_len - 1700.0) / pitch), 2, 5);
        n_dp = std::max(2, n_sp - 1);
    } else {
        n_sp = std::clamp(static_cast<int>(std::max(1000.0, min_len - 3000.0) / pitch), 3, 8);
        n_dp = std::clamp(n_sp - 2, 2, 5);
    }
    if ((n_sp % 2) == (n_dp % 2)) {
        n_dp = std::max(2, n_sp - 1);
    }

    double sh_a = 0.0;
    if (spec.kind == 3) {
        sh_a = 380.0 * (ends_a_in[1][0] > ends_a_in[0][0] ? -1.0 : 1.0);
    } else if (spec.kind == 4) {
        sh_a = 240.0 * (ends_a_in[1][0] > ends_a_in[0][0] ? 1.0 : -1.0);
    }
    const double sh_b = -sh_a;

    emit_extend_tab(
        ends_a_out, ref_t, v_ins_a, l_tab_a, true, 0.5 * n_sp * pitch + 160.0, sh_a,
        edge_polys[2][fa_i][sa], edge_polys[3][fa_i][sa]
    );
    emit_extend_pins(
        ends_a_in, ref_b, v_ins_a, l_tab_a, l_offs_a, false, n_dp, pitch, sh_a, fb, heights[2], heights[3],
        edge_polys[0][fa_i][sa], edge_polys[1][fa_i][sa], plate_slots[1][fb_i]
    );
    emit_multiple_single_pins(
        ends_b_out, ref_t, v_ins_b, l_tab_b, false, n_sp, pitch, sh_b, fa, heights[2], heights[3],
        edge_polys[2][fb_i][sb], edge_polys[3][fb_i][sb], plate_slots[1][fa_i]
    );
    emit_multiple_double_pins(
        ends_b_in, ref_b, v_ins_b, l_tab_b, l_offs_b, false, n_dp, pitch, sh_b, fa, heights, 0, 1,
        edge_polys[0][fb_i][sb], edge_polys[1][fb_i][sb], plate_slots[0][fa_i], plate_slots[1][fa_i]
    );
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// VidyPavilion
// ═══════════════════════════════════════════════════════════════════════════

VidyPavilion::VidyPavilion(
    const VidyGuide& guide,
    bool with_joints,
    double corner_scale,
    double pin_pitch,
    const std::string& name
)
    : WoodSession(name),
      guide(guide),
      with_joints(with_joints),
      corner_scale(std::clamp(corner_scale, 0.0, 2.0)),
      pin_pitch(std::clamp(pin_pitch, 350.0, 650.0)) {

    add_group("inner_walls");
    add_group("outer_walls");
    add_group("inner_roof");
    add_group("outer_roof");

    build_panels_and_joints();
    draw_panels();
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::inner_plates() const {
    return _inner_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::outer_plates() const {
    return _outer_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::all_plates() const {
    return _all_plates;
}

size_t VidyPavilion::total_mortise_slots() const {
    return _total_mortise_slots;
}

std::shared_ptr<FacePlanarElement> VidyPavilion::copy_plate(
    const FacePlanarElement& src,
    const Vector& delta
) {

    const Polyline bot = src.polylines[0].translated(delta);
    const Polyline top = src.polylines[1].translated(delta);
    std::shared_ptr<FacePlanarElement> dst =
        std::make_shared<FacePlanarElement>(bot, top, src.name);
    for (const Polyline& pb : src.features.bottom) {
        dst->features.bottom.push_back(pb.translated(delta));
    }
    for (const Polyline& pt : src.features.top) {
        dst->features.top.push_back(pt.translated(delta));
    }
    dst->invalidate_geometry();
    return dst;
}

Vector VidyPavilion::plate_insertion_vector(size_t facet_idx, size_t layer) const {

    const VidyFacet& facet = guide.facets()[facet_idx];
    const size_t loc = facet.local_index;
    if (facet.role != VidyFacetRole::Roof) {
        const size_t mate_loc = (loc == 0) ? 1 : ((loc == 1) ? 0 : ((loc == 2) ? 3 : 2));
        const VidyFacet& mate = guide.facets()[14 * facet.bay + mate_loc];
        std::vector<size_t> shared;
        for (const size_t u : facet.vertices) {
            for (const size_t v : mate.vertices) {
                if (u == v) {
                    shared.push_back(u);
                }
            }
        }
        const size_t s = find_shared_side(facet, shared[0], shared[1]);
        const size_t m = facet.vertices.size();
        const Vector v_pleat =
            inplane_outward_perp(facet, facet.miter_rings[0][s], facet.miter_rings[0][(s + 1) % m]);
        return (layer == 0) ? v_pleat : (v_pleat * 0.85 - facet.normal * 0.52).normalized();
    }
    if (layer == 1 && (loc == 4 || loc == 6 || loc == 8 || loc == 9 || loc == 11 || loc == 13)) {
        return -facet.normal;
    }
    if (loc == 6 || loc == 11) {
        return crown_bisector_vector(facet, guide.vertices());
    }
    if (loc == 4 || loc == 8 || loc == 9 || loc == 13) {
        const size_t haunch_loc = (loc == 4) ? 5 : ((loc == 9) ? 10 : ((loc == 8) ? 7 : 12));
        const VidyFacet& haunch = guide.facets()[14 * facet.bay + haunch_loc];
        std::vector<size_t> shared;
        for (const size_t u : facet.vertices) {
            for (const size_t v : haunch.vertices) {
                if (u == v) {
                    shared.push_back(u);
                }
            }
        }
        const size_t s = find_shared_side(facet, shared[0], shared[1]);
        return inplane_outward_perp(facet, facet.miter_rings[0][s], facet.miter_rings[0][(s + 1) % 3]);
    }
    const size_t valley_mate = (loc == 5) ? 10 : ((loc == 10) ? 5 : ((loc == 7) ? 12 : 7));
    const VidyFacet& mate = guide.facets()[14 * facet.bay + valley_mate];
    std::vector<size_t> shared;
    for (const size_t u : facet.vertices) {
        for (const size_t v : mate.vertices) {
            if (u == v) {
                shared.push_back(u);
            }
        }
    }
    const size_t s = find_shared_side(facet, shared[0], shared[1]);
    const Vector v_valley =
        inplane_outward_perp(facet, facet.miter_rings[0][s], facet.miter_rings[0][(s + 1) % 3]);
    return (layer == 0) ? v_valley : (-facet.normal * 0.88 + v_valley * 0.47).normalized();
}

void VidyPavilion::add_erection_sequence_groups(size_t bay_idx) {

    const size_t b = std::min(bay_idx, guide.bays - 1);
    const size_t base = 14 * b;
    const std::array<size_t, 10> roof_order = {10, 5, 12, 7, 6, 11, 4, 9, 8, 13};

    // Stage 1: Two vertical V-pleated walls assembled first, plates inserted gradually one after another.
    const std::shared_ptr<TreeNode> g1 = add_group("erection_1_walls");
    const std::array<double, 8> wall_lags = {0.0, 260.0, 520.0, 780.0, 0.0, 360.0, 720.0, 1080.0};
    const std::array<size_t, 8> wall_locs = {0, 1, 0, 1, 2, 3, 2, 3};
    const std::array<size_t, 8> wall_layers = {0, 0, 1, 1, 0, 0, 1, 1};
    for (size_t i = 0; i < 8; i++) {
        const size_t f = base + wall_locs[i];
        const size_t lyr = wall_layers[i];
        const Vector retreat = -plate_insertion_vector(f, lyr) * wall_lags[i];
        const FacePlanarElement& src = (lyr == 0) ? *_inner_plates[f] : *_outer_plates[f];
        add(copy_plate(src, retreat), g1);
    }

    // Stage 2: Roof bottom (inner) layer assembled separately, 10 triangles inserted one after another.
    const std::shared_ptr<TreeNode> g2 = add_group("erection_2_roof_bottom");
    for (size_t step = 0; step < 10; step++) {
        const size_t f = base + roof_order[step];
        const Vector retreat = -plate_insertion_vector(f, 0) * (160.0 * static_cast<double>(step));
        add(copy_plate(*_inner_plates[f], retreat), g2);
    }

    // Stage 3: Roof top (outer) layer inserted gradually one after another onto the locked bottom roof.
    const std::shared_ptr<TreeNode> g3 = add_group("erection_3_roof_top");
    for (size_t step = 0; step < 10; step++) {
        const size_t f = base + roof_order[step];
        add(copy_plate(*_inner_plates[f]), g3);
        const Vector retreat = -plate_insertion_vector(f, 1) * (130.0 * static_cast<double>(step));
        add(copy_plate(*_outer_plates[f], retreat), g3);
    }

    // Stage 4: Complete pre-assembled 20-plate double-layer roof lowered vertically (-Z) onto vertical walls.
    const std::shared_ptr<TreeNode> g4 = add_group("erection_4_roof_onto_walls");
    for (size_t loc = 0; loc < 4; loc++) {
        add(copy_plate(*_inner_plates[base + loc]), g4);
        add(copy_plate(*_outer_plates[base + loc]), g4);
    }
    for (size_t loc = 4; loc < 14; loc++) {
        add(copy_plate(*_inner_plates[base + loc], Vector(0.0, 0.0, 1650.0)), g4);
        add(copy_plate(*_outer_plates[base + loc], Vector(0.0, 0.0, 1650.0)), g4);
    }
}

void VidyPavilion::build_panels_and_joints() {

    const std::vector<VidyFacet>& facets = guide.facets();
    const size_t fcount = facets.size();

    std::array<std::vector<std::vector<std::vector<Point>>>, 4> stock_polys;
    std::array<std::vector<std::vector<std::vector<Point>>>, 4> edge_polys;
    std::array<std::vector<std::vector<MortiseSlot>>, 2> plate_slots;
    for (size_t hi = 0; hi < 4; hi++) {
        stock_polys[hi].resize(fcount);
        edge_polys[hi].resize(fcount);
        for (size_t f = 0; f < fcount; f++) {
            const size_t m = facets[f].vertices.size();
            stock_polys[hi][f].resize(m);
            edge_polys[hi][f].resize(m);
            for (size_t k = 0; k < m; k++) {
                const Point& p0 = facets[f].miter_rings[hi][k];
                const Point& p1 = facets[f].miter_rings[hi][(k + 1) % m];
                const Vector nf = (p1 - p0).normalized();
                stock_polys[hi][f][k] = {p0 + nf * 500.0, p1 - nf * 500.0};
                edge_polys[hi][f][k] = {p0 + nf * 140.0, p1 - nf * 140.0};
            }
        }
    }
    plate_slots[0].resize(fcount);
    plate_slots[1].resize(fcount);

    if (with_joints) {
        const std::array<BayCreaseSpec, 16> specs = {{
            {1, 0, 0}, {3, 2, 0},
            {4, 0, 1}, {9, 1, 1}, {8, 2, 1}, {13, 3, 1},
            {4, 5, 2}, {9, 10, 2}, {8, 7, 2}, {13, 12, 2},
            {6, 5, 3}, {11, 10, 3}, {6, 7, 4}, {11, 12, 4},
            {5, 10, 5}, {7, 12, 5}
        }};
        for (size_t b = 0; b < guide.bays; b++) {
            for (const BayCreaseSpec& spec : specs) {
                build_bay_crease_joinery(guide, b, spec, pin_pitch, edge_polys, plate_slots);
            }
        }
    }

    const std::array<double, 4> s3_factors = {
        0.30 * corner_scale, 0.70 * corner_scale, 0.70 * corner_scale, 0.70 * corner_scale
    };
    const std::array<double, 4> s4_factors = {
        0.25 * corner_scale, 0.65 * corner_scale, 0.65 * corner_scale, 0.65 * corner_scale
    };

    _inner_plates.clear();
    _outer_plates.clear();
    _all_plates.clear();
    _total_mortise_slots = 0;

    for (size_t f = 0; f < fcount; f++) {
        const VidyFacet& facet = facets[f];
        const std::string role_tag = (facet.role == VidyFacetRole::LeftWall)
            ? "left_wall"
            : ((facet.role == VidyFacetRole::RightWall) ? "right_wall" : "roof");

        for (size_t layer = 0; layer < 2; layer++) {
            const size_t h_b = 2 * layer;
            const size_t h_t = 2 * layer + 1;
            const std::string prefix = (layer == 0) ? "inner" : "outer";
            const std::string plate_name =
                fmt::format("{}_{}_b{}_f{}", prefix, role_tag, facet.bay, facet.local_index);

            const Polyline s_bot =
                assemble_chamfered_ring(facet, h_b, s3_factors[h_b], stock_polys[h_b][f]);
            const Polyline s_top =
                assemble_chamfered_ring(facet, h_t, s3_factors[h_t], stock_polys[h_t][f]);
            std::shared_ptr<FacePlanarElement> plate =
                std::make_shared<FacePlanarElement>(s_bot, s_top, plate_name);

            if (with_joints) {
                const Polyline j_bot =
                    assemble_chamfered_ring(facet, h_b, s4_factors[h_b], edge_polys[h_b][f]);
                const Polyline j_top =
                    assemble_chamfered_ring(facet, h_t, s4_factors[h_t], edge_polys[h_t][f]);
                attach_plate_features(*plate, facet, j_bot, j_top, plate_slots[layer][f]);
                _total_mortise_slots += plate_slots[layer][f].size();
            }

            if (layer == 0) {
                _inner_plates.push_back(plate);
            } else {
                _outer_plates.push_back(plate);
            }
            _all_plates.push_back(plate);
        }
    }
}

void VidyPavilion::draw_panels() {

    const std::shared_ptr<TreeNode> inner_walls = group_named("inner_walls");
    const std::shared_ptr<TreeNode> outer_walls = group_named("outer_walls");
    const std::shared_ptr<TreeNode> inner_roof = group_named("inner_roof");
    const std::shared_ptr<TreeNode> outer_roof = group_named("outer_roof");

    const std::vector<VidyFacet>& facets = guide.facets();
    for (size_t f = 0; f < facets.size(); f++) {
        const bool is_roof = (facets[f].role == VidyFacetRole::Roof);
        add(_inner_plates[f], is_roof ? inner_roof : inner_walls);
        add(_outer_plates[f], is_roof ? outer_roof : outer_walls);
    }
}

} // namespace wood_vidy
