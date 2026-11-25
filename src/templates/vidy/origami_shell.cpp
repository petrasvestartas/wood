#include "pch.h"
#include "src/templates/cavity_border.h"
#include "src/templates/vidy/origami_shell.h"
#include "src/templates/vidy/vidy_joint_kernel.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

/// Four-point cavity footprint of a stepped Double_Pin shaft or Extend_NoPins shelf.
struct PinFootprint {
    Point quad[4];
};

/// Resolved 3D miter endpoints, insertion vectors and bevel stepbacks of one origami crease.
struct OrigamiCreaseFrame {
    std::array<Point, 4> ends_rec_in; // Receiver inner miter endpoints {b0, b1, t0, t1}.
    std::array<Point, 4> ends_rec_out; // Receiver outer miter endpoints {b0, b1, t0, t1}.
    std::array<Point, 4> ends_mal_in; // Male inner miter endpoints {b0, b1, t0, t1}.
    std::array<Point, 4> ends_mal_out; // Male outer miter endpoints {b0, b1, t0, t1}.
    Point ref_b; // Shared inner overlap midpoint.
    Point ref_t; // Shared outer overlap midpoint.
    Vector v_ins_rec; // In-plane outward direction of receiver across the crease.
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
    double skew_deg = 0.0; // Robeller insertion skew angle theta in degrees.
};

/// Per-ring directed edge profiles, per-layer mortises and pin counters across all facets.
struct OrigamiJoineryTables {
    std::array<std::vector<std::vector<std::vector<Point>>>, 4> edge_polys; // [ring][facet][side].
    std::array<std::vector<std::vector<VidyMortiseSlot>>, 2> plate_slots; // [layer][facet].
    std::vector<std::vector<PinFootprint>> cavity_pins; // [facet] Double_Pin / shelf footprints.
    size_t seams = 0;
    size_t double_pins = 0;
    size_t single_pins = 0;
    double max_skew_deg = 0.0;
};

/// Centroid, unit normal and area of a polygon vertex index loop.
std::tuple<Point, Vector, double>
compute_polygon_frame(const std::vector<Point>& pts, const std::vector<size_t>& loop) {

    std::vector<Point> ring;
    ring.reserve(loop.size());

    for (const size_t vid : loop)
        ring.push_back(pts[vid]);

    const Vector n_raw = compute_newell(ring);
    const double len = n_raw.magnitude();

    return {Point::centroid(ring), len > 1e-12 ? (n_raw / len) : Vector{0.0, 0.0, 1.0}, 0.5 * len};
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

    for (size_t f = 0; f < faces.size(); f++)
        for (size_t k = 0; k < faces[f].size(); k++) {
            const size_t u = faces[f][k];
            const size_t v = faces[f][(k + 1) % faces[f].size()];
            adj[std::minmax(u, v)].push_back({f, u < v});
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

        double score = 0.0;

        for (const size_t f : comp) {
            if (flipped[f])
                std::reverse(faces[f].begin(), faces[f].end());

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
    const Vector natural_out = vidy_inplane_outward(facet.centroid, facet.normal, pu, pv);
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

/// Outward separation plane through corner_vertex oriented from facet's sector toward other's.
std::optional<Plane> compute_origami_separation_plane(
    const Point& corner_vertex, const OrigamiFacet& facet, const OrigamiFacet& other
) {

    const Vector sector_dir = (other.centroid - corner_vertex).normalized() -
                              (facet.centroid - corner_vertex).normalized();
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

/// Counts vertices shared by facets fa and fb.
size_t count_shared_vertices(const OrigamiFacet& fa, const OrigamiFacet& fb) {

    size_t shared = 0;

    for (const size_t u : fa.vertices)
        for (const size_t v : fb.vertices)
            if (u == v)
                shared++;

    return shared;
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
        if (f == facet_idx || count_shared_vertices(facet, facets[f]) != 1)
            continue;

        const OrigamiFacet& other = facets[f];
        const size_t om = other.vertices.size();

        for (size_t ok = 0; ok < om; ok++) {
            if (!edges[static_cast<size_t>(other.edge_ids[ok])].is_boundary)
                continue;

            if (other.vertices[ok] != vertex_id && other.vertices[(ok + 1) % om] != vertex_id)
                continue;

            if (bp.z_axis().dot(other.side_planes[ok].z_axis()) > 0.96)
                return f;
        }
    }

    return std::nullopt;
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
    const Point& partner_apex = facet.miter_rings[h_partner][k];
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

        const bool shares_vid =
            std::find(other.vertices.begin(), other.vertices.end(), vid) != other.vertices.end();

        if (shares_vid && count_shared_vertices(facet, other) == 1) {
            const std::optional<Plane> bis =
                compute_origami_separation_plane(vertices[vid], facet, other);

            if (bis) {
                const std::pair<double, double> ps =
                    vidy_plane_corner_snips(apex, ua, ub, *bis, 1.5, 0.02, max_s);
                snip_a = std::max(snip_a, ps.first * factor);
                snip_b = std::max(snip_b, ps.second * factor);

                if (other.assembly_step < facet.assembly_step &&
                    (-v_ins).dot(bis->z_axis()) > 0.01) {
                    const Vector to_other = (other.centroid - vertices[vid]).normalized();

                    for (const double d : {8.0, 25.0, 75.0, 125.0}) {
                        const Point apex_d = apex - v_ins * d;

                        if ((apex_d - vertices[vid]).dot(to_other) < -0.75 * total_offset)
                            continue;

                        const std::pair<double, double> sd_s =
                            vidy_plane_corner_snips(apex_d, ua, ub, *bis, 1.8, 0.02, max_s);
                        snip_a = std::max(snip_a, sd_s.first * factor);
                        snip_b = std::max(snip_b, sd_s.second * factor);
                    }
                }
            }
        }

        const size_t om = other.vertices.size();

        for (size_t ok = 0; ok < om; ok++) {
            if (!edges[static_cast<size_t>(other.edge_ids[ok])].is_boundary)
                continue;

            if (other.vertices[ok] != vid && other.vertices[(ok + 1) % om] != vid)
                continue;

            const Plane& bp = other.side_planes[ok];
            bool same_datum = false;

            for (size_t sk = 0; sk < m; sk++)
                if (edges[static_cast<size_t>(facet.edge_ids[sk])].is_boundary &&
                    bp.z_axis().dot(facet.side_planes[sk].z_axis()) > 0.96)
                    same_datum = true;

            if (same_datum)
                continue;

            const std::pair<double, double> env_s =
                vidy_plane_corner_snips(apex, ua, ub, bp, 0.8, 0.02, max_s);
            snip_a = std::max(snip_a, env_s.first * factor);
            snip_b = std::max(snip_b, env_s.second * factor);

            if (!with_borders || !find_boundary_knee_mate(facets, edges, other.index, ok, vid))
                continue;

            const double oh1 = (apex - vertices[vid]).dot(other.normal);
            const double oh2 = (partner_apex - vertices[vid]).dot(other.normal);

            if (std::max(oh1, oh2) >= plate_thickness - 0.2 &&
                std::min(oh1, oh2) <= total_offset - plate_thickness + 0.2) {
                const Plane inner_bp = bp.translate_by_normal(-(plate_thickness + 1.5));
                const std::pair<double, double> brd_s =
                    vidy_plane_corner_snips(apex, ua, ub, inner_bp, 0.0, 0.02, max_s);
                snip_a = std::max(snip_a, brd_s.first * factor);
                snip_b = std::max(snip_b, brd_s.second * factor);
            }
        }
    }

    return {snip_a, snip_b};
}

/// Builds paired bottom/top closed plate rings with sharp corners where snips are 0.
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
    std::vector<std::pair<double, double>> snips_b(m);
    std::vector<std::pair<double, double>> snips_t(m);

    for (size_t k = 0; k < m; k++) {
        snips_b[k] = compute_origami_corner_snips(
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
        snips_t[k] = compute_origami_corner_snips(
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
    }

    return vidy_stitch_layer_rings(
        facet.miter_rings, facet.normal, h_bot, h_top, snips_b, snips_t, prof_bot, prof_top
    );
}

/// True when pt clears every segment of closed_ring (optionally skipping collinear crease_line).
bool point_clears_ring(
    const Point& pt,
    const Polyline& closed_ring,
    const Vector& normal,
    double min_clear,
    const Line* skip_line = nullptr
) {

    const size_t n = closed_ring.point_count();

    if (n < 4)
        return false;

    const double orient = compute_newell(closed_ring.get_points()).dot(normal) >= 0.0 ? 1.0 : -1.0;

    for (size_t i = 0; i + 1 < n; i++) {
        const Point& a = closed_ring[i];
        const Point& b = closed_ring[i + 1];
        const Vector edge = b - a;

        if (edge.magnitude() < 1e-6)
            continue;

        if (skip_line && skip_line->closest_point(a, false).second.distance(a) < 1.5 &&
            skip_line->closest_point(b, false).second.distance(b) < 1.5)
            continue;

        const Vector inward = (normal.cross(edge.normalized()) * orient).normalized();

        if ((pt - a).dot(inward) < min_clear)
            return false;

        if (!skip_line &&
            Line::from_points(a, b).closest_point(pt, true).second.distance(pt) < min_clear)
            return false;
    }

    return true;
}

/// True when pt clears all non-crease sides of facet (including pocket stepback on interior sides).
bool point_clears_facet_sides(
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

        if ((b - a).magnitude() < 1e-6)
            continue;

        const Vector inward = -vidy_inplane_outward(facet.centroid, facet.normal, a, b);
        const OrigamiEdge& ed = edges[static_cast<size_t>(facet.edge_ids[k])];
        const double pocket =
            ed.is_boundary ? 0.0
                           : (2.0 * plate_thickness / std::max(0.25, std::sin(ed.dihedral_rad)));

        if ((pt - a).dot(inward) < pocket + 0.65 * plate_thickness)
            return false;
    }

    return true;
}

/// True when slot clears all required edges, stock chamfers and prior/local mortise slots.
bool slot_clears_plate(
    const std::vector<OrigamiEdge>& edges,
    const VidyMortiseSlot& slot,
    const OrigamiFacet& facet,
    size_t layer,
    size_t crease_side,
    bool has_extend_tab,
    const Polyline& bot_stock,
    const Polyline& top_stock,
    const std::vector<VidyMortiseSlot>& existing_slots,
    const std::vector<VidyMortiseSlot>& local_slots,
    double plate_thickness,
    double min_edge_clear,
    double min_slot_gap
) {

    const size_t rb = 2 * layer;
    const size_t rt = 2 * layer + 1;
    const size_t m = facet.miter_rings[rb].size();
    const Line crease_b = Line::from_points(
        facet.miter_rings[rb][crease_side], facet.miter_rings[rb][(crease_side + 1) % m]
    );
    const Line crease_t = Line::from_points(
        facet.miter_rings[rt][crease_side], facet.miter_rings[rt][(crease_side + 1) % m]
    );

    for (size_t c = 0; c < 4; c++) {
        if (!point_clears_facet_sides(
                edges, slot.bottom[c], facet, rb, crease_side, has_extend_tab, plate_thickness
            ) ||
            !point_clears_facet_sides(
                edges, slot.top[c], facet, rt, crease_side, has_extend_tab, plate_thickness
            ))
            return false;

        if (!point_clears_ring(
                slot.bottom[c],
                bot_stock,
                facet.normal,
                min_edge_clear,
                has_extend_tab ? &crease_b : nullptr
            ) ||
            !point_clears_ring(
                slot.top[c],
                top_stock,
                facet.normal,
                min_edge_clear,
                has_extend_tab ? &crease_t : nullptr
            ))
            return false;
    }

    for (const VidyMortiseSlot& prev : existing_slots)
        if (vidy_quads_closer_than_2d(slot.bottom, prev.bottom, facet.normal, min_slot_gap) ||
            vidy_quads_closer_than_2d(slot.top, prev.top, facet.normal, min_slot_gap))
            return false;

    for (const VidyMortiseSlot& prev : local_slots)
        if (vidy_quads_closer_than_2d(slot.bottom, prev.bottom, facet.normal, min_slot_gap) ||
            vidy_quads_closer_than_2d(slot.top, prev.top, facet.normal, min_slot_gap))
            return false;

    return true;
}

/// True when every corner of slot stays >= 10 mm inside rec_ends' longitudinal tab span.
bool slot_within_extend_span(
    const VidyMortiseSlot& slot,
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
    const Point m_t0 = vidy_align_top_ref(m_b0, rec_ends[2], rec_ends[3], nf, v_ins_rec);
    const double lim = 0.5 * static_cast<double>(cnt) * pitch + rec_extra - 10.0;

    for (size_t c = 0; c < 4; c++)
        if (std::abs((slot.bottom[c] - m_b0).dot(nf)) > lim ||
            std::abs((slot.top[c] - m_t0).dot(nf)) > lim)
            return false;

    return true;
}

/// Swept four-point Double_Pin footprint covering retraction (-125 mm) through full tip reach.
PinFootprint make_double_pin_footprint(
    const std::pair<Point, Point>& base_b, const Vector& v_ins, double le_tab, double le_offs
) {

    return {
        {base_b.first - v_ins * 125.0,
         base_b.second - v_ins * 125.0,
         base_b.second + v_ins * (le_offs + 2.1 * le_tab),
         base_b.first + v_ins * (le_offs + 2.1 * le_tab)}
    };
}

/// Four-point cavity footprint of an Extend_NoPins shelf projecting into the male facet's cavity.
PinFootprint make_extend_shelf_footprint(
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
        {m_b0 - nf * span,
         m_b0 + nf * span,
         (m_b0 + nf * span) + v_ins_rec * l_offs_rec,
         (m_b0 - nf * span) + v_ins_rec * l_offs_rec}
    };
}

/// Largest pin count in 1..max_count matching required_parity whose slots and shafts keep clearance.
int find_safe_pin_count(
    const std::vector<OrigamiEdge>& edges,
    const VidyPinBand& proto,
    const std::array<Point, 4>& rec_ends,
    const Vector& v_ins_rec,
    double l_offs_rec,
    double rec_extra,
    int max_count,
    int required_parity,
    bool is_double_pin,
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
    const Point m_t0 = vidy_align_top_ref(m_b0, proto.ends[2], proto.ends[3], nf, proto.v_ins);
    const Point pm_b = m_b0 - proto.v_ins * (proto.step_bot ? proto.le_tab + 0.5 : 0.5);
    const Point pm_t = m_t0 - proto.v_ins * (proto.step_bot ? 0.5 : proto.le_tab + 0.5);
    const size_t opp_layer = 1 - same_layer;
    const double min_slot_gap = std::max(45.0, 0.22 * proto.pitch);

    for (int cnt = max_count; cnt >= 1; cnt--) {
        if (required_parity >= 0 && (cnt % 2) != required_parity)
            continue;

        const double f_off = vidy_first_pin_offset(cnt, proto.pitch);
        const Point ptc_b = pm_b - nf * f_off;
        const Point ptc_t = pm_t - nf * f_off;
        std::vector<VidyMortiseSlot> local_same;
        std::vector<VidyMortiseSlot> local_opp;
        bool ok = true;

        if (is_double_pin) {
            const PinFootprint shelf_fp = make_extend_shelf_footprint(
                rec_ends, proto.ref_pt, v_ins_rec, l_offs_rec, cnt, proto.pitch, rec_extra
            );

            for (const PinFootprint& fp : tables.cavity_pins[mal_facet.index])
                if (vidy_quads_closer_than_2d(shelf_fp.quad, fp.quad, mal_facet.normal, 24.0))
                    ok = false;
        }

        for (int i = 0; i < cnt && ok; i++) {
            const double step = proto.pitch * static_cast<double>(i);

            if (is_double_pin) {
                std::pair<Point, Point> bb;
                std::pair<Point, Point> tb;
                std::pair<Point, Point> bt;
                std::pair<Point, Point> tt;
                vidy_double_pin(
                    ptc_b + nf * step,
                    nf,
                    proto.v_ins,
                    proto.le_tab,
                    proto.le_offs,
                    proto.scale,
                    bb,
                    tb
                );
                vidy_double_pin(
                    ptc_t + nf * step,
                    nf,
                    proto.v_ins,
                    proto.le_tab,
                    proto.le_offs,
                    proto.scale,
                    bt,
                    tt
                );
                const VidyMortiseSlot s_same = vidy_project_facet_slot(
                    bb,
                    bt,
                    proto.v_ins,
                    rec_facet.centroid,
                    rec_facet.normal,
                    heights[2 * same_layer],
                    heights[2 * same_layer + 1]
                );
                const VidyMortiseSlot s_opp = vidy_project_facet_slot(
                    tb,
                    tt,
                    proto.v_ins,
                    rec_facet.centroid,
                    rec_facet.normal,
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
                         min_edge_clear,
                         min_slot_gap
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
                         min_edge_clear,
                         min_slot_gap
                     );
                local_same.push_back(s_same);
                local_opp.push_back(s_opp);
                const PinFootprint pin_fp =
                    make_double_pin_footprint(bb, proto.v_ins, proto.le_tab, proto.le_offs);

                for (const PinFootprint& fp : tables.cavity_pins[rec_facet.index])
                    if (vidy_quads_closer_than_2d(pin_fp.quad, fp.quad, rec_facet.normal, 24.0))
                        ok = false;
            } else {
                std::pair<Point, Point> sb;
                std::pair<Point, Point> st;
                vidy_single_pin(ptc_b + nf * step, nf, proto.v_ins, proto.le_tab, proto.scale, sb);
                vidy_single_pin(ptc_t + nf * step, nf, proto.v_ins, proto.le_tab, proto.scale, st);
                const VidyMortiseSlot s_same = vidy_project_facet_slot(
                    sb,
                    st,
                    proto.v_ins,
                    rec_facet.centroid,
                    rec_facet.normal,
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
                         min_edge_clear,
                         min_slot_gap
                     );
                local_same.push_back(s_same);
            }
        }

        if (ok)
            return cnt;
    }

    return 0;
}

/// Resolves 3D miter endpoints, overlap midpoint and bevel stepbacks for one interior edge.
std::optional<OrigamiCreaseFrame> compute_origami_crease_frame(
    const std::vector<OrigamiFacet>& facets,
    const OrigamiEdge& edge,
    const std::vector<Vector>& ins_vecs,
    double plate_thickness,
    double total_offset
) {

    if (edge.is_boundary || edge.facet_b < 0 || edge.dihedral_rad < 0.65 ||
        edge.dihedral_rad > 2.53)
        return std::nullopt;

    const size_t fa = static_cast<size_t>(edge.facet_a);
    const size_t fb = static_cast<size_t>(edge.facet_b);
    const bool a_first = facets[fa].assembly_step < facets[fb].assembly_step;
    OrigamiCreaseFrame cf;
    cf.rec_f = a_first ? fa : fb;
    cf.mal_f = a_first ? fb : fa;
    cf.positive_fold = edge.positive_fold;
    const OrigamiFacet& rec = facets[cf.rec_f];
    const OrigamiFacet& mal = facets[cf.mal_f];
    cf.rec_s = vidy_find_shared_side(rec.vertices, edge.u, edge.v);
    cf.mal_s = vidy_find_shared_side(mal.vertices, edge.u, edge.v);
    const size_t rm = rec.vertices.size();
    const size_t mm = mal.vertices.size();
    cf.ends_rec_in = {
        rec.miter_rings[0][cf.rec_s],
        rec.miter_rings[0][(cf.rec_s + 1) % rm],
        rec.miter_rings[1][cf.rec_s],
        rec.miter_rings[1][(cf.rec_s + 1) % rm]
    };
    cf.ends_rec_out = {
        rec.miter_rings[2][cf.rec_s],
        rec.miter_rings[2][(cf.rec_s + 1) % rm],
        rec.miter_rings[3][cf.rec_s],
        rec.miter_rings[3][(cf.rec_s + 1) % rm]
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
    const Vector axis_in = (cf.ends_rec_in[1] - cf.ends_rec_in[0]).normalized();
    const Vector axis_out = (cf.ends_rec_out[1] - cf.ends_rec_out[0]).normalized();
    const double r_in1 = (cf.ends_rec_in[1] - cf.ends_rec_in[0]).dot(axis_in);
    const double m_in0 = (cf.ends_mal_in[0] - cf.ends_rec_in[0]).dot(axis_in);
    const double m_in1 = (cf.ends_mal_in[1] - cf.ends_rec_in[0]).dot(axis_in);
    const double lo_in = std::max(0.0, std::min(m_in0, m_in1));
    const double hi_in = std::min(r_in1, std::max(m_in0, m_in1));
    const double r_out1 = (cf.ends_rec_out[1] - cf.ends_rec_out[0]).dot(axis_out);
    const double m_out0 = (cf.ends_mal_out[0] - cf.ends_rec_out[0]).dot(axis_out);
    const double m_out1 = (cf.ends_mal_out[1] - cf.ends_rec_out[0]).dot(axis_out);
    const double lo_out = std::max(0.0, std::min(m_out0, m_out1));
    const double hi_out = std::min(r_out1, std::max(m_out0, m_out1));

    if (hi_in - lo_in < 120.0 || hi_out - lo_out < 120.0)
        return std::nullopt;

    cf.ref_b = cf.ends_rec_in[0] + axis_in * (0.5 * (lo_in + hi_in));
    cf.ref_t = cf.ends_rec_out[0] + axis_out * (0.5 * (lo_out + hi_out));
    cf.min_len = std::min(
        {hi_in - lo_in,
         hi_out - lo_out,
         cf.ends_rec_in[2].distance(cf.ends_rec_in[3]),
         cf.ends_mal_in[2].distance(cf.ends_mal_in[3])}
    );
    cf.v_ins_rec =
        vidy_inplane_outward(rec.centroid, rec.normal, cf.ends_rec_in[0], cf.ends_rec_in[1]);
    cf.v_ins_mal = ins_vecs[cf.mal_f];
    const Vector perp_mal =
        vidy_inplane_outward(mal.centroid, mal.normal, cf.ends_mal_in[0], cf.ends_mal_in[1]);
    const double cos_mal = cf.v_ins_mal.dot(perp_mal);

    // Robeller (EPFL Thesis 6564, 2015): 1DOF simultaneous multi-edge skew |theta| <= ~36.8 deg.
    if (cos_mal < 0.80)
        return std::nullopt;

    cf.skew_deg = std::acos(std::clamp(cos_mal, -1.0, 1.0)) * Tolerance::TO_DEGREES;
    const double sin_alpha = std::max(0.18, std::sin(edge.dihedral_rad));
    cf.l_tab_rec = plate_thickness / sin_alpha;
    cf.l_offs_rec = (total_offset - plate_thickness) / sin_alpha;
    cf.l_tab_mal = cf.l_tab_rec / cos_mal;
    cf.l_offs_mal = cf.l_offs_rec / cos_mal;

    return cf;
}

/// Minimum sin(angle) across both interior facet corners incident to side_idx.
double min_crease_corner_sin(const OrigamiFacet& facet, size_t side_idx) {

    const std::vector<Point>& r = facet.miter_rings[0];
    const size_t m = r.size();
    const Vector e_prev = (r[(side_idx + m - 1) % m] - r[side_idx]).normalized();
    const Vector e_curr = (r[(side_idx + 1) % m] - r[side_idx]).normalized();
    const Vector e_next = (r[(side_idx + 2) % m] - r[(side_idx + 1) % m]).normalized();

    return std::max(
        0.25, std::min(e_prev.cross(e_curr).magnitude(), (-e_curr).cross(e_next).magnitude())
    );
}

/// Populates staggered Double_Pin, Single_Pin, Extend_NoPins and Extend contours on one crease.
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

    const double scale = std::clamp(plate_thickness / 45.0, 0.80, 1.25);
    const OrigamiFacet& rec = facets[cf.rec_f];
    const OrigamiFacet& mal = facets[cf.mal_f];
    const double c_sin =
        std::min(min_crease_corner_sin(rec, cf.rec_s), min_crease_corner_sin(mal, cf.mal_s));
    const double corner_margin = std::max(0.55 * pitch, 1.40 * total_offset) / c_sin;

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
    const double min_clear = std::max(45.0, 1.05 * plate_thickness);
    const size_t dp_layer = cf.positive_fold ? 0 : 1;
    const size_t sp_layer = 1 - dp_layer;
    const std::array<Point, 4>& mal_dp_ends = cf.positive_fold ? cf.ends_mal_in : cf.ends_mal_out;
    const std::array<Point, 4>& mal_sp_ends = cf.positive_fold ? cf.ends_mal_out : cf.ends_mal_in;
    const std::array<Point, 4>& rec_sh_ends = cf.positive_fold ? cf.ends_rec_in : cf.ends_rec_out;
    const std::array<Point, 4>& rec_ex_ends = cf.positive_fold ? cf.ends_rec_out : cf.ends_rec_in;
    const Point& dp_ref = cf.positive_fold ? cf.ref_b : cf.ref_t;
    const Point& sp_ref = cf.positive_fold ? cf.ref_t : cf.ref_b;
    const double av_dp = std::min(
        vidy_band_half_span(mal_dp_ends, dp_ref, cf.v_ins_mal, corner_margin) - dp_extra,
        vidy_band_half_span(rec_sh_ends, dp_ref, cf.v_ins_rec, corner_margin) - rec_extra
    );
    const double av_sp = std::min(
        vidy_band_half_span(mal_sp_ends, sp_ref, cf.v_ins_mal, corner_margin) - sp_extra,
        vidy_band_half_span(rec_ex_ends, sp_ref, cf.v_ins_rec, corner_margin) - rec_extra
    );
    const int raw_max_dp = std::max(0, static_cast<int>(std::floor(2.0 * av_dp / pitch)));
    const int max_sp =
        std::min(max_cnt, std::max(0, static_cast<int>(std::floor(2.0 * av_sp / pitch))));
    VidyPinBand sp_band = {
        mal_sp_ends,
        sp_ref,
        cf.v_ins_mal,
        cf.l_tab_mal,
        0.0,
        !cf.positive_fold,
        0,
        pitch,
        sp_extra,
        0.0,
        scale
    };
    const int n_sp = max_sp > 0 ? find_safe_pin_count(
                                      edges,
                                      sp_band,
                                      rec_ex_ends,
                                      cf.v_ins_rec,
                                      0.0,
                                      rec_extra,
                                      max_sp,
                                      -1,
                                      false,
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

    if (n_sp > 0) {
        sp_band.count = n_sp;
        const VidyPinBand ex_band = {
            rec_ex_ends,
            sp_ref,
            cf.v_ins_rec,
            cf.l_tab_rec,
            0.0,
            cf.positive_fold,
            n_sp,
            pitch,
            rec_extra,
            0.0,
            scale
        };
        const std::array<std::vector<Point>, 2> sp_pair = vidy_emit_single_pins(
            sp_band,
            rec.centroid,
            rec.normal,
            heights[2 * sp_layer],
            heights[2 * sp_layer + 1],
            tables.plate_slots[sp_layer][cf.rec_f]
        );
        const std::array<std::vector<Point>, 2> ex_pair = vidy_emit_extend_lip(ex_band, false);
        tables.edge_polys[2 * sp_layer][cf.mal_f][cf.mal_s] = sp_pair[0];
        tables.edge_polys[2 * sp_layer + 1][cf.mal_f][cf.mal_s] = sp_pair[1];
        tables.edge_polys[2 * sp_layer][cf.rec_f][cf.rec_s] = ex_pair[0];
        tables.edge_polys[2 * sp_layer + 1][cf.rec_f][cf.rec_s] = ex_pair[1];
        tables.single_pins += static_cast<size_t>(n_sp);
    }

    VidyPinBand dp_band = {
        mal_dp_ends,
        dp_ref,
        cf.v_ins_mal,
        cf.l_tab_mal,
        cf.l_offs_mal,
        !cf.positive_fold,
        0,
        pitch,
        dp_extra,
        0.0,
        scale
    };
    const int max_dp =
        n_sp >= 2 ? std::min(raw_max_dp, n_sp - 1) : std::min(raw_max_dp, std::max(1, n_sp));
    const int dp_parity = n_sp >= 2 ? ((n_sp + 1) % 2) : -1;
    const int n_dp = max_dp > 0 ? find_safe_pin_count(
                                      edges,
                                      dp_band,
                                      rec_sh_ends,
                                      cf.v_ins_rec,
                                      cf.l_offs_rec,
                                      rec_extra,
                                      max_dp,
                                      dp_parity,
                                      true,
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

    if (n_dp <= 0 && n_sp <= 0)
        return;

    if (n_dp > 0) {
        dp_band.count = n_dp;
        const VidyPinBand sh_band = {
            rec_sh_ends,
            dp_ref,
            cf.v_ins_rec,
            cf.l_tab_rec,
            cf.l_offs_rec,
            !cf.positive_fold,
            n_dp,
            pitch,
            rec_extra,
            0.0,
            scale
        };
        const std::array<std::vector<Point>, 2> dp_pair = vidy_emit_double_pins(
            dp_band,
            rec.centroid,
            rec.normal,
            heights,
            dp_layer,
            tables.plate_slots[dp_layer][cf.rec_f],
            tables.plate_slots[1 - dp_layer][cf.rec_f]
        );
        const std::array<std::vector<Point>, 2> sh_pair = vidy_emit_extend_lip(sh_band, true);
        const Vector nf_dp = (dp_band.ends[1] - dp_band.ends[0]).normalized();
        const Point m_b0 = Line::from_points(dp_band.ends[0], dp_band.ends[1])
                               .closest_point(dp_band.ref_pt, false)
                               .second;
        const Point pm_b = m_b0 - dp_band.v_ins * (dp_band.step_bot ? dp_band.le_tab + 0.5 : 0.5);
        const double f_off = vidy_first_pin_offset(n_dp, pitch);

        for (int i = 0; i < n_dp; i++) {
            const Point pc = (pm_b - nf_dp * f_off) + nf_dp * (pitch * static_cast<double>(i));
            tables.cavity_pins[cf.rec_f].push_back(make_double_pin_footprint(
                {pc - nf_dp * (135.0 * scale), pc + nf_dp * (135.0 * scale)},
                dp_band.v_ins,
                dp_band.le_tab,
                dp_band.le_offs
            ));
        }

        tables.cavity_pins[cf.mal_f].push_back(make_extend_shelf_footprint(
            rec_sh_ends, dp_ref, cf.v_ins_rec, cf.l_offs_rec, n_dp, pitch, rec_extra
        ));
        tables.edge_polys[2 * dp_layer][cf.mal_f][cf.mal_s] = dp_pair[0];
        tables.edge_polys[2 * dp_layer + 1][cf.mal_f][cf.mal_s] = dp_pair[1];
        tables.edge_polys[2 * dp_layer][cf.rec_f][cf.rec_s] = sh_pair[0];
        tables.edge_polys[2 * dp_layer + 1][cf.rec_f][cf.rec_s] = sh_pair[1];
        tables.double_pins += static_cast<size_t>(n_dp);
    }

    tables.seams++;
    tables.max_skew_deg = std::max(tables.max_skew_deg, cf.skew_deg);
}

}

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
      pin_pitch(
          std::clamp(
              pin_pitch,
              450.0 * std::clamp(std::max(12.0, plate_thickness) / 45.0, 0.80, 1.25),
              1200.0
          )
      ),
      corner_scale(std::clamp(corner_scale, 0.0, 2.0)) {

    extract_oriented_facets(input_mesh);
    compute_edges();
    compute_miter_rings();
    _inner_cavity_mesh = compute_offset_mesh(1);
    _outer_cavity_mesh = compute_offset_mesh(2);
    _outer_mesh = compute_offset_mesh(3);
    _miter_planes_mesh = compute_miter_planes_mesh();

    compute_assembly_and_vectors();
    compute_shell_plates();

    if (this->with_borders)
        compute_border_plates();

    populate_scene();

    // assembly: facet by facet in the order the insertion vectors were found for, the inner plate, its cavity boards, the outer plate
    std::vector<AssemblyModule> parts;
    for (size_t layer = 0; layer < 2; layer++) {
        for (const size_t facet : _assembly_order)
            parts.push_back(AssemblyModule::part(layer == 0 ? _inner_plates[facet] : _outer_plates[facet], {_insertion_vectors[layer][facet]}));
        if (layer == 0)
            for (const std::shared_ptr<FacePlanarElement>& board : _border_plates)
                parts.push_back(AssemblyModule::part(board));
    }
    assemblies = {AssemblyModule::module(name, parts)};
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

double OrigamiShell::max_insertion_skew_deg() const {
    return _max_insertion_skew_deg;
}

size_t OrigamiShell::unjoined_interior_creases() const {
    return _unjoined_interior_creases;
}

void OrigamiShell::copy_plates_to(
    WoodSession& target, const std::string& group_name, const Vector& delta
) const {

    const std::shared_ptr<TreeNode> grp = target.add_group(group_name);

    std::map<std::string, std::string> copies;

    for (const std::shared_ptr<FacePlanarElement>& plate : _all_plates) {
        std::shared_ptr<FacePlanarElement> copy =
            std::dynamic_pointer_cast<FacePlanarElement>(plate->clone());
        copies[plate->guid()] = copy->guid();

        if (!delta.is_zero())
            copy->place(Xform::translation(delta[0], delta[1], delta[2]));

        target.add(copy, grp);
    }

    for (const AssemblyModule& whole : assemblies)
        target.assemblies.push_back(whole.of_copies(copies));
}

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
            const auto [it, inserted] = lookup.emplace(std::minmax(u, v), _edges.size());

            if (inserted) {
                OrigamiEdge edge;
                edge.u = u;
                edge.v = v;
                edge.facet_a = static_cast<int>(f);
                edge.side_a = static_cast<int>(k);
                _edges.push_back(edge);
            } else {
                _edges[it->second].facet_b = static_cast<int>(f);
                _edges[it->second].side_b = static_cast<int>(k);
            }

            facet.edge_ids[k] = static_cast<int>(it->second);
        }
    }

    for (OrigamiEdge& edge : _edges) {
        const Point& pu = _vertices[edge.u];
        const Point& pv = _vertices[edge.v];
        const OrigamiFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];

        if (edge.facet_b < 0) {
            edge.is_boundary = true;
            edge.on_ground = has_rise && std::abs(pu[2] - _ground_z) < 1.5 &&
                             std::abs(pv[2] - _ground_z) < 1.5 && std::abs(fa.normal[2]) < 0.88;
            continue;
        }

        const OrigamiFacet& fb = _facets[static_cast<size_t>(edge.facet_b)];
        edge.is_boundary = false;
        edge.positive_fold = (fb.centroid - fa.centroid).dot(fa.normal) < 0.0;
        edge.dihedral_rad = std::acos(std::clamp(fa.normal.dot(fb.normal), -1.0, 1.0));
        edge.tool_tilt_deg =
            std::abs(0.5 * (180.0 - edge.dihedral_rad * Tolerance::TO_DEGREES) - 90.0);
    }
}

void OrigamiShell::compute_miter_rings() {

    const std::array<double, 4> heights = layer_heights();
    const double max_drift = 4.5 * total_offset;

    for (size_t f = 0; f < _facets.size(); f++) {
        OrigamiFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.side_planes.resize(m);

        for (size_t k = 0; k < m; k++)
            facet.side_planes[k] = compute_origami_side_plane(
                _vertices,
                _facets,
                f,
                _edges[static_cast<size_t>(facet.edge_ids[k])],
                facet.vertices[k],
                facet.vertices[(k + 1) % m],
                _ground_z,
                _plan_bounds
            );

        for (size_t hi = 0; hi < 4; hi++) {
            const Plane offset_plane =
                Plane::from_point_normal(facet.centroid + facet.normal * heights[hi], facet.normal);
            facet.miter_rings[hi].resize(m);

            for (size_t k = 0; k < m; k++) {
                const Point fallback = _vertices[facet.vertices[k]] + facet.normal * heights[hi];
                const std::optional<Point> hit = Intersection::plane_plane_plane(
                    offset_plane, facet.side_planes[(k + m - 1) % m], facet.side_planes[k]
                );
                facet.miter_rings[hi][k] =
                    (hit && hit->distance(fallback) <= max_drift) ? *hit : fallback;
            }
        }
    }

    for (OrigamiEdge& edge : _edges)
        edge.miter_plane = _facets[static_cast<size_t>(edge.facet_a)]
                               .side_planes[static_cast<size_t>(edge.side_a)];
}

Mesh OrigamiShell::compute_offset_mesh(size_t ring_index) const {

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> faces;

    for (const OrigamiFacet& facet : _facets) {
        std::vector<size_t> loop;

        for (const Point& pt : facet.miter_rings[ring_index]) {
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

    const std::array<Vector, 4> sweep_axes = {
        Vector(0.0, 1.0, 0.0), Vector(0.0, -1.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.0)
    };
    double best_global_min_cos = -2.0;

    for (const Vector& axis : sweep_axes) {
        std::vector<bool> remaining(fcount, true);
        std::vector<size_t> trial_order(fcount, 0);
        double worst_cos = 1.0;

        for (size_t rev = 0; rev < fcount; rev++) {
            const size_t step = fcount - 1 - rev;
            size_t best_f = 0;
            double best_score = -1e18;
            double chosen_cos = 1.0;

            for (size_t f = 0; f < fcount; f++) {
                if (!remaining[f])
                    continue;

                const OrigamiFacet& facet = _facets[f];
                const size_t m = facet.vertices.size();
                size_t exposed = 0;
                std::vector<Vector> perps;

                for (size_t k = 0; k < m; k++) {
                    const OrigamiEdge& e = _edges[static_cast<size_t>(facet.edge_ids[k])];

                    if (e.is_boundary) {
                        exposed++;
                        continue;
                    }

                    const size_t nb = static_cast<size_t>(
                        e.facet_a == static_cast<int>(f) ? e.facet_b : e.facet_a
                    );

                    if (!remaining[nb])
                        exposed++;
                    else
                        perps.push_back(vidy_inplane_outward(
                            facet.centroid,
                            facet.normal,
                            facet.miter_rings[0][k],
                            facet.miter_rings[0][(k + 1) % m]
                        ));
                }

                if (exposed == 0 && rev > 0)
                    continue;

                double min_cos = 1.0;

                if (perps.size() >= 2) {
                    Vector sum = {0.0, 0.0, 0.0};

                    for (const Vector& p : perps)
                        sum = sum + p;

                    const Vector dir = sum.magnitude() > 1e-6 ? sum.normalized() : perps[0];

                    for (const Vector& p : perps)
                        min_cos = std::min(min_cos, dir.dot(p));
                }

                const double feas_bonus = min_cos >= 0.80 ? 1e7 : (2e5 * min_cos);
                const double sweep_coord =
                    Vector(facet.centroid[0], facet.centroid[1], facet.centroid[2]).dot(axis);
                const double score = feas_bonus + sweep_coord + 50.0 * min_cos -
                                     10.0 * static_cast<double>(perps.size());

                if (score > best_score) {
                    best_score = score;
                    best_f = f;
                    chosen_cos = min_cos;
                }
            }

            remaining[best_f] = false;
            trial_order[step] = best_f;
            worst_cos = std::min(worst_cos, chosen_cos);
        }

        if (worst_cos > best_global_min_cos) {
            best_global_min_cos = worst_cos;
            _assembly_order = trial_order;
        }
    }

    for (size_t step = 0; step < fcount; step++)
        _facets[_assembly_order[step]].assembly_step = step;

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
                sum = sum + vidy_inplane_outward(
                                facet.centroid,
                                facet.normal,
                                facet.miter_rings[0][k],
                                facet.miter_rings[0][(k + 1) % m]
                            );
        }

        const Vector dir = sum.magnitude() > 1e-6 ? sum.normalized() : -facet.normal;
        _insertion_vectors[0][f] = dir;
        _insertion_vectors[1][f] = dir;
    }
}

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

        std::sort(
            creases.begin(),
            creases.end(),
            [this](const OrigamiCreaseFrame& a, const OrigamiCreaseFrame& b) {
                return std::make_pair(
                           _facets[a.rec_f].assembly_step, _facets[a.mal_f].assembly_step
                       ) <
                       std::make_pair(
                           _facets[b.rec_f].assembly_step, _facets[b.mal_f].assembly_step
                       );
            }
        );

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

    size_t interior_creases = 0;

    for (const OrigamiEdge& edge : _edges)
        if (!edge.is_boundary && edge.facet_b >= 0 && edge.dihedral_rad >= 0.22)
            interior_creases++;

    _total_jointed_seams = tables.seams;
    _total_double_pins = tables.double_pins;
    _total_single_pins = tables.single_pins;
    _max_insertion_skew_deg = tables.max_skew_deg;
    _unjoined_interior_creases =
        (with_joints && interior_creases >= tables.seams) ? (interior_creases - tables.seams) : 0;

    for (size_t f = 0; f < fcount; f++) {
        const OrigamiFacet& facet = _facets[f];

        for (size_t layer = 0; layer < 2; layer++) {
            const size_t h_b = 2 * layer;
            const size_t h_t = 2 * layer + 1;
            const std::shared_ptr<FacePlanarElement> plate = std::make_shared<FacePlanarElement>(
                stock_rings[f][layer].first,
                stock_rings[f][layer].second,
                fmt::format("{}_plate_f{}", layer == 0 ? "inner" : "outer", f)
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
            spec.miter_start =
                prev_edge.is_boundary && bp.z_axis().cross(sp.z_axis()).magnitude() > 0.15;
            spec.start_neighbor_thickness = plate_thickness;
            spec.miter_end =
                next_edge.is_boundary && bp.z_axis().cross(ep.z_axis()).magnitude() > 0.15;
            spec.end_neighbor_thickness = plate_thickness;
            spec.start_setback = 0.5;
            spec.end_setback = 0.5;

            if (const std::optional<size_t> mate_s =
                    find_boundary_knee_mate(_facets, _edges, f, k, facet.vertices[k]))
                if (const std::optional<Plane> bis = compute_origami_separation_plane(
                        _vertices[facet.vertices[k]], facet, _facets[*mate_s]
                    )) {
                    spec.start_plane = *bis;
                    spec.miter_start = false;
                }

            if (const std::optional<size_t> mate_e =
                    find_boundary_knee_mate(_facets, _edges, f, k, facet.vertices[(k + 1) % m]))
                if (const std::optional<Plane> bis = compute_origami_separation_plane(
                        _vertices[facet.vertices[(k + 1) % m]], facet, _facets[*mate_e]
                    )) {
                    spec.end_plane = *bis;
                    spec.miter_end = false;
                }

            if (const std::shared_ptr<FacePlanarElement> board = compute_cavity_border_board(
                    spec, fmt::format("border_{}_f{}_s{}", edge.on_ground ? "sill" : "edge", f, k)
                )) {
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
