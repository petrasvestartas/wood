#include "pch.h"
#include "src/templates/cavity_border.h"
#include "src/templates/vidy/origami_shell.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Internal joinery structures
// ═══════════════════════════════════════════════════════════════════════════

/// Paired bottom and top four-corner hole loops of one rectangular through-mortise slot.
struct OrigamiSlot {
    std::array<Point, 4> bottom; // Four corners on the receiving plate's bottom plane.
    std::array<Point, 4> top; // Matching four corners on the receiving plate's top plane.
};

/// Swept four-point footprint of one cavity-crossing Double_Pin inside a receiving facet.
struct PinFootprint {
    std::array<Point, 4> quad; // Base-to-tip swept corners for 2D SAT in the facet plane.
};

/// Edge frame, insertion vector and layout parameters of one tenon or rabbet band.
struct OrigamiBand {
    std::array<Point, 4> ends; // Bottom and top miter endpoints {b0, b1, t0, t1}.
    Point ref_pt; // Shared seam midpoint projected onto both edge lines.
    Vector v_ins; // Unit in-plane insertion direction across the crease.
    double le_tab = 0.0; // Single-layer bevel step length along v_ins.
    double le_offs = 0.0; // Cavity-spanning bevel offset length along v_ins.
    bool step_bot = false; // True when the bottom face steps back by le_tab.
    int count = 0; // Pin count (or mating pin count on rabbet shelves).
    double pitch = 450.0; // Centre-to-centre pin spacing along the crease.
    double pocket_extra = 0.0; // Extra pocket half-margin beyond 0.5 * count * pitch.
};

/// Resolved 3D miter endpoints, insertion vectors and bevel stepbacks of one interior crease.
struct OrigamiCreaseFrame {
    std::array<Point, 4> ends_rec_in; // Receiver inner bottom and top miter endpoints.
    std::array<Point, 4> ends_rec_out; // Receiver outer bottom and top miter endpoints.
    std::array<Point, 4> ends_mal_in; // Male inner bottom and top miter endpoints.
    std::array<Point, 4> ends_mal_out; // Male outer bottom and top miter endpoints.
    Point ref_b; // Shared inner bottom reference midpoint.
    Point ref_t; // Shared outer bottom reference midpoint.
    Vector v_ins_rec; // Outward perpendicular in-plane direction on receiver facet.
    Vector v_ins_mal; // Single rigid-body insertion direction of male facet.
    double min_len = 0.0; // Shortest of the four miter edge segments.
    double l_tab_rec = 0.0; // Receiver single-layer bevel stepback along v_ins_rec.
    double l_offs_rec = 0.0; // Receiver cavity-spanning shelf length along v_ins_rec.
    double l_tab_mal = 0.0; // Male single-layer bevel stepback along v_ins_mal.
    double l_offs_mal = 0.0; // Male cavity-spanning offset length along v_ins_mal.
    size_t rec_f = 0; // Receiver facet index (assembled earlier).
    size_t rec_s = 0; // Directed side index on receiver facet.
    size_t mal_f = 0; // Male facet index (assembled later).
    size_t mal_s = 0; // Directed side index on male facet.
    bool positive_fold = true; // True on convex fold, false on valley fold.
};

/// Per-ring directed edge profiles, per-layer mortises and pin counters across all facets.
struct OrigamiJoineryTables {
    std::array<std::vector<std::vector<std::vector<Point>>>, 4> edge_polys; // [ring][facet][side].
    std::array<std::vector<std::vector<OrigamiSlot>>, 2> plate_slots; // [layer][facet].
    std::vector<std::vector<PinFootprint>> cavity_pins; // [facet] Double_Pin 3D footprints.
    size_t seams = 0; // Jointed crease seam count.
    size_t double_pins = 0; // Stepped Double_Pin count.
    size_t single_pins = 0; // Single_Pin count.
};

// ═══════════════════════════════════════════════════════════════════════════
// Polygon orientation and frame helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Centroid, unit normal and area magnitude of a polygon vertex index loop.
std::tuple<Point, Vector, double>
compute_polygon_frame(const std::vector<Point>& pts, const std::vector<size_t>& loop) {

    std::vector<Point> ring;
    ring.reserve(loop.size());

    for (const size_t vid : loop)
        ring.push_back(pts[vid]);

    const Vector n_raw = compute_newell(ring);
    const double len = n_raw.magnitude();
    const Vector unit_n = len > 1e-12 ? (n_raw / len) : Vector{0.0, 0.0, 1.0};

    return {Point::centroid(ring), unit_n, 0.5 * len};
}

/// Consistently orients face loops across shared edges and flips each component outward from base.
void orient_face_loops_outward(
    const std::vector<Point>& pts,
    std::vector<std::vector<size_t>>& faces,
    double z_min,
    double z_max,
    const Point& bbox_mid
) {

    std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, bool>>> adj;

    for (size_t f = 0; f < faces.size(); f++) {
        const size_t m = faces[f].size();

        for (size_t k = 0; k < m; k++) {
            const size_t u = faces[f][k];
            const size_t v = faces[f][(k + 1) % m];
            adj[std::minmax(u, v)].push_back({f, u < v});
        }
    }

    std::vector<bool> visited(faces.size(), false);
    const Point ref_pt = {bbox_mid[0], bbox_mid[1], z_min - 0.35 * (z_max - z_min + 200.0)};

    for (size_t seed = 0; seed < faces.size(); seed++) {
        if (visited[seed])
            continue;

        std::vector<size_t> comp = {seed};
        std::vector<bool> flipped(faces.size(), false);
        visited[seed] = true;

        for (size_t qi = 0; qi < comp.size(); qi++) {
            const size_t cur = comp[qi];
            const size_t m = faces[cur].size();

            for (size_t k = 0; k < m; k++) {
                const size_t u = faces[cur][k];
                const size_t v = faces[cur][(k + 1) % m];
                const bool dir_cur = (u < v) != flipped[cur];

                for (const std::pair<size_t, bool>& nb : adj[std::minmax(u, v)]) {
                    if (visited[nb.first])
                        continue;

                    visited[nb.first] = true;
                    flipped[nb.first] = (nb.second == dir_cur);
                    comp.push_back(nb.first);
                }
            }
        }

        for (const size_t f : comp)
            if (flipped[f])
                std::reverse(faces[f].begin(), faces[f].end());

        double score = 0.0;

        for (const size_t f : comp) {
            const std::tuple<Point, Vector, double> fr = compute_polygon_frame(pts, faces[f]);
            score += (std::get<0>(fr) - ref_pt).dot(std::get<1>(fr)) * std::get<2>(fr);
        }

        if (score < 0.0)
            for (const size_t f : comp)
                std::reverse(faces[f].begin(), faces[f].end());
    }
}

/// Outward-facing side miter or boundary datum plane for directed edge (u -> v) of facet_idx.
Plane compute_origami_side_plane(
    const std::vector<Point>& vertices,
    const std::vector<OrigamiFacet>& facets,
    size_t facet_idx,
    const OrigamiEdge& edge,
    size_t u,
    size_t v,
    double ground_z,
    const std::array<double, 4>& plan_bounds
) {

    const Point& pu = vertices[u];
    const Point& pv = vertices[v];
    const Point mid = Point::mid_point(pu, pv);
    const OrigamiFacet& facet = facets[facet_idx];
    const Vector edge_dir = (pv - pu).normalized();
    Vector natural_out = edge_dir.cross(facet.normal).normalized();

    if ((mid - facet.centroid).dot(natural_out) < 0.0)
        natural_out = -natural_out;

    Vector side_normal = natural_out;

    if (!edge.is_boundary) {
        const int other_idx =
            edge.facet_a == static_cast<int>(facet_idx) ? edge.facet_b : edge.facet_a;
        const Vector sum = facet.normal + facets[static_cast<size_t>(other_idx)].normal;
        const Vector bisector = sum.magnitude() < 1e-9 ? facet.normal : sum.normalized();
        side_normal = edge_dir.cross(bisector).normalized();
    } else if (edge.on_ground) {
        side_normal = {0.0, 0.0, mid[2] <= ground_z + 2.0 ? -1.0 : 1.0};
    } else {
        const std::array<std::pair<double, Vector>, 4> env = {{
            {plan_bounds[0], {-1.0, 0.0, 0.0}},
            {plan_bounds[1], {1.0, 0.0, 0.0}},
            {plan_bounds[2], {0.0, -1.0, 0.0}},
            {plan_bounds[3], {0.0, 1.0, 0.0}},
        }};

        for (size_t a = 0; a < 4; a++) {
            const size_t coord = a < 2 ? 0 : 1;
            const double lim = env[a].first;
            const Vector& ax = env[a].second;

            if (std::abs(pu[coord] - lim) < 1.5 && std::abs(pv[coord] - lim) < 1.5 &&
                std::abs(facet.normal.dot(ax)) < 0.80 && natural_out.dot(ax) > 0.35) {
                side_normal = ax;
                break;
            }
        }
    }

    if ((mid - facet.centroid).dot(side_normal) < 0.0)
        side_normal = -side_normal;

    return Plane::from_point_normal(mid, side_normal);
}

/// Outward unit vector in facet's plane perpendicular to directed edge (p_start -> p_end).
Vector
compute_inplane_outward(const OrigamiFacet& facet, const Point& p_start, const Point& p_end) {

    Vector out = (p_end - p_start).normalized().cross(facet.normal).normalized();

    if ((Point::mid_point(p_start, p_end) - facet.centroid).dot(out) < 0.0)
        out = -out;

    return out;
}

/// Directed side index on facet sharing undirected vertices (u, v).
size_t find_shared_side(const OrigamiFacet& facet, size_t u, size_t v) {

    const size_t m = facet.vertices.size();

    for (size_t k = 0; k < m; k++) {
        const size_t a = facet.vertices[k];
        const size_t b = facet.vertices[(k + 1) % m];

        if ((a == u && b == v) || (a == v && b == u))
            return k;
    }

    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Selective non-uniform corner chamfers and border helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Incoming and outgoing edge snips separating apex from limit_plane by clearance.
std::pair<double, double> compute_plane_corner_snips(
    const Point& apex,
    const Vector& ua,
    const Vector& ub,
    const Plane& limit_plane,
    double clearance,
    double max_snip
) {

    const double raw = limit_plane.signed_distance(apex);
    const double ra = ua.dot(limit_plane.z_axis());
    const double rb = ub.dot(limit_plane.z_axis());

    if (raw <= 0.02 || std::max(ra, rb) < 0.02)
        return {0.0, 0.0};

    const double sd = raw + clearance;

    return {
        ra >= 0.008 ? std::min(max_snip, sd / ra) : 0.0,
        rb >= 0.008 ? std::min(max_snip, sd / rb) : 0.0
    };
}

/// Outward separation plane through corner_vertex oriented from facet's sector toward other's.
std::optional<Plane> compute_origami_separation_plane(
    const Point& corner_vertex, const OrigamiFacet& facet, const OrigamiFacet& other
) {

    const Vector u_self = (facet.centroid - corner_vertex).normalized();
    const Vector u_other = (other.centroid - corner_vertex).normalized();
    const Vector sector_dir = u_other - u_self;
    Vector diff = other.normal - facet.normal;

    if (diff.magnitude() < 1e-4) {
        if (sector_dir.magnitude() < 1e-6)
            return std::nullopt;

        return Plane::from_point_normal(corner_vertex, sector_dir.normalized());
    }

    diff = diff.normalized();

    if (diff.dot(sector_dir) < 0.0)
        diff = -diff;

    return Plane::from_point_normal(corner_vertex, diff);
}

/// Non-adjacent boundary-bearing facet sharing vertex_id on a common boundary plane, if any.
std::optional<size_t> find_boundary_knee_mate(
    const std::vector<OrigamiFacet>& facets,
    const std::vector<OrigamiEdge>& edges,
    size_t facet_idx,
    size_t side_idx,
    size_t vertex_id
) {

    const OrigamiFacet& facet = facets[facet_idx];
    const Plane& bp = facet.side_planes[side_idx];

    for (size_t f = 0; f < facets.size(); f++) {
        if (f == facet_idx)
            continue;

        const OrigamiFacet& other = facets[f];
        size_t shared = 0;

        for (const size_t ov : other.vertices)
            for (const size_t fv : facet.vertices)
                if (ov == fv)
                    shared++;

        if (shared != 1)
            continue;

        const size_t om = other.vertices.size();

        for (size_t ok = 0; ok < om; ok++) {
            const OrigamiEdge& oe = edges[static_cast<size_t>(other.edge_ids[ok])];

            if (!oe.is_boundary)
                continue;

            if (other.vertices[ok] != vertex_id && other.vertices[(ok + 1) % om] != vertex_id)
                continue;

            if (bp.z_axis().dot(other.side_planes[ok].z_axis()) > 0.96)
                return f;
        }
    }

    return std::nullopt;
}

/// Snips a point-contact facet corner clear of boundary datum planes and bridging border boards.
std::pair<double, double> compute_boundary_point_snips(
    const std::vector<Point>& vertices,
    const std::vector<OrigamiFacet>& facets,
    const std::vector<OrigamiEdge>& edges,
    const OrigamiFacet& facet,
    size_t vertex_id,
    const Point& apex,
    const Point& partner_apex,
    const Vector& ua,
    const Vector& ub,
    double plate_thickness,
    double total_offset,
    bool with_borders
) {

    double snip_a = 0.0;
    double snip_b = 0.0;
    const double max_s = 2.4 * total_offset;

    for (size_t f = 0; f < facets.size(); f++) {
        if (f == facet.index)
            continue;

        const OrigamiFacet& other = facets[f];
        const size_t om = other.vertices.size();

        for (size_t ok = 0; ok < om; ok++) {
            const OrigamiEdge& oe = edges[static_cast<size_t>(other.edge_ids[ok])];

            if (!oe.is_boundary)
                continue;

            if (other.vertices[ok] != vertex_id && other.vertices[(ok + 1) % om] != vertex_id)
                continue;

            const Plane& bp = other.side_planes[ok];
            bool same_datum_on_self = false;

            for (size_t sk = 0; sk < facet.vertices.size(); sk++)
                if (edges[static_cast<size_t>(facet.edge_ids[sk])].is_boundary &&
                    bp.z_axis().dot(facet.side_planes[sk].z_axis()) > 0.96)
                    same_datum_on_self = true;

            if (same_datum_on_self)
                continue;

            const std::pair<double, double> env_s =
                compute_plane_corner_snips(apex, ua, ub, bp, 0.8, max_s);
            snip_a = std::max(snip_a, env_s.first);
            snip_b = std::max(snip_b, env_s.second);

            if (!with_borders || !find_boundary_knee_mate(facets, edges, f, ok, vertex_id))
                continue;

            const double oh1 = (apex - vertices[vertex_id]).dot(other.normal);
            const double oh2 = (partner_apex - vertices[vertex_id]).dot(other.normal);

            if (std::max(oh1, oh2) >= plate_thickness - 0.2 &&
                std::min(oh1, oh2) <= total_offset - plate_thickness + 0.2) {
                const Plane inner_bp = bp.translate_by_normal(-(plate_thickness + 1.5));
                const std::pair<double, double> brd_s =
                    compute_plane_corner_snips(apex, ua, ub, inner_bp, 0.0, max_s);
                snip_a = std::max(snip_a, brd_s.first);
                snip_b = std::max(snip_b, brd_s.second);
            }
        }
    }

    return {snip_a, snip_b};
}

/// Incoming and outgoing corner snip distances (0 when no chamfer is needed at this corner).
std::pair<double, double> compute_origami_corner_snips(
    const std::vector<Point>& vertices,
    const std::vector<size_t>& valences,
    const std::vector<OrigamiFacet>& facets,
    const std::vector<OrigamiEdge>& edges,
    const OrigamiFacet& facet,
    const Vector& v_ins,
    size_t k,
    size_t hi,
    size_t h_partner,
    double plate_thickness,
    double total_offset,
    bool with_borders,
    double factor
) {

    if (factor <= 0.0)
        return {0.0, 0.0};

    const size_t m = facet.vertices.size();
    const size_t vid = facet.vertices[k];
    const Point& apex = facet.miter_rings[hi][k];
    const Vector ua = (apex - facet.miter_rings[hi][(k + m - 1) % m]).normalized();
    const Vector ub = (apex - facet.miter_rings[hi][(k + 1) % m]).normalized();
    const OrigamiEdge& prev_edge = edges[static_cast<size_t>(facet.edge_ids[(k + m - 1) % m])];
    const OrigamiEdge& curr_edge = edges[static_cast<size_t>(facet.edge_ids[k])];

    if (prev_edge.is_boundary && curr_edge.is_boundary && valences[vid] == 1)
        return {0.0, 0.0};

    double snip_a = 0.0;
    double snip_b = 0.0;
    const double max_s = 2.4 * total_offset;

    for (const OrigamiFacet& other : facets) {
        if (other.index == facet.index)
            continue;

        size_t shared = 0;
        bool has_v = false;

        for (const size_t ov : other.vertices) {
            if (ov == vid)
                has_v = true;

            for (const size_t fv : facet.vertices)
                if (ov == fv)
                    shared++;
        }

        if (!has_v || shared != 1)
            continue;

        const std::optional<Plane> bis =
            compute_origami_separation_plane(vertices[vid], facet, other);

        if (!bis)
            continue;

        const std::pair<double, double> ps =
            compute_plane_corner_snips(apex, ua, ub, *bis, 1.5, max_s);
        snip_a = std::max(snip_a, ps.first * factor);
        snip_b = std::max(snip_b, ps.second * factor);

        if (other.assembly_step < facet.assembly_step && (-v_ins).dot(bis->z_axis()) > 0.01) {
            const Vector to_other = (other.centroid - vertices[vid]).normalized();

            for (const double d : {8.0, 25.0, 75.0, 125.0}) {
                const Point apex_d = apex - v_ins * d;

                if ((apex_d - vertices[vid]).dot(to_other) < -0.75 * total_offset)
                    continue;

                const std::pair<double, double> sd_s =
                    compute_plane_corner_snips(apex_d, ua, ub, *bis, 1.8, max_s);
                snip_a = std::max(snip_a, sd_s.first * factor);
                snip_b = std::max(snip_b, sd_s.second * factor);
            }
        }
    }

    if (!prev_edge.is_boundary && !curr_edge.is_boundary && valences[vid] >= 6) {
        const double ang_deg = std::acos(std::clamp(ua.dot(ub), -1.0, 1.0)) * Tolerance::TO_DEGREES;
        const double scale = total_offset / 300.0;
        const double base_c = (180.0 - ang_deg) * (hi == 0 ? 0.16 : 0.45) * scale * factor;
        snip_a = std::max(snip_a, base_c);
        snip_b = std::max(snip_b, base_c);
    }

    const Point& partner_apex = facet.miter_rings[h_partner][k];
    const std::pair<double, double> bp_s = compute_boundary_point_snips(
        vertices,
        facets,
        edges,
        facet,
        vid,
        apex,
        partner_apex,
        ua,
        ub,
        plate_thickness,
        total_offset,
        with_borders
    );

    return {std::max(snip_a, bp_s.first * factor), std::max(snip_b, bp_s.second * factor)};
}

/// Paired bottom and top closed plate rings with sharp unchamfered corners where snips are 0.
std::pair<Polyline, Polyline> compute_origami_layer_rings(
    const std::vector<Point>& vertices,
    const std::vector<size_t>& valences,
    const std::vector<OrigamiFacet>& facets,
    const std::vector<OrigamiEdge>& edges,
    const OrigamiFacet& facet,
    const Vector& v_ins,
    size_t h_bot,
    size_t h_top,
    double plate_thickness,
    double total_offset,
    bool with_borders,
    double factor,
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
        const std::pair<double, double> sb = compute_origami_corner_snips(
            vertices,
            valences,
            facets,
            edges,
            facet,
            v_ins,
            k,
            h_bot,
            h_top,
            plate_thickness,
            total_offset,
            with_borders,
            factor
        );
        const std::pair<double, double> st = compute_origami_corner_snips(
            vertices,
            valences,
            facets,
            edges,
            facet,
            v_ins,
            k,
            h_top,
            h_bot,
            plate_thickness,
            total_offset,
            with_borders,
            factor
        );

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
                2.0, std::min(0.35 * apex_b.distance(raw_prev_b), 0.88 * apex_b.distance(prev_b))
            );
            const double lim_b2 = std::max(
                2.0, std::min(0.35 * apex_b.distance(raw_next_b), 0.88 * apex_b.distance(next_b))
            );
            const double lim_t1 = std::max(
                2.0, std::min(0.35 * apex_t.distance(raw_prev_t), 0.88 * apex_t.distance(prev_t))
            );
            const double lim_t2 = std::max(
                2.0, std::min(0.35 * apex_t.distance(raw_next_t), 0.88 * apex_t.distance(next_t))
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

// ═══════════════════════════════════════════════════════════════════════════
// Tenon profiles, slot clearance verification and seam builder
// ═══════════════════════════════════════════════════════════════════════════

/// Reference point on top_start..top_end aligned with m_b0 along a plane containing v_ins.
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

    return Intersection::line_plane(
               top_line, Plane::from_point_normal(m_b0, w_p.normalized()), false
    )
        .value_or(top_line.closest_point(m_b0, false).second);
}

/// Half-span offset from the band midpoint to the first pin centre.
double compute_first_pin_offset(int count, double pitch) {

    if (count % 2 == 0)
        return static_cast<double>(count / 2 - 1) * pitch + 0.5 * pitch;

    return static_cast<double>((count - 1) / 2) * pitch;
}

/// Available longitudinal half-span from ref_pt/m_t0 to the nearest corner minus corner_keep.
double band_available_half_span(
    const std::array<Point, 4>& ends, const Point& ref_pt, const Vector& v_ins, double corner_keep
) {

    const Vector nf_b = (ends[1] - ends[0]).normalized();
    const Vector nf_t = (ends[3] - ends[2]).normalized();
    const double len_b = ends[0].distance(ends[1]);
    const double len_t = ends[2].distance(ends[3]);
    const Point m_b0 = Line::from_points(ends[0], ends[1]).closest_point(ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, ends[2], ends[3], nf_b, v_ins);
    const double sb = (m_b0 - ends[0]).dot(nf_b);
    const double st = (m_t0 - ends[2]).dot(nf_t);

    return std::min({sb, len_b - sb, st, len_t - st}) - corner_keep;
}

/// Pitch-scaled twelve-point stepped Double_Pin contour and its base and tip arris pairs.
std::vector<Point> compute_scaled_double_pin(
    const Point& pm,
    const Vector& nf,
    const Vector& v_ins,
    double le_tab,
    double le_offs,
    double scale,
    std::pair<Point, Point>& out_base,
    std::pair<Point, Point>& out_tip
) {

    const Point mi1 = pm - nf * (135.0 * scale);
    const Point pl1 = pm + nf * (135.0 * scale);
    const Point mi2 = mi1 + v_ins * (0.30 * le_offs);
    const Point pl2 = pl1 + v_ins * (0.30 * le_offs);
    const Point mi3 = (pm - nf * (125.0 * scale)) + v_ins * le_offs;
    const Point pl3 = (pm + nf * (125.0 * scale)) + v_ins * le_offs;
    const Point mi4 = mi3 + nf * (35.0 * scale);
    const Point pl4 = pl3 - nf * (35.0 * scale);
    const Point mi5 = mi4 + v_ins * (1.20 * le_tab);
    const Point pl5 = pl4 + v_ins * (1.20 * le_tab);
    const Point mi6 = (mi4 + v_ins * (2.00 * le_tab)) + nf * (20.0 * scale);
    const Point pl6 = (pl4 + v_ins * (2.00 * le_tab)) - nf * (20.0 * scale);
    out_base = {mi1, pl1};
    out_tip = {mi4, pl4};

    return {mi1, mi2, mi3, mi4, mi5, mi6, pl6, pl5, pl4, pl3, pl2, pl1};
}

/// Pitch-scaled six-point chamfered Single_Pin contour and its shaft arris pair.
std::vector<Point> compute_scaled_single_pin(
    const Point& pm,
    const Vector& nf,
    const Vector& v_ins,
    double le_tab,
    double scale,
    std::pair<Point, Point>& out_shaft
) {

    const Point mi1 = pm - nf * (90.0 * scale);
    const Point pl1 = pm + nf * (90.0 * scale);
    const Point mi2 = mi1 + v_ins * (1.20 * le_tab);
    const Point pl2 = pl1 + v_ins * (1.20 * le_tab);
    const Point mi3 = (mi1 + v_ins * (2.00 * le_tab)) + nf * (20.0 * scale);
    const Point pl3 = (pl1 + v_ins * (2.00 * le_tab)) - nf * (20.0 * scale);
    out_shaft = {mi1, pl1};

    return {mi1, mi2, mi3, pl3, pl2, pl1};
}

/// Projects a four-arris tenon shaft along v_ins onto fem_facet with 0.5 mm corner clearance.
OrigamiSlot project_origami_slot(
    const std::pair<Point, Point>& shaft_bot,
    const std::pair<Point, Point>& shaft_top,
    const Vector& v_ins,
    const OrigamiFacet& fem_facet,
    double h_bot,
    double h_top
) {

    const Plane p_bot =
        Plane::from_point_normal(fem_facet.centroid + fem_facet.normal * h_bot, fem_facet.normal);
    const Plane p_top =
        Plane::from_point_normal(fem_facet.centroid + fem_facet.normal * h_top, fem_facet.normal);
    const std::array<Point, 4> arris = {
        shaft_bot.first, shaft_bot.second, shaft_top.second, shaft_top.first
    };
    OrigamiSlot slot;

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

    return slot;
}

/// Minimum clearance of pt inside convex/snipped planar ring (negative when outside).
double point_ring_clearance(const Point& pt, const Polyline& closed_ring, const Vector& normal) {

    const size_t n = closed_ring.point_count();

    if (n < 4)
        return -1e9;

    const Vector ring_n = compute_newell(closed_ring.get_points());
    const double orient = ring_n.dot(normal) >= 0.0 ? 1.0 : -1.0;
    double min_dist = 1e9;

    for (size_t i = 0; i + 1 < n; i++) {
        const Point& a = closed_ring[i];
        const Point& b = closed_ring[i + 1];
        const Vector edge = b - a;

        if (edge.magnitude() < 1e-6)
            continue;

        const Vector inward = (normal.cross(edge.normalized()) * orient).normalized();
        const double signed_in = (pt - a).dot(inward);

        if (signed_in < 0.0)
            return signed_in;

        const double seg_d = Line::from_points(a, b).closest_point(pt, true).second.distance(pt);
        min_dist = std::min(min_dist, seg_d);
    }

    return min_dist;
}

/// True when two 4-corner quads projected onto normal's plane come closer than min_gap (2D SAT).
bool quads_closer_than_2d(
    const std::array<Point, 4>& qa,
    const std::array<Point, 4>& qb,
    const Vector& normal,
    double min_gap
) {

    const Plane frame = Plane::from_point_normal(qa[0], normal);
    const Vector ex = frame.x_axis();
    const Vector ey = frame.y_axis();
    std::array<std::pair<double, double>, 4> a2;
    std::array<std::pair<double, double>, 4> b2;

    for (size_t i = 0; i < 4; i++) {
        const Vector da = qa[i] - qa[0];
        const Vector db = qb[i] - qa[0];
        a2[i] = {da.dot(ex), da.dot(ey)};
        b2[i] = {db.dot(ex), db.dot(ey)};
    }

    for (int which = 0; which < 2; which++) {
        const std::array<std::pair<double, double>, 4>& poly = which == 0 ? a2 : b2;

        for (size_t i = 0; i < 4; i++) {
            const double dx = poly[(i + 1) % 4].first - poly[i].first;
            const double dy = poly[(i + 1) % 4].second - poly[i].second;
            const double len = std::hypot(dx, dy);

            if (len < 1e-9)
                continue;

            const double nx = -dy / len;
            const double ny = dx / len;
            double min_a = 1e18;
            double max_a = -1e18;
            double min_b = 1e18;
            double max_b = -1e18;

            for (size_t c = 0; c < 4; c++) {
                const double pa = a2[c].first * nx + a2[c].second * ny;
                const double pb = b2[c].first * nx + b2[c].second * ny;
                min_a = std::min(min_a, pa);
                max_a = std::max(max_a, pa);
                min_b = std::min(min_b, pb);
                max_b = std::max(max_b, pb);
            }

            if (max_a + min_gap <= min_b || max_b + min_gap <= min_a)
                return false;
        }
    }

    return true;
}

/// True when pt clears all non-crease sides of facet (including pocket stepback on interior sides).
bool origami_pt_clears_non_crease_sides(
    const std::vector<OrigamiEdge>& edges,
    const Point& pt,
    const OrigamiFacet& facet,
    size_t ring_idx,
    size_t crease_side,
    bool skip_crease,
    double plate_thickness
) {

    const std::vector<Point>& ring = facet.miter_rings[ring_idx];
    const size_t m = ring.size();

    for (size_t k = 0; k < m; k++) {
        if (skip_crease && k == crease_side)
            continue;

        const Point& a = ring[k];
        const Point& b = ring[(k + 1) % m];
        const Vector edge = b - a;

        if (edge.magnitude() < 1e-6)
            continue;

        Vector inward = facet.normal.cross(edge.normalized()).normalized();

        if ((facet.centroid - a).dot(inward) < 0.0)
            inward = -inward;

        const OrigamiEdge& ed = edges[static_cast<size_t>(facet.edge_ids[k])];
        const double pocket =
            ed.is_boundary ? 0.0
                           : (2.0 * plate_thickness / std::max(0.25, std::sin(ed.dihedral_rad)));
        const double req = pocket + 0.65 * plate_thickness;

        if ((pt - a).dot(inward) < req)
            return false;
    }

    return true;
}

/// True when pt stays >= min_clear inside every segment of closed_ring except crease_a..crease_b.
bool point_clears_non_crease_stock(
    const Point& pt,
    const Polyline& closed_ring,
    const Point& crease_a,
    const Point& crease_b,
    const Vector& normal,
    double min_clear
) {

    const size_t n = closed_ring.point_count();

    if (n < 4)
        return false;

    const Line crease_line = Line::from_points(crease_a, crease_b);
    const Vector ring_n = compute_newell(closed_ring.get_points());
    const double orient = ring_n.dot(normal) >= 0.0 ? 1.0 : -1.0;

    for (size_t i = 0; i + 1 < n; i++) {
        const Point& a = closed_ring[i];
        const Point& b = closed_ring[i + 1];
        const Vector edge = b - a;

        if (edge.magnitude() < 1e-6)
            continue;

        if (crease_line.closest_point(a, false).second.distance(a) < 1.5 &&
            crease_line.closest_point(b, false).second.distance(b) < 1.5)
            continue;

        const Vector inward = (normal.cross(edge.normalized()) * orient).normalized();

        if ((pt - a).dot(inward) < min_clear)
            return false;
    }

    return true;
}

/// True when slot clears all required edges, stock chamfers and prior/local mortise slots.
bool slot_clears_plate(
    const std::vector<OrigamiEdge>& edges,
    const OrigamiSlot& slot,
    const OrigamiFacet& facet,
    size_t layer,
    size_t crease_side,
    bool has_extend_tab,
    const Polyline& bot_stock,
    const Polyline& top_stock,
    const std::vector<OrigamiSlot>& existing_slots,
    const std::vector<OrigamiSlot>& local_slots,
    double plate_thickness,
    double min_edge_clear
) {

    const size_t rb = 2 * layer;
    const size_t rt = 2 * layer + 1;
    const size_t m = facet.miter_rings[rb].size();
    const Point& cb0 = facet.miter_rings[rb][crease_side];
    const Point& cb1 = facet.miter_rings[rb][(crease_side + 1) % m];
    const Point& ct0 = facet.miter_rings[rt][crease_side];
    const Point& ct1 = facet.miter_rings[rt][(crease_side + 1) % m];

    for (size_t c = 0; c < 4; c++) {
        if (!origami_pt_clears_non_crease_sides(
                edges, slot.bottom[c], facet, rb, crease_side, has_extend_tab, plate_thickness
            ) ||
            !origami_pt_clears_non_crease_sides(
                edges, slot.top[c], facet, rt, crease_side, has_extend_tab, plate_thickness
            ))
            return false;

        if (!point_clears_non_crease_stock(
                slot.bottom[c], bot_stock, cb0, cb1, facet.normal, min_edge_clear
            ) ||
            !point_clears_non_crease_stock(
                slot.top[c], top_stock, ct0, ct1, facet.normal, min_edge_clear
            ))
            return false;

        if (!has_extend_tab &&
            (point_ring_clearance(slot.bottom[c], bot_stock, facet.normal) < min_edge_clear ||
             point_ring_clearance(slot.top[c], top_stock, facet.normal) < min_edge_clear))
            return false;
    }

    for (const OrigamiSlot& prev : existing_slots)
        if (quads_closer_than_2d(slot.bottom, prev.bottom, facet.normal, 18.0) ||
            quads_closer_than_2d(slot.top, prev.top, facet.normal, 18.0))
            return false;

    for (const OrigamiSlot& prev : local_slots)
        if (quads_closer_than_2d(slot.bottom, prev.bottom, facet.normal, 14.0) ||
            quads_closer_than_2d(slot.top, prev.top, facet.normal, 14.0))
            return false;

    return true;
}

/// True when every corner of slot stays >= 10 mm inside rec_band's longitudinal tab span.
bool slot_within_extend_span(
    const OrigamiSlot& slot,
    const std::array<Point, 4>& rec_ends,
    const Point& ref_pt,
    const Vector& v_ins_rec,
    int cnt,
    double pitch,
    double rec_extra
) {

    const Vector nf = (rec_ends[1] - rec_ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(rec_ends[0], rec_ends[1]).closest_point(ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, rec_ends[2], rec_ends[3], nf, v_ins_rec);
    const double lim = 0.5 * static_cast<double>(cnt) * pitch + rec_extra - 10.0;

    for (size_t c = 0; c < 4; c++)
        if (std::abs((slot.bottom[c] - m_b0).dot(nf)) > lim ||
            std::abs((slot.top[c] - m_t0).dot(nf)) > lim)
            return false;

    return true;
}

/// Swept four-point Double_Pin footprint covering retraction (-125 mm) through full tip reach.
std::array<Point, 4> make_double_pin_quad(
    const std::pair<Point, Point>& base_b, const Vector& v_ins, double le_tab, double le_offs
) {

    return {
        base_b.first - v_ins * 125.0,
        base_b.second - v_ins * 125.0,
        base_b.second + v_ins * (le_offs + 2.1 * le_tab),
        base_b.first + v_ins * (le_offs + 2.1 * le_tab)
    };
}

/// Four-point cavity footprint of an Extend_NoPins shelf projecting into the male facet's cavity.
std::array<Point, 4> make_extend_shelf_quad(
    const std::array<Point, 4>& rec_ends,
    const Point& ref_pt,
    const Vector& v_ins_rec,
    double l_offs_rec,
    int cnt,
    double pitch,
    double rec_extra
) {

    const Vector nf = (rec_ends[1] - rec_ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(rec_ends[0], rec_ends[1]).closest_point(ref_pt, false).second;
    const double span = 0.5 * static_cast<double>(cnt) * pitch + rec_extra;

    return {
        m_b0 - nf * span,
        m_b0 + nf * span,
        (m_b0 + nf * span) + v_ins_rec * l_offs_rec,
        (m_b0 - nf * span) + v_ins_rec * l_offs_rec
    };
}

/// Largest pin count in 1..max_count whose projected slots, shelves and shafts keep full clearance.
int find_safe_pin_count(
    const std::vector<OrigamiEdge>& edges,
    const OrigamiBand& proto,
    const std::array<Point, 4>& rec_ends,
    const Vector& v_ins_rec,
    double l_offs_rec,
    double rec_extra,
    int max_count,
    bool is_double_pin,
    double scale,
    const OrigamiFacet& rec_facet,
    const OrigamiFacet& mal_facet,
    size_t rec_side,
    const std::array<double, 4>& heights,
    size_t same_layer,
    const std::array<std::pair<Polyline, Polyline>, 2>& rec_stock,
    const OrigamiJoineryTables& tables,
    double plate_thickness,
    double min_edge_clear
) {

    const Vector nf = (proto.ends[1] - proto.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(proto.ends[0], proto.ends[1]).closest_point(proto.ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, proto.ends[2], proto.ends[3], nf, proto.v_ins);
    const Point pm_b = m_b0 - proto.v_ins * (proto.step_bot ? proto.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - proto.v_ins * (proto.step_bot ? 0.5 : proto.le_tab + 0.5);
    const size_t opp_layer = 1 - same_layer;

    for (int cnt = max_count; cnt >= 1; cnt--) {
        const double f_off = compute_first_pin_offset(cnt, proto.pitch);
        const Point ptc_b = pm_b - nf * f_off;
        const Point ptc_t = pm_t - nf * f_off;
        std::vector<OrigamiSlot> local_same;
        std::vector<OrigamiSlot> local_opp;
        bool ok = true;

        if (is_double_pin) {
            const std::array<Point, 4> shelf_q = make_extend_shelf_quad(
                rec_ends, proto.ref_pt, v_ins_rec, l_offs_rec, cnt, proto.pitch, rec_extra
            );

            for (const PinFootprint& fp : tables.cavity_pins[mal_facet.index])
                if (quads_closer_than_2d(shelf_q, fp.quad, mal_facet.normal, 24.0))
                    ok = false;
        }

        for (int i = 0; i < cnt && ok; i++) {
            const double step = proto.pitch * static_cast<double>(i);

            if (is_double_pin) {
                std::pair<Point, Point> bb;
                std::pair<Point, Point> tb;
                std::pair<Point, Point> bt;
                std::pair<Point, Point> tt;
                compute_scaled_double_pin(
                    ptc_b + nf * step, nf, proto.v_ins, proto.le_tab, proto.le_offs, scale, bb, tb
                );
                compute_scaled_double_pin(
                    ptc_t + nf * step, nf, proto.v_ins, proto.le_tab, proto.le_offs, scale, bt, tt
                );
                const OrigamiSlot s_same = project_origami_slot(
                    bb,
                    bt,
                    proto.v_ins,
                    rec_facet,
                    heights[2 * same_layer],
                    heights[2 * same_layer + 1]
                );
                const OrigamiSlot s_opp = project_origami_slot(
                    tb,
                    tt,
                    proto.v_ins,
                    rec_facet,
                    heights[2 * opp_layer],
                    heights[2 * opp_layer + 1]
                );
                ok = slot_within_extend_span(
                         s_same, rec_ends, proto.ref_pt, v_ins_rec, cnt, proto.pitch, rec_extra
                     ) &&
                     slot_clears_plate(
                         edges,
                         s_same,
                         rec_facet,
                         same_layer,
                         rec_side,
                         true,
                         rec_stock[same_layer].first,
                         rec_stock[same_layer].second,
                         tables.plate_slots[same_layer][rec_facet.index],
                         local_same,
                         plate_thickness,
                         min_edge_clear
                     ) &&
                     slot_clears_plate(
                         edges,
                         s_opp,
                         rec_facet,
                         opp_layer,
                         rec_side,
                         false,
                         rec_stock[opp_layer].first,
                         rec_stock[opp_layer].second,
                         tables.plate_slots[opp_layer][rec_facet.index],
                         local_opp,
                         plate_thickness,
                         min_edge_clear
                     );
                local_same.push_back(s_same);
                local_opp.push_back(s_opp);
                const std::array<Point, 4> pin_quad =
                    make_double_pin_quad(bb, proto.v_ins, proto.le_tab, proto.le_offs);

                for (const PinFootprint& fp : tables.cavity_pins[rec_facet.index])
                    if (quads_closer_than_2d(pin_quad, fp.quad, rec_facet.normal, 24.0))
                        ok = false;
            } else {
                std::pair<Point, Point> sb;
                std::pair<Point, Point> st;
                compute_scaled_single_pin(
                    ptc_b + nf * step, nf, proto.v_ins, proto.le_tab, scale, sb
                );
                compute_scaled_single_pin(
                    ptc_t + nf * step, nf, proto.v_ins, proto.le_tab, scale, st
                );
                const OrigamiSlot s_same = project_origami_slot(
                    sb,
                    st,
                    proto.v_ins,
                    rec_facet,
                    heights[2 * same_layer],
                    heights[2 * same_layer + 1]
                );
                ok = slot_within_extend_span(
                         s_same, rec_ends, proto.ref_pt, v_ins_rec, cnt, proto.pitch, rec_extra
                     ) &&
                     slot_clears_plate(
                         edges,
                         s_same,
                         rec_facet,
                         same_layer,
                         rec_side,
                         true,
                         rec_stock[same_layer].first,
                         rec_stock[same_layer].second,
                         tables.plate_slots[same_layer][rec_facet.index],
                         local_same,
                         plate_thickness,
                         min_edge_clear
                     );
                local_same.push_back(s_same);
            }
        }

        if (ok)
            return cnt;
    }

    return 0;
}

/// Emits stepped Double_Pin contours, records 3D shaft footprints and appends mortise slots.
std::array<std::vector<Point>, 2> emit_origami_double_pins(
    const OrigamiBand& band,
    double scale,
    const OrigamiFacet& rec_facet,
    const std::array<double, 4>& heights,
    size_t same_layer,
    OrigamiJoineryTables& tables
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
        f_off + 145.0 * scale
    );
    const size_t opp_layer = 1 - same_layer;
    std::vector<Point> bot = {m_b0 - nf * sh_half, pm_b - nf * sh_half};
    std::vector<Point> top = {m_t0 - nf * sh_half, pm_t - nf * sh_half};
    const Point ptc_b = pm_b - nf * f_off;
    const Point ptc_t = pm_t - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<Point, Point> bb;
        std::pair<Point, Point> tb;
        std::pair<Point, Point> bt;
        std::pair<Point, Point> tt;
        const std::vector<Point> pb = compute_scaled_double_pin(
            ptc_b + nf * step, nf, band.v_ins, band.le_tab, band.le_offs, scale, bb, tb
        );
        const std::vector<Point> pt = compute_scaled_double_pin(
            ptc_t + nf * step, nf, band.v_ins, band.le_tab, band.le_offs, scale, bt, tt
        );
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        tables.plate_slots[same_layer][rec_facet.index].push_back(project_origami_slot(
            bb, bt, band.v_ins, rec_facet, heights[2 * same_layer], heights[2 * same_layer + 1]
        ));
        tables.plate_slots[opp_layer][rec_facet.index].push_back(project_origami_slot(
            tb, tt, band.v_ins, rec_facet, heights[2 * opp_layer], heights[2 * opp_layer + 1]
        ));
        tables.cavity_pins[rec_facet.index].push_back(
            {make_double_pin_quad(bb, band.v_ins, band.le_tab, band.le_offs)}
        );
    }

    bot.push_back(pm_b + nf * sh_half);
    bot.push_back(m_b0 + nf * sh_half);
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * sh_half);

    return {bot, top};
}

/// Emits chamfered Single_Pin contours and appends projected mortise slots.
std::array<std::vector<Point>, 2> emit_origami_single_pins(
    const OrigamiBand& band,
    double scale,
    const OrigamiFacet& rec_facet,
    double h_bot,
    double h_top,
    std::vector<OrigamiSlot>& slots
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
        f_off + 100.0 * scale
    );
    std::vector<Point> bot = {m_b0 - nf * sh_half, pm_b - nf * sh_half};
    std::vector<Point> top = {m_t0 - nf * sh_half, pm_t - nf * sh_half};
    const Point ptc_b = pm_b - nf * f_off;
    const Point ptc_t = pm_t - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<Point, Point> sb;
        std::pair<Point, Point> st;
        const std::vector<Point> pb =
            compute_scaled_single_pin(ptc_b + nf * step, nf, band.v_ins, band.le_tab, scale, sb);
        const std::vector<Point> pt =
            compute_scaled_single_pin(ptc_t + nf * step, nf, band.v_ins, band.le_tab, scale, st);
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        slots.push_back(project_origami_slot(sb, st, band.v_ins, rec_facet, h_bot, h_top));
    }

    bot.push_back(pm_b + nf * sh_half);
    bot.push_back(m_b0 + nf * sh_half);
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * sh_half);

    return {bot, top};
}

/// Emits paired bottom and top female Extend rabbet lip contours.
std::array<std::vector<Point>, 2> emit_origami_extend_tab(const OrigamiBand& band) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const double span =
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra - 0.5;
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

/// Emits paired bottom and top Extend_NoPins cavity-bridging female shelf contours.
std::array<std::vector<Point>, 2> emit_origami_extend_shelf(const OrigamiBand& band) {

    const Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const Point m_b0 =
        Line::from_points(band.ends[0], band.ends[1]).closest_point(band.ref_pt, false).second;
    const Point m_t0 = align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const double span =
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra - 0.5;
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

/// Resolves 3D miter endpoints, overlap midpoint and bevel stepbacks for one interior edge.
std::optional<OrigamiCreaseFrame> compute_origami_crease_frame(
    const std::vector<OrigamiFacet>& facets,
    const OrigamiEdge& edge,
    const std::vector<Vector>& ins_vecs,
    double plate_thickness,
    double total_offset
) {

    if (edge.is_boundary || edge.facet_b < 0 || edge.dihedral_rad < 0.22)
        return std::nullopt;

    const size_t fa = static_cast<size_t>(edge.facet_a);
    const size_t fb = static_cast<size_t>(edge.facet_b);
    const bool a_first = facets[fa].assembly_step < facets[fb].assembly_step;
    OrigamiCreaseFrame cf;
    cf.rec_f = a_first ? fa : fb;
    cf.mal_f = a_first ? fb : fa;
    const OrigamiFacet& rec = facets[cf.rec_f];
    const OrigamiFacet& mal = facets[cf.mal_f];
    cf.rec_s = find_shared_side(rec, edge.u, edge.v);
    cf.mal_s = find_shared_side(mal, edge.u, edge.v);
    cf.positive_fold = edge.positive_fold;
    const size_t mr = rec.vertices.size();
    const size_t mm = mal.vertices.size();
    cf.ends_rec_in = {
        rec.miter_rings[0][cf.rec_s],
        rec.miter_rings[0][(cf.rec_s + 1) % mr],
        rec.miter_rings[1][cf.rec_s],
        rec.miter_rings[1][(cf.rec_s + 1) % mr]
    };
    cf.ends_rec_out = {
        rec.miter_rings[2][cf.rec_s],
        rec.miter_rings[2][(cf.rec_s + 1) % mr],
        rec.miter_rings[3][cf.rec_s],
        rec.miter_rings[3][(cf.rec_s + 1) % mr]
    };
    cf.ends_mal_in = {
        mal.miter_rings[0][cf.mal_s],
        mal.miter_rings[0][(cf.mal_s + 1) % mm],
        mal.miter_rings[1][cf.mal_s],
        mal.miter_rings[1][(cf.mal_s + 1) % mm]
    };
    cf.ends_mal_out = {
        mal.miter_rings[2][cf.mal_s],
        mal.miter_rings[2][(cf.mal_s + 1) % mm],
        mal.miter_rings[3][cf.mal_s],
        mal.miter_rings[3][(cf.mal_s + 1) % mm]
    };
    const Vector u_in = (cf.ends_rec_in[1] - cf.ends_rec_in[0]).normalized();
    const double ti0 = (cf.ends_mal_in[0] - cf.ends_rec_in[0]).dot(u_in);
    const double ti1 = (cf.ends_mal_in[1] - cf.ends_rec_in[0]).dot(u_in);
    const double lo_in = std::max(0.0, std::min(ti0, ti1));
    const double hi_in =
        std::min(cf.ends_rec_in[0].distance(cf.ends_rec_in[1]), std::max(ti0, ti1));
    cf.ref_b = hi_in > lo_in ? (cf.ends_rec_in[0] + u_in * (0.5 * (lo_in + hi_in)))
                             : Point::mid_point(cf.ends_rec_in[0], cf.ends_rec_in[1]);
    const Vector u_out = (cf.ends_rec_out[1] - cf.ends_rec_out[0]).normalized();
    const double to0 = (cf.ends_mal_out[0] - cf.ends_rec_out[0]).dot(u_out);
    const double to1 = (cf.ends_mal_out[1] - cf.ends_rec_out[0]).dot(u_out);
    const double lo_out = std::max(0.0, std::min(to0, to1));
    const double hi_out =
        std::min(cf.ends_rec_out[0].distance(cf.ends_rec_out[1]), std::max(to0, to1));
    cf.ref_t = hi_out > lo_out ? (cf.ends_rec_out[0] + u_out * (0.5 * (lo_out + hi_out)))
                               : Point::mid_point(cf.ends_rec_out[0], cf.ends_rec_out[1]);
    cf.min_len = std::min(
        {hi_in - lo_in,
         hi_out - lo_out,
         cf.ends_rec_in[2].distance(cf.ends_rec_in[3]),
         cf.ends_mal_in[2].distance(cf.ends_mal_in[3])}
    );
    const Vector perp_rec = compute_inplane_outward(rec, cf.ends_rec_in[0], cf.ends_rec_in[1]);
    const Vector perp_mal = compute_inplane_outward(mal, cf.ends_mal_in[0], cf.ends_mal_in[1]);
    cf.v_ins_rec = perp_rec;
    cf.v_ins_mal = ins_vecs[cf.mal_f];
    const double cos_mal = cf.v_ins_mal.dot(perp_mal);

    if (cos_mal < 0.30)
        return std::nullopt;

    const double sin_alpha = std::max(0.18, std::sin(edge.dihedral_rad));
    cf.l_tab_rec = plate_thickness / sin_alpha;
    cf.l_offs_rec = (total_offset - plate_thickness) / sin_alpha;
    cf.l_tab_mal = (plate_thickness / sin_alpha) / cos_mal;
    cf.l_offs_mal = ((total_offset - plate_thickness) / sin_alpha) / cos_mal;

    return cf;
}

/// Minimum sin(angle) across both interior facet corners incident to side_idx.
double min_crease_corner_sin(const OrigamiFacet& facet, size_t side_idx) {

    const std::vector<Point>& r = facet.miter_rings[0];
    const size_t m = r.size();
    const Vector e_prev = (r[(side_idx + m - 1) % m] - r[side_idx]).normalized();
    const Vector e_curr = (r[(side_idx + 1) % m] - r[side_idx]).normalized();
    const Vector e_next = (r[(side_idx + 2) % m] - r[(side_idx + 1) % m]).normalized();
    const double sin0 = e_prev.cross(e_curr).magnitude();
    const double sin1 = (-e_curr).cross(e_next).magnitude();

    return std::max(0.25, std::min(sin0, sin1));
}

/// Populates Double_Pin, Single_Pin, Extend_NoPins and Extend contours on one origami crease.
void apply_origami_crease_joinery(
    const std::vector<OrigamiFacet>& facets,
    const std::vector<OrigamiEdge>& edges,
    const OrigamiCreaseFrame& cf,
    const std::array<double, 4>& heights,
    const std::vector<std::array<std::pair<Polyline, Polyline>, 2>>& stock_rings,
    double plate_thickness,
    double total_offset,
    double pitch,
    OrigamiJoineryTables& tables
) {

    const double scale = std::clamp(pitch / 500.0, 0.35, 1.15);
    const OrigamiFacet& rec = facets[cf.rec_f];
    const OrigamiFacet& mal = facets[cf.mal_f];
    const double c_sin =
        std::min(min_crease_corner_sin(rec, cf.rec_s), min_crease_corner_sin(mal, cf.mal_s));
    const double corner_margin = std::max(0.45 * pitch, 1.35 * total_offset) / c_sin;

    if (cf.min_len < 2.0 * corner_margin + 0.5 * pitch)
        return;

    const int max_cnt =
        std::max(1, static_cast<int>(std::floor((cf.min_len - 2.0 * corner_margin) / pitch)));
    const Vector nf_mal = (cf.ends_mal_in[1] - cf.ends_mal_in[0]).normalized();
    const double skew_mal = std::abs(cf.v_ins_mal.dot(nf_mal));
    const double rec_extra =
        std::max(0.0, 135.0 * scale + skew_mal * (1.8 * cf.l_tab_mal) + 14.0 - 0.5 * pitch);
    const double dp_extra = rec_extra + skew_mal * (cf.l_offs_mal + cf.l_tab_mal + 125.0) + 6.0;
    const double sp_extra = rec_extra + skew_mal * (2.0 * cf.l_tab_mal + 125.0) + 6.0;
    const double min_clear = std::max(32.0, 0.85 * plate_thickness);
    const size_t dp_layer = cf.positive_fold ? 0 : 1;
    const size_t sp_layer = 1 - dp_layer;
    const bool mal_dp_step_bot = !cf.positive_fold;
    const bool mal_sp_step_bot = !cf.positive_fold;
    const std::array<Point, 4>& mal_dp_ends = cf.positive_fold ? cf.ends_mal_in : cf.ends_mal_out;
    const std::array<Point, 4>& mal_sp_ends = cf.positive_fold ? cf.ends_mal_out : cf.ends_mal_in;
    const std::array<Point, 4>& rec_sh_ends = cf.positive_fold ? cf.ends_rec_in : cf.ends_rec_out;
    const std::array<Point, 4>& rec_ex_ends = cf.positive_fold ? cf.ends_rec_out : cf.ends_rec_in;
    const Point& dp_ref = cf.positive_fold ? cf.ref_b : cf.ref_t;
    const Point& sp_ref = cf.positive_fold ? cf.ref_t : cf.ref_b;

    const double av_dp = std::min(
        band_available_half_span(mal_dp_ends, dp_ref, cf.v_ins_mal, corner_margin) - dp_extra,
        band_available_half_span(rec_sh_ends, dp_ref, cf.v_ins_rec, corner_margin) - rec_extra
    );
    const double av_sp = std::min(
        band_available_half_span(mal_sp_ends, sp_ref, cf.v_ins_mal, corner_margin) - sp_extra,
        band_available_half_span(rec_ex_ends, sp_ref, cf.v_ins_rec, corner_margin) - rec_extra
    );
    const int max_dp = std::max(0, static_cast<int>(std::floor(2.0 * av_dp / pitch)));
    const int max_sp =
        std::min(max_cnt, std::max(0, static_cast<int>(std::floor(2.0 * av_sp / pitch))));
    OrigamiBand dp_band = {
        mal_dp_ends,
        dp_ref,
        cf.v_ins_mal,
        cf.l_tab_mal,
        cf.l_offs_mal,
        mal_dp_step_bot,
        0,
        pitch,
        dp_extra
    };
    OrigamiBand sp_band = {
        mal_sp_ends, sp_ref, cf.v_ins_mal, cf.l_tab_mal, 0.0, mal_sp_step_bot, 0, pitch, sp_extra
    };
    const int n_dp = max_dp > 0 ? find_safe_pin_count(
                                      edges,
                                      dp_band,
                                      rec_sh_ends,
                                      cf.v_ins_rec,
                                      cf.l_offs_rec,
                                      rec_extra,
                                      max_dp,
                                      true,
                                      scale,
                                      rec,
                                      mal,
                                      cf.rec_s,
                                      heights,
                                      dp_layer,
                                      stock_rings[cf.rec_f],
                                      tables,
                                      plate_thickness,
                                      min_clear
                                  )
                                : 0;
    const int n_sp = max_sp > 0 ? find_safe_pin_count(
                                      edges,
                                      sp_band,
                                      rec_ex_ends,
                                      cf.v_ins_rec,
                                      0.0,
                                      rec_extra,
                                      max_sp,
                                      false,
                                      scale,
                                      rec,
                                      mal,
                                      cf.rec_s,
                                      heights,
                                      sp_layer,
                                      stock_rings[cf.rec_f],
                                      tables,
                                      plate_thickness,
                                      min_clear
                                  )
                                : 0;

    if (n_dp <= 0 && n_sp <= 0)
        return;

    if (n_dp > 0) {
        dp_band.count = n_dp;
        const OrigamiBand sh_band = {
            rec_sh_ends,
            dp_ref,
            cf.v_ins_rec,
            cf.l_tab_rec,
            cf.l_offs_rec,
            !cf.positive_fold,
            n_dp,
            pitch,
            rec_extra
        };
        const std::array<std::vector<Point>, 2> dp_pair =
            emit_origami_double_pins(dp_band, scale, rec, heights, dp_layer, tables);
        const std::array<std::vector<Point>, 2> sh_pair = emit_origami_extend_shelf(sh_band);
        tables.cavity_pins[cf.mal_f].push_back({make_extend_shelf_quad(
            rec_sh_ends, dp_ref, cf.v_ins_rec, cf.l_offs_rec, n_dp, pitch, rec_extra
        )});
        tables.edge_polys[2 * dp_layer][cf.mal_f][cf.mal_s] = dp_pair[0];
        tables.edge_polys[2 * dp_layer + 1][cf.mal_f][cf.mal_s] = dp_pair[1];
        tables.edge_polys[2 * dp_layer][cf.rec_f][cf.rec_s] = sh_pair[0];
        tables.edge_polys[2 * dp_layer + 1][cf.rec_f][cf.rec_s] = sh_pair[1];
        tables.double_pins += static_cast<size_t>(n_dp);
    }

    if (n_sp > 0) {
        sp_band.count = n_sp;
        const OrigamiBand ex_band = {
            rec_ex_ends,
            sp_ref,
            cf.v_ins_rec,
            cf.l_tab_rec,
            0.0,
            cf.positive_fold,
            n_sp,
            pitch,
            rec_extra
        };
        const std::array<std::vector<Point>, 2> sp_pair = emit_origami_single_pins(
            sp_band,
            scale,
            rec,
            heights[2 * sp_layer],
            heights[2 * sp_layer + 1],
            tables.plate_slots[sp_layer][cf.rec_f]
        );
        const std::array<std::vector<Point>, 2> ex_pair = emit_origami_extend_tab(ex_band);
        tables.edge_polys[2 * sp_layer][cf.mal_f][cf.mal_s] = sp_pair[0];
        tables.edge_polys[2 * sp_layer + 1][cf.mal_f][cf.mal_s] = sp_pair[1];
        tables.edge_polys[2 * sp_layer][cf.rec_f][cf.rec_s] = ex_pair[0];
        tables.edge_polys[2 * sp_layer + 1][cf.rec_f][cf.rec_s] = ex_pair[1];
        tables.single_pins += static_cast<size_t>(n_sp);
    }

    tables.seams++;
}

/// Attaches jointed outer outlines and consistently wound through-mortise hole loops onto plate.
void attach_origami_features(
    FacePlanarElement& plate,
    const OrigamiFacet& facet,
    const Polyline& bot_outline,
    const Polyline& top_outline,
    const std::vector<OrigamiSlot>& slots
) {

    plate.features.bottom = {bot_outline};
    plate.features.top = {top_outline};

    for (const OrigamiSlot& slot : slots) {
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

}

// ═══════════════════════════════════════════════════════════════════════════
// Parametric origami mesh generators
// ═══════════════════════════════════════════════════════════════════════════

Mesh OrigamiShell::yoshimura_arch_mesh(
    size_t bays,
    size_t arch_segments,
    double span,
    double bay_length,
    double rise,
    double pleat_depth
) {

    const size_t nb = std::max<size_t>(1, bays);
    const size_t ns = std::max<size_t>(3, arch_segments);
    const double half_span = 0.5 * std::max(2000.0, span);
    const double h = std::max(800.0, rise);
    const double pitch_y = std::max(800.0, bay_length);
    const double half_len = 0.5 * static_cast<double>(nb) * pitch_y;
    const double r0 = (half_span * half_span + h * h) / (2.0 * h);
    const double cz = h - r0;
    const double theta_max = std::atan2(half_span, r0 - h);
    std::vector<Point> pts;
    std::vector<std::vector<size_t>> st(2 * nb + 1);

    for (size_t s = 0; s <= 2 * nb; s++) {
        const double y = -half_len + 0.5 * pitch_y * static_cast<double>(s);

        if (s % 2 == 0) {
            for (size_t j = 0; j <= ns; j++) {
                const double th =
                    -theta_max + 2.0 * theta_max * static_cast<double>(j) / static_cast<double>(ns);
                const double z = (j == 0 || j == ns) ? 0.0 : (cz + r0 * std::cos(th));
                st[s].push_back(pts.size());
                pts.push_back({r0 * std::sin(th), y, z});
            }
        } else {
            for (size_t j = 0; j < ns; j++) {
                const double th = -theta_max + 2.0 * theta_max * (static_cast<double>(j) + 0.5) /
                                                   static_cast<double>(ns);
                const double r_odd = r0 + pleat_depth;
                st[s].push_back(pts.size());
                pts.push_back(
                    {r_odd * std::sin(th), y, std::max(200.0, cz + r_odd * std::cos(th))}
                );
            }
        }
    }

    std::vector<std::vector<size_t>> faces;

    for (size_t b = 0; b < nb; b++) {
        const std::vector<size_t>& e0 = st[2 * b];
        const std::vector<size_t>& o1 = st[2 * b + 1];
        const std::vector<size_t>& e2 = st[2 * b + 2];
        faces.push_back({e0[0], o1[0], e2[0]});

        for (size_t j = 0; j < ns; j++) {
            faces.push_back({e0[j], e0[j + 1], o1[j]});
            faces.push_back({e2[j + 1], e2[j], o1[j]});
        }

        for (size_t j = 0; j + 1 < ns; j++) {
            faces.push_back({o1[j], e0[j + 1], o1[j + 1]});
            faces.push_back({o1[j + 1], e2[j + 1], o1[j]});
        }

        faces.push_back({e2[ns], o1[ns - 1], e0[ns]});
    }

    return Mesh::from_vertices_and_faces(pts, faces);
}

Mesh OrigamiShell::miura_arch_mesh(
    size_t arch_folds,
    size_t strips,
    double span,
    double length,
    double rise,
    double pleat_depth,
    double chevron_shift
) {

    const size_t m = std::max<size_t>(4, arch_folds + (arch_folds % 2));
    const size_t rows = 2 * std::max<size_t>(1, strips);
    const double half_span = 0.5 * std::max(2000.0, span);
    const double h = std::max(800.0, rise);
    const double half_len = 0.5 * std::max(1600.0, length);
    const double step_y = (2.0 * half_len) / static_cast<double>(rows);
    const double r0 = (half_span * half_span + h * h) / (2.0 * h);
    const double cz = h - r0;
    const double theta_max = std::atan2(half_span, r0 - h);
    std::vector<Point> arc(m + 1);

    for (size_t i = 0; i <= m; i++) {
        const double th =
            -theta_max + 2.0 * theta_max * static_cast<double>(i) / static_cast<double>(m);
        arc[i] = {r0 * std::sin(th), 0.0, (i == 0 || i == m) ? 0.0 : (cz + r0 * std::cos(th))};
    }

    std::vector<Vector> tangents(m);

    for (size_t i = 0; i < m; i++)
        tangents[i] = (arc[i + 1] - arc[i]).normalized();

    const size_t mid_i = m / 2;
    std::vector<std::vector<Point>> grid(m + 1, std::vector<Point>(rows + 1));

    for (size_t j = 0; j <= rows; j++) {
        const double sgn = (j % 2 == 0) ? -1.0 : 1.0;
        const double y = -half_len + step_y * static_cast<double>(j);
        grid[mid_i][j] = {sgn * chevron_shift, y, h + sgn * (0.5 * pleat_depth)};
    }

    for (size_t step = 0; step < mid_i; step++) {
        const size_t il = mid_i - 1 - step;
        const size_t ir = mid_i + 1 + step;

        for (size_t j = 0; j <= rows; j++) {
            const double sgn = (j % 2 == 0) ? -1.0 : 1.0;
            Plane pl = Plane::xy_plane();
            Plane pr = Plane::xy_plane();

            if (il > 0) {
                const Vector nl = (tangents[il - 1] + tangents[il]).normalized();
                const Vector rl = Vector{-nl[2], 0.0, nl[0]}.normalized();
                pl = Plane::from_point_normal(
                    arc[il] + rl * (sgn * 0.5 * pleat_depth) + nl * (sgn * chevron_shift), nl
                );
            }

            if (ir < m) {
                const Vector nr = (tangents[ir - 1] + tangents[ir]).normalized();
                const Vector rr = Vector{-nr[2], 0.0, nr[0]}.normalized();
                pr = Plane::from_point_normal(
                    arc[ir] + rr * (sgn * 0.5 * pleat_depth) + nr * (sgn * chevron_shift), nr
                );
            }

            grid[il][j] = Intersection::line_plane(
                              Line::from_point_and_vector(grid[il + 1][j], -tangents[il]), pl, false
            )
                              .value_or(arc[il]);
            grid[ir][j] =
                Intersection::line_plane(
                    Line::from_point_and_vector(grid[ir - 1][j], tangents[ir - 1]), pr, false
                )
                    .value_or(arc[ir]);
        }
    }

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 0; i <= m; i++)
        for (size_t j = 0; j <= rows; j++)
            pts.push_back(grid[i][j]);

    for (size_t i = 0; i < m; i++)
        for (size_t j = 0; j < rows; j++) {
            const size_t v00 = i * (rows + 1) + j;
            const size_t v10 = (i + 1) * (rows + 1) + j;
            faces.push_back({v00, v10, v10 + 1, v00 + 1});
        }

    return Mesh::from_vertices_and_faces(pts, faces);
}

// ═══════════════════════════════════════════════════════════════════════════
// Constructor and accessors
// ═══════════════════════════════════════════════════════════════════════════

OrigamiShell::OrigamiShell(
    const Mesh& input_mesh,
    double plate_thickness,
    double total_offset,
    bool with_joints,
    bool with_borders,
    double pin_pitch,
    double corner_scale,
    const std::string& name
)
    : WoodSession(name), plate_thickness(std::max(12.0, plate_thickness)),
      total_offset(std::max(2.4 * std::max(12.0, plate_thickness), total_offset)),
      with_joints(with_joints), with_borders(with_borders),
      pin_pitch(std::clamp(pin_pitch, 200.0, 650.0)),
      corner_scale(std::clamp(corner_scale, 0.0, 2.0)) {

    // base mesh and four miter offset shells
    extract_oriented_facets(input_mesh);
    compute_edges();
    compute_miter_rings();
    _inner_cavity_mesh = compute_offset_mesh(1);
    _outer_cavity_mesh = compute_offset_mesh(2);
    _outer_mesh = compute_offset_mesh(3);
    _miter_planes_mesh = compute_miter_planes_mesh();

    // sequential assembly order, single-direction insertion vectors, shell plates and borders
    compute_assembly_and_vectors();
    compute_shell_plates();

    if (this->with_borders)
        compute_border_plates();

    populate_scene();
}

const std::vector<Point>& OrigamiShell::vertices() const {
    return _vertices;
}

const std::vector<size_t>& OrigamiShell::vertex_valences() const {
    return _vertex_valences;
}

std::array<double, 4> OrigamiShell::layer_heights() const {
    return {0.0, plate_thickness, total_offset - plate_thickness, total_offset};
}

double OrigamiShell::cavity_gap() const {
    return total_offset - 2.0 * plate_thickness;
}

const Mesh& OrigamiShell::base_mesh() const {
    return _base_mesh;
}

const Mesh& OrigamiShell::inner_cavity_mesh() const {
    return _inner_cavity_mesh;
}

const Mesh& OrigamiShell::outer_cavity_mesh() const {
    return _outer_cavity_mesh;
}

const Mesh& OrigamiShell::outer_mesh() const {
    return _outer_mesh;
}

const Mesh& OrigamiShell::miter_planes_mesh() const {
    return _miter_planes_mesh;
}

const std::vector<OrigamiFacet>& OrigamiShell::facets() const {
    return _facets;
}

const std::vector<OrigamiEdge>& OrigamiShell::edges() const {
    return _edges;
}

const std::vector<size_t>& OrigamiShell::assembly_order() const {
    return _assembly_order;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& OrigamiShell::inner_plates() const {
    return _inner_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& OrigamiShell::outer_plates() const {
    return _outer_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& OrigamiShell::shell_plates() const {
    return _shell_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& OrigamiShell::border_plates() const {
    return _border_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>&
OrigamiShell::border_plates_for_facet(size_t facet_idx) const {
    return _facet_border_plates.at(facet_idx);
}

const std::vector<std::shared_ptr<FacePlanarElement>>& OrigamiShell::all_plates() const {
    return _all_plates;
}

Vector OrigamiShell::plate_insertion_vector(size_t facet_idx, size_t layer) const {
    return _insertion_vectors.at(layer).at(facet_idx);
}

size_t OrigamiShell::total_jointed_seams() const {
    return _total_jointed_seams;
}

size_t OrigamiShell::total_double_pins() const {
    return _total_double_pins;
}

size_t OrigamiShell::total_single_pins() const {
    return _total_single_pins;
}

size_t OrigamiShell::total_pins() const {
    return _total_double_pins + _total_single_pins;
}

size_t OrigamiShell::total_mortise_slots() const {
    return _total_mortise_slots;
}

void OrigamiShell::copy_plates_to(
    WoodSession& target, const std::string& group_name, const Vector& delta
) const {

    const std::shared_ptr<TreeNode> grp = target.add_group(group_name);

    for (const std::shared_ptr<FacePlanarElement>& plate : _all_plates) {
        std::shared_ptr<FacePlanarElement> copy =
            std::dynamic_pointer_cast<FacePlanarElement>(plate->clone());

        if (!delta.is_zero())
            copy->place(Xform::translation(delta[0], delta[1], delta[2]));

        target.add(copy, grp);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Mesh topology, miter shells and assembly ordering
// ═══════════════════════════════════════════════════════════════════════════

void OrigamiShell::extract_oriented_facets(const Mesh& input_mesh) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> raw =
        input_mesh.weld(0.05).to_vertices_and_faces();
    _vertices = raw.first;
    std::vector<std::vector<size_t>> clean_faces;

    for (const std::vector<size_t>& loop : raw.second) {
        std::vector<size_t> dedup;

        for (size_t i = 0; i < loop.size(); i++)
            if (loop[i] != loop[(i + loop.size() - 1) % loop.size()])
                dedup.push_back(loop[i]);

        if (dedup.size() >= 3)
            clean_faces.push_back(dedup);
    }

    double xmin = 1e18;
    double xmax = -1e18;
    double ymin = 1e18;
    double ymax = -1e18;
    double zmin = 1e18;
    double zmax = -1e18;

    for (const Point& p : _vertices) {
        xmin = std::min(xmin, p[0]);
        xmax = std::max(xmax, p[0]);
        ymin = std::min(ymin, p[1]);
        ymax = std::max(ymax, p[1]);
        zmin = std::min(zmin, p[2]);
        zmax = std::max(zmax, p[2]);
    }

    _ground_z = _vertices.empty() ? 0.0 : zmin;
    _plan_bounds = {xmin, xmax, ymin, ymax};
    orient_face_loops_outward(
        _vertices,
        clean_faces,
        zmin,
        zmax,
        {0.5 * (xmin + xmax), 0.5 * (ymin + ymax), 0.5 * (zmin + zmax)}
    );
    _vertex_valences.assign(_vertices.size(), 0);
    _facets.clear();

    for (size_t f = 0; f < clean_faces.size(); f++) {
        const std::tuple<Point, Vector, double> fr =
            compute_polygon_frame(_vertices, clean_faces[f]);
        OrigamiFacet facet;
        facet.index = f;
        facet.vertices = clean_faces[f];
        facet.centroid = std::get<0>(fr);
        facet.normal = std::get<1>(fr);
        _facets.push_back(facet);

        for (const size_t vid : clean_faces[f])
            _vertex_valences[vid]++;
    }

    _base_mesh = Mesh::from_vertices_and_faces(_vertices, clean_faces);
}

void OrigamiShell::compute_edges() {

    std::map<std::pair<size_t, size_t>, size_t> lookup;
    _edges.clear();
    double zmax = _ground_z;

    for (const Point& p : _vertices)
        zmax = std::max(zmax, p[2]);

    const bool has_rise = (zmax - _ground_z) > 150.0;

    for (size_t f = 0; f < _facets.size(); f++) {
        OrigamiFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.edge_ids.assign(m, -1);

        for (size_t k = 0; k < m; k++) {
            const size_t u = facet.vertices[k];
            const size_t v = facet.vertices[(k + 1) % m];
            const std::pair<size_t, size_t> key = std::minmax(u, v);
            const std::map<std::pair<size_t, size_t>, size_t>::iterator found = lookup.find(key);

            if (found == lookup.end()) {
                const size_t eid = _edges.size();
                OrigamiEdge edge;
                edge.u = u;
                edge.v = v;
                edge.facet_a = static_cast<int>(f);
                edge.side_a = static_cast<int>(k);
                _edges.push_back(edge);
                lookup.emplace(key, eid);
                facet.edge_ids[k] = static_cast<int>(eid);
            } else {
                _edges[found->second].facet_b = static_cast<int>(f);
                _edges[found->second].side_b = static_cast<int>(k);
                facet.edge_ids[k] = static_cast<int>(found->second);
            }
        }
    }

    for (OrigamiEdge& edge : _edges) {
        const Point& pu = _vertices[edge.u];
        const Point& pv = _vertices[edge.v];

        if (edge.facet_b < 0) {
            edge.is_boundary = true;
            const OrigamiFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
            edge.on_ground = has_rise && std::abs(pu[2] - _ground_z) < 1.5 &&
                             std::abs(pv[2] - _ground_z) < 1.5 && std::abs(fa.normal[2]) < 0.88;
            continue;
        }

        const OrigamiFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        const OrigamiFacet& fb = _facets[static_cast<size_t>(edge.facet_b)];
        edge.is_boundary = false;
        edge.positive_fold = (fb.centroid - fa.centroid).dot(fa.normal) < 0.0;
        const double cos_n = std::clamp(fa.normal.dot(fb.normal), -1.0, 1.0);
        edge.dihedral_rad = std::acos(cos_n);
        const double phi_deg = 0.5 * (180.0 - edge.dihedral_rad * Tolerance::TO_DEGREES);
        edge.tool_tilt_deg = std::abs(phi_deg - 90.0);
    }
}

void OrigamiShell::compute_miter_rings() {

    const std::array<double, 4> heights = layer_heights();
    const double max_drift = 4.5 * total_offset;

    for (size_t f = 0; f < _facets.size(); f++) {
        OrigamiFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.side_planes.resize(m);

        for (size_t k = 0; k < m; k++) {
            const OrigamiEdge& edge = _edges[static_cast<size_t>(facet.edge_ids[k])];
            facet.side_planes[k] = compute_origami_side_plane(
                _vertices,
                _facets,
                f,
                edge,
                facet.vertices[k],
                facet.vertices[(k + 1) % m],
                _ground_z,
                _plan_bounds
            );
        }

        for (size_t hi = 0; hi < 4; hi++) {
            const Plane offset_plane =
                Plane::from_point_normal(facet.centroid + facet.normal * heights[hi], facet.normal);
            facet.miter_rings[hi].resize(m);

            for (size_t k = 0; k < m; k++) {
                const Plane& prev_side = facet.side_planes[(k + m - 1) % m];
                const Plane& curr_side = facet.side_planes[k];
                const Point fallback = _vertices[facet.vertices[k]] + facet.normal * heights[hi];
                const std::optional<Point> hit =
                    Intersection::plane_plane_plane(offset_plane, prev_side, curr_side);
                facet.miter_rings[hi][k] =
                    (hit && hit->distance(fallback) <= max_drift) ? *hit : fallback;
            }
        }
    }

    for (OrigamiEdge& edge : _edges) {
        const OrigamiFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        edge.miter_plane = fa.side_planes[static_cast<size_t>(edge.side_a)];
    }
}

Mesh OrigamiShell::compute_offset_mesh(size_t ring_index) const {

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> faces;

    for (const OrigamiFacet& facet : _facets) {
        const std::vector<Point>& ring = facet.miter_rings[ring_index];
        std::vector<size_t> loop;

        for (const Point& pt : ring) {
            loop.push_back(pts.size());
            pts.push_back(pt);
        }

        faces.push_back(loop);
    }

    return Mesh::from_vertices_and_faces(pts, faces);
}

Mesh OrigamiShell::compute_miter_planes_mesh() const {

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> faces;

    for (const OrigamiEdge& edge : _edges) {
        if (edge.on_ground)
            continue;

        const OrigamiFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        const size_t k = static_cast<size_t>(edge.side_a);
        const size_t m = fa.vertices.size();
        const size_t base = pts.size();
        pts.push_back(fa.miter_rings[0][k]);
        pts.push_back(fa.miter_rings[0][(k + 1) % m]);
        pts.push_back(fa.miter_rings[3][(k + 1) % m]);
        pts.push_back(fa.miter_rings[3][k]);
        faces.push_back({base, base + 1, base + 2, base + 3});
    }

    return Mesh::from_vertices_and_faces(pts, faces);
}

void OrigamiShell::compute_assembly_and_vectors() {

    const size_t fcount = _facets.size();
    _assembly_order.assign(fcount, 0);
    _insertion_vectors[0].assign(fcount, {0.0, 0.0, 1.0});
    _insertion_vectors[1].assign(fcount, {0.0, 0.0, 1.0});

    if (fcount == 0)
        return;

    // Reverse boundary peeling: peels exposed boundary facets from last step down to 0.
    std::vector<bool> remaining(fcount, true);

    for (size_t rev = 0; rev < fcount; rev++) {
        const size_t step = fcount - 1 - rev;
        size_t best_f = 0;
        double best_score = -1e18;

        for (size_t f = 0; f < fcount; f++) {
            if (!remaining[f])
                continue;

            const OrigamiFacet& facet = _facets[f];
            const size_t m = facet.vertices.size();
            size_t exposed_edges = 0;
            std::vector<Vector> perps;

            for (size_t k = 0; k < m; k++) {
                const OrigamiEdge& e = _edges[static_cast<size_t>(facet.edge_ids[k])];

                if (e.is_boundary) {
                    exposed_edges++;
                    continue;
                }

                const size_t nb =
                    static_cast<size_t>(e.facet_a == static_cast<int>(f) ? e.facet_b : e.facet_a);

                if (!remaining[nb])
                    exposed_edges++;
                else
                    perps.push_back(compute_inplane_outward(
                        facet, facet.miter_rings[0][k], facet.miter_rings[0][(k + 1) % m]
                    ));
            }

            double min_cos = 1.0;

            if (perps.size() >= 2) {
                Vector sum = {0.0, 0.0, 0.0};

                for (const Vector& p : perps)
                    sum = sum + p;

                const Vector dir = sum.magnitude() > 1e-6 ? sum.normalized() : perps[0];
                min_cos = 1.0;

                for (const Vector& p : perps)
                    min_cos = std::min(min_cos, dir.dot(p));
            }

            const double exp_bonus = exposed_edges > 0 ? 1000.0 : 0.0;
            const double deg_bonus = -25.0 * static_cast<double>(perps.size());
            const double cos_bonus = 200.0 * min_cos;
            const double tie = 1e-4 * (facet.centroid[1] + 0.3 * facet.centroid[0]);
            const double score = exp_bonus + cos_bonus + deg_bonus + tie;

            if (score > best_score) {
                best_score = score;
                best_f = f;
            }
        }

        remaining[best_f] = false;
        _facets[best_f].assembly_step = step;
        _assembly_order[step] = best_f;
    }

    // Single insertion vector per facet: normalized sum of orthogonal vectors to earlier creases.
    for (size_t f = 0; f < fcount; f++) {
        const OrigamiFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        Vector sum = {0.0, 0.0, 0.0};

        for (size_t k = 0; k < m; k++) {
            const OrigamiEdge& e = _edges[static_cast<size_t>(facet.edge_ids[k])];

            if (e.is_boundary)
                continue;

            const size_t nb =
                static_cast<size_t>(e.facet_a == static_cast<int>(f) ? e.facet_b : e.facet_a);

            if (_facets[nb].assembly_step < facet.assembly_step)
                sum = sum + compute_inplane_outward(
                                facet, facet.miter_rings[0][k], facet.miter_rings[0][(k + 1) % m]
                            );
        }

        const Vector dir = sum.magnitude() > 1e-6 ? sum.normalized() : -facet.normal;
        _insertion_vectors[0][f] = dir;
        _insertion_vectors[1][f] = dir;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Shell plates, cavity border boards and scene groups
// ═══════════════════════════════════════════════════════════════════════════

void OrigamiShell::compute_shell_plates() {

    const size_t fcount = _facets.size();
    const std::array<double, 4> heights = layer_heights();
    std::vector<std::array<std::pair<Polyline, Polyline>, 2>> stock_rings(fcount);

    for (size_t f = 0; f < fcount; f++) {
        const std::vector<std::vector<Point>> empty_sides(_facets[f].vertices.size());

        for (size_t layer = 0; layer < 2; layer++)
            stock_rings[f][layer] = compute_origami_layer_rings(
                _vertices,
                _vertex_valences,
                _facets,
                _edges,
                _facets[f],
                _insertion_vectors[layer][f],
                2 * layer,
                2 * layer + 1,
                plate_thickness,
                total_offset,
                with_borders,
                corner_scale,
                empty_sides,
                empty_sides
            );
    }

    OrigamiJoineryTables tables;
    tables.plate_slots[0].resize(fcount);
    tables.plate_slots[1].resize(fcount);
    tables.cavity_pins.resize(fcount);

    for (size_t hi = 0; hi < 4; hi++) {
        tables.edge_polys[hi].resize(fcount);

        for (size_t f = 0; f < fcount; f++)
            tables.edge_polys[hi][f].resize(_facets[f].vertices.size());
    }

    if (with_joints) {
        std::vector<OrigamiCreaseFrame> creases;

        for (const OrigamiEdge& edge : _edges) {
            const std::optional<OrigamiCreaseFrame> cf = compute_origami_crease_frame(
                _facets, edge, _insertion_vectors[0], plate_thickness, total_offset
            );

            if (cf)
                creases.push_back(*cf);
        }

        for (size_t i = 1; i < creases.size(); i++) {
            const OrigamiCreaseFrame cur = creases[i];
            size_t j = i;

            while (
                j > 0 &&
                std::make_pair(_facets[cur.rec_f].assembly_step, _facets[cur.mal_f].assembly_step) <
                    std::make_pair(
                        _facets[creases[j - 1].rec_f].assembly_step,
                        _facets[creases[j - 1].mal_f].assembly_step
                    )) {
                creases[j] = creases[j - 1];
                j--;
            }

            creases[j] = cur;
        }

        for (const OrigamiCreaseFrame& cf : creases)
            apply_origami_crease_joinery(
                _facets,
                _edges,
                cf,
                heights,
                stock_rings,
                plate_thickness,
                total_offset,
                pin_pitch,
                tables
            );
    }

    _total_jointed_seams = tables.seams;
    _total_double_pins = tables.double_pins;
    _total_single_pins = tables.single_pins;

    for (size_t f = 0; f < fcount; f++) {
        const OrigamiFacet& facet = _facets[f];

        for (size_t layer = 0; layer < 2; layer++) {
            const size_t h_b = 2 * layer;
            const size_t h_t = 2 * layer + 1;
            const std::string plate_name =
                fmt::format("{}_plate_f{}", layer == 0 ? "inner" : "outer", f);
            const std::shared_ptr<FacePlanarElement> plate = std::make_shared<FacePlanarElement>(
                stock_rings[f][layer].first, stock_rings[f][layer].second, plate_name
            );

            if (with_joints) {
                const std::pair<Polyline, Polyline> jointed = compute_origami_layer_rings(
                    _vertices,
                    _vertex_valences,
                    _facets,
                    _edges,
                    facet,
                    _insertion_vectors[layer][f],
                    h_b,
                    h_t,
                    plate_thickness,
                    total_offset,
                    with_borders,
                    corner_scale,
                    tables.edge_polys[h_b][f],
                    tables.edge_polys[h_t][f]
                );
                attach_origami_features(
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

void OrigamiShell::compute_border_plates() {

    _facet_border_plates.assign(_facets.size(), {});

    for (size_t f = 0; f < _facets.size(); f++) {
        const OrigamiFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();

        for (size_t k = 0; k < m; k++) {
            const OrigamiEdge& edge = _edges[static_cast<size_t>(facet.edge_ids[k])];

            if (!edge.is_boundary)
                continue;

            const OrigamiEdge& prev_edge =
                _edges[static_cast<size_t>(facet.edge_ids[(k + m - 1) % m])];
            const OrigamiEdge& next_edge = _edges[static_cast<size_t>(facet.edge_ids[(k + 1) % m])];
            const Plane& bp = facet.side_planes[k];
            const Plane& sp = facet.side_planes[(k + m - 1) % m];
            const Plane& ep = facet.side_planes[(k + 1) % m];
            CavityBorderSpec spec;
            spec.facet_plane = Plane::from_point_normal(facet.centroid, facet.normal);
            spec.boundary_plane = bp;
            spec.start_plane = sp;
            spec.end_plane = ep;
            spec.inner_level = plate_thickness + 0.4;
            spec.outer_level = total_offset - plate_thickness - 0.4;
            spec.thickness = plate_thickness;
            spec.outer_gap = 0.0;
            spec.miter_start =
                prev_edge.is_boundary && bp.z_axis().cross(sp.z_axis()).magnitude() > 0.15;
            spec.start_neighbor_thickness = plate_thickness;
            spec.miter_end =
                next_edge.is_boundary && bp.z_axis().cross(ep.z_axis()).magnitude() > 0.15;
            spec.end_neighbor_thickness = plate_thickness;
            spec.start_setback = 0.5;
            spec.end_setback = 0.5;
            const std::optional<size_t> mate_s =
                find_boundary_knee_mate(_facets, _edges, f, k, facet.vertices[k]);
            const std::optional<size_t> mate_e =
                find_boundary_knee_mate(_facets, _edges, f, k, facet.vertices[(k + 1) % m]);

            if (mate_s) {
                const std::optional<Plane> bis = compute_origami_separation_plane(
                    _vertices[facet.vertices[k]], facet, _facets[*mate_s]
                );

                if (bis) {
                    spec.start_plane = *bis;
                    spec.miter_start = false;
                }
            }

            if (mate_e) {
                const std::optional<Plane> bis = compute_origami_separation_plane(
                    _vertices[facet.vertices[(k + 1) % m]], facet, _facets[*mate_e]
                );

                if (bis) {
                    spec.end_plane = *bis;
                    spec.miter_end = false;
                }
            }

            const std::string bname =
                fmt::format("border_{}_f{}_s{}", edge.on_ground ? "sill" : "edge", f, k);
            const std::shared_ptr<FacePlanarElement> board =
                compute_cavity_border_board(spec, bname);

            if (board) {
                _border_plates.push_back(board);
                _facet_border_plates[f].push_back(board);
                _all_plates.push_back(board);
            }
        }
    }
}

void OrigamiShell::populate_scene() {

    add_mesh(_base_mesh, add_group("base_mesh"));
    add_mesh(_inner_cavity_mesh, add_group("inner_cavity"));
    add_mesh(_outer_cavity_mesh, add_group("outer_cavity"));
    add_mesh(_outer_mesh, add_group("outer_surface"));
    add_mesh(_miter_planes_mesh, add_group("miter_planes"));

    const std::shared_ptr<TreeNode> inner_grp = add_group("inner_plates");
    const std::shared_ptr<TreeNode> outer_grp = add_group("outer_plates");
    const std::shared_ptr<TreeNode> border_grp = add_group("border_plates");

    for (const std::shared_ptr<FacePlanarElement>& p : _inner_plates)
        add(p, inner_grp);

    for (const std::shared_ptr<FacePlanarElement>& p : _outer_plates)
        add(p, outer_grp);

    for (const std::shared_ptr<FacePlanarElement>& b : _border_plates)
        add(b, border_grp);
}

}
