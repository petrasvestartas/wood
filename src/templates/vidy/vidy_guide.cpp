#include "pch.h"
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

/// Evaluates a symmetric circular arc profile at y given reference half-length y_ref, v(0) and v(y_ref).
double circular_arc_value(double y, double y_ref, double v_mid, double v_end) {

    const double delta = v_mid - v_end;
    if (std::abs(delta) < 1e-6 || y_ref < 1e-6) {
        return v_mid;
    }

    const double radius = (y_ref * y_ref + delta * delta) / (2.0 * std::abs(delta));
    const double sign = delta > 0.0 ? 1.0 : -1.0;
    const double radicand = std::max(0.0, radius * radius - y * y);
    return (v_mid - sign * radius) + sign * std::sqrt(radicand);
}

/// Ordered undirected vertex pair key.
std::pair<size_t, size_t> undirected_edge_key(size_t a, size_t b) {

    if (a < b) {
        return {a, b};
    }
    return {b, a};
}

/// Outward unit normal and centroid of a planar triangle or quad facet.
void compute_facet_frame(
    const std::vector<Point>& vertices,
    const std::vector<size_t>& loop,
    Point& out_centroid,
    Vector& out_normal
) {

    double sx = 0.0;
    double sy = 0.0;
    double sz = 0.0;
    for (const size_t vid : loop) {
        sx += vertices[vid][0];
        sy += vertices[vid][1];
        sz += vertices[vid][2];
    }
    const double inv = 1.0 / static_cast<double>(loop.size());
    out_centroid = Point(sx * inv, sy * inv, sz * inv);

    const Vector e1 = vertices[loop[1]] - vertices[loop[0]];
    const Vector e2 = vertices[loop[2]] - vertices[loop[0]];
    out_normal = e1.cross(e2).normalized();
}

/// Side miter or boundary plane for directed edge (u -> v) of facet f, oriented outward from facet f.
Plane make_side_miter_plane(
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
    Vector side_normal(0.0, 0.0, -1.0);

    if (edge.kind == VidyEdgeKind::Ground) {
        side_normal = Vector(0.0, 0.0, -1.0);
    } else if (edge.kind == VidyEdgeKind::GableEnd || edge.kind == VidyEdgeKind::BaySeparation) {
        side_normal = Vector(0.0, facet.centroid[1] < mid[1] ? 1.0 : -1.0, 0.0);
    } else {
        const int other_idx =
            (edge.facet_a == static_cast<int>(facet_idx)) ? edge.facet_b : edge.facet_a;
        const Vector& n_other = facets[static_cast<size_t>(other_idx)].normal;
        const Vector edge_dir = (pv - pu).normalized();
        Vector bisector = facet.normal + n_other;
        if (bisector.magnitude() < 1e-9) {
            bisector = facet.normal;
        } else {
            bisector = bisector.normalized();
        }
        side_normal = edge_dir.cross(bisector).normalized();
    }

    if ((mid - facet.centroid).dot(side_normal) < 0.0) {
        side_normal = -side_normal;
    }
    return Plane::from_point_normal(mid, side_normal);
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// VidyGuide
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
    : WoodSession(name),
      bays(std::max<size_t>(1, bays)),
      bay_length(std::max(800.0, bay_length)),
      span_mid(std::max(4000.0, span_mid)),
      span_end(std::max(4000.0, span_end)),
      wall_pleat(wall_pleat),
      eave_height_mid(std::max(1500.0, eave_height_mid)),
      eave_height_end(std::max(1500.0, eave_height_end)),
      crown_height_mid(std::max(2000.0, crown_height_mid)),
      crown_height_end(std::max(2000.0, crown_height_end)),
      knee_half_span(std::clamp(knee_half_span, 1000.0, 0.45 * std::min(span_mid, span_end))),
      knee_rise(knee_rise),
      asymmetry(asymmetry),
      plate_thickness(std::max(15.0, plate_thickness)),
      total_offset(std::max(2.2 * std::max(15.0, plate_thickness), total_offset)) {

    // Stage 1: alternating 6-vertex even bay stations and 5-vertex odd V-pleat mid-stations
    build_stations_and_facets();

    // Stage 2: edge classification (regl/vcon/vend/grnd), miter planes, and 4 miter offset shells
    build_edges_and_miter_rings();
    _inner_cavity_mesh = build_offset_mesh(1);
    _outer_cavity_mesh = build_offset_mesh(2);
    _outer_mesh = build_offset_mesh(3);
    _miter_planes_mesh = build_miter_planes_mesh();

    draw();
}

const std::vector<Point>& VidyGuide::vertices() const {
    return _vertices;
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

void VidyGuide::build_stations_and_facets() {

    const double half_length = 0.5 * static_cast<double>(bays) * bay_length;
    const double ref_even_y = 0.5 * 11.0 * bay_length;
    const double ref_odd_y = 5.0 * bay_length;
    const double half_span_mid = 0.5 * span_mid;
    const double half_span_end = 0.5 * span_end;
    const double odd_half_mid = half_span_mid + 0.88 * wall_pleat;
    const double odd_half_end = half_span_end + 1.27 * wall_pleat;
    const double axis_extent = std::max(odd_half_mid, odd_half_end) + 1800.0;

    std::vector<std::vector<size_t>> station_vids(2 * bays + 1);
    _vertices.clear();
    _bay_axes.clear();

    for (size_t s = 0; s <= 2 * bays; s++) {
        const double y = -half_length + 0.5 * bay_length * static_cast<double>(s);
        std::vector<Point> pts;
        if (s % 2 == 0) {
            _bay_axes.push_back(
                Line::from_points(Point(-axis_extent, y, 0.0), Point(axis_extent, y, 0.0))
            );
            const double xw = circular_arc_value(y, ref_even_y, half_span_mid, half_span_end);
            const double ze = circular_arc_value(y, ref_even_y, eave_height_mid, eave_height_end);
            const double zkl = circular_arc_value(
                y, ref_even_y, eave_height_mid + 0.92 * knee_rise, eave_height_end + 1.08 * knee_rise
            );
            const double zkr = zkl + asymmetry;
            pts = {
                Point(-xw, y, 0.0),
                Point(-xw, y, ze),
                Point(-knee_half_span, y, zkl),
                Point(knee_half_span, y, zkr),
                Point(xw, y, ze),
                Point(xw, y, 0.0)
            };
        } else {
            const double xwm = circular_arc_value(y, ref_odd_y, odd_half_mid, odd_half_end);
            const double zem = circular_arc_value(
                y, ref_odd_y, eave_height_mid + 396.0, eave_height_end + 572.0
            );
            const double xcr = circular_arc_value(
                y, ref_odd_y, 2.75 * asymmetry, 0.94 * asymmetry
            );
            const double zcr = circular_arc_value(y, ref_odd_y, crown_height_mid, crown_height_end);
            pts = {
                Point(-xwm, y, 0.0),
                Point(-xwm, y, zem),
                Point(xcr, y, zcr),
                Point(xwm, y, zem),
                Point(xwm, y, 0.0)
            };
        }

        for (const Point& pt : pts) {
            station_vids[s].push_back(_vertices.size());
            _vertices.push_back(pt);
        }
    }

    std::vector<std::vector<size_t>> mesh_faces;
    mesh_faces.reserve(14 * bays);
    _facets.clear();
    _facets.reserve(14 * bays);

    for (size_t b = 0; b < bays; b++) {
        const std::vector<size_t>& e0 = station_vids[2 * b];
        const std::vector<size_t>& o1 = station_vids[2 * b + 1];
        const std::vector<size_t>& e2 = station_vids[2 * b + 2];

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
            VidyFacet facet;
            facet.bay = b;
            facet.local_index = local;
            facet.role = bay_defs[local].second;
            facet.vertices = bay_defs[local].first;
            compute_facet_frame(_vertices, facet.vertices, facet.centroid, facet.normal);
            mesh_faces.push_back(facet.vertices);
            _facets.push_back(facet);
        }
    }

    _base_mesh = Mesh::from_vertices_and_faces(_vertices, mesh_faces);
}

void VidyGuide::build_edges_and_miter_rings() {

    std::map<std::pair<size_t, size_t>, size_t> lookup;
    _edges.clear();

    for (size_t f = 0; f < _facets.size(); f++) {
        VidyFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.edge_ids.assign(m, -1);

        for (size_t k = 0; k < m; k++) {
            const size_t u = facet.vertices[k];
            const size_t v = facet.vertices[(k + 1) % m];
            const std::pair<size_t, size_t> key = undirected_edge_key(u, v);
            const std::map<std::pair<size_t, size_t>, size_t>::iterator found = lookup.find(key);
            if (found == lookup.end()) {
                const size_t eid = _edges.size();
                VidyEdge edge;
                edge.u = u;
                edge.v = v;
                edge.facet_a = static_cast<int>(f);
                edge.side_a = static_cast<int>(k);
                _edges.push_back(edge);
                lookup.emplace(key, eid);
                facet.edge_ids[k] = static_cast<int>(eid);
            } else {
                VidyEdge& edge = _edges[found->second];
                edge.facet_b = static_cast<int>(f);
                edge.side_b = static_cast<int>(k);
                facet.edge_ids[k] = static_cast<int>(found->second);
            }
        }
    }

    for (VidyEdge& edge : _edges) {
        const Point& pu = _vertices[edge.u];
        const Point& pv = _vertices[edge.v];
        if (edge.facet_b < 0) {
            const bool on_ground = (std::abs(pu[2]) < 1e-3 && std::abs(pv[2]) < 1e-3);
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
        const double phi_deg = 0.5 * (180.0 - std::acos(cos_n) * 180.0 / Tolerance::PI);
        edge.tool_tilt_deg = std::abs(phi_deg - 90.0);
    }

    const std::array<double, 4> heights = layer_heights();
    for (size_t f = 0; f < _facets.size(); f++) {
        VidyFacet& facet = _facets[f];
        const size_t m = facet.vertices.size();
        facet.side_planes.resize(m);
        for (size_t k = 0; k < m; k++) {
            const VidyEdge& edge = _edges[static_cast<size_t>(facet.edge_ids[k])];
            facet.side_planes[k] = make_side_miter_plane(
                _vertices, _facets, f, edge, facet.vertices[k], facet.vertices[(k + 1) % m]
            );
        }

        for (size_t hi = 0; hi < 4; hi++) {
            const Plane offset_plane = Plane::from_point_normal(
                facet.centroid + facet.normal * heights[hi], facet.normal
            );
            facet.miter_rings[hi].resize(m);
            for (size_t k = 0; k < m; k++) {
                const Plane& prev_side = facet.side_planes[(k + m - 1) % m];
                const Plane& curr_side = facet.side_planes[k];
                Point corner = _vertices[facet.vertices[k]] + facet.normal * heights[hi];
                Intersection::plane_plane_plane(offset_plane, prev_side, curr_side, corner);
                facet.miter_rings[hi][k] = corner;
            }
        }
    }

    for (VidyEdge& edge : _edges) {
        const VidyFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        edge.miter_plane = fa.side_planes[static_cast<size_t>(edge.side_a)];
    }
}

Mesh VidyGuide::build_offset_mesh(size_t ring_index) const {

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

Mesh VidyGuide::build_miter_planes_mesh() const {

    std::vector<Point> pts;
    std::vector<std::vector<size_t>> quads;
    pts.reserve(4 * _edges.size());
    quads.reserve(_edges.size());

    for (const VidyEdge& edge : _edges) {
        if (edge.kind == VidyEdgeKind::Ground) {
            continue;
        }
        const VidyFacet& fa = _facets[static_cast<size_t>(edge.facet_a)];
        const size_t sa = static_cast<size_t>(edge.side_a);
        const size_t ma = fa.vertices.size();
        const Point& p0_in = fa.miter_rings[0][sa];
        const Point& p1_in = fa.miter_rings[0][(sa + 1) % ma];
        const Point& p1_out = fa.miter_rings[3][(sa + 1) % ma];
        const Point& p0_out = fa.miter_rings[3][sa];
        const size_t base = pts.size();
        pts.push_back(p0_in);
        pts.push_back(p1_in);
        pts.push_back(p1_out);
        pts.push_back(p0_out);
        quads.push_back({base, base + 1, base + 2, base + 3});
    }
    return Mesh::from_vertices_and_faces(pts, quads);
}

void VidyGuide::draw() {

    const std::shared_ptr<TreeNode> base_group = add_group("base_mesh");
    add_mesh(_base_mesh, base_group);

    const std::shared_ptr<TreeNode> axes_group = add_group("axes");
    for (Line axis : _bay_axes) {
        axis.linecolor = Color::black();
        add_line(axis, axes_group);
    }

    const std::shared_ptr<TreeNode> folds_group = add_group("folds");
    for (const VidyEdge& edge : _edges) {
        if (edge.kind != VidyEdgeKind::Regular && edge.kind != VidyEdgeKind::BaySeparation) {
            continue;
        }
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

} // namespace wood_vidy
