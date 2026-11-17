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

/// Projects a 4-arris tenon shaft along v_ins onto receiving facet_fem with 0.4 mm CNC slot clearance.
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
    Point cb(0.0, 0.0, 0.0);
    Point ct(0.0, 0.0, 0.0);
    for (size_t c = 0; c < 4; c++) {
        slot.bottom[c] = intersect_line_plane(arris[c], v_ins, p_bot);
        slot.top[c] = intersect_line_plane(arris[c], v_ins, p_top);
        cb = cb + Vector(slot.bottom[c][0], slot.bottom[c][1], slot.bottom[c][2]) * 0.25;
        ct = ct + Vector(slot.top[c][0], slot.top[c][1], slot.top[c][2]) * 0.25;
    }
    for (size_t c = 0; c < 4; c++) {
        const Vector db = (slot.bottom[c] - cb).normalized();
        const Vector dt = (slot.top[c] - ct).normalized();
        slot.bottom[c] = slot.bottom[c] + db * 0.5;
        slot.top[c] = slot.top[c] + dt * 0.5;
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
    double min_pocket_half,
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
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt);
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt);
    const Point pm_b = m_b0 - v_ins * (step_bot ? (le_tab + 0.5) : 0.5);
    const Point pm_t = m_t0 - v_ins * (step_bot ? 0.5 : (le_tab + 0.5));
    const double f_off = first_pin_offset(n_dp, pitch);
    const double obliq = std::abs(v_ins.dot(nf)) * le_tab + 12.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(ends[0]), m_b0.distance(ends[1])),
        std::min(m_t0.distance(ends[2]), m_t0.distance(ends[3]))
    );
    const double sh_half = std::min(
        std::max(f_off + std::abs(shift_val) + 135.0 + 40.0, min_pocket_half),
        std::max(f_off + std::abs(shift_val) + 145.0, max_reach - obliq - 95.0)
    );

    out_bot = {m_b0 - nf * (sh_half + obliq), pm_b - nf * sh_half};
    out_top = {m_t0 - nf * (sh_half + obliq), pm_t - nf * sh_half};
    const Point ptc_b = (pm_b + nf * shift_val) - nf * f_off;
    const Point ptc_t = (pm_t + nf * shift_val) - nf * f_off;

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
    double min_pocket_half,
    double shift_val,
    const VidyFacet& fem_facet,
    double h_bot,
    double h_top,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top,
    std::vector<MortiseSlot>& out_slots
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt);
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt);
    const Point pm_b = m_b0 - v_ins * (step_bot ? (le_tab + 0.5) : 0.5);
    const Point pm_t = m_t0 - v_ins * (step_bot ? 0.5 : (le_tab + 0.5));
    const double f_off = first_pin_offset(n_sp, pitch);
    const double obliq = std::abs(v_ins.dot(nf)) * le_tab + 12.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(ends[0]), m_b0.distance(ends[1])),
        std::min(m_t0.distance(ends[2]), m_t0.distance(ends[3]))
    );
    const double sh_half = std::min(
        std::max(f_off + std::abs(shift_val) + 90.0 + 40.0, min_pocket_half),
        std::max(f_off + std::abs(shift_val) + 100.0, max_reach - obliq - 95.0)
    );

    out_bot = {m_b0 - nf * (sh_half + obliq), pm_b - nf * sh_half};
    out_top = {m_t0 - nf * (sh_half + obliq), pm_t - nf * sh_half};
    const Point ptc_b = (pm_b + nf * shift_val) - nf * f_off;
    const Point ptc_t = (pm_t + nf * shift_val) - nf * f_off;

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
    const double max_reach = std::min(
        std::min(pm_b.distance(ends[0]), pm_b.distance(ends[1])),
        std::min(pm_t.distance(ends[2]), pm_t.distance(ends[3]))
    );
    const double span = std::min(half_span, std::max(120.0, max_reach - obliq - 110.0));

    out_bot = {
        pm_b - nf * (span + obliq),
        (pm_b - nf * span) + v_ins * ext_b,
        (pm_b + nf * span) + v_ins * ext_b,
        pm_b + nf * (span + obliq)
    };
    out_top = {
        pm_t - nf * (span + obliq),
        (pm_t - nf * span) + v_ins * ext_t,
        (pm_t + nf * span) + v_ins * ext_t,
        pm_t + nf * (span + obliq)
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
    const double max_reach = std::min(
        std::min(m_b0.distance(ends[0]), m_b0.distance(ends[1])),
        std::min(m_t0.distance(ends[2]), m_t0.distance(ends[3]))
    );
    const double span = std::min(half_span, std::max(120.0, max_reach - obliq - 95.0));

    out_bot = {
        m_b0 - nf * (span + obliq),
        (m_b0 - nf * span) + v_ins * ext_b,
        (m_b0 + nf * span) + v_ins * ext_b,
        m_b0 + nf * (span + obliq)
    };
    out_top = {
        m_t0 - nf * (span + obliq),
        (m_t0 - nf * span) + v_ins * ext_t,
        (m_t0 + nf * span) + v_ins * ext_t,
        m_t0 + nf * (span + obliq)
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
    double min_shelf_half,
    double shift_val,
    const VidyFacet& fem_facet,
    double h_bot,
    double h_top,
    std::vector<Point>& out_bot,
    std::vector<Point>& out_top,
    std::vector<MortiseSlot>& out_slots
) {

    const Vector nf = (ends[1] - ends[0]).normalized();
    const Point m_b0 = correct_midpoint(ends[0], ends[1], ref_pt);
    const Point m_t0 = correct_midpoint(ends[2], ends[3], ref_pt);
    const Point pm_b = m_b0 - v_ins * (step_bot ? (le_tab + 0.5) : 0.5);
    const Point pm_t = m_t0 - v_ins * (step_bot ? 0.5 : (le_tab + 0.5));
    const double f_off = first_pin_offset(n_dp, pitch);
    const double obliq = std::abs(v_ins.dot(nf)) * (le_offs + le_tab) + 20.0;
    const double max_reach = std::min(
        std::min(m_b0.distance(ends[0]), m_b0.distance(ends[1])),
        std::min(m_t0.distance(ends[2]), m_t0.distance(ends[3]))
    );
    const double req_shelf = std::max(f_off + std::abs(shift_val) + 125.0, min_shelf_half);
    const double half_span = std::min(
        req_shelf,
        std::max(f_off + std::abs(shift_val) + 98.0, max_reach - obliq - 95.0)
    );

    const Point p3_b = (pm_b - nf * half_span) + v_ins * le_offs;
    const Point p4_b = (pm_b + nf * half_span) + v_ins * le_offs;
    const Point p3_t = (pm_t - nf * half_span) + v_ins * le_offs;
    const Point p4_t = (pm_t + nf * half_span) + v_ins * le_offs;

    out_bot = {m_b0 - nf * (half_span + obliq), p3_b};
    out_top = {m_t0 - nf * (half_span + obliq), p3_t};
    const Point ptc_b = (Point::mid_point(p3_b, p4_b) + nf * shift_val) - nf * f_off;
    const Point ptc_t = (Point::mid_point(p3_t, p4_t) + nf * shift_val) - nf * f_off;

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

/// Required edge snip along -u from apex so a roof corner clears transverse station bounds [y_min, y_max].
double station_clearance_snip(const Point& apex, const Vector& u, double y_min, double y_max) {

    double snip = 0.0;
    if (apex[1] < y_min - 1e-3 && u[1] < -1e-3) {
        snip = std::max(snip, (y_min + 0.6 - apex[1]) / (-u[1]));
    }
    if (apex[1] > y_max + 1e-3 && u[1] > 1e-3) {
        snip = std::max(snip, (apex[1] - (y_max - 0.6)) / u[1]);
    }
    return snip;
}

/// Connects directed edge profiles around facet at ring hi with non-colliding multi-valent corner chamfers.
Polyline assemble_chamfered_ring(
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
    const double y_min =
        (facet.local_index <= 8) ? y_bay0 : (y_bay0 + 0.5 * guide.bay_length);
    const double y_max =
        (facet.local_index <= 8) ? (y_bay0 + 0.5 * guide.bay_length) : (y_bay0 + guide.bay_length);

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
        const double base_c = (180.0 - ang_deg) * factor;
        double ca = base_c;
        double cb = base_c;
        if (facet.role == VidyFacetRole::Roof) {
            ca = std::max(ca, station_clearance_snip(apex, ua, y_min, y_max));
            cb = std::max(cb, station_clearance_snip(apex, ub, y_min, y_max));
        }
        pts.push_back(apex - ua * std::min(ca, 0.88 * len_a));
        pts.push_back(apex - ub * std::min(cb, 0.88 * len_b));
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
    size_t loc_a; // Primary segment local index in 0..13 (female Extend on outer roof/wall).
    size_t loc_b; // Mating segment local index in 0..13.
    int kind; // 0 = wall_pleat, 1 = situ_eave, 2 = knee_L, 3 = crown_L, 4 = crown_R, 5 = valley, 6 = knee_R.
};

/// Resolved 3D miter endpoints, insertion vectors, and bevel stepbacks for one intra-bay Regular crease.
struct CreaseFrame {
    std::array<Point, 4> ends_a_in; // Inner bottom/top miter endpoints on facet A.
    std::array<Point, 4> ends_a_out; // Outer bottom/top miter endpoints on facet A.
    std::array<Point, 4> ends_b_in; // Inner bottom/top miter endpoints on facet B.
    std::array<Point, 4> ends_b_out; // Outer bottom/top miter endpoints on facet B.
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

/// Emits wall V-pleat (kind 0) or on-site situ eave (kind 1) contours and updates pin totals.
void emit_wall_or_situ_crease(
    const BayCreaseSpec& spec,
    const CreaseFrame& cf,
    double pitch,
    const VidyFacet& fa,
    const VidyFacet& fb,
    const std::array<double, 4>& heights,
    std::array<std::vector<std::vector<std::vector<Point>>>, 4>& edge_polys,
    std::array<std::vector<std::vector<MortiseSlot>>, 2>& plate_slots,
    size_t& dp_count,
    size_t& sp_count
) {

    if (spec.kind == 1) {
        const double situ_pitch = std::min(pitch, 400.0);
        emit_multiple_double_pins(
            cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, false, 2, situ_pitch, 420.0, 0.0,
            fa, heights, 0, 1, edge_polys[0][cf.fb_i][cf.sb], edge_polys[1][cf.fb_i][cf.sb],
            plate_slots[0][cf.fa_i], plate_slots[1][cf.fa_i]
        );
        emit_multiple_single_pins(
            cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, false, 2, situ_pitch, 390.0, 0.0,
            fa, heights[2], heights[3], edge_polys[2][cf.fb_i][cf.sb], edge_polys[3][cf.fb_i][cf.sb],
            plate_slots[1][cf.fa_i]
        );
        emit_extend_no_pins(
            cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, false, 355.0, 0.0,
            edge_polys[0][cf.fa_i][cf.sa], edge_polys[1][cf.fa_i][cf.sa]
        );
        emit_extend_tab(
            cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, true, 315.0, 0.0,
            edge_polys[2][cf.fa_i][cf.sa], edge_polys[3][cf.fa_i][cf.sa]
        );
        dp_count += 2;
        sp_count += 2;
        return;
    }

    const int n_dp = 12;
    const int n_sp = 12;
    const double sh_dp = -125.0;
    const double sh_sp = 125.0;
    const double f_dp = first_pin_offset(n_dp, pitch);
    const double ext_half = first_pin_offset(n_sp, pitch) + 125.0 + 90.0 + 25.0;
    const double ep_shelf = f_dp + 125.0 + 135.0 + 45.0;
    const double sp_pocket = ext_half + 55.0;
    const double dp_pocket = ep_shelf + 55.0;

    emit_extend_tab(
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, true, ext_half, 0.0,
        edge_polys[2][cf.fa_i][cf.sa], edge_polys[3][cf.fa_i][cf.sa]
    );
    emit_extend_pins(
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, false, n_dp, pitch, ep_shelf, -sh_dp,
        fb, heights[2], heights[3], edge_polys[0][cf.fa_i][cf.sa], edge_polys[1][cf.fa_i][cf.sa],
        plate_slots[1][cf.fb_i]
    );
    emit_multiple_single_pins(
        cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, false, n_sp, pitch, sp_pocket, sh_sp,
        fa, heights[2], heights[3], edge_polys[2][cf.fb_i][cf.sb], edge_polys[3][cf.fb_i][cf.sb],
        plate_slots[1][cf.fa_i]
    );
    emit_multiple_double_pins(
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, false, n_dp, pitch, dp_pocket, sh_dp,
        fa, heights, 0, 1, edge_polys[0][cf.fb_i][cf.sb], edge_polys[1][cf.fb_i][cf.sb],
        plate_slots[0][cf.fa_i], plate_slots[1][cf.fa_i]
    );
    dp_count += static_cast<size_t>(n_dp);
    sp_count += static_cast<size_t>(n_dp + n_sp);
}

/// Emits negative-fold central valley (kind 5) contours and updates pin totals.
void emit_valley_crease(
    const BayCreaseSpec& spec,
    const CreaseFrame& cf,
    double pitch,
    const VidyFacet& fa,
    const VidyFacet& fb,
    const std::array<double, 4>& heights,
    std::array<std::vector<std::vector<std::vector<Point>>>, 4>& edge_polys,
    std::array<std::vector<std::vector<MortiseSlot>>, 2>& plate_slots,
    size_t& dp_count,
    size_t& sp_count
) {

    const int n_dp = 11;
    const int max_even_sp = std::clamp(
        2 * static_cast<int>(std::max(1000.0, cf.min_len - 1850.0) / (2.0 * pitch)), 10, 16
    );
    const int n_sp = std::min(14, max_even_sp);
    const double f_sp = first_pin_offset(n_sp, pitch);
    const double f_dp = first_pin_offset(n_dp, pitch);
    const double ext_val = std::max(f_sp, f_dp) + 135.0 + 25.0;
    const double skew =
        std::abs(cf.v_ins_a.dot((cf.ends_a_out[1] - cf.ends_a_out[0]).normalized()));
    const double ep_shelf = f_dp + 135.0 + skew * cf.l_tab_b + 45.0;
    const double ep_reach = ep_shelf + skew * (cf.l_offs_a + 2.0 * cf.l_tab_a) + 55.0;

    emit_multiple_double_pins(
        cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, true, n_dp, pitch, ep_reach, 0.0,
        fa, heights, 1, 0, edge_polys[2][cf.fb_i][cf.sb], edge_polys[3][cf.fb_i][cf.sb],
        plate_slots[1][cf.fa_i], plate_slots[0][cf.fa_i]
    );
    emit_extend_tab(
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, false, ext_val, 0.0,
        edge_polys[0][cf.fb_i][cf.sb], edge_polys[1][cf.fb_i][cf.sb]
    );
    emit_extend_pins(
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, true, n_dp, pitch, ep_shelf, 0.0,
        fb, heights[0], heights[1], edge_polys[2][cf.fa_i][cf.sa], edge_polys[3][cf.fa_i][cf.sa],
        plate_slots[0][cf.fb_i]
    );
    emit_multiple_single_pins(
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, true, n_sp, pitch, ext_val + 65.0, 0.0,
        fb, heights[0], heights[1], edge_polys[0][cf.fa_i][cf.sa], edge_polys[1][cf.fa_i][cf.sa],
        plate_slots[0][cf.fb_i]
    );
    dp_count += static_cast<size_t>(n_dp);
    sp_count += static_cast<size_t>(n_dp + n_sp);
}

/// Emits Flip_Insert knee and crown (kinds 2, 3, 4, 6) contours and updates pin totals.
void emit_knee_or_crown_crease(
    const BayCreaseSpec& spec,
    const CreaseFrame& cf,
    double pitch,
    const VidyFacet& fa,
    const VidyFacet& fb,
    const std::array<double, 4>& heights,
    std::array<std::vector<std::vector<std::vector<Point>>>, 4>& edge_polys,
    std::array<std::vector<std::vector<MortiseSlot>>, 2>& plate_slots,
    size_t& dp_count,
    size_t& sp_count
) {

    const int n_dp = (spec.kind == 2) ? 3 : ((spec.kind == 6) ? 4 : ((spec.kind == 3) ? 6 : 5));
    int n_sp = (spec.kind == 2) ? 5 : ((spec.kind == 6) ? 5 : ((spec.kind == 3) ? 10 : 8));

    const Vector nf_a = (cf.ends_a_in[1] - cf.ends_a_in[0]).normalized();
    const double skew_a = std::abs(cf.v_ins_a.dot(nf_a));
    double sh_to_apex_b = 0.0;
    double sh_dp_a = 0.0;
    double sh_b = 0.0;
    if (spec.kind == 3 || spec.kind == 4) {
        sh_to_apex_b = (spec.kind == 3) ? 260.0 : 220.0;
        const double to_apex_a =
            (std::abs(cf.ends_a_in[1][0]) < std::abs(cf.ends_a_in[0][0])) ? 1.0 : -1.0;
        sh_b = -sh_to_apex_b * to_apex_a;
        sh_dp_a = -0.45 * skew_a * cf.l_offs_a * to_apex_a;
    }

    const double avail_apex = 0.5 * cf.min_len - sh_to_apex_b - 360.0;
    const double avail_acute = (spec.kind == 3 || spec.kind == 4)
        ? (0.5 * cf.min_len + sh_to_apex_b - 820.0)
        : (0.5 * cf.min_len - 260.0);
    const double avail = std::min(avail_apex, avail_acute);
    for (int guard = 0; guard < 8 && n_sp > 3; guard++) {
        const double p_try = ((n_dp % 2) == (n_sp % 2)) ? 125.0 : 0.0;
        const double reach = first_pin_offset(n_sp, pitch) + p_try + 115.0 + skew_a * (2.0 * cf.l_tab_a) + 45.0;
        if (reach <= avail) {
            break;
        }
        n_sp--;
    }

    const double p_shift = ((n_dp % 2) == (n_sp % 2)) ? 125.0 : 0.0;
    const double f_dp = first_pin_offset(n_dp, pitch);
    const double f_max = std::max(first_pin_offset(n_sp, pitch), f_dp) + sh_to_apex_b + p_shift;
    const double ext_half = f_max + 90.0 + 24.0 + skew_a * (2.0 * cf.l_tab_a);
    const double sp_pocket =
        std::max(ext_half + skew_a * (2.0 * cf.l_tab_a), f_dp + std::abs(sh_dp_a) + skew_a * cf.l_offs_a + 135.0) + 55.0;
    const double ep_shelf =
        f_dp + std::max(sh_to_apex_b + p_shift, std::abs(sh_dp_a)) + 135.0 + skew_a * (cf.l_tab_a + cf.l_tab_b) + 45.0;
    const double dp_pocket = ep_shelf + skew_a * cf.l_offs_b + 55.0;

    emit_extend_tab(
        cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, true, ext_half, 0.0,
        edge_polys[2][cf.fa_i][cf.sa], edge_polys[3][cf.fa_i][cf.sa]
    );
    emit_multiple_double_pins(
        cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, cf.l_offs_a, false, n_dp, pitch, dp_pocket,
        sh_dp_a, fb, heights, 0, 1, edge_polys[0][cf.fa_i][cf.sa], edge_polys[1][cf.fa_i][cf.sa],
        plate_slots[0][cf.fb_i], plate_slots[1][cf.fb_i]
    );
    emit_extend_pins(
        cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, cf.l_offs_b, false, n_dp, pitch, ep_shelf,
        sh_b - p_shift, fa, heights[2], heights[3], edge_polys[0][cf.fb_i][cf.sb],
        edge_polys[1][cf.fb_i][cf.sb], plate_slots[1][cf.fa_i]
    );
    emit_multiple_single_pins(
        cf.ends_b_out, cf.ref_t, cf.v_ins_b, cf.l_tab_b, false, n_sp, pitch, sp_pocket, sh_b + p_shift,
        fa, heights[2], heights[3], edge_polys[2][cf.fb_i][cf.sb], edge_polys[3][cf.fb_i][cf.sb],
        plate_slots[1][cf.fa_i]
    );
    dp_count += static_cast<size_t>(n_dp);
    sp_count += static_cast<size_t>(n_dp + n_sp);
}

/// Populates all 8 contour profiles and mortise slots for one intra-bay Regular crease.
void build_bay_crease_joinery(
    const VidyGuide& guide,
    size_t bay,
    const BayCreaseSpec& spec,
    double pitch,
    std::array<std::vector<std::vector<std::vector<Point>>>, 4>& edge_polys,
    std::array<std::vector<std::vector<MortiseSlot>>, 2>& plate_slots,
    size_t& dp_count,
    size_t& sp_count
) {

    const std::vector<VidyFacet>& facets = guide.facets();
    const std::array<double, 4> heights = guide.layer_heights();
    CreaseFrame cf;
    cf.fa_i = 14 * bay + spec.loc_a;
    cf.fb_i = 14 * bay + spec.loc_b;
    const VidyFacet& fa = facets[cf.fa_i];
    const VidyFacet& fb = facets[cf.fb_i];

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

    cf.sa = find_shared_side(fa, shared[0], shared[1]);
    cf.sb = find_shared_side(fb, shared[0], shared[1]);
    const size_t ma = fa.vertices.size();
    const size_t mb = fb.vertices.size();
    cf.ends_a_in = {
        fa.miter_rings[0][cf.sa], fa.miter_rings[0][(cf.sa + 1) % ma],
        fa.miter_rings[1][cf.sa], fa.miter_rings[1][(cf.sa + 1) % ma]
    };
    cf.ends_a_out = {
        fa.miter_rings[2][cf.sa], fa.miter_rings[2][(cf.sa + 1) % ma],
        fa.miter_rings[3][cf.sa], fa.miter_rings[3][(cf.sa + 1) % ma]
    };
    cf.ends_b_in = {
        fb.miter_rings[0][cf.sb], fb.miter_rings[0][(cf.sb + 1) % mb],
        fb.miter_rings[1][cf.sb], fb.miter_rings[1][(cf.sb + 1) % mb]
    };
    cf.ends_b_out = {
        fb.miter_rings[2][cf.sb], fb.miter_rings[2][(cf.sb + 1) % mb],
        fb.miter_rings[3][cf.sb], fb.miter_rings[3][(cf.sb + 1) % mb]
    };
    cf.ref_b = Point::mid_point(cf.ends_a_in[0], cf.ends_a_in[1]);
    cf.ref_t = Point::mid_point(cf.ends_a_out[0], cf.ends_a_out[1]);
    cf.min_len = std::min({
        cf.ends_a_in[0].distance(cf.ends_a_in[1]), cf.ends_a_out[2].distance(cf.ends_a_out[3]),
        cf.ends_b_in[0].distance(cf.ends_b_in[1]), cf.ends_b_out[2].distance(cf.ends_b_out[3])
    });

    const double phi = 0.5 * std::acos(std::clamp(fa.normal.dot(fb.normal), -1.0, 1.0));
    const double cos_dev = std::max(0.05, std::cos(std::abs(0.5 * Tolerance::PI - 2.0 * phi)));
    const Vector perp_a = inplane_outward_perp(fa, cf.ends_a_in[0], cf.ends_a_in[1]);
    const Vector perp_b = inplane_outward_perp(fb, cf.ends_b_in[0], cf.ends_b_in[1]);
    cf.v_ins_a = (spec.kind == 3 || spec.kind == 4) ? crown_bisector_vector(fa, guide.vertices()) : perp_a;
    cf.v_ins_b = (spec.kind == 1) ? Vector(0.0, 0.0, 1.0) : perp_b;
    const double cos_a = std::max(0.05, std::abs(cf.v_ins_a.dot(perp_a)));
    const double cos_b = std::max(0.05, std::abs(cf.v_ins_b.dot(perp_b)));
    cf.l_tab_a = (guide.plate_thickness / cos_dev) / cos_a;
    cf.l_offs_a = ((guide.total_offset - guide.plate_thickness) / cos_dev) / cos_a;
    cf.l_tab_b = (guide.plate_thickness / cos_dev) / cos_b;
    cf.l_offs_b = ((guide.total_offset - guide.plate_thickness) / cos_dev) / cos_b;

    if (spec.kind == 0 || spec.kind == 1) {
        emit_wall_or_situ_crease(spec, cf, pitch, fa, fb, heights, edge_polys, plate_slots, dp_count, sp_count);
    } else if (spec.kind == 5) {
        emit_valley_crease(spec, cf, pitch, fa, fb, heights, edge_polys, plate_slots, dp_count, sp_count);
    } else {
        emit_knee_or_crown_crease(spec, cf, pitch, fa, fb, heights, edge_polys, plate_slots, dp_count, sp_count);
    }
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

void VidyPavilion::add_module_assembly_groups(size_t bay_idx, double roof_lift) {

    const size_t b = std::min(bay_idx, guide.bays - 1);
    const size_t base = 14 * b;
    const std::vector<VidyFacet>& facets = guide.facets();

    // Step 1 (29-31s): Two 4-plate double-layer haunch wings closing across their valley creases.
    const std::shared_ptr<TreeNode> s1 = add_group("module_1_haunch_wings");
    for (const size_t loc : {size_t{5}, size_t{10}, size_t{7}, size_t{12}}) {
        const double x_wing = (loc == 5 || loc == 10) ? -750.0 : 750.0;
        const double y_val = (loc == 5 || loc == 7) ? -580.0 : 580.0;
        const Vector delta(x_wing, y_val, 0.0);
        add(copy_plate(*_inner_plates[base + loc], delta), s1);
        add(copy_plate(*_outer_plates[base + loc], delta), s1);
    }

    // Step 2 (32-33s): Left and right double-layer haunch wings sliding inward to meet at the crown apex.
    const std::shared_ptr<TreeNode> s2 = add_group("module_2_haunches_joined");
    for (const size_t loc : {size_t{5}, size_t{10}, size_t{7}, size_t{12}}) {
        const Vector delta((loc == 5 || loc == 10) ? -520.0 : 520.0, 0.0, 0.0);
        add(copy_plate(*_inner_plates[base + loc], delta), s2);
        add(copy_plate(*_outer_plates[base + loc], delta), s2);
    }

    // Step 3 (34-35s): Inner crown and eave triangles sliding in-plane with stepped Double_Pins into haunches.
    const std::shared_ptr<TreeNode> s3 = add_group("module_3_inner_crowns_eaves");
    for (const size_t loc : {size_t{5}, size_t{10}, size_t{7}, size_t{12}}) {
        add(copy_plate(*_inner_plates[base + loc]), s3);
        add(copy_plate(*_outer_plates[base + loc]), s3);
    }
    for (const size_t loc : {size_t{6}, size_t{11}, size_t{4}, size_t{9}, size_t{8}, size_t{13}}) {
        const VidyFacet& fc = facets[base + loc];
        Vector delta(0.0, 0.0, 0.0);
        if (loc == 6 || loc == 11) {
            delta = -crown_bisector_vector(fc, guide.vertices()) * 1350.0;
        } else {
            const size_t mate = (loc == 4) ? 5 : ((loc == 9) ? 10 : ((loc == 8) ? 7 : 12));
            const VidyFacet& fm = facets[base + mate];
            std::vector<size_t> sh;
            for (const size_t va : fc.vertices) {
                for (const size_t vb : fm.vertices) {
                    if (va == vb) {
                        sh.push_back(va);
                    }
                }
            }
            const size_t sk = find_shared_side(fc, sh[0], sh[1]);
            const Vector v_knee = inplane_outward_perp(
                fc, fc.miter_rings[0][sk], fc.miter_rings[0][(sk + 1) % fc.vertices.size()]
            );
            delta = -v_knee * 1250.0;
        }
        add(copy_plate(*_inner_plates[base + loc], delta), s3);
    }

    // Step 4 (36-37s): Six female outer cover triangles closing onto exposed Single_Pins and Extend_Pins.
    const std::shared_ptr<TreeNode> s4 = add_group("module_4_outer_covers");
    for (size_t loc = 4; loc < 14; loc++) {
        add(copy_plate(*_inner_plates[base + loc]), s4);
        const bool is_cover =
            (loc == 4 || loc == 6 || loc == 8 || loc == 9 || loc == 11 || loc == 13);
        const Vector delta = is_cover ? (facets[base + loc].normal * 760.0) : Vector(0.0, 0.0, 0.0);
        add(copy_plate(*_outer_plates[base + loc], delta), s4);
    }

    // Step 5 (38-43s): Vertical V-pleated double-layer walls assembling horizontally below the completed roof.
    const std::shared_ptr<TreeNode> s5 = add_group("module_5_vertical_walls");
    for (size_t loc = 4; loc < 14; loc++) {
        add(copy_plate(*_inner_plates[base + loc], Vector(0.0, 0.0, roof_lift)), s5);
        add(copy_plate(*_outer_plates[base + loc], Vector(0.0, 0.0, roof_lift)), s5);
    }
    for (size_t loc = 0; loc < 4; loc++) {
        const double y_in = (loc == 0 || loc == 2) ? -920.0 : 0.0;
        const Vector n_out = (loc == 0 || loc == 2) ? (facets[base + loc].normal * 640.0) : Vector(0.0, 0.0, 0.0);
        add(copy_plate(*_inner_plates[base + loc], Vector(0.0, y_in, 0.0)), s5);
        add(copy_plate(*_outer_plates[base + loc], Vector(0.0, y_in, 0.0) + n_out), s5);
    }

    // Step 6 (44-48s): Complete 20-plate double-layer roof lowering vertically (-Z) onto the vertical walls.
    const std::shared_ptr<TreeNode> s6 = add_group("module_6_roof_onto_walls");
    for (size_t loc = 0; loc < 4; loc++) {
        add(copy_plate(*_inner_plates[base + loc]), s6);
        add(copy_plate(*_outer_plates[base + loc]), s6);
    }
    for (size_t loc = 4; loc < 14; loc++) {
        add(copy_plate(*_inner_plates[base + loc], Vector(0.0, 0.0, roof_lift)), s6);
        add(copy_plate(*_outer_plates[base + loc], Vector(0.0, 0.0, roof_lift)), s6);
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
                const double inset = std::min(460.0, 0.42 * p0.distance(p1));
                stock_polys[hi][f][k] = {p0 + nf * inset, p1 - nf * inset};
                edge_polys[hi][f][k] = {p0 + nf * inset, p1 - nf * inset};
            }
        }
    }
    plate_slots[0].resize(fcount);
    plate_slots[1].resize(fcount);

    _total_jointed_seams = 0;
    _total_double_pins = 0;
    _total_single_pins = 0;

    if (with_joints) {
        const std::array<BayCreaseSpec, 16> specs = {{
            {0, 1, 0}, {2, 3, 0},
            {4, 0, 1}, {9, 1, 1}, {8, 2, 1}, {13, 3, 1},
            {4, 5, 2}, {9, 10, 2}, {8, 7, 6}, {13, 12, 6},
            {6, 5, 3}, {11, 10, 3}, {6, 7, 4}, {11, 12, 4},
            {5, 10, 5}, {7, 12, 5}
        }};
        for (size_t b = 0; b < guide.bays; b++) {
            for (const BayCreaseSpec& spec : specs) {
                build_bay_crease_joinery(
                    guide, b, spec, pin_pitch, edge_polys, plate_slots,
                    _total_double_pins, _total_single_pins
                );
                _total_jointed_seams++;
            }
        }
    }

    const std::array<double, 4> s3_factors = {
        0.40 * corner_scale, 0.75 * corner_scale, 1.08 * corner_scale, 1.25 * corner_scale
    };
    const std::array<double, 4> s4_factors = {
        0.40 * corner_scale, 0.75 * corner_scale, 1.08 * corner_scale, 1.25 * corner_scale
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
                assemble_chamfered_ring(guide, facet, h_b, s3_factors[h_b], stock_polys[h_b][f]);
            const Polyline s_top =
                assemble_chamfered_ring(guide, facet, h_t, s3_factors[h_t], stock_polys[h_t][f]);
            std::shared_ptr<FacePlanarElement> plate =
                std::make_shared<FacePlanarElement>(s_bot, s_top, plate_name);

            if (with_joints) {
                const Polyline j_bot =
                    assemble_chamfered_ring(guide, facet, h_b, s4_factors[h_b], edge_polys[h_b][f]);
                const Polyline j_top =
                    assemble_chamfered_ring(guide, facet, h_t, s4_factors[h_t], edge_polys[h_t][f]);
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
