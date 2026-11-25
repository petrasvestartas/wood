#include "pch.h"
#include "src/templates/cavity_border.h"
#include "src/templates/vidy/vidy.h"
#include "src/templates/vidy/vidy_joint_kernel.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

using MortiseSlot = VidyMortiseSlot;
using PinBand = VidyPinBand;

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

/// Directed side index on facet connecting facet_idx to neighbor_idx.
size_t find_neighbor_side(const VidyGuide& guide, size_t facet_idx, size_t neighbor_idx) {

    const VidyFacet& fa = guide.facets()[facet_idx];
    const VidyFacet& fb = guide.facets()[neighbor_idx];
    std::vector<size_t> shared;

    for (const size_t va : fa.vertices)
        for (const size_t vb : fb.vertices)
            if (va == vb)
                shared.push_back(va);

    return shared.size() >= 2 ? vidy_find_shared_side(fa.vertices, shared[0], shared[1]) : 0;
}

/// Outward unit vector on facet's side shared with neighbor_idx.
Vector compute_neighbor_outward(const VidyGuide& guide, size_t facet_idx, size_t neighbor_idx) {

    const VidyFacet& f = guide.facets()[facet_idx];
    const size_t s = find_neighbor_side(guide, facet_idx, neighbor_idx);
    const size_t m = f.vertices.size();

    return vidy_inplane_outward(
        f.centroid, f.normal, f.miter_rings[0][s], f.miter_rings[0][(s + 1) % m]
    );
}

/// Single in-plane insertion vector of inner crown (6 or 11): normalized sum of Regular orthogonals.
Vector compute_crown_inplane_vector(const VidyGuide& guide, const VidyFacet& crown) {

    const std::vector<VidyEdge>& edges = guide.edges();
    const size_t m = crown.vertices.size();
    Vector sum = {0.0, 0.0, 0.0};

    for (size_t k = 0; k < m; k++)
        if (edges[static_cast<size_t>(crown.edge_ids[k])].kind == VidyEdgeKind::Regular)
            sum = sum + vidy_inplane_outward(
                            crown.centroid,
                            crown.normal,
                            crown.miter_rings[0][k],
                            crown.miter_rings[0][(k + 1) % m]
                        );

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

    return dir.dot(perp_outward) < 0.0 ? -dir : dir;
}

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

/// True when pt clears all non-crease sides of facet (including pocket stepback on Regular sides).
bool vidy_pt_clears_non_crease_sides(
    const VidyGuide& guide,
    const Point& pt,
    const VidyFacet& facet,
    size_t ring_idx,
    size_t crease_side
) {

    const std::vector<VidyEdge>& edges = guide.edges();
    const std::vector<Point>& ring = facet.miter_rings[ring_idx];
    const size_t m = ring.size();

    for (size_t k = 0; k < m; k++) {
        if (k == crease_side)
            continue;

        const Point& a = ring[k];
        const Point& b = ring[(k + 1) % m];

        if ((b - a).magnitude() < 1e-6)
            continue;

        const Vector inward = -vidy_inplane_outward(facet.centroid, facet.normal, a, b);
        const VidyEdge& ed = edges[static_cast<size_t>(facet.edge_ids[k])];
        const double req = ed.kind == VidyEdgeKind::Regular ? (1.55 * guide.plate_thickness)
                                                            : (0.85 * guide.plate_thickness);

        if ((pt - a).dot(inward) < req)
            return false;
    }

    return true;
}

/// True when all slots from start_idx onward clear non-crease sides and stay >= 14 mm from prior slots.
bool new_vidy_slots_valid(
    const VidyGuide& guide,
    const std::vector<MortiseSlot>& slots,
    size_t start_idx,
    const VidyFacet& facet,
    size_t layer,
    size_t crease_side
) {

    const size_t rb = 2 * layer;
    const size_t rt = 2 * layer + 1;

    for (size_t idx = start_idx; idx < slots.size(); idx++) {
        const MortiseSlot& s = slots[idx];

        for (size_t c = 0; c < 4; c++)
            if (!vidy_pt_clears_non_crease_sides(guide, s.bottom[c], facet, rb, crease_side) ||
                !vidy_pt_clears_non_crease_sides(guide, s.top[c], facet, rt, crease_side))
                return false;

        for (size_t prev = 0; prev < idx; prev++)
            if (vidy_quads_closer_than_2d(s.bottom, slots[prev].bottom, facet.normal, 14.0) ||
                vidy_quads_closer_than_2d(s.top, slots[prev].top, facet.normal, 14.0))
                return false;
    }

    return true;
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
        const Vector to_even = std::abs(cf.ends_b_out[0][0]) < std::abs(cf.ends_b_out[1][0])
                                   ? (cf.ends_b_out[0] - cf.ends_b_out[1]).normalized()
                                   : (cf.ends_b_out[1] - cf.ends_b_out[0]).normalized();
        const Point situ_ref_b =
            Point::mid_point(cf.ends_b_in[0], cf.ends_b_in[1]) + to_even * 35.0;
        const Point situ_ref_t =
            Point::mid_point(cf.ends_b_out[0], cf.ends_b_out[1]) + to_even * 35.0;
        const double situ_ex = 80.0 * std::min(1.0, pitch / 500.0);
        const int max_situ = cf.min_len >= 2.0 * pitch + 260.0 ? 2 : 1;
        const size_t sa0 = tables.plate_slots[0][cf.fa_i].size();
        const size_t sa1 = tables.plate_slots[1][cf.fa_i].size();
        int n_situ = max_situ;

        for (int try_n = max_situ; try_n >= 1; try_n--) {
            tables.plate_slots[0][cf.fa_i].resize(sa0);
            tables.plate_slots[1][cf.fa_i].resize(sa1);
            const PinBand dp_band = {
                cf.ends_b_in,
                situ_ref_b,
                cf.v_ins_b,
                cf.l_tab_b,
                cf.l_offs_b,
                false,
                try_n,
                pitch,
                situ_ex,
                0.0
            };
            const PinBand sp_band = {
                cf.ends_b_out,
                situ_ref_t,
                cf.v_ins_b,
                cf.l_tab_b,
                0.0,
                false,
                try_n,
                pitch,
                situ_ex,
                0.0
            };
            const PinBand np_band = {
                cf.ends_a_in,
                situ_ref_b,
                cf.v_ins_a,
                cf.l_tab_a,
                cf.l_offs_a,
                false,
                try_n,
                pitch,
                0.0,
                0.0
            };
            const PinBand ex_band = {
                cf.ends_a_out, situ_ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, try_n, pitch, 0.0, 0.0
            };
            assign_layer_edge(
                tables.edge_polys,
                0,
                cf.fb_i,
                cf.sb,
                vidy_emit_double_pins(
                    dp_band,
                    fa.centroid,
                    fa.normal,
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
                vidy_emit_single_pins(
                    sp_band,
                    fa.centroid,
                    fa.normal,
                    heights[2],
                    heights[3],
                    tables.plate_slots[1][cf.fa_i]
                )
            );
            assign_layer_edge(
                tables.edge_polys, 0, cf.fa_i, cf.sa, vidy_emit_extend_lip(np_band, true)
            );
            assign_layer_edge(
                tables.edge_polys, 1, cf.fa_i, cf.sa, vidy_emit_extend_lip(ex_band, false)
            );
            n_situ = try_n;
            const bool ok0 =
                new_vidy_slots_valid(guide, tables.plate_slots[0][cf.fa_i], sa0, fa, 0, cf.sa);
            const bool ok1 =
                new_vidy_slots_valid(guide, tables.plate_slots[1][cf.fa_i], sa1, fa, 1, cf.sa);

            if (ok0 && ok1)
                break;
        }

        tables.double_pins += static_cast<size_t>(n_situ);
        tables.single_pins += static_cast<size_t>(n_situ);
        return;
    }

    const int max_w_dp = std::max(1, static_cast<int>(std::floor(0.76 * cf.min_len / pitch)));
    const int max_w_sp = std::max(1, static_cast<int>(std::floor(0.86 * cf.min_len / pitch)));
    const int n_dp = std::min(cf.min_len >= 7200.0 ? 11 : 10, max_w_dp);
    const int n_sp = std::min(cf.min_len >= 7000.0 ? 12 : 10, max_w_sp);
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
    assign_layer_edge(tables.edge_polys, 1, cf.fa_i, cf.sa, vidy_emit_extend_lip(ex_band, false));
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fa_i,
        cf.sa,
        vidy_emit_extend_pins(
            ep_band, fb.centroid, fb.normal, heights[2], heights[3], tables.plate_slots[1][cf.fb_i]
        )
    );
    assign_layer_edge(
        tables.edge_polys,
        1,
        cf.fb_i,
        cf.sb,
        vidy_emit_single_pins(
            sp_band, fa.centroid, fa.normal, heights[2], heights[3], tables.plate_slots[1][cf.fa_i]
        )
    );
    assign_layer_edge(
        tables.edge_polys,
        0,
        cf.fb_i,
        cf.sb,
        vidy_emit_double_pins(
            dp_band,
            fa.centroid,
            fa.normal,
            heights,
            0,
            tables.plate_slots[0][cf.fa_i],
            tables.plate_slots[1][cf.fa_i]
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
    const double keep_v_dp = 1.35 * guide.total_offset;
    const double keep_v_sp = 0.85 * guide.total_offset;
    const double av_v_dp = std::min(
        vidy_band_half_span(cf.ends_b_out, cf.ref_t, cf.v_ins_b, keep_v_dp) - 3.0,
        vidy_band_half_span(cf.ends_a_out, cf.ref_t, cf.v_ins_a, keep_v_dp)
    );
    const double av_v_sp = std::min(
        vidy_band_half_span(cf.ends_b_in, cf.ref_b, cf.v_ins_b, keep_v_sp),
        vidy_band_half_span(cf.ends_a_in, cf.ref_b, cf.v_ins_a, keep_v_sp) - 2.0
    );
    const int max_v_dp = std::min(
        std::max(1, static_cast<int>(std::floor(0.64 * cf.min_len / pitch))),
        std::max(1, static_cast<int>(std::floor(2.0 * av_v_dp / pitch)))
    );
    const int max_v_sp = std::min(
        std::max(1, static_cast<int>(std::floor(0.80 * cf.min_len / pitch))),
        std::max(1, static_cast<int>(std::floor(2.0 * av_v_sp / pitch)))
    );
    const int max_dp = std::min(11, max_v_dp);
    const int max_sp =
        std::min(cf.min_len >= 10200.0 ? 16 : (cf.min_len >= 9200.0 ? 14 : 13), max_v_sp);
    const size_t sa0 = tables.plate_slots[0][cf.fa_i].size();
    const size_t sa1 = tables.plate_slots[1][cf.fa_i].size();
    const size_t sb0 = tables.plate_slots[0][cf.fb_i].size();
    int n_dp = max_dp;

    for (int try_dp = max_dp; try_dp >= 1; try_dp--) {
        tables.plate_slots[0][cf.fa_i].resize(sa0);
        tables.plate_slots[1][cf.fa_i].resize(sa1);
        tables.plate_slots[0][cf.fb_i].resize(sb0);
        const PinBand dp_band = {
            cf.ends_b_out,
            cf.ref_t,
            cf.v_ins_b,
            cf.l_tab_b,
            cf.l_offs_b,
            true,
            try_dp,
            pitch,
            3.0,
            0.0
        };
        const PinBand ep_band = {
            cf.ends_a_out,
            cf.ref_t,
            cf.v_ins_a,
            cf.l_tab_a,
            cf.l_offs_a,
            true,
            try_dp,
            pitch,
            0.0,
            0.0
        };
        assign_layer_edge(
            tables.edge_polys,
            1,
            cf.fb_i,
            cf.sb,
            vidy_emit_double_pins(
                dp_band,
                fa.centroid,
                fa.normal,
                heights,
                1,
                tables.plate_slots[1][cf.fa_i],
                tables.plate_slots[0][cf.fa_i]
            )
        );
        assign_layer_edge(
            tables.edge_polys,
            1,
            cf.fa_i,
            cf.sa,
            vidy_emit_extend_pins(
                ep_band,
                fb.centroid,
                fb.normal,
                heights[0],
                heights[1],
                tables.plate_slots[0][cf.fb_i]
            )
        );
        n_dp = try_dp;
        const bool ok_a0 =
            new_vidy_slots_valid(guide, tables.plate_slots[0][cf.fa_i], sa0, fa, 0, cf.sa);
        const bool ok_a1 =
            new_vidy_slots_valid(guide, tables.plate_slots[1][cf.fa_i], sa1, fa, 1, cf.sa);
        const bool ok_b0 =
            new_vidy_slots_valid(guide, tables.plate_slots[0][cf.fb_i], sb0, fb, 0, cf.sb);

        if (ok_a0 && ok_a1 && ok_b0)
            break;
    }

    const size_t sb0_sp = tables.plate_slots[0][cf.fb_i].size();
    int n_sp = max_sp;

    for (int try_sp = max_sp; try_sp >= 1; try_sp--) {
        tables.plate_slots[0][cf.fb_i].resize(sb0_sp);
        const PinBand ex_band = {
            cf.ends_b_in, cf.ref_b, cf.v_ins_b, cf.l_tab_b, 0.0, false, try_sp, pitch, 0.0, 0.0
        };
        const PinBand sp_band = {
            cf.ends_a_in, cf.ref_b, cf.v_ins_a, cf.l_tab_a, 0.0, true, try_sp, pitch, 2.0, 0.0
        };
        assign_layer_edge(
            tables.edge_polys, 0, cf.fb_i, cf.sb, vidy_emit_extend_lip(ex_band, false)
        );
        assign_layer_edge(
            tables.edge_polys,
            0,
            cf.fa_i,
            cf.sa,
            vidy_emit_single_pins(
                sp_band,
                fb.centroid,
                fb.normal,
                heights[0],
                heights[1],
                tables.plate_slots[0][cf.fb_i]
            )
        );
        n_sp = try_sp;

        if (new_vidy_slots_valid(guide, tables.plate_slots[0][cf.fb_i], sb0_sp, fb, 0, cf.sb))
            break;
    }
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
    const int raw_dp =
        spec.kind == 2
            ? 3
            : (spec.kind == 6 ? 4 : (spec.kind == 3 ? 6 : (cf.min_len >= 4900.0 ? 5 : 4)));
    const int raw_sp =
        spec.kind == 2
            ? (cf.min_len >= 3950.0 ? 6 : 5)
            : (spec.kind == 6
                   ? (cf.min_len >= 4250.0 ? 7 : (cf.min_len >= 3950.0 ? 6 : 5))
                   : (spec.kind == 3
                          ? (cf.min_len >= 6950.0 ? 12 : 11)
                          : (cf.min_len >= 5150.0 ? 9 : (cf.min_len >= 4820.0 ? 8 : 7))));
    const double dp_fill = spec.kind == 2 ? 0.45 : (spec.kind == 3 ? 0.48 : 0.54);
    const double sp_fill = spec.kind == 2 ? 0.80 : (spec.kind == 6 ? 0.84 : 0.88);
    const Vector nf_a = (cf.ends_a_in[1] - cf.ends_a_in[0]).normalized();
    const Vector nf_b = (cf.ends_b_in[1] - cf.ends_b_in[0]).normalized();
    const double skew_a = std::abs(cf.v_ins_a.dot(nf_a));
    const double skew_b = std::abs(cf.v_ins_b.dot(nf_b));
    const double ep_extra = skew_a * cf.l_tab_a;
    const double dp_extra = ep_extra + skew_b * cf.l_offs_b + 3.0;
    const double sp_extra =
        std::max(skew_b * (2.0 * cf.l_tab_b), skew_a * (2.0 * cf.l_tab_a)) + 3.0;
    const double keep_dp = 1.45 * guide.total_offset;
    const double keep_sp = 0.85 * guide.total_offset;
    const double av_dp = std::min(
        vidy_band_half_span(cf.ends_a_in, cf.ref_b, cf.v_ins_a, keep_dp) - dp_extra,
        vidy_band_half_span(cf.ends_b_in, cf.ref_b, cf.v_ins_b, keep_dp) - ep_extra
    );
    const double av_sp = std::min(
        vidy_band_half_span(cf.ends_a_out, cf.ref_t, cf.v_ins_a, keep_sp),
        vidy_band_half_span(cf.ends_b_out, cf.ref_t, cf.v_ins_b, keep_sp) - sp_extra
    );
    const int max_dp = std::min(
        {raw_dp,
         std::max(1, static_cast<int>(std::floor(dp_fill * cf.min_len / pitch))),
         std::max(1, static_cast<int>(std::floor(2.0 * av_dp / pitch)))}
    );
    const int max_sp = std::min(
        {raw_sp,
         std::max(1, static_cast<int>(std::floor(sp_fill * cf.min_len / pitch))),
         std::max(1, static_cast<int>(std::floor(2.0 * av_sp / pitch)))}
    );
    const size_t sb0 = tables.plate_slots[0][cf.fb_i].size();
    const size_t sb1 = tables.plate_slots[1][cf.fb_i].size();
    const size_t sa1_dp = tables.plate_slots[1][cf.fa_i].size();
    int n_dp = max_dp;

    for (int try_dp = max_dp; try_dp >= 1; try_dp--) {
        tables.plate_slots[0][cf.fb_i].resize(sb0);
        tables.plate_slots[1][cf.fb_i].resize(sb1);
        tables.plate_slots[1][cf.fa_i].resize(sa1_dp);
        const PinBand dp_band = {
            cf.ends_a_in,
            cf.ref_b,
            cf.v_ins_a,
            cf.l_tab_a,
            cf.l_offs_a,
            false,
            try_dp,
            pitch,
            dp_extra,
            0.0
        };
        const PinBand ep_band = {
            cf.ends_b_in,
            cf.ref_b,
            cf.v_ins_b,
            cf.l_tab_b,
            cf.l_offs_b,
            false,
            try_dp,
            pitch,
            ep_extra,
            0.0
        };
        assign_layer_edge(
            tables.edge_polys,
            0,
            cf.fa_i,
            cf.sa,
            vidy_emit_double_pins(
                dp_band,
                fb.centroid,
                fb.normal,
                heights,
                0,
                tables.plate_slots[0][cf.fb_i],
                tables.plate_slots[1][cf.fb_i]
            )
        );
        assign_layer_edge(
            tables.edge_polys,
            0,
            cf.fb_i,
            cf.sb,
            vidy_emit_extend_pins(
                ep_band,
                fa.centroid,
                fa.normal,
                heights[2],
                heights[3],
                tables.plate_slots[1][cf.fa_i]
            )
        );
        n_dp = try_dp;
        const bool ok_b0 =
            new_vidy_slots_valid(guide, tables.plate_slots[0][cf.fb_i], sb0, fb, 0, cf.sb);
        const bool ok_b1 =
            new_vidy_slots_valid(guide, tables.plate_slots[1][cf.fb_i], sb1, fb, 1, cf.sb);
        const bool ok_a1 =
            new_vidy_slots_valid(guide, tables.plate_slots[1][cf.fa_i], sa1_dp, fa, 1, cf.sa);

        if (ok_b0 && ok_b1 && ok_a1)
            break;
    }

    const size_t sa1_sp = tables.plate_slots[1][cf.fa_i].size();
    int n_sp = max_sp;

    for (int try_sp = max_sp; try_sp >= 1; try_sp--) {
        tables.plate_slots[1][cf.fa_i].resize(sa1_sp);
        const PinBand ex_band = {
            cf.ends_a_out, cf.ref_t, cf.v_ins_a, cf.l_tab_a, 0.0, true, try_sp, pitch, 0.0, 0.0
        };
        const PinBand sp_band = {
            cf.ends_b_out,
            cf.ref_t,
            cf.v_ins_b,
            cf.l_tab_b,
            0.0,
            false,
            try_sp,
            pitch,
            sp_extra,
            0.0
        };
        assign_layer_edge(
            tables.edge_polys, 1, cf.fa_i, cf.sa, vidy_emit_extend_lip(ex_band, false)
        );
        assign_layer_edge(
            tables.edge_polys,
            1,
            cf.fb_i,
            cf.sb,
            vidy_emit_single_pins(
                sp_band,
                fa.centroid,
                fa.normal,
                heights[2],
                heights[3],
                tables.plate_slots[1][cf.fa_i]
            )
        );
        n_sp = try_sp;

        if (new_vidy_slots_valid(guide, tables.plate_slots[1][cf.fa_i], sa1_sp, fa, 1, cf.sa))
            break;
    }
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

    cf.sa = vidy_find_shared_side(fa.vertices, shared[0], shared[1]);
    cf.sb = vidy_find_shared_side(fb.vertices, shared[0], shared[1]);
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
    const double hi_out = std::min(cf.ends_a_out[0].distance(cf.ends_a_out[1]), std::max(tb0, tb1));
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
    const Vector perp_a =
        vidy_inplane_outward(fa.centroid, fa.normal, cf.ends_a_in[0], cf.ends_a_in[1]);
    const Vector perp_b =
        vidy_inplane_outward(fb.centroid, fb.normal, cf.ends_b_in[0], cf.ends_b_in[1]);
    Vector situ_a = fa.normal.cross(Vector{0.0, 1.0, 0.0});
    situ_a = situ_a.magnitude() > 1e-9 ? situ_a.normalized() : perp_a;

    if (situ_a.dot(perp_a) < 0.0)
        situ_a = -situ_a;

    const bool is_crown = spec.kind == 3 || spec.kind == 4;
    const Vector hc_b =
        is_crown ? compute_haunch_crown_vector(guide, bay, spec.loc_a, perp_b) : perp_b;
    const bool use_hc = is_crown && std::abs(hc_b.dot(perp_b)) >= 0.38;
    cf.v_ins_a =
        is_crown ? compute_crown_inplane_vector(guide, fa) : (spec.kind == 1 ? situ_a : perp_a);
    cf.v_ins_b = use_hc ? hc_b : (spec.kind == 1 ? Vector{0.0, 0.0, 1.0} : perp_b);
    const double cos_a = std::max(0.25, std::abs(cf.v_ins_a.dot(perp_a)));
    const double cos_b = std::max(0.25, std::abs(cf.v_ins_b.dot(perp_b)));
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
            vidy_plane_corner_snips(apex, ua, ub, *bis, 0.8, 0.15, 520.0);
        snip_a = std::max(snip_a, ps.first);
        snip_b = std::max(snip_b, ps.second);
    }

    const double half_total = 0.5 * static_cast<double>(guide.bays) * guide.bay_length;
    const double y_mid = -half_total + (static_cast<double>(facet.bay) + 0.5) * guide.bay_length;
    const size_t loc = facet.local_index;

    if ((loc == 4 || loc == 8 || loc == 9 || loc == 13) &&
        std::abs(guide.vertices()[vertex_id][1] - y_mid) < 1.0) {
        const Plane mid_plane = Plane::from_point_normal(
            {0.0, y_mid, 0.0}, {0.0, (loc == 4 || loc == 8) ? 1.0 : -1.0, 0.0}
        );
        const std::pair<double, double> mp =
            vidy_plane_corner_snips(apex, ua, ub, mid_plane, 0.8, 0.15, 520.0);
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
    const double y_min = outer_haunch ? (y_bay0 + guide.plate_thickness + 1.2)
                                      : (facet.bay > 0 ? y_bay0 + 0.6 : -1e9);
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
    std::vector<std::pair<double, double>> snips_b(m);
    std::vector<std::pair<double, double>> snips_t(m);

    for (size_t k = 0; k < m; k++) {
        snips_b[k] = compute_corner_snips(guide, facet, k, h_bot, factor_bot);
        snips_t[k] = compute_corner_snips(guide, facet, k, h_top, factor_top);
    }

    return vidy_stitch_layer_rings(
        facet.miter_rings,
        facet.normal,
        h_bot,
        h_top,
        snips_b,
        snips_t,
        prof_bot,
        prof_top,
        2.5,
        0.90
    );
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
    const std::optional<Plane> start_bis = find_arch_knee_bisector(guide, facet, facet.vertices[k]);
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

    // assembly: per bay its two walls and its roof, each plate by plate, the roof lowered on the walls, the bays one after another
    assemblies = {compute_assembly()};
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
    const std::array<double, 4> factors = {corner_scale, corner_scale, corner_scale, corner_scale};

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
                vidy_attach_plate_features(
                    *plate,
                    facet.normal,
                    jointed.first,
                    jointed.second,
                    tables.plate_slots[layer][f]
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

AssemblyModule VidyPavilion::compute_assembly() const {

    // bay after bay, its two walls stand first, then its roof
    std::vector<AssemblyModule> rows;
    for (size_t b = 0; b < guide.bays; b++) {

        // walls: the two pleats' room-side plates, the boards that close their cavities, their outer plates
        for (const size_t first : {size_t{0}, size_t{2}}) {
            AssemblyModule wall = facets_module(first == 0 ? fmt::format("wall_left_{}", b) : fmt::format("wall_right_{}", b), {14 * b + first, 14 * b + first + 1});
            wall.ways = {Vector(0.0, -1.0, 0.0)};
            rows.push_back(wall);
        }

        // roof: the haunch pairs, the crowns, then the eaves
        std::vector<size_t> roof;
        for (const size_t facet : {5, 10, 7, 12, 6, 11, 4, 9, 8, 13})
            roof.push_back(14 * b + facet);
        AssemblyModule whole_roof = facets_module(fmt::format("roof_{}", b), roof);
        whole_roof.ways = {Vector(0.0, 0.0, -1.0)};
        rows.push_back(whole_roof);
    }

    AssemblyModule whole = AssemblyModule::module(name, rows);
    whole.one_by_one = true;
    whole.play = 0.9; // a plate's corner grazes the next roof's at one seam of the eleven, 46 mm3 of it

    return whole;
}

AssemblyModule VidyPavilion::facets_module(const std::string& module, const std::vector<size_t>& facets) const {

    std::vector<AssemblyModule> parts;
    for (const size_t facet : facets)
        parts.push_back(AssemblyModule::part(_inner_plates[facet], {_insertion_vectors[0][facet]}));
    for (const size_t facet : facets)
        for (const std::shared_ptr<FacePlanarElement>& board : _facet_border_plates[facet])
            parts.push_back(AssemblyModule::part(board));
    for (const size_t facet : facets)
        parts.push_back(AssemblyModule::part(_outer_plates[facet], {_insertion_vectors[1][facet]}));

    return AssemblyModule::module(module, parts);
}

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
