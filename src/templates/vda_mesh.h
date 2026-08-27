#pragma once
#include "session.h"
#include "intersection.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

using namespace session_cpp;

/// Mesh-to-plates-and-connectors: given any triangulated/quad mesh, produces
/// top+bottom polyline pairs for each face at configured face_positions/thickness,
/// plus rectangular connector plate pairs at each internal (non-naked) edge.
///
/// Translated from the Grasshopper Python component case_2_vda.
///
/// Usage:
///   VdaMesh vda;                                     // default 2-quad mesh
///   VdaMesh vda(my_mesh, 3.0, {0.0}, {2}, {}, {}, 10.0, 10.0, 5.0);
class VdaMesh {
public:
    // ── outputs ──────────────────────────────────────────────────────────────
    /// [face_i][pos_j*2+0/1]  — bottom/top outline per face per position
    std::vector<std::vector<Polyline>>      f_polylines;
    /// [face_i][pos_j]  — top plane per face per position
    std::vector<std::vector<Plane>>         f_polylines_planes;
    /// [face_i][pos_j]  — label string per face per position
    std::vector<std::vector<std::string>>   f_polylines_index;

    /// [edge_i][div_j*2+0/1]  — two connector rectangles per subdivision
    std::vector<std::vector<Polyline>>      e_polylines;
    /// [edge_i][div_j]  — connector plane per edge subdivision
    std::vector<std::vector<Plane>>         e_polylines_planes;
    /// [edge_i][div_j]  — label "f0-f1_j" per edge subdivision
    std::vector<std::vector<std::string>>   e_polylines_index;

    // ── constructor ──────────────────────────────────────────────────────────
    VdaMesh(
        const Mesh&          input_mesh         = default_mesh(),
        double               face_thickness     = 20.0,
        std::vector<double>  face_positions     = {0.0},
        std::vector<int>     edge_divisions     = {2},
        std::vector<double>  edge_division_len  = {},
        std::vector<Line>    insertion_lines    = {},
        double               rect_width         = 200.0,
        double               rect_height        = 200.0,
        double               rect_thickness     = 20.0)
    {
        // Validate / normalise inputs
        Mesh m = input_mesh.weld(0.01);
        if (face_positions.empty()) face_positions = {0.0};
        std::sort(face_positions.begin(), face_positions.end());
        if (edge_divisions.empty()) edge_divisions = {2};
        if (!edge_division_len.empty() && edge_division_len[0] < 0.01) {
            edge_division_len.clear();
        }

        // Build topology maps
        build_topology(m);

        // Compute geometry
        get_faces_planes(m);
        get_face_edge_planes(m);
        get_bisector_planes();
        get_face_polylines(face_positions, face_thickness);
        get_edge_vectors(m, insertion_lines);
        get_edge_planes(m, edge_divisions, edge_division_len);
        get_connectors(rect_width, rect_height, rect_thickness);
    }

    // ── default mesh: hex mesh (15 faces), centered at origin, scaled 10× ──────
    // Source data centered by bbox midpoint (-33.754, -22.862, 13.445) and scaled ×10.
    static Mesh default_mesh()
    {
        std::vector<std::vector<Point>> polys = {
            { Point( 760.01,-2621.81,-1344.23), Point( 678.55,-2638.25,-1344.47), Point( 312.96,-2711.90,-1344.47), Point( 268.58,-2647.07, -802.86), Point( 785.47,-2537.90, -765.84) },
            { Point(1029.30,-1734.25,  502.84), Point( 484.63,-1736.48,  651.50), Point( 406.90,  -527.14, 1226.27), Point(1366.88,  -621.66,  919.21) },
            { Point(1796.71,  794.99,  551.90), Point( 941.91, 1284.49,  761.48), Point( 983.61, 2177.67, -275.86), Point(2053.18, 1640.26, -622.17) },
            { Point( 785.47,-2537.90, -765.84), Point( 268.58,-2647.07, -802.86), Point(-230.08,-2574.84, -543.21), Point(-202.02,-1930.01,  521.65), Point( 484.63,-1736.48,  651.50), Point(1029.30,-1734.25,  502.84) },
            { Point(1366.88,  -621.66,  919.21), Point( 406.90,  -527.14, 1226.27), Point(  -87.79,  -224.28, 1344.47), Point(  -28.04,   991.06, 1133.00), Point( 941.91, 1284.49,  761.48), Point(1796.71,  794.99,  551.90) },
            { Point(2053.18, 1640.26, -622.17), Point( 983.61, 2177.67, -275.86), Point(  75.35, 2683.99, -833.95), Point(  79.22, 2711.90,-1344.47), Point(2064.27, 1676.82,-1344.47) },
            { Point( -782.27,-2617.50,-1344.47), Point( -732.62,-2561.48, -809.19), Point(-230.08,-2574.84, -543.21), Point( 268.58,-2647.07, -802.86), Point( 312.96,-2711.90,-1344.47) },
            { Point( 484.63,-1736.48,  651.50), Point(-202.02,-1930.01,  521.65), Point(-834.72,-1644.25,  624.19), Point(-735.12,  -527.94, 1164.03), Point(  -87.79,  -224.28, 1344.47), Point( 406.90,  -527.14, 1226.27) },
            { Point( 941.91, 1284.49,  761.48), Point(  -28.04,   991.06, 1133.00), Point(-940.95, 1369.34,  723.57), Point(-907.43, 2222.88, -267.53), Point(  75.35, 2683.99, -833.95), Point( 983.61, 2177.67, -275.86) },
            { Point(-1403.20,-1577.93,  406.59), Point(-834.72,-1644.25,  624.19), Point(-202.02,-1930.01,  521.65), Point(-230.08,-2574.84, -543.21), Point(-732.62,-2561.48, -809.19), Point(-1247.95,-2364.14, -783.26) },
            { Point(-1890.38,  889.09,  465.62), Point(-940.95, 1369.34,  723.57), Point(  -28.04,   991.06, 1133.00), Point(  -87.79,  -224.28, 1344.47), Point(-735.12,  -527.94, 1164.03), Point(-1628.78,  -435.63,  816.65) },
            { Point(  79.22, 2711.90,-1344.47), Point(  75.35, 2683.99, -833.95), Point(-907.43, 2222.88, -267.53), Point(-2058.23, 1739.09, -697.73), Point(-2064.27, 1769.65,-1344.47) },
            { Point(-1101.44,-2497.51,-1344.47), Point(-1231.26,-2448.62,-1343.85), Point(-1247.95,-2364.14, -783.26), Point(-732.62,-2561.48, -809.19), Point(-782.27,-2617.50,-1344.47) },
            { Point(-1628.78,  -435.63,  816.65), Point(-735.12,  -527.94, 1164.03), Point(-834.72,-1644.25,  624.19), Point(-1403.20,-1577.93,  406.59) },
            { Point(-2058.23, 1739.09, -697.73), Point(-907.43, 2222.88, -267.53), Point(-940.95, 1369.34,  723.57), Point(-1890.38,  889.09,  465.62) },
        };
        return Mesh::from_polylines(polys, 10.0);
    }

private:
    // ── topology ─────────────────────────────────────────────────────────────
    int f_count = 0;
    int e_count = 0;
    std::vector<size_t>                          face_keys;
    std::vector<std::pair<size_t,size_t>>        edge_keys;   // sequential
    std::map<std::pair<size_t,size_t>, int>      edge_to_idx; // canonical → seq idx
    std::vector<std::vector<int>>                e_f_idx;     // seq edge → seq face indices

    // ── geometry ─────────────────────────────────────────────────────────────
    std::vector<Plane>                    f_planes;
    std::vector<std::vector<Plane>>       fe_planes;   // [face][edge_j]
    std::vector<std::vector<Plane>>       bi_planes;   // [face][corner_j]
    std::vector<Line>                     e_lines;
    std::vector<Plane>                    e90_planes;
    std::vector<std::vector<Plane>>       e90_multi;   // per subdivision
    std::vector<Vector>                   insertion_vectors;

    // ── canonical edge key (always low→high) ─────────────────────────────────
    static std::pair<size_t,size_t> canon(size_t u, size_t v)
    {
        return u < v ? std::make_pair(u, v) : std::make_pair(v, u);
    }

    // ── build face/edge sequential indices ───────────────────────────────────
    void build_topology(const Mesh& m)
    {
        face_keys = m.faces(); // sorted
        f_count   = (int)face_keys.size();

        for (int fi = 0; fi < f_count; ++fi) {
            std::optional<std::vector<std::pair<size_t,size_t>>> edges_opt = m.face_edges(face_keys[fi]);
            if (!edges_opt) {
                continue;
            }
            for (const std::pair<size_t,size_t>& edge_uv : *edges_opt) {
                size_t u = edge_uv.first;
                size_t v = edge_uv.second;
                std::pair<size_t,size_t> key = canon(u, v);
                if (edge_to_idx.find(key) == edge_to_idx.end()) {
                    edge_to_idx[key] = e_count++;
                    edge_keys.push_back(key);
                }
            }
        }

        // Build e_f_idx: for each sequential edge, which sequential faces touch it
        e_f_idx.resize(e_count);
        for (int fi = 0; fi < f_count; ++fi) {
            std::optional<std::vector<std::pair<size_t,size_t>>> edges_opt = m.face_edges(face_keys[fi]);
            if (!edges_opt) {
                continue;
            }
            for (const std::pair<size_t,size_t>& edge_uv : *edges_opt) {
                size_t u = edge_uv.first;
                size_t v = edge_uv.second;
                int ei = edge_to_idx[canon(u, v)];
                // Avoid duplicates
                std::vector<int>& efs = e_f_idx[ei];
                if (std::find(efs.begin(), efs.end(), fi) == efs.end()) {
                    efs.push_back(fi);
                }
            }
        }
    }

    // ── face centroid + normal → Plane ───────────────────────────────────────
    void get_faces_planes(const Mesh& m)
    {
        f_planes.resize(f_count);
        for (int fi = 0; fi < f_count; ++fi) {
            std::optional<Point>  c = m.face_centroid(face_keys[fi]);
            std::optional<Vector> n = m.face_normal(face_keys[fi]);
            if (!c || !n) { f_planes[fi] = Plane::invalid(); continue; }
            Point  origin = *c;
            Vector normal = *n;
            // from_point_normal takes non-const refs
            f_planes[fi] = Plane::from_point_normal(origin, normal);
        }
    }

    // ── face-edge cutting planes ──────────────────────────────────────────────
    // For each edge j of face i:
    //   x_axis = (vertex_j - vertex_{j+1})  [matches Python convention]
    //   y_axis = average normal of all faces sharing that edge
    //   origin = edge midpoint
    void get_face_edge_planes(const Mesh& m)
    {
        fe_planes.resize(f_count);
        // One pass over the faces: undirected edge -> adjacent face indices.
        std::map<std::pair<size_t,size_t>, std::vector<int>> edge_to_faces;
        for (int fi2 = 0; fi2 < f_count; ++fi2) {
            auto fe_opt2 = m.face_edges(face_keys[fi2]);
            if (!fe_opt2) continue;
            for (auto& [eu, ev] : *fe_opt2) {
                edge_to_faces[{std::min(eu, ev), std::max(eu, ev)}].push_back(fi2);
            }
        }
        for (int fi = 0; fi < f_count; ++fi) {
            std::optional<std::vector<std::pair<size_t,size_t>>> edges_opt = m.face_edges(face_keys[fi]);
            if (!edges_opt) {
                continue;
            }
            const std::vector<std::pair<size_t,size_t>>& edges = *edges_opt;
            int n = (int)edges.size();
            fe_planes[fi].resize(n);

            for (int j = 0; j < n; ++j) {
                size_t u = edges[j].first;
                size_t v = edges[j].second;
                std::optional<Point> lu = m.vertex_point(u);
                std::optional<Point> lv = m.vertex_point(v);
                if (!lu || !lv) { fe_planes[fi][j] = Plane::invalid(); continue; }

                Point midpt = Point::mid_point(*lu, *lv);
                // x_axis: u→v direction, reversed to match Python (v1-v2)
                Vector xaxis = Vector::from_points(*lv, *lu);
                if (!xaxis.normalize_self()) {
                    xaxis = Vector::x_axis();
                }

                // y_axis: average normal of all adjacent faces.
                // (Adjacency comes from the map built once below the face
                // loop's entry - edge_faces() scans every face per call, and
                // this ran per edge of every face: O(E^2). face_normal is
                // likewise served from the cached f_planes.)
                Vector yaxis(0, 0, 0);
                {
                    auto key = std::make_pair(std::min(u, v), std::max(u, v));
                    auto it = edge_to_faces.find(key);
                    if (it != edge_to_faces.end()) {
                        for (int adj_fi : it->second) {
                            if (!f_planes[adj_fi].is_valid()) continue;
                            yaxis += f_planes[adj_fi].z_axis();
                        }
                    }
                }
                if (!yaxis.normalize_self()) {
                    yaxis = Vector::z_axis();
                }

                fe_planes[fi][j] = Plane(midpt, xaxis, yaxis);
            }
        }
    }

    // ── dihedral (bisector) plane between two face-edge planes ────────────────
    static Plane dihedral_plane(const Plane& p0, const Plane& p1)
    {
        // Intersection line of p0 and p1
        Line line;
        if (!Intersection::plane_plane(p0, p1, line)) {
            return Plane::invalid();
        }

        // Point on that line at p0 (use infinite line)
        Point center_dihedral;
        if (!Intersection::line_plane(line, p0, center_dihedral, false)) {
            center_dihedral = line.center();
        }

        // Check if normals are (anti-)parallel
        // is_parallel_to is non-const → copy first
        Vector z0_copy = p0.z_axis();
        if (z0_copy.is_parallel_to(p1.z_axis()) != 0) {
            return Plane::invalid();
        }

        // Find where the two z-axis rays converge
        Line n0 = Line::from_point_and_vector(p0.origin(), p0.z_axis());
        Line n1 = Line::from_point_and_vector(p1.origin(), p1.z_axis());
        double t0 = 0, t1 = 0;
        bool ok = Intersection::line_line_parameters(
            n0, n1, t0, t1, 0.01, /*segments=*/false, /*near_parallel_as_closest=*/true);
        if (!ok) {
            return Plane::invalid();
        }

        Point center = n0.point_at(t0);
        Vector v0    = Vector::from_points(center, p0.origin());
        Vector v1    = Vector::from_points(center, p1.origin());
        if (!v0.normalize_self() || !v1.normalize_self()) {
            return Plane::invalid();
        }

        Vector bisector = v0 + v1;
        return Plane(center_dihedral, line.to_direction(), bisector);
    }

    // ── bisector planes at each face corner ───────────────────────────────────
    void get_bisector_planes()
    {
        bi_planes.resize(f_count);
        for (int fi = 0; fi < f_count; ++fi) {
            int n = (int)fe_planes[fi].size();
            bi_planes[fi].resize(n);
            for (int j = 0; j < n; ++j) {
                const Plane& pl0 = fe_planes[fi][(j + 1) % n];
                const Plane& pl1 = fe_planes[fi][j];
                bi_planes[fi][j] = dihedral_plane(pl0, pl1);
            }
        }
    }

    // ── face outline by consecutive edge-plane intersections ─────────────────
    // Intersects consecutive face-edge planes to get face corners, then
    // projects onto the offset face plane.
    static Polyline outline_from_face(
        const Plane&              base_plane,
        const std::vector<Plane>& edge_planes,
        const std::vector<Plane>& bise_planes,
        double                    tol_zz_deg = 0.57, // ~0.01 rad
        double                    tol_xx_deg = 5.7)  // ~0.1 rad
    {
        int n = (int)edge_planes.size();
        std::vector<Plane> ep(edge_planes);
        std::vector<Point> corners;
        corners.reserve(n + 1);

        for (int j = 0; j < n; ++j) {
            Plane& ep0 = ep[j];
            Plane& ep1 = ep[(j + 1) % n];

            // Skip if z-axes nearly parallel (same or anti-parallel)
            // angle() is non-const → copy vectors first
            if (ep0.is_valid() && ep1.is_valid()) {
                Vector z0 = ep0.z_axis(), z1 = ep1.z_axis();
                double zz = z0.angle(z1, /*sign=*/false, /*degrees=*/true);
                if (zz < tol_zz_deg || std::abs(zz - 180.0) < tol_zz_deg) {
                    ep[(j + 1) % n] = ep0;
                    continue;
                }
            }

            // Fallback if x-axes nearly aligned → use bisector plane origin
            if (ep0.is_valid() && ep1.is_valid() && j < (int)bise_planes.size() &&
                bise_planes[j].is_valid())
            {
                Vector x0 = ep0.x_axis(), x1 = ep1.x_axis();
                double xx = x0.angle(x1, false, true);
                if (xx < tol_xx_deg) {
                    // Rotate ep0.x_axis by 90° around ep0.y_axis = y × x = -z
                    Vector y0 = ep0.y_axis();
                    Vector x0c = ep0.x_axis();
                    Vector vec = y0.cross(x0c);
                    ep[(j + 1) % n] = Plane(bise_planes[j].origin(), vec, y0);
                }
            }

            // Standard: plane-plane → line, line-face-plane → corner
            Line  line;
            Point pt;
            if (!ep[j].is_valid() || !ep[(j + 1) % n].is_valid()) {
                continue;
            }
            if (!Intersection::plane_plane(ep[j], ep[(j + 1) % n], line)) {
                continue;
            }
            if (!Intersection::line_plane(line, base_plane, pt, /*is_finite=*/false)) {
                continue;
            }
            corners.push_back(pt);
        }

        if (corners.empty()) return Polyline(std::vector<session_cpp::Point>{});
        corners.push_back(corners[0]); // close
        return Polyline(corners);
    }

    // ── face plate polylines ──────────────────────────────────────────────────
    void get_face_polylines(const std::vector<double>& face_positions, double face_thickness)
    {
        f_polylines.resize(f_count);
        f_polylines_planes.resize(f_count);
        f_polylines_index.resize(f_count);

        for (int fi = 0; fi < f_count; ++fi) {
            int np = (int)face_positions.size();
            f_polylines[fi].resize(np * 2);
            f_polylines_planes[fi].resize(np);
            f_polylines_index[fi].resize(np);

            for (int j = 0; j < np; ++j) {
                double off_bot = face_positions[j] - face_thickness * 0.5;
                double off_top = face_positions[j] + face_thickness * 0.5;

                Plane pl_bot = f_planes[fi].translate_by_normal(off_bot);
                Plane pl_top = f_planes[fi].translate_by_normal(off_top);

                f_polylines[fi][j * 2 + 0] = outline_from_face(pl_bot, fe_planes[fi], bi_planes[fi]);
                f_polylines[fi][j * 2 + 1] = outline_from_face(pl_top, fe_planes[fi], bi_planes[fi]);
                f_polylines_planes[fi][j]   = pl_top;
                f_polylines_index[fi][j]    = np == 1
                    ? std::to_string(fi)
                    : std::to_string(fi) + "-" + std::to_string(j);
            }
        }
    }

    // ── match insertion lines to mesh edges ──────────────────────────────────
    void get_edge_vectors(const Mesh& m, const std::vector<Line>& lines)
    {
        insertion_vectors.assign(e_count, Vector(0, 0, 0));
        e_lines.resize(e_count);

        for (int ei = 0; ei < e_count; ++ei) {
            // edge_line() rebuilds the full directed-edge set per call only
            // to validate an edge that came from face_edges and is known to
            // exist: two vertex lookups suffice.
            size_t u = edge_keys[ei].first;
            size_t v = edge_keys[ei].second;
            auto pu = m.vertex_point(u);
            auto pv = m.vertex_point(v);
            e_lines[ei] = (pu && pv) ? Line::from_points(*pu, *pv) : Line();
        }

        if (lines.empty()) {
            return;
        }

        const double tol = 0.01;
        for (const Line& L : lines) {
            for (int ei = 0; ei < e_count; ++ei) {
                const Line& el = e_lines[ei];
                std::pair<double,Point> res_s = el.closest_point(L.start(), true);
                Point cp_s = res_s.second;
                std::pair<double,Point> res_e = el.closest_point(L.end(),   true);
                Point cp_e = res_e.second;
                if (cp_s.squared_distance(L.start()) < tol * tol ||
                    cp_e.squared_distance(L.end())   < tol * tol)
                {
                    insertion_vectors[ei] = L.to_direction();
                    insertion_vectors[ei].normalize_self();
                    break;
                }
            }
        }
    }

    // ── edge perpendicular planes + subdivision ────────────────────────────────
    void get_edge_planes(
        const Mesh&               m,
        const std::vector<int>&   edge_divisions,
        const std::vector<double>& edge_division_len)
    {
        e90_planes.resize(e_count, Plane::invalid());
        e90_multi.resize(e_count);

        for (int ei = 0; ei < e_count; ++ei) {
            // Only internal edges (2 adjacent faces)
            if ((int)e_f_idx[ei].size() < 2) {
                e90_multi[ei] = {Plane::invalid()};
                continue;
            }

            const Line& el = e_lines[ei];
            Point  origin  = el.center();
            Vector zaxis   = el.to_direction();
            if (!zaxis.normalize_self()) {
                zaxis = Vector::z_axis();
            }

            int f0 = e_f_idx[ei][0];
            int f1 = e_f_idx[ei][1];
            Vector yaxis = f_planes[f0].z_axis() + f_planes[f1].z_axis();
            if (!yaxis.normalize_self()) {
                yaxis = Vector::z_axis();
            }

            Vector xaxis = zaxis.cross(yaxis);
            if (!xaxis.normalize_self()) {
                xaxis = Vector::x_axis();
            }

            // Orient xaxis toward face f0's centroid
            Point  c0 = f_planes[f0].origin();
            if ((origin + xaxis).squared_distance(c0) >
                (origin - xaxis).squared_distance(c0)) {
                xaxis = -xaxis;
            }

            // Override with insertion vector projected onto the yaxis plane
            if (!insertion_vectors[ei].is_zero()) {
                Vector iv = insertion_vectors[ei];
                // Project onto the plane perpendicular to yaxis
                double dot = iv.dot(yaxis);
                iv = iv - yaxis * dot;
                if (iv.normalize_self()) {
                    // Align with existing xaxis direction
                    if ((origin + iv).squared_distance(c0) >
                        (origin - iv).squared_distance(c0)) {
                        iv = -iv;
                    }
                    xaxis = iv;
                }
            }

            e90_planes[ei] = Plane(origin, xaxis, yaxis);

            // Compute subdivision count
            int divisions = 1;
            double elen = el.length();
            if (!edge_division_len.empty()) {
                double dlen = edge_division_len.size() == (size_t)e_count
                    ? edge_division_len[ei] : edge_division_len[0];
                if (dlen > 0.01) {
                    divisions = std::max(1, std::min(10, (int)(elen / dlen)));
                }
            } else if (!edge_divisions.empty()) {
                divisions = edge_divisions.size() == (size_t)e_count
                    ? edge_divisions[ei] : edge_divisions[0];
                divisions = std::max(1, divisions);
            }

            // Interior subdivision points
            e90_multi[ei].clear();
            for (int k = 1; k <= divisions; ++k) {
                double t = (double)k / (1.0 + divisions);
                Point pt = el.point_at(t);
                e90_multi[ei].push_back(Plane(pt, xaxis, yaxis));
            }
        }
    }

    // ── build rectangle polyline in a plane ──────────────────────────────────
    static Polyline make_rect(const Plane& pl, double w, double h)
    {
        Point  o = pl.origin();
        Vector x = pl.x_axis();
        Vector y = pl.y_axis();
        double hw = w * 0.5, hh = h * 0.5;
        std::vector<Point> pts = {
            o - x*hw - y*hh,
            o + x*hw - y*hh,
            o + x*hw + y*hh,
            o - x*hw + y*hh,
            o - x*hw - y*hh, // close
        };
        return Polyline(pts);
    }

    // ── edge connector rectangles ─────────────────────────────────────────────
    void get_connectors(double rect_width, double rect_height, double rect_thickness)
    {
        e_polylines.resize(e_count);
        e_polylines_planes.resize(e_count);
        e_polylines_index.resize(e_count);

        for (int ei = 0; ei < e_count; ++ei) {
            int nd = (int)e90_multi[ei].size();
            e_polylines[ei].resize(nd * 2);
            e_polylines_planes[ei].resize(nd);
            e_polylines_index[ei].resize(nd);

            for (int j = 0; j < nd; ++j) {
                const Plane& pl = e90_multi[ei][j];

                if (!pl.is_valid() || rect_width <= 0.0 || rect_height <= 0.0) {
                    e_polylines[ei][j * 2 + 0] = Polyline(std::vector<session_cpp::Point>{});
                    e_polylines[ei][j * 2 + 1] = Polyline(std::vector<session_cpp::Point>{});
                    e_polylines_planes[ei][j]   = Plane::invalid();
                    e_polylines_index[ei][j]    = "";
                    continue;
                }

                // Two rectangles offset ±rect_thickness/2 along z_axis (edge dir)
                // Index 0 = bot (-z), index 1 = top (+z) — matches face plate convention
                Plane pl_bot(pl.origin() + pl.z_axis() * (-rect_thickness * 0.5),
                             pl.x_axis(), pl.y_axis());
                Plane pl_top(pl.origin() + pl.z_axis() * ( rect_thickness * 0.5),
                             pl.x_axis(), pl.y_axis());

                e_polylines[ei][j * 2 + 0] = make_rect(pl_bot, rect_width, rect_height);
                e_polylines[ei][j * 2 + 1] = make_rect(pl_top, rect_width, rect_height);
                e_polylines_planes[ei][j]   = pl_top;

                std::string label;
                if ((int)e_f_idx[ei].size() >= 2) {
                    label = std::to_string(e_f_idx[ei][0]) + "-" +
                            std::to_string(e_f_idx[ei][1]) + "_" +
                            std::to_string(j);
                } else {
                    label = "naked_" + std::to_string(ei) + "_" + std::to_string(j);
                }
                e_polylines_index[ei][j] = label;
            }
        }
    }
};
