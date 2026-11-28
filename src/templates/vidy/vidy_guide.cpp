#include "pch.h"
#include "src/templates/vidy/vidy_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

/// Symmetric circular arc height at y from reference half-length y_ref, v_mid and v_end.
double compute_arc_profile(double y, double y_ref, double v_mid, double v_end) {

    const double delta = v_mid - v_end;

    if (std::abs(delta) < 1e-6 || y_ref < 1e-6)
        return v_mid;

    const double radius = (y_ref * y_ref + delta * delta) / (2.0 * std::abs(delta));
    const double sign = delta > 0.0 ? 1.0 : -1.0;
    const double radicand = std::max(0.0, radius * radius - y * y);

    return (v_mid - sign * radius) + sign * std::sqrt(radicand);
}

/// Outward unit normal and centroid of a planar triangle or quad facet.
std::pair<Point, Vector>
compute_facet_frame(const std::vector<Point>& vertices, const std::vector<size_t>& loop) {

    std::vector<Point> ring;
    ring.reserve(loop.size());

    for (const size_t vid : loop)
        ring.push_back(vertices[vid]);

    const Vector e1 = ring[1] - ring[0];
    const Vector e2 = ring[2] - ring[0];

    return {Point::centroid(ring), e1.cross(e2).normalized()};
}

/// Outward-facing side miter or boundary plane for directed edge (u -> v) of facet_idx.
Plane compute_side_miter_plane(
    const std::vector<Point>& vertices,
    const std::vector<VidyFacet>& facets,
    size_t facet_idx,
    const VidyEdge& edge,
    size_t u,
    size_t v
) {

    const Point& pu = vertices[u];
    const Point& pv = vertices[v];
    const Point mid = Point::mid_point(pu, pv);
    const VidyFacet& facet = facets[facet_idx];
    Vector side_normal = {0.0, 0.0, -1.0};

    if (edge.kind == VidyEdgeKind::GableEnd || edge.kind == VidyEdgeKind::BaySeparation) {
        side_normal = {0.0, facet.centroid[1] < mid[1] ? 1.0 : -1.0, 0.0};
    } else if (edge.kind == VidyEdgeKind::Regular) {
        const int other_idx =
            edge.facet_a == static_cast<int>(facet_idx) ? edge.facet_b : edge.facet_a;
        const Vector& other_normal = facets[static_cast<size_t>(other_idx)].normal;
        const Vector sum = facet.normal + other_normal;
        const Vector bisector = sum.magnitude() < 1e-9 ? facet.normal : sum.normalized();
        side_normal = (pv - pu).normalized().cross(bisector).normalized();
    }

    if ((mid - facet.centroid).dot(side_normal) < 0.0)
        side_normal = -side_normal;

    return Plane::from_point_normal(mid, side_normal);
}

}

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

VidyGuide::VidyGuide(
    size_t bays,
    double bay_length,
    double span_mid,
    double span_end,
    double wall_pleat,
    double eave_height_mid,
    double eave_height_end,
    double crown_height_mid,
    double crown_height_end,
    double knee_half_span,
    double knee_rise,
    double asymmetry,
    double plate_thickness,
    double total_offset,
    const std::string& name
)
    : WoodSession(name), bays(std::max<size_t>(1, bays)), bay_length(std::max(800.0, bay_length)),
      span_mid(std::max(4000.0, span_mid)), span_end(std::max(4000.0, span_end)),
      wall_pleat(wall_pleat), eave_height_mid(std::max(1500.0, eave_height_mid)),
      eave_height_end(std::max(1500.0, eave_height_end)),
      crown_height_mid(std::max(2000.0, crown_height_mid)),
      crown_height_end(std::max(2000.0, crown_height_end)),
      knee_half_span(std::clamp(knee_half_span, 1000.0, 0.45 * std::min(span_mid, span_end))),
      knee_rise(knee_rise), asymmetry(asymmetry), plate_thickness(std::max(15.0, plate_thickness)),
      total_offset(std::max(2.2 * std::max(15.0, plate_thickness), total_offset)) {

    // stations: alternating 6-vertex even bay stations and 5-vertex odd V-pleat mid-stations
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> stations =
        compute_stations();
    _vertices = stations.first;
    const std::vector<Line> bay_axes = compute_bay_axes();
    _bay_axes = bay_axes;

    // facets: ten antiprismatic roof triangles and four vertical wall quads per bay
    const std::vector<VidyFacet> facets = compute_facets(stations.second);
    _facets = facets;
    const std::vector<size_t> valences = compute_vertex_valences();
    _vertex_valences = valences;
    const Mesh base_mesh = compute_base_mesh();
    _base_mesh = base_mesh;

    // edges and miter shells: classified topology edges, side miter planes and four offset shells
    const std::vector<VidyEdge> edges = compute_edges();
    _edges = edges;
    compute_miter_rings();
    _inner_cavity_mesh = compute_offset_mesh(1);
    _outer_cavity_mesh = compute_offset_mesh(2);
    _outer_mesh = compute_offset_mesh(3);
    _miter_planes_mesh = compute_miter_planes_mesh();

    // scene: guide meshes, bay axes and crease lines
    draw();
}

// ═══════════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════════

const std::vector<Point>& VidyGuide::vertices() const {
    return _vertices;
}

const std::vector<size_t>& VidyGuide::vertex_valences() const {
    return _vertex_valences;
}

double VidyGuide::cavity_gap() const {
    return total_offset - 2.0 * plate_thickness;
}

const Mesh& VidyGuide::base_mesh() const {
    return _base_mesh;
}

const Mesh& VidyGuide::inner_cavity_mesh() const {
    return _inner_cavity_mesh;
}

const Mesh& VidyGuide::outer_cavity_mesh() const {
    return _outer_cavity_mesh;
}

const Mesh& VidyGuide::outer_mesh() const {
    return _outer_mesh;
}

const Mesh& VidyGuide::miter_planes_mesh() const {
    return _miter_planes_mesh;
}

const std::vector<Line>& VidyGuide::bay_axes() const {
    return _bay_axes;
}

const std::vector<VidyFacet>& VidyGuide::facets() const {
    return _facets;
}

const std::vector<VidyEdge>& VidyGuide::edges() const {
    return _edges;
}

std::array<double, 4> VidyGuide::layer_heights() const {
    return {0.0, plate_thickness, total_offset - plate_thickness, total_offset};
}

// ═══════════════════════════════════════════════════════════════════════════
// Stations and facets
// ═══════════════════════════════════════════════════════════════════════════

std::pair<std::vector<Point>, std::vector<std::vector<size_t>>>
VidyGuide::compute_stations() const {

    const double half_length = 0.5 * static_cast<double>(bays) * bay_length;
    const double ref_even_y = std::max(5.5, 0.5 * static_cast<double>(bays)) * bay_length;
    const double ref_odd_y = std::max(5.0, (0.5 * static_cast<double>(bays) - 0.5)) * bay_length;
    const double half_span_mid = 0.5 * span_mid;
    const double half_span_end = 0.5 * span_end;
    const double odd_half_mid = half_span_mid + 0.8809 * wall_pleat;
    const double odd_half_end = half_span_end + 1.2724 * wall_pleat;
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> station_vids(2 * bays + 1);

    for (size_t s = 0; s <= 2 * bays; s++) {
        const double y = -half_length + 0.5 * bay_length * static_cast<double>(s);
        std::vector<Point> ring;

        if (s % 2 == 0) {
            const double xw = compute_arc_profile(y, ref_even_y, half_span_mid, half_span_end);
            const double ze = compute_arc_profile(y, ref_even_y, eave_height_mid, eave_height_end);
            const double zkl = compute_arc_profile(
                y,
                ref_even_y,
                eave_height_mid + 0.9221 * knee_rise,
                eave_height_end + 1.0788 * knee_rise
            );
            const double zkr = compute_arc_profile(
                y,
                ref_even_y,
                eave_height_mid + 0.9221 * knee_rise + 0.9991 * asymmetry,
                eave_height_end + 1.0788 * knee_rise + 1.0495 * asymmetry
            );
            ring = {
                {-xw, y, 0.0},
                {-xw, y, ze},
                {-knee_half_span, y, zkl},
                {knee_half_span, y, zkr},
                {xw, y, ze},
                {xw, y, 0.0}
            };
        } else {
            const double xwm = compute_arc_profile(y, ref_odd_y, odd_half_mid, odd_half_end);
            const double zem = compute_arc_profile(
                y, ref_odd_y, eave_height_mid + 396.29, eave_height_end + 572.45
            );
            const double xcr =
                compute_arc_profile(y, ref_odd_y, 2.7484 * asymmetry, 0.9414 * asymmetry);
            const double zcr =
                compute_arc_profile(y, ref_odd_y, crown_height_mid, crown_height_end);
            ring = {{-xwm, y, 0.0}, {-xwm, y, zem}, {xcr, y, zcr}, {xwm, y, zem}, {xwm, y, 0.0}};
        }

        for (const Point& pt : ring) {
            station_vids[s].push_back(vertices.size());
            vertices.push_back(pt);
        }
    }

    return {vertices, station_vids};
}

std::vector<Line> VidyGuide::compute_bay_axes() const {

    const double half_length = 0.5 * static_cast<double>(bays) * bay_length;
    const double extent =
        std::max(0.5 * span_mid + 0.88 * wall_pleat, 0.5 * span_end + 1.27 * wall_pleat) + 1800.0;
    std::vector<Line> axes;

    for (size_t b = 0; b <= bays; b++) {
        const double y = -half_length + bay_length * static_cast<double>(b);
        axes.push_back(Line::from_points({-extent, y, 0.0}, {extent, y, 0.0}));
    }

    return axes;
}

std::vector<VidyFacet>
VidyGuide::compute_facets(const std::vector<std::vector<size_t>>& stations) const {

    std::vector<VidyFacet> facets;
    facets.reserve(14 * bays);

    for (size_t b = 0; b < bays; b++) {
        const std::vector<size_t>& e0 = stations[2 * b];
        const std::vector<size_t>& o1 = stations[2 * b + 1];
        const std::vector<size_t>& e2 = stations[2 * b + 2];
        const std::array<std::pair<std::vector<size_t>, VidyFacetRole>, 14> bay_defs = {{
            {{e0[0], e0[1], o1[1], o1[0]}, VidyFacetRole::LeftWall},
            {{o1[1], e2[1], e2[0], o1[0]}, VidyFacetRole::LeftWall},
            {{e0[5], o1[4], o1[3], e0[4]}, VidyFacetRole::RightWall},
            {{o1[3], o1[4], e2[5], e2[4]}, VidyFacetRole::RightWall},
            {{e0[1], e0[2], o1[1]}, VidyFacetRole::Roof},
            {{o1[1], e0[2], o1[2]}, VidyFacetRole::Roof},
            {{e0[2], e0[3], o1[2]}, VidyFacetRole::Roof},
            {{o1[2], e0[3], o1[3]}, VidyFacetRole::Roof},
            {{o1[3], e0[3], e0[4]}, VidyFacetRole::Roof},
            {{o1[1], e2[2], e2[1]}, VidyFacetRole::Roof},
            {{e2[2], o1[1], o1[2]}, VidyFacetRole::Roof},
            {{o1[2], e2[3], e2[2]}, VidyFacetRole::Roof},
            {{e2[3], o1[2], o1[3]}, VidyFacetRole::Roof},
            {{e2[4], e2[3], o1[3]}, VidyFacetRole::Roof},
        }};

        for (size_t local = 0; local < bay_defs.size(); local++) {
            const std::pair<Point, Vector> frame =
                compute_facet_frame(_vertices, bay_defs[local].first);
            VidyFacet facet;
            facet.bay = b;
            facet.local_index = local;
            facet.role = bay_defs[local].second;
            facet.vertices = bay_defs[local].first;
            facet.centroid = frame.first;
            facet.normal = frame.second;
            facets.push_back(facet);
        }
    }

    return facets;
}

std::vector<size_t> VidyGuide::compute_vertex_valences() const {

    std::vector<size_t> valences(_vertices.size(), 0);

    for (const VidyFacet& facet : _facets)
        for (const size_t vid : facet.vertices)
            valences[vid]++;

    return valences;
}

Mesh VidyGuide::compute_base_mesh() const {

    std::vector<std::vector<size_t>> faces;
    faces.reserve(_facets.size());

    for (const VidyFacet& facet : _facets)
        faces.push_back(facet.vertices);

    return Mesh::from_vertices_and_faces(_vertices, faces);
}

// ═══════════════════════════════════════════════════════════════════════════
// Edges and miter shells
// ═══════════════════════════════════════════════════════════════════════════

std::vector<VidyEdge> VidyGuide::compute_edges() {

    std::map<std::pair<size_t, size_t>, size_t> lookup;
    std::vector<VidyEdge> edges;

    for (size_t f = 0; f < _facets.size(); f++) {
        VidyFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.edge_ids.assign(m, -1);

        for (size_t k = 0; k < m; k++) {
            const size_t u = facet.vertices[k];
            const size_t v = facet.vertices[(k + 1) % m];
            const std::pair<size_t, size_t> key = std::minmax(u, v);
            const std::map<std::pair<size_t, size_t>, size_t>::iterator found = lookup.find(key);

            if (found == lookup.end()) {
                const size_t eid = edges.size();
                VidyEdge edge;
                edge.u = u;
                edge.v = v;
                edge.facet_a = static_cast<int>(f);
                edge.side_a = static_cast<int>(k);
                edges.push_back(edge);
                lookup.emplace(key, eid);
                facet.edge_ids[k] = static_cast<int>(eid);
            } else {
                edges[found->second].facet_b = static_cast<int>(f);
                edges[found->second].side_b = static_cast<int>(k);
                facet.edge_ids[k] = static_cast<int>(found->second);
            }
        }
    }

    for (VidyEdge& edge : edges) {
        const Point& pu = _vertices[edge.u];
        const Point& pv = _vertices[edge.v];

        if (edge.facet_b < 0) {
            const bool on_ground = std::abs(pu[2]) < 1e-3 && std::abs(pv[2]) < 1e-3;
            edge.kind = on_ground ? VidyEdgeKind::Ground : VidyEdgeKind::GableEnd;
            continue;
        }

        const VidyFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        const VidyFacet& fb = _facets[static_cast<size_t>(edge.facet_b)];

        if (fa.bay != fb.bay) {
            edge.kind = VidyEdgeKind::BaySeparation;
            edge.positive_fold = false;
            continue;
        }

        edge.kind = VidyEdgeKind::Regular;
        edge.positive_fold = (fb.centroid - fa.centroid).dot(fa.normal) < 0.0;
        edge.in_situ_eave = (fa.role == VidyFacetRole::Roof) != (fb.role == VidyFacetRole::Roof);
        const double cos_n = std::clamp(fa.normal.dot(fb.normal), -1.0, 1.0);
        const double phi_deg = 0.5 * (180.0 - std::acos(cos_n) * Tolerance::TO_DEGREES);
        edge.tool_tilt_deg = std::abs(phi_deg - 90.0);
    }

    return edges;
}

void VidyGuide::compute_miter_rings() {

    const std::array<double, 4> heights = layer_heights();

    for (size_t f = 0; f < _facets.size(); f++) {
        VidyFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.side_planes.resize(m);

        for (size_t k = 0; k < m; k++) {
            const VidyEdge& edge = _edges[static_cast<size_t>(facet.edge_ids[k])];
            facet.side_planes[k] = compute_side_miter_plane(
                _vertices, _facets, f, edge, facet.vertices[k], facet.vertices[(k + 1) % m]
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
                facet.miter_rings[hi][k] =
                    Intersection::plane_plane_plane(offset_plane, prev_side, curr_side)
                        .value_or(fallback);
            }
        }
    }

    for (VidyEdge& edge : _edges) {
        const VidyFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        edge.miter_plane = fa.side_planes[static_cast<size_t>(edge.side_a)];
    }
}

Mesh VidyGuide::compute_offset_mesh(size_t ring_index) const {

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> faces;
    pts.reserve(4 * _facets.size());
    faces.reserve(_facets.size());

    for (const VidyFacet& facet : _facets) {
        const std::vector<Point>& ring = facet.miter_rings[ring_index];
        std::vector<size_t> loop;
        loop.reserve(ring.size());

        for (const Point& pt : ring) {
            loop.push_back(pts.size());
            pts.push_back(pt);
        }

        faces.push_back(loop);
    }

    return Mesh::from_vertices_and_faces(pts, faces);
}

Mesh VidyGuide::compute_miter_planes_mesh() const {

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> quads;
    pts.reserve(4 * _edges.size());
    quads.reserve(_edges.size());

    for (const VidyEdge& edge : _edges) {
        if (edge.kind == VidyEdgeKind::Ground)
            continue;

        const VidyFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        const size_t sa = static_cast<size_t>(edge.side_a);
        const size_t ma = fa.vertices.size();
        const size_t base = pts.size();
        pts.push_back(fa.miter_rings[0][sa]);
        pts.push_back(fa.miter_rings[0][(sa + 1) % ma]);
        pts.push_back(fa.miter_rings[3][(sa + 1) % ma]);
        pts.push_back(fa.miter_rings[3][sa]);
        quads.push_back({base, base + 1, base + 2, base + 3});
    }

    return Mesh::from_vertices_and_faces(pts, quads);
}

// ═══════════════════════════════════════════════════════════════════════════
// Scene
// ═══════════════════════════════════════════════════════════════════════════

void VidyGuide::draw() {

    add_mesh(_base_mesh, add_group("base_mesh"));
    const std::shared_ptr<TreeNode> axes_group = add_group("axes");

    for (Line axis : _bay_axes) {
        axis.linecolor = Color::black();
        add_line(axis, axes_group);
    }

    const std::shared_ptr<TreeNode> folds_group = add_group("folds");

    for (const VidyEdge& edge : _edges) {
        if (edge.kind != VidyEdgeKind::Regular && edge.kind != VidyEdgeKind::BaySeparation)
            continue;

        Line crease = Line::from_points(_vertices[edge.u], _vertices[edge.v]);
        crease.linecolor = Color::black();
        add_line(crease, folds_group);
    }

    add_mesh(_inner_cavity_mesh, add_group("inner_cavity"));
    add_mesh(_outer_cavity_mesh, add_group("outer_cavity"));
    add_mesh(_outer_mesh, add_group("outer_surface"));
    add_mesh(_base_mesh, add_group("inner_surface"));
    add_mesh(_miter_planes_mesh, add_group("miter_planes"));
}

}
