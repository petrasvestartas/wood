#include "pch.h"
#include "src/templates/vault/surface_division.h"
#include "plane.h"
#include "tolerance.h"

namespace wood_vault {

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Surface Patch Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Evaluates surface at normalized [0, 1] x [0, 1] domain fractions.
Point eval_unit(const NurbsSurface& surface, double fu, double fv) {

    const std::pair<double, double> du = surface.domain(0);
    const std::pair<double, double> dv = surface.domain(1);
    const double u = du.first + std::clamp(fu, 0.0, 1.0) * (du.second - du.first);
    const double v = dv.first + std::clamp(fv, 0.0, 1.0) * (dv.second - dv.first);

    return surface.point_at(u, v);
}

/// Evaluates unit normal of surface at normalized [0, 1] x [0, 1] domain fractions.
Vector normal_unit(const NurbsSurface& surface, double fu, double fv) {

    const std::pair<double, double> du = surface.domain(0);
    const std::pair<double, double> dv = surface.domain(1);
    const double u = du.first + std::clamp(fu, 0.0, 1.0) * (du.second - du.first);
    const double v = dv.first + std::clamp(fv, 0.0, 1.0) * (dv.second - dv.first);
    Vector n = surface.normal_at(u, v);
    if (!n.normalize_self())
        n = Vector(0.0, 0.0, 1.0);

    return n;
}

/// Evaluates point on side a (0: v=0, 1: u=1, 2: v=1, 3: u=0) at parameter t in [0, 1].
Point side_point(const NurbsSurface& surface, int side, double t) {

    if (side == 0)
        return eval_unit(surface, t, 0.0);
    if (side == 1)
        return eval_unit(surface, 1.0, t);
    if (side == 2)
        return eval_unit(surface, t, 1.0);

    return eval_unit(surface, 0.0, t);
}

/// Solves 1D cubic Greville collocation for four points at parameters {0, 1/3, 2/3, 1}.
void solve_cubic_four(
    const Point& p0,
    const Point& p1,
    const Point& p2,
    const Point& p3,
    Point& c0,
    Point& c1,
    Point& c2,
    Point& c3
) {

    c0 = p0;
    c3 = p3;
    const Point r1(
        (27.0 * p1[0] - 8.0 * p0[0] - p3[0]) / 6.0,
        (27.0 * p1[1] - 8.0 * p0[1] - p3[1]) / 6.0,
        (27.0 * p1[2] - 8.0 * p0[2] - p3[2]) / 6.0
    );
    const Point r2(
        (27.0 * p2[0] - p0[0] - 8.0 * p3[0]) / 6.0,
        (27.0 * p2[1] - p0[1] - 8.0 * p3[1]) / 6.0,
        (27.0 * p2[2] - p0[2] - 8.0 * p3[2]) / 6.0
    );
    c1 = Point(
        (2.0 * r1[0] - r2[0]) / 3.0,
        (2.0 * r1[1] - r2[1]) / 3.0,
        (2.0 * r1[2] - r2[2]) / 3.0
    );
    c2 = Point(
        (2.0 * r2[0] - r1[0]) / 3.0,
        (2.0 * r2[1] - r1[1]) / 3.0,
        (2.0 * r2[2] - r1[2]) / 3.0
    );
}

/// Builds a bicubic NURBS patch interpolating a 4 x 4 grid of surface points.
NurbsSurface bicubic_from_grid(const std::array<std::array<Point, 4>, 4>& grid) {

    std::array<std::array<Point, 4>, 4> temp;
    for (int i = 0; i < 4; i++) {
        solve_cubic_four(
            grid[i][0],
            grid[i][1],
            grid[i][2],
            grid[i][3],
            temp[i][0],
            temp[i][1],
            temp[i][2],
            temp[i][3]
        );
    }

    std::array<std::array<Point, 4>, 4> cvs;
    for (int j = 0; j < 4; j++) {
        solve_cubic_four(
            temp[0][j],
            temp[1][j],
            temp[2][j],
            temp[3][j],
            cvs[0][j],
            cvs[1][j],
            cvs[2][j],
            cvs[3][j]
        );
    }

    std::vector<Point> flat;
    flat.reserve(16);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            flat.push_back(cvs[i][j]);

    return NurbsSurface::create(false, false, 3, 3, 4, 4, flat);
}

/// Finds an existing welded vertex within tol_sq or appends a new vertex and normal.
size_t find_or_add_vertex(
    std::vector<Point>& verts,
    std::vector<Vector>& norms,
    const Point& pt,
    const Vector& nrm,
    double tol_sq
) {

    for (size_t k = 0; k < verts.size(); k++) {
        const Vector d = verts[k] - pt;
        if (d.magnitude_squared() <= tol_sq) {
            norms[k] = norms[k] + nrm;
            return k;
        }
    }

    verts.push_back(pt);
    norms.push_back(nrm);

    return verts.size() - 1;
}

/// Welds polygon soup with vertex normals and builds a Mesh whose vertices store unit normals.
Mesh build_welded_mesh(
    const std::vector<std::vector<Point>>& polygons,
    const std::vector<std::vector<Vector>>& poly_normals,
    double weld_tol = 1e-2
) {

    std::vector<Point> verts;
    std::vector<Vector> norms;
    std::vector<std::vector<size_t>> faces;
    const double tol_sq = weld_tol * weld_tol;

    for (size_t p = 0; p < polygons.size(); p++) {
        const std::vector<Point>& ring = polygons[p];
        if (ring.size() < 3)
            continue;

        std::vector<size_t> raw_ids;
        Vector up_avg(0.0, 0.0, 0.0);
        for (size_t k = 0; k < ring.size(); k++) {
            const Vector nrm = (p < poly_normals.size() && k < poly_normals[p].size())
                                   ? poly_normals[p][k]
                                   : Vector(0.0, 0.0, 1.0);
            const size_t id = find_or_add_vertex(verts, norms, ring[k], nrm, tol_sq);
            if (raw_ids.empty() || raw_ids.back() != id)
                raw_ids.push_back(id);
            up_avg = up_avg + nrm;
        }
        while (raw_ids.size() > 1 && raw_ids.back() == raw_ids.front())
            raw_ids.pop_back();

        std::vector<size_t> clean;
        for (const size_t id : raw_ids)
            if (std::find(clean.begin(), clean.end(), id) == clean.end())
                clean.push_back(id);
        if (clean.size() < 3)
            continue;

        Point centre(0.0, 0.0, 0.0);
        for (const size_t id : clean)
            centre = centre + Vector(verts[id][0], verts[id][1], verts[id][2]);
        centre = Point(
            centre[0] / static_cast<double>(clean.size()),
            centre[1] / static_cast<double>(clean.size()),
            centre[2] / static_cast<double>(clean.size())
        );

        Vector f_norm(0.0, 0.0, 0.0);
        for (size_t k = 0; k < clean.size(); k++) {
            const Vector a = verts[clean[k]] - centre;
            const Vector b = verts[clean[(k + 1) % clean.size()]] - centre;
            f_norm = f_norm + a.cross(b);
        }
        if (f_norm.magnitude_squared() <= 1e-8)
            continue;
        if (up_avg.magnitude_squared() > 1e-8 && f_norm.dot(up_avg) < 0.0)
            std::reverse(clean.begin(), clean.end());

        faces.push_back(clean);
    }

    Mesh mesh = Mesh::from_vertices_and_faces(verts, faces);
    for (size_t i = 0; i < verts.size(); i++) {
        Vector n = norms[i];
        if (!n.normalize_self())
            n = Vector(0.0, 0.0, 1.0);
        if (mesh.vertex.count(i))
            mesh.vertex[i].set_normal(n[0], n[1], n[2]);
    }

    return mesh;
}

/// Finds disjoint-set root with path compression.
int uf_find(std::vector<int>& parent, int x) {

    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }

    return x;
}

/// Dissolves shared seam edges between boundary cells from different patches so cells become whole across seams.
void merge_seam_cells(
    std::vector<std::vector<Point>>& polygons,
    std::vector<std::vector<Vector>>& normals,
    const std::vector<size_t>& patch_ids,
    const std::vector<bool>& is_boundary_cell,
    double weld_tol = 1e-1
) {

    std::vector<Point> pts;
    std::vector<Vector> nrms;
    const double tol_sq = weld_tol * weld_tol;

    std::vector<std::vector<size_t>> loops(polygons.size());
    for (size_t f = 0; f < polygons.size(); f++) {
        for (size_t k = 0; k < polygons[f].size(); k++) {
            const size_t id = find_or_add_vertex(pts, nrms, polygons[f][k], normals[f][k], tol_sq);
            if (loops[f].empty() || loops[f].back() != id)
                loops[f].push_back(id);
        }
        while (loops[f].size() > 1 && loops[f].back() == loops[f].front())
            loops[f].pop_back();

        if (loops[f].size() >= 3) {
            Point c(0.0, 0.0, 0.0);
            Vector up(0.0, 0.0, 0.0);
            for (size_t k = 0; k < loops[f].size(); k++) {
                c = c + Vector(pts[loops[f][k]][0], pts[loops[f][k]][1], pts[loops[f][k]][2]);
                up = up + nrms[loops[f][k]];
            }
            c = Point(
                c[0] / static_cast<double>(loops[f].size()),
                c[1] / static_cast<double>(loops[f].size()),
                c[2] / static_cast<double>(loops[f].size())
            );
            Vector area(0.0, 0.0, 0.0);
            for (size_t k = 0; k < loops[f].size(); k++) {
                const Vector da = pts[loops[f][k]] - c;
                const Vector db = pts[loops[f][(k + 1) % loops[f].size()]] - c;
                area = area + da.cross(db);
            }
            if (area.dot(up) < 0.0)
                std::reverse(loops[f].begin(), loops[f].end());
        }
    }

    std::map<std::pair<size_t, size_t>, std::vector<size_t>> edge_cells;
    std::vector<int> vertex_valence(pts.size(), 0);
    for (size_t f = 0; f < loops.size(); f++) {
        for (const size_t v : loops[f])
            vertex_valence[v]++;
        if (!is_boundary_cell[f] || loops[f].size() < 3)
            continue;
        for (size_t k = 0; k < loops[f].size(); k++) {
            const size_t a = loops[f][k];
            const size_t b = loops[f][(k + 1) % loops[f].size()];
            const std::pair<size_t, size_t> key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
            edge_cells[key].push_back(f);
        }
    }

    std::vector<int> parent(loops.size());
    for (size_t i = 0; i < loops.size(); i++)
        parent[i] = static_cast<int>(i);

    for (const std::pair<const std::pair<size_t, size_t>, std::vector<size_t>>& kv : edge_cells) {
        if (kv.second.size() != 2)
            continue;
        const size_t f0 = kv.second[0];
        const size_t f1 = kv.second[1];
        if (patch_ids[f0] != patch_ids[f1]) {
            const int r0 = uf_find(parent, static_cast<int>(f0));
            const int r1 = uf_find(parent, static_cast<int>(f1));
            if (r0 != r1)
                parent[r1] = r0;
        }
    }

    std::map<int, std::vector<size_t>> groups;
    for (size_t f = 0; f < loops.size(); f++)
        if (loops[f].size() >= 3)
            groups[uf_find(parent, static_cast<int>(f))].push_back(f);

    std::vector<std::vector<Point>> out_polys;
    std::vector<std::vector<Vector>> out_norms;
    for (const std::pair<const int, std::vector<size_t>>& grp : groups) {
        if (grp.second.size() == 1) {
            const size_t f = grp.second[0];
            std::vector<Point> ring;
            std::vector<Vector> rn;
            for (const size_t id : loops[f]) {
                ring.push_back(pts[id]);
                rn.push_back(nrms[id]);
            }
            out_polys.push_back(ring);
            out_norms.push_back(rn);
            continue;
        }

        std::set<std::pair<size_t, size_t>> halfedges;
        std::map<size_t, int> internal_uses;
        for (const size_t f : grp.second) {
            for (const size_t v : loops[f])
                internal_uses[v]++;
            for (size_t k = 0; k < loops[f].size(); k++) {
                const size_t a = loops[f][k];
                const size_t b = loops[f][(k + 1) % loops[f].size()];
                if (halfedges.count({b, a}))
                    halfedges.erase({b, a});
                else
                    halfedges.insert({a, b});
            }
        }
        if (halfedges.size() < 3)
            continue;

        std::map<size_t, size_t> next_of;
        for (const std::pair<size_t, size_t>& he : halfedges)
            next_of[he.first] = he.second;

        std::vector<size_t> merged;
        size_t cur = halfedges.begin()->first;
        for (size_t step = 0; step < halfedges.size() + 2; step++) {
            merged.push_back(cur);
            if (!next_of.count(cur))
                break;
            cur = next_of[cur];
            if (cur == merged.front()) {
                merged.pop_back();
                break;
            }
        }
        if (merged.size() < 3)
            continue;

        std::vector<Point> ring;
        std::vector<Vector> rn;
        for (size_t k = 0; k < merged.size(); k++) {
            const size_t vid = merged[k];
            const bool touched_outside = (vertex_valence[vid] > internal_uses[vid]);
            if (!touched_outside) {
                const Point& prev_p = pts[merged[(k + merged.size() - 1) % merged.size()]];
                const Point& curr_p = pts[vid];
                const Point& next_p = pts[merged[(k + 1) % merged.size()]];
                Vector d0 = curr_p - prev_p;
                Vector d1 = next_p - curr_p;
                if (d0.normalize_self() && d1.normalize_self() && d0.dot(d1) > 0.985)
                    continue;
            }
            ring.push_back(pts[vid]);
            rn.push_back(nrms[vid]);
        }
        if (ring.size() >= 3) {
            out_polys.push_back(ring);
            out_norms.push_back(rn);
        }
    }

    polygons = std::move(out_polys);
    normals = std::move(out_norms);
}

/// Clips a 2D UV polygon against one axis-aligned halfplane.
std::vector<std::pair<double, double>> clip_uv_halfplane(
    const std::vector<std::pair<double, double>>& in,
    int axis,
    double limit,
    bool keep_ge
) {

    std::vector<std::pair<double, double>> out;
    if (in.empty())
        return out;

    for (size_t k = 0; k < in.size(); k++) {
        const std::pair<double, double>& a = in[k];
        const std::pair<double, double>& b = in[(k + 1) % in.size()];
        const double va = axis == 0 ? a.first : a.second;
        const double vb = axis == 0 ? b.first : b.second;
        const bool in_a = keep_ge ? (va >= limit - 1e-7) : (va <= limit + 1e-7);
        const bool in_b = keep_ge ? (vb >= limit - 1e-7) : (vb <= limit + 1e-7);
        if (in_a)
            out.push_back(a);
        if (in_a != in_b) {
            const double alpha = (limit - va) / (vb - va);
            out.emplace_back(
                a.first + alpha * (b.first - a.first),
                a.second + alpha * (b.second - a.second)
            );
        }
    }

    return out;
}

/// Clips a UV polygon to the [0, 1] x [0, 1] unit square.
std::vector<std::pair<double, double>> clip_uv_unit_square(
    std::vector<std::pair<double, double>> poly
) {

    poly = clip_uv_halfplane(poly, 0, 0.0, true);
    poly = clip_uv_halfplane(poly, 0, 1.0, false);
    poly = clip_uv_halfplane(poly, 1, 0.0, true);
    poly = clip_uv_halfplane(poly, 1, 1.0, false);

    return poly;
}

/// Solves one Chebyshev axis step on surf of chord length edge along tangent dir via 2D Newton.
bool chebyshev_axis_step(
    const NurbsSurface& surf,
    bool flip_normal,
    double edge,
    double u_start,
    double v_start,
    Vector& dir,
    double& u_out,
    double& v_out
) {

    // NurbsSurface::evaluate(u, v, 1) returns [S, Sv, Su] (3 elements).
    const std::vector<Vector> d0 = surf.evaluate(u_start, v_start, 1);
    if (d0.size() < 3)
        return false;

    const Point p0(d0[0][0], d0[0][1], d0[0][2]);
    Vector n0 = surf.normal_at(u_start, v_start);
    if (!n0.normalize_self())
        n0 = Vector(0.0, 0.0, 1.0);
    if (flip_normal)
        n0 = -n0;

    Vector fwd = dir - n0 * dir.dot(n0);
    if (!fwd.normalize_self())
        return false;
    const Vector perp = n0.cross(fwd).normalized();

    const Vector su = d0[2];
    const Vector sv = d0[1];
    const double g00 = su.dot(su);
    const double g01 = su.dot(sv);
    const double g11 = sv.dot(sv);
    const double det_g = g00 * g11 - g01 * g01;
    if (std::abs(det_g) <= 1e-12)
        return false;

    const Vector rhs = fwd * edge;
    double u = u_start + (g11 * su.dot(rhs) - g01 * sv.dot(rhs)) / det_g;
    double v = v_start + (-g01 * su.dot(rhs) + g00 * sv.dot(rhs)) / det_g;

    for (int iter = 0; iter < 24; iter++) {
        const std::vector<Vector> der = surf.evaluate(u, v, 1);
        if (der.size() < 3)
            return false;
        const Vector diff(der[0][0] - p0[0], der[0][1] - p0[1], der[0][2] - p0[2]);
        const double f1 = diff.dot(diff) - edge * edge;
        const double f2 = diff.dot(perp);
        if (std::abs(f1) < 1e-4 * edge * edge && std::abs(f2) < 1e-3 * edge)
            break;

        const double j11 = 2.0 * diff.dot(der[2]);
        const double j12 = 2.0 * diff.dot(der[1]);
        const double j21 = der[2].dot(perp);
        const double j22 = der[1].dot(perp);
        const double det = j11 * j22 - j12 * j21;
        if (std::abs(det) <= 1e-12)
            return false;
        u -= (j22 * f1 - j12 * f2) / det;
        v -= (-j21 * f1 + j11 * f2) / det;
    }

    if (u < -0.25 || u > 1.25 || v < -0.25 || v > 1.25)
        return false;
    const Point p_new = surf.point_at(u, v);
    if (std::abs(p_new.distance(p0) - edge) > 1.5e-2 * edge)
        return false;

    dir = (p_new - p0).normalized();
    u_out = u;
    v_out = v;

    return true;
}

/// Solves Chebyshev rhombus corner on surf at distance edge from both (ua,va) and (ub,vb) away from (ud,vd).
bool chebyshev_rhombus_corner(
    const NurbsSurface& surf,
    double edge,
    double ua,
    double va,
    double ub,
    double vb,
    double ud,
    double vd,
    double& u_out,
    double& v_out
) {

    const Point pa = surf.point_at(ua, va);
    const Point pb = surf.point_at(ub, vb);
    const Point pd = surf.point_at(ud, vd);
    double u = ua + ub - ud;
    double v = va + vb - vd;

    for (int iter = 0; iter < 24; iter++) {
        const std::vector<Vector> der = surf.evaluate(u, v, 1);
        if (der.size() < 3)
            return false;
        const Point cur(der[0][0], der[0][1], der[0][2]);
        const Vector da = cur - pa;
        const Vector db = cur - pb;
        const double f1 = da.dot(da) - edge * edge;
        const double f2 = db.dot(db) - edge * edge;
        if (std::abs(f1) < 1e-4 * edge * edge && std::abs(f2) < 1e-4 * edge * edge)
            break;

        const double j11 = 2.0 * da.dot(der[2]);
        const double j12 = 2.0 * da.dot(der[1]);
        const double j21 = 2.0 * db.dot(der[2]);
        const double j22 = 2.0 * db.dot(der[1]);
        const double det = j11 * j22 - j12 * j21;
        if (std::abs(det) <= 1e-12)
            return false;
        u -= (j22 * f1 - j12 * f2) / det;
        v -= (-j21 * f1 + j11 * f2) / det;
    }

    if (u < -0.25 || u > 1.25 || v < -0.25 || v > 1.25)
        return false;
    const Point p_new = surf.point_at(u, v);
    if (std::abs(p_new.distance(pa) - edge) > 2e-2 * edge ||
        std::abs(p_new.distance(pb) - edge) > 2e-2 * edge ||
        p_new.distance(pd) < 0.25 * edge)
        return false;

    u_out = u;
    v_out = v;

    return true;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Canonical Vault NURBS & Sewn Multi-Surface Builders
// ═══════════════════════════════════════════════════════════════════════════

NurbsSurface saddle_vault_surface(double span_x, double span_y, double arch_rise, double sag) {

    const double hx = 0.5 * span_x;
    const double hy = 0.5 * span_y;
    std::array<std::array<Point, 4>, 4> grid;
    for (int i = 0; i < 4; i++) {
        const double u = static_cast<double>(i) / 3.0;
        const double x = -hx + span_x * u;
        const double arch_z = arch_rise * (1.0 - (2.0 * u - 1.0) * (2.0 * u - 1.0));
        for (int j = 0; j < 4; j++) {
            const double v = static_cast<double>(j) / 3.0;
            const double y = -hy + span_y * v;
            const double saddle_z = -sag * (1.0 - (2.0 * v - 1.0) * (2.0 * v - 1.0)) *
                                    (0.35 + 0.65 * (1.0 - (2.0 * u - 1.0) * (2.0 * u - 1.0)));
            grid[i][j] = Point(x, y, std::max(0.0, arch_z + saddle_z));
        }
    }

    return bicubic_from_grid(grid);
}

NurbsSurface pillow_dome_surface(double span_x, double span_y, double rise) {

    const double hx = 0.5 * span_x;
    const double hy = 0.5 * span_y;
    // Pendentive / sail dome: 4 corner piers at z = 0, 4 perimeter arches rising to 0.52 * rise, crown at rise.
    std::array<std::array<Point, 4>, 4> grid;
    for (int i = 0; i < 4; i++) {
        const double u = 2.0 * (static_cast<double>(i) / 3.0) - 1.0;
        for (int j = 0; j < 4; j++) {
            const double v = 2.0 * (static_cast<double>(j) / 3.0) - 1.0;
            const double dome_z = rise * (1.0 - 0.48 * u * u) * (1.0 - 0.48 * v * v) -
                                  rise * 0.52 * 0.52 * (u * u * v * v);
            const double corner_taper = (std::abs(i == 0 || i == 3) && std::abs(j == 0 || j == 3))
                                            ? 0.0
                                            : std::max(0.0, dome_z);
            grid[i][j] = Point(hx * u, hy * v, corner_taper);
        }
    }

    return bicubic_from_grid(grid);
}

/// Solves 1D clamped cubic Greville collocation along n >= 4 points so the cubic spline interpolates targets at its Greville abscissae.
std::vector<Point> solve_clamped_cubic_1d(const std::vector<Point>& targets) {

    const int n = static_cast<int>(targets.size());
    if (n < 4)
        return targets;

    const double span = static_cast<double>(n - 3);
    std::vector<double> greville(static_cast<size_t>(n), 0.0);
    for (int i = 0; i < n; i++) {
        const double k0 = std::clamp(static_cast<double>(i - 2), 0.0, span);
        const double k1 = std::clamp(static_cast<double>(i - 1), 0.0, span);
        const double k2 = std::clamp(static_cast<double>(i), 0.0, span);
        greville[static_cast<size_t>(i)] = (k0 + k1 + k2) / 3.0;
    }

    NurbsSurface dummy = NurbsSurface::create(
        false,
        false,
        3,
        1,
        n,
        2,
        std::vector<Point>(static_cast<size_t>(2 * n), Point(0.0, 0.0, 0.0))
    );
    std::vector<Point> cvs = targets;
    for (int iter = 0; iter < 36; iter++) {
        for (int i = 0; i < n; i++) {
            dummy.set_cv(i, 0, cvs[static_cast<size_t>(i)]);
            dummy.set_cv(i, 1, cvs[static_cast<size_t>(i)]);
        }
        for (int i = 1; i + 1 < n; i++) {
            const Point eval = dummy.point_at(greville[static_cast<size_t>(i)], 0.0);
            const Vector err = targets[static_cast<size_t>(i)] - eval;
            cvs[static_cast<size_t>(i)] = cvs[static_cast<size_t>(i)] + err;
        }
    }

    return cvs;
}

/// Evaluates clamped uniform cubic Greville fractions in [0, 1] for n >= 4 control points.
std::vector<double> clamped_cubic_fractions(int n) {

    const double span = static_cast<double>(std::max(1, n - 3));
    std::vector<double> fracs(static_cast<size_t>(n), 0.0);
    for (int i = 0; i < n; i++) {
        const double k0 = std::clamp(static_cast<double>(i - 2), 0.0, span);
        const double k1 = std::clamp(static_cast<double>(i - 1), 0.0, span);
        const double k2 = std::clamp(static_cast<double>(i), 0.0, span);
        fracs[static_cast<size_t>(i)] = (k0 + k1 + k2) / (3.0 * span);
    }

    return fracs;
}

NurbsSurface torus_vault_surface(
    double major_radius,
    double half_width,
    double arch_rise,
    double sector_degrees
) {

    const bool full_circle = std::abs(sector_degrees - 360.0) <= 1e-4;
    const int n_across = 7;
    const std::vector<double> v_fracs = clamped_cubic_fractions(n_across);
    const double max_theta = 66.0 * Tolerance::TO_RADIANS;
    const double cos_base = std::cos(max_theta);

    // Collocate the cross-arch profile in (r_offset, z) so the clamped v-direction has exact circular curvature.
    std::vector<Point> profile_targets(static_cast<size_t>(n_across));
    for (int j = 0; j < n_across; j++) {
        const double s = -1.0 + 2.0 * v_fracs[static_cast<size_t>(j)];
        const double theta = s * max_theta;
        const double r = major_radius + half_width * (std::sin(theta) / std::sin(max_theta));
        const double z = arch_rise * (std::cos(theta) - cos_base) / (1.0 - cos_base);
        profile_targets[static_cast<size_t>(j)] = Point(r, 0.0, z);
    }
    const std::vector<Point> profile_cvs = solve_clamped_cubic_1d(profile_targets);

    if (full_circle) {
        const int n_unique = 16;
        const int n_around = n_unique + 3;
        const double dphi = Tolerance::TWO_PI / static_cast<double>(n_unique);
        const double radial_comp = 3.0 / (2.0 + std::cos(dphi));
        std::vector<Point> pts;
        pts.reserve(static_cast<size_t>(n_around * n_across));
        for (int i = 0; i < n_around; i++) {
            const double phi = dphi * static_cast<double>(i % n_unique);
            for (int j = 0; j < n_across; j++) {
                const double r = profile_cvs[static_cast<size_t>(j)][0] * radial_comp;
                const double z = profile_cvs[static_cast<size_t>(j)][2];
                pts.emplace_back(r * std::cos(phi), r * std::sin(phi), z);
            }
        }
        return NurbsSurface::create(true, false, 3, 3, n_around, n_across, pts);
    }

    const int n_around = std::max(9, static_cast<int>(std::ceil(sector_degrees / 22.5)) + 1);
    const std::vector<double> u_fracs = clamped_cubic_fractions(n_around);
    const double total_rad = sector_degrees * Tolerance::TO_RADIANS;

    // Collocate each constant-v hoop arc at Greville abscissae so open sector ends have zero clamped B-spline bulge.
    std::vector<std::vector<Point>> cols(
        static_cast<size_t>(n_around), std::vector<Point>(static_cast<size_t>(n_across))
    );
    for (int j = 0; j < n_across; j++) {
        const double r = profile_cvs[static_cast<size_t>(j)][0];
        const double z = profile_cvs[static_cast<size_t>(j)][2];
        std::vector<Point> hoop_targets(static_cast<size_t>(n_around));
        for (int i = 0; i < n_around; i++) {
            const double phi = total_rad * u_fracs[static_cast<size_t>(i)];
            hoop_targets[static_cast<size_t>(i)] = Point(r * std::cos(phi), r * std::sin(phi), z);
        }
        const std::vector<Point> hoop_cvs = solve_clamped_cubic_1d(hoop_targets);
        for (int i = 0; i < n_around; i++)
            cols[static_cast<size_t>(i)][static_cast<size_t>(j)] = hoop_cvs[static_cast<size_t>(i)];
    }

    std::vector<Point> pts;
    pts.reserve(static_cast<size_t>(n_around * n_across));
    for (int i = 0; i < n_around; i++)
        for (int j = 0; j < n_across; j++)
            pts.push_back(cols[static_cast<size_t>(i)][static_cast<size_t>(j)]);

    return NurbsSurface::create(false, false, 3, 3, n_around, n_across, pts);
}

NurbsSurface funnel_vault_surface(double outer_radius, double throat_radius, double height) {

    const int n_unique = 16;
    const int n_around = n_unique + 3;
    const int n_up = 7;
    const std::vector<double> v_fracs = clamped_cubic_fractions(n_up);
    const double dphi = Tolerance::TWO_PI / static_cast<double>(n_unique);
    const double radial_comp = 3.0 / (2.0 + std::cos(dphi));

    std::vector<Point> meridian_targets(static_cast<size_t>(n_up));
    for (int j = 0; j < n_up; j++) {
        const double t = v_fracs[static_cast<size_t>(j)];
        const double r = throat_radius + (outer_radius - throat_radius) * std::pow(t, 1.75);
        const double z = height * std::sin(0.5 * Tolerance::PI * t);
        meridian_targets[static_cast<size_t>(j)] = Point(r, 0.0, z);
    }
    const std::vector<Point> meridian_cvs = solve_clamped_cubic_1d(meridian_targets);

    std::vector<Point> pts;
    pts.reserve(static_cast<size_t>(n_around * n_up));
    for (int i = 0; i < n_around; i++) {
        const double phi = dphi * static_cast<double>(i % n_unique);
        for (int j = 0; j < n_up; j++) {
            const double r = meridian_cvs[static_cast<size_t>(j)][0] * radial_comp;
            const double z = meridian_cvs[static_cast<size_t>(j)][2];
            pts.emplace_back(r * std::cos(phi), r * std::sin(phi), z);
        }
    }

    return NurbsSurface::create(true, false, 3, 3, n_around, n_up, pts);
}

std::vector<NurbsSurface> groin_vault_surfaces(double span, double crown_rise, double arch_rise) {

    const double h = 0.5 * span;
    std::vector<NurbsSurface> patches;
    patches.reserve(4);

    for (int q = 0; q < 4; q++) {
        const double a0 = static_cast<double>(q) * 0.5 * Tolerance::PI;
        const Vector ex(std::cos(a0), std::sin(a0), 0.0);
        const Vector ey(-std::sin(a0), std::cos(a0), 0.0);

        std::array<std::array<Point, 4>, 4> grid;
        for (int i = 0; i < 4; i++) {
            const double u = static_cast<double>(i) / 3.0;
            for (int j = 0; j < 4; j++) {
                const double v = static_cast<double>(j) / 3.0;
                const Vector plan = ex * (u * h) + ey * (v * h);
                const double z_crown = crown_rise * (1.0 - u * u) * (1.0 - v * v);
                const double z_u_ridge = arch_rise * (1.0 - v * v) * (u * u);
                const double z_v_ridge = arch_rise * (1.0 - u * u) * (v * v);
                grid[i][j] = Point(plan[0], plan[1], std::max(0.0, z_crown + z_u_ridge + z_v_ridge));
            }
        }
        patches.push_back(bicubic_from_grid(grid));
    }

    return patches;
}

std::vector<NurbsSurface> sewn_funnel_surfaces(
    double outer_radius,
    double throat_radius,
    double height,
    double petal_rise,
    int lobes
) {

    // Build a single C2-periodic uniform cubic spline of M = 4 * count spans around 2*pi, then extract
    // each petal [4*k, 4*(k+1)] via exact Bohm knot clamping so all radial patch seams are C2-continuous.
    const int count = std::max(3, lobes);
    const int spans_per_lobe = 4;
    const int total_spans = count * spans_per_lobe;
    const int n_up = 7;
    const std::vector<double> v_fracs = clamped_cubic_fractions(n_up);
    const double dphi = Tolerance::TWO_PI / static_cast<double>(total_spans);
    const double radial_comp = 3.0 / (2.0 + std::cos(dphi));
    const double lobe_comp = 3.0 / (2.0 + std::cos(static_cast<double>(count) * dphi));

    std::vector<std::vector<Point>> periodic_cols(
        static_cast<size_t>(total_spans), std::vector<Point>(static_cast<size_t>(n_up))
    );
    for (int m = 0; m < total_spans; m++) {
        const double phi = dphi * static_cast<double>(m);
        const double cos_wave = std::cos(static_cast<double>(count) * phi) * lobe_comp;
        const double lobe = 0.5 * (1.0 - cos_wave);

        std::vector<Point> meridian_targets(static_cast<size_t>(n_up));
        for (int j = 0; j < n_up; j++) {
            const double v = v_fracs[static_cast<size_t>(j)];
            const double flare = std::pow(v, 1.65);
            const double r =
                (throat_radius + (outer_radius * (0.84 + 0.22 * lobe) - throat_radius) * flare) *
                radial_comp;
            const double z =
                height * std::sin(0.5 * Tolerance::PI * v) + petal_rise * flare * lobe;
            meridian_targets[static_cast<size_t>(j)] = Point(r * std::cos(phi), r * std::sin(phi), z);
        }
        periodic_cols[static_cast<size_t>(m)] = solve_clamped_cubic_1d(meridian_targets);
    }

    std::vector<NurbsSurface> patches;
    patches.reserve(static_cast<size_t>(count));
    const int patch_u_cvs = 7;

    for (int k = 0; k < count; k++) {
        const int base = k * spans_per_lobe;
        std::array<int, 7> ring_ids;
        for (int offset = -1; offset <= 5; offset++) {
            const int wrapped = (base + offset + total_spans) % total_spans;
            ring_ids[static_cast<size_t>(offset + 1)] = wrapped;
        }

        std::vector<Point> flat;
        flat.reserve(static_cast<size_t>(patch_u_cvs * n_up));
        std::array<std::vector<Point>, 7> clamped_u;
        for (int c = 0; c < 7; c++)
            clamped_u[static_cast<size_t>(c)].resize(static_cast<size_t>(n_up));

        for (int j = 0; j < n_up; j++) {
            const Point& q_m1 = periodic_cols[static_cast<size_t>(ring_ids[0])][static_cast<size_t>(j)];
            const Point& q_0 = periodic_cols[static_cast<size_t>(ring_ids[1])][static_cast<size_t>(j)];
            const Point& q_1 = periodic_cols[static_cast<size_t>(ring_ids[2])][static_cast<size_t>(j)];
            const Point& q_2 = periodic_cols[static_cast<size_t>(ring_ids[3])][static_cast<size_t>(j)];
            const Point& q_3 = periodic_cols[static_cast<size_t>(ring_ids[4])][static_cast<size_t>(j)];
            const Point& q_4 = periodic_cols[static_cast<size_t>(ring_ids[5])][static_cast<size_t>(j)];
            const Point& q_5 = periodic_cols[static_cast<size_t>(ring_ids[6])][static_cast<size_t>(j)];

            clamped_u[0][static_cast<size_t>(j)] = Point(
                (q_m1[0] + 4.0 * q_0[0] + q_1[0]) / 6.0,
                (q_m1[1] + 4.0 * q_0[1] + q_1[1]) / 6.0,
                (q_m1[2] + 4.0 * q_0[2] + q_1[2]) / 6.0
            );
            clamped_u[1][static_cast<size_t>(j)] = Point(
                (2.0 * q_0[0] + q_1[0]) / 3.0,
                (2.0 * q_0[1] + q_1[1]) / 3.0,
                (2.0 * q_0[2] + q_1[2]) / 3.0
            );
            clamped_u[2][static_cast<size_t>(j)] = q_1;
            clamped_u[3][static_cast<size_t>(j)] = q_2;
            clamped_u[4][static_cast<size_t>(j)] = q_3;
            clamped_u[5][static_cast<size_t>(j)] = Point(
                (q_3[0] + 2.0 * q_4[0]) / 3.0,
                (q_3[1] + 2.0 * q_4[1]) / 3.0,
                (q_3[2] + 2.0 * q_4[2]) / 3.0
            );
            clamped_u[6][static_cast<size_t>(j)] = Point(
                (q_3[0] + 4.0 * q_4[0] + q_5[0]) / 6.0,
                (q_3[1] + 4.0 * q_4[1] + q_5[1]) / 6.0,
                (q_3[2] + 4.0 * q_4[2] + q_5[2]) / 6.0
            );
        }

        for (int i = 0; i < patch_u_cvs; i++)
            for (int j = 0; j < n_up; j++)
                flat.push_back(clamped_u[static_cast<size_t>(i)][static_cast<size_t>(j)]);

        patches.push_back(NurbsSurface::create(false, false, 3, 3, patch_u_cvs, n_up, flat));
    }

    return patches;
}

// ═══════════════════════════════════════════════════════════════════════════
// SurfaceDivision Implementation
// ═══════════════════════════════════════════════════════════════════════════

double SurfaceDivision::ArcTable::at_fraction(double s) const {

    const double clamped = std::clamp(s, 0.0, 1.0);
    if (fractions.size() < 2 || total <= 1e-9)
        return clamped;

    const double target = clamped * total;
    for (size_t k = 0; k + 1 < lengths.size(); k++) {
        if (target <= lengths[k + 1] + 1e-12) {
            const double span = lengths[k + 1] - lengths[k];
            const double alpha = span > 1e-12 ? (target - lengths[k]) / span : 0.0;
            return fractions[k] + std::clamp(alpha, 0.0, 1.0) * (fractions[k + 1] - fractions[k]);
        }
    }

    return 1.0;
}

SurfaceDivision::SurfaceDivision(
    const std::vector<NurbsSurface>& surfaces,
    double size_u,
    double size_v
)
    : surfaces(surfaces),
      size_u(std::max(10.0, size_u)),
      size_v(std::max(10.0, size_v)) {

    // Sewn boundary sides and per-side arc-length tables
    _sides = compute_sides(_side_arcs);

    // Strip division counts and consistent sheet normals across seams
    _grids = compute_grids();

    // Welded base grid vertices, sheet normals, global parities, and quad cells
    compute_cells();
}

SurfaceDivision SurfaceDivision::from_brep(const BRep& brep, double size_u, double size_v) {

    std::vector<NurbsSurface> patches;
    for (const NurbsSurface& surface : brep.m_surfaces)
        if (surface.is_valid())
            patches.push_back(surface);

    return SurfaceDivision(patches, size_u, size_v);
}

std::vector<std::array<std::pair<size_t, bool>, 4>> SurfaceDivision::compute_sides(
    std::vector<ArcTable>& out_arcs
) const {

    const int samples = 7;
    struct SideSamples {
        std::array<Point, 7> pts;
        double length = 0.0;
    };

    std::vector<std::array<SideSamples, 4>> sampled(surfaces.size());
    for (size_t s = 0; s < surfaces.size(); s++) {
        for (int a = 0; a < 4; a++) {
            for (int k = 0; k < samples; k++) {
                const double t = static_cast<double>(k) / (samples - 1);
                sampled[s][a].pts[k] = side_point(surfaces[s], a, t);
            }
            double len = 0.0;
            for (int k = 0; k + 1 < samples; k++)
                len += sampled[s][a].pts[k].distance(sampled[s][a].pts[k + 1]);
            sampled[s][a].length = len;
        }
    }

    std::vector<std::array<std::pair<size_t, bool>, 4>> result(surfaces.size());
    std::vector<std::array<bool, 4>> assigned(surfaces.size(), {false, false, false, false});
    out_arcs.clear();

    const int arc_samples = 128;
    for (size_t s1 = 0; s1 < surfaces.size(); s1++) {
        for (int a1 = 0; a1 < 4; a1++) {
            if (assigned[s1][a1])
                continue;

            const size_t canonical = out_arcs.size();
            ArcTable table;
            table.fractions.resize(arc_samples + 1);
            table.lengths.resize(arc_samples + 1, 0.0);
            Point prev = side_point(surfaces[s1], a1, 0.0);
            table.fractions[0] = 0.0;
            for (int k = 1; k <= arc_samples; k++) {
                const double f = static_cast<double>(k) / arc_samples;
                const Point cur = side_point(surfaces[s1], a1, f);
                table.fractions[k] = f;
                table.lengths[k] = table.lengths[k - 1] + prev.distance(cur);
                prev = cur;
            }
            table.total = table.lengths.back();
            out_arcs.push_back(table);

            result[s1][a1] = {canonical, false};
            assigned[s1][a1] = true;

            if (sampled[s1][a1].length <= 1e-3)
                continue;

            const double match_tol = std::max(0.5, 1e-3 * sampled[s1][a1].length);
            for (size_t s2 = s1; s2 < surfaces.size(); s2++) {
                for (int a2 = 0; a2 < 4; a2++) {
                    if (assigned[s2][a2] || sampled[s2][a2].length <= 1e-3)
                        continue;

                    bool same_fwd = true;
                    bool same_rev = true;
                    for (int k = 0; k < samples; k++) {
                        if (sampled[s1][a1].pts[k].distance(sampled[s2][a2].pts[k]) > match_tol)
                            same_fwd = false;
                        if (sampled[s1][a1].pts[k].distance(sampled[s2][a2].pts[samples - 1 - k]) >
                            match_tol)
                            same_rev = false;
                    }
                    if (same_fwd || same_rev) {
                        result[s2][a2] = {canonical, !same_fwd && same_rev};
                        assigned[s2][a2] = true;
                    }
                }
            }
        }
    }

    return result;
}

std::vector<SurfaceDivision::Grid> SurfaceDivision::compute_grids() const {

    std::vector<Grid> grids(surfaces.size());
    if (surfaces.empty())
        return grids;

    const Vector n0 = normal_unit(surfaces[0], 0.5, 0.5);
    grids[0].flip_normal = (n0[2] < -1e-6);
    grids[0].transpose_uv = false;

    std::map<size_t, std::vector<std::pair<size_t, int>>> side_owners;
    for (size_t s = 0; s < surfaces.size(); s++)
        for (int a = 0; a < 4; a++)
            side_owners[_sides[s][a].first].emplace_back(s, a);

    std::vector<bool> visited(surfaces.size(), false);
    for (size_t start = 0; start < surfaces.size(); start++) {
        if (visited[start])
            continue;
        if (start > 0)
            grids[start].flip_normal = (normal_unit(surfaces[start], 0.5, 0.5)[2] < -1e-6);

        visited[start] = true;
        std::vector<size_t> queue = {start};
        size_t head = 0;
        while (head < queue.size()) {
            const size_t s = queue[head++];
            for (int a = 0; a < 4; a++) {
                const size_t sid = _sides[s][a].first;
                if (_side_arcs[sid].total <= 1e-3)
                    continue;
                for (const std::pair<size_t, int>& nb : side_owners[sid]) {
                    if (nb.first == s || visited[nb.first])
                        continue;

                    const Point mid_s = side_point(surfaces[s], a, 0.5);
                    const Point in_s = eval_unit(surfaces[s], 0.5, 0.5);
                    const Point in_nb = eval_unit(surfaces[nb.first], 0.5, 0.5);
                    const Vector raw_ns = normal_unit(surfaces[s], 0.5, 0.5) *
                                          (grids[s].flip_normal ? -1.0 : 1.0);
                    const Vector raw_nnb = normal_unit(surfaces[nb.first], 0.5, 0.5);
                    const Vector seam_tan =
                        (side_point(surfaces[s], a, 0.6) - side_point(surfaces[s], a, 0.4))
                            .normalized();
                    const Vector cross_s = (in_s - mid_s).cross(seam_tan);
                    const Vector cross_nb = (mid_s - in_nb).cross(seam_tan);
                    if (cross_s.magnitude_squared() > 1e-6 && cross_nb.magnitude_squared() > 1e-6) {
                        const double sign_s = cross_s.dot(raw_ns) >= 0.0 ? 1.0 : -1.0;
                        const double sign_nb = cross_nb.dot(raw_nnb) >= 0.0 ? 1.0 : -1.0;
                        grids[nb.first].flip_normal = (sign_s * sign_nb < 0.0);
                    } else {
                        grids[nb.first].flip_normal = (raw_ns.dot(raw_nnb) < 0.0);
                    }

                    const bool s_varies_u = (a % 2 == 0);
                    const bool nb_varies_u = (nb.second % 2 == 0);
                    grids[nb.first].transpose_uv =
                        (s_varies_u == nb_varies_u) ? grids[s].transpose_uv : !grids[s].transpose_uv;

                    visited[nb.first] = true;
                    queue.push_back(nb.first);
                }
            }
        }
    }

    const size_t n_sides = _side_arcs.size();
    std::vector<int> parent(n_sides);
    for (size_t i = 0; i < n_sides; i++)
        parent[i] = static_cast<int>(i);

    for (size_t s = 0; s < surfaces.size(); s++) {
        const int r0 = uf_find(parent, static_cast<int>(_sides[s][0].first));
        const int r2 = uf_find(parent, static_cast<int>(_sides[s][2].first));
        if (r0 != r2)
            parent[r2] = r0;
        const int r1 = uf_find(parent, static_cast<int>(_sides[s][1].first));
        const int r3 = uf_find(parent, static_cast<int>(_sides[s][3].first));
        if (r1 != r3)
            parent[r3] = r1;
    }

    std::map<int, std::vector<double>> class_lengths;
    std::map<int, int> class_primary_votes;
    for (size_t s = 0; s < surfaces.size(); s++) {
        const int ru = uf_find(parent, static_cast<int>(_sides[s][0].first));
        const int rv = uf_find(parent, static_cast<int>(_sides[s][1].first));
        const double len_u =
            0.5 * (_side_arcs[_sides[s][0].first].total + _side_arcs[_sides[s][2].first].total);
        const double len_v =
            0.5 * (_side_arcs[_sides[s][1].first].total + _side_arcs[_sides[s][3].first].total);
        if (len_u > 1e-3)
            class_lengths[ru].push_back(len_u);
        if (len_v > 1e-3)
            class_lengths[rv].push_back(len_v);
        class_primary_votes[ru] += grids[s].transpose_uv ? -1 : 1;
        class_primary_votes[rv] += grids[s].transpose_uv ? 1 : -1;
    }

    std::map<int, int> class_divisions;
    for (size_t i = 0; i < n_sides; i++) {
        const int r = uf_find(parent, static_cast<int>(i));
        if (class_divisions.count(r))
            continue;
        double sum = 0.0;
        for (const double l : class_lengths[r])
            sum += l;
        const double avg = class_lengths[r].empty()
                               ? size_u
                               : sum / static_cast<double>(class_lengths[r].size());
        const double target = class_primary_votes[r] >= 0 ? size_u : size_v;
        const int pairs = std::max(1, static_cast<int>(std::lround(avg / (2.0 * target))));
        class_divisions[r] = 2 * pairs;
    }

    for (size_t s = 0; s < surfaces.size(); s++) {
        grids[s].u = class_divisions[uf_find(parent, static_cast<int>(_sides[s][0].first))];
        grids[s].v = class_divisions[uf_find(parent, static_cast<int>(_sides[s][1].first))];
    }

    return grids;
}

std::pair<Point, Vector> SurfaceDivision::evaluate(size_t s, double gi, double gj) const {

    const Grid& g = _grids[s];
    const double xi = std::clamp(gi / static_cast<double>(std::max(1, g.u)), 0.0, 1.0);
    const double eta = std::clamp(gj / static_cast<double>(std::max(1, g.v)), 0.0, 1.0);

    std::array<double, 4> mapped = {0.0, 0.0, 0.0, 0.0};
    const std::array<double, 4> params = {xi, eta, xi, eta};
    for (int a = 0; a < 4; a++) {
        const std::pair<size_t, bool>& ref = _sides[s][a];
        const ArcTable& arc = _side_arcs[ref.first];
        if (arc.total <= 1e-3)
            mapped[a] = params[a];
        else if (!ref.second)
            mapped[a] = arc.at_fraction(params[a]);
        else
            mapped[a] = 1.0 - arc.at_fraction(1.0 - params[a]);
    }

    const double fu = (1.0 - eta) * mapped[0] + eta * mapped[2];
    const double fv = (1.0 - xi) * mapped[3] + xi * mapped[1];

    const Point pt = eval_unit(surfaces[s], fu, fv);
    Vector nrm = normal_unit(surfaces[s], fu, fv);
    if (g.flip_normal)
        nrm = -nrm;

    return {pt, nrm};
}

std::pair<Point, Vector> SurfaceDivision::evaluate_strip(size_t s, double a, double b) const {

    const Grid& g = _grids[s];
    const double cu = std::clamp(g.transpose_uv ? b : a, 0.0, static_cast<double>(g.u));
    const double cv = std::clamp(g.transpose_uv ? a : b, 0.0, static_cast<double>(g.v));

    return evaluate(s, cu, cv);
}

void SurfaceDivision::compute_cells() {

    _points.clear();
    _normals.clear();
    _parities.clear();
    _cells.clear();

    const double weld_tol_sq = 1e-2 * 1e-2;
    for (size_t s = 0; s < surfaces.size(); s++) {
        const int nu = _grids[s].u;
        const int nv = _grids[s].v;
        std::vector<std::vector<size_t>> grid_v(
            static_cast<size_t>(nu + 1), std::vector<size_t>(static_cast<size_t>(nv + 1), 0)
        );
        for (int i = 0; i <= nu; i++) {
            for (int j = 0; j <= nv; j++) {
                const std::pair<Point, Vector> ev = evaluate(s, i, j);
                size_t found = _points.size();
                for (size_t k = 0; k < _points.size(); k++) {
                    if ((_points[k] - ev.first).magnitude_squared() <= weld_tol_sq) {
                        _normals[k] = _normals[k] + ev.second;
                        found = k;
                        break;
                    }
                }
                if (found == _points.size()) {
                    _points.push_back(ev.first);
                    _normals.push_back(ev.second);
                    _parities.push_back((i + j) % 2);
                }
                grid_v[i][j] = found;
            }
        }

        for (int i = 0; i < nu; i++) {
            for (int j = 0; j < nv; j++) {
                std::array<size_t, 4> c = {
                    grid_v[i][j], grid_v[i + 1][j], grid_v[i + 1][j + 1], grid_v[i][j + 1]
                };
                if (_grids[s].flip_normal)
                    std::swap(c[1], c[3]);
                _cells.push_back({s, c});
            }
        }
    }

    for (Vector& n : _normals)
        if (!n.normalize_self())
            n = Vector(0.0, 0.0, 1.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Subdivision Modes
// ═══════════════════════════════════════════════════════════════════════════

Mesh SurfaceDivision::quads(double stagger) const {

    if (std::abs(stagger) < 1e-6) {
        std::vector<std::vector<Point>> polys;
        std::vector<std::vector<Vector>> norms;
        for (const Cell& cell : _cells) {
            std::vector<Point> ring;
            std::vector<Vector> rn;
            for (const size_t v : cell.corners) {
                ring.push_back(_points[v]);
                rn.push_back(_normals[v]);
            }
            polys.push_back(ring);
            norms.push_back(rn);
        }
        return build_welded_mesh(polys, norms);
    }

    const double shift = std::clamp(stagger, 0.1, 0.9);
    std::vector<std::vector<Point>> polys;
    std::vector<std::vector<Vector>> norms;

    for (size_t s = 0; s < surfaces.size(); s++) {
        const Grid& g = _grids[s];
        const int na = g.transpose_uv ? g.v : g.u;
        const int nb = g.transpose_uv ? g.u : g.v;
        const bool closed_a =
            (_sides[s][g.transpose_uv ? 0 : 3].first == _sides[s][g.transpose_uv ? 2 : 1].first);

        std::vector<std::vector<double>> breaks(static_cast<size_t>(nb));
        for (int row = 0; row < nb; row++) {
            const double off = (row % 2 != 0) ? shift : 0.0;
            if (!closed_a && off > 1e-6)
                breaks[row].push_back(0.0);
            for (int k = 0; k < na; k++) {
                const double pos = static_cast<double>(k) + off;
                if (pos <= static_cast<double>(na) + 1e-6)
                    breaks[row].push_back(std::min(static_cast<double>(na), pos));
            }
            if (breaks[row].empty() || breaks[row].back() < static_cast<double>(na) - 1e-6)
                breaks[row].push_back(static_cast<double>(na));
        }

        for (int j = 0; j < nb; j++) {
            const std::vector<double>& cur = breaks[j];
            std::vector<double> lower_splits = cur;
            std::vector<double> upper_splits = cur;
            if (j > 0)
                lower_splits.insert(lower_splits.end(), breaks[j - 1].begin(), breaks[j - 1].end());
            if (j + 1 < nb)
                upper_splits.insert(upper_splits.end(), breaks[j + 1].begin(), breaks[j + 1].end());
            std::sort(lower_splits.begin(), lower_splits.end());
            std::sort(upper_splits.begin(), upper_splits.end());

            for (size_t m = 0; m + 1 < cur.size(); m++) {
                const double a0 = cur[m];
                const double a1 = cur[m + 1];
                if (a1 - a0 <= 1e-4)
                    continue;

                std::vector<Point> ring;
                std::vector<Vector> rn;
                for (const double x : lower_splits) {
                    if (x >= a0 - 1e-6 && x <= a1 + 1e-6) {
                        const std::pair<Point, Vector> ev =
                            g.transpose_uv ? evaluate(s, j, x) : evaluate(s, x, j);
                        if (ring.empty() || ring.back().distance(ev.first) > 1e-3) {
                            ring.push_back(ev.first);
                            rn.push_back(ev.second);
                        }
                    }
                }
                for (int idx = static_cast<int>(upper_splits.size()) - 1; idx >= 0; idx--) {
                    const double x = upper_splits[idx];
                    if (x >= a0 - 1e-6 && x <= a1 + 1e-6) {
                        const std::pair<Point, Vector> ev =
                            g.transpose_uv ? evaluate(s, j + 1, x) : evaluate(s, x, j + 1);
                        if (ring.empty() || ring.back().distance(ev.first) > 1e-3) {
                            ring.push_back(ev.first);
                            rn.push_back(ev.second);
                        }
                    }
                }
                if (ring.size() >= 3) {
                    polys.push_back(ring);
                    norms.push_back(rn);
                }
            }
        }
    }

    return build_welded_mesh(polys, norms);
}

Mesh SurfaceDivision::diamonds() const {

    std::vector<std::vector<size_t>> quad_faces;
    for (const Cell& cell : _cells) {
        std::vector<size_t> f;
        for (const size_t v : cell.corners)
            if (f.empty() || f.back() != v)
                f.push_back(v);
        while (f.size() > 1 && f.back() == f.front())
            f.pop_back();
        if (f.size() >= 3)
            quad_faces.push_back(f);
    }

    std::vector<std::map<size_t, size_t>> next_around(_points.size());
    std::vector<std::set<size_t>> incoming(_points.size());
    for (const std::vector<size_t>& q : quad_faces) {
        const size_t m = q.size();
        for (size_t k = 0; k < m; k++) {
            const size_t v = q[k];
            const size_t out_nb = q[(k + 1) % m];
            const size_t in_nb = q[(k + m - 1) % m];
            next_around[v][out_nb] = in_nb;
            incoming[v].insert(in_nb);
        }
    }

    std::vector<std::vector<Point>> polys;
    std::vector<std::vector<Vector>> norms;

    for (size_t v = 0; v < _points.size(); v++) {
        if (_parities[v] != 0 || next_around[v].empty())
            continue;

        size_t start_nb = next_around[v].begin()->first;
        bool on_boundary = false;
        for (const std::pair<const size_t, size_t>& kv : next_around[v]) {
            if (!incoming[v].count(kv.first)) {
                start_nb = kv.first;
                on_boundary = true;
                break;
            }
        }

        std::vector<size_t> fan;
        size_t cur = start_nb;
        for (size_t step = 0; step < next_around[v].size() + 1; step++) {
            fan.push_back(cur);
            if (!next_around[v].count(cur))
                break;
            cur = next_around[v][cur];
            if (!on_boundary && cur == start_nb)
                break;
        }

        // Smooth boundary edge (2 quads -> 3 odd neighbors): triangle {fan[0], fan[1], fan[2]} is convex and strictly planar!
        // Patch corner (1 quad -> 2 odd neighbors): corner triangle {v, fan[0], fan[1]}.
        std::vector<size_t> ring_ids = fan;
        if (on_boundary && fan.size() == 2)
            ring_ids = {v, fan[0], fan[1]};

        if (ring_ids.size() >= 3) {
            std::vector<Point> ring;
            std::vector<Vector> rn;
            for (const size_t id : ring_ids) {
                ring.push_back(_points[id]);
                rn.push_back(_normals[id]);
            }
            polys.push_back(ring);
            norms.push_back(rn);
        }
    }

    return build_welded_mesh(polys, norms);
}

Mesh SurfaceDivision::hexagons(double side) const {

    const double t = std::clamp(side, 0.1, 0.45);
    std::vector<std::vector<Point>> polys;
    std::vector<std::vector<Vector>> norms;
    std::vector<size_t> patch_ids;
    std::vector<bool> boundary_flags;

    for (size_t s = 0; s < surfaces.size(); s++) {
        const Grid& g = _grids[s];
        const int num = g.transpose_uv ? g.v : g.u;
        const int num2 = g.transpose_uv ? g.u : g.v;

        for (int i = 0; i <= num; i++) {
            for (int j = 0; j <= num2; j++) {
                if ((i + j) % 2 != 0)
                    continue;

                const std::pair<Point, Vector> item =
                    (i > 0) ? evaluate_strip(s, i - 1, std::min(static_cast<double>(num2), j + t))
                            : evaluate_strip(s, i, j);
                const std::pair<Point, Vector> item2 =
                    (i > 0) ? evaluate_strip(s, i - 1, std::max(0.0, j - t))
                            : evaluate_strip(s, i, j);
                const std::pair<Point, Vector> item4 =
                    (j > 0) ? evaluate_strip(s, i, std::max(0.0, j - (1.0 - t)))
                            : evaluate_strip(s, i, j);
                const std::pair<Point, Vector> item5 =
                    (i < num)
                        ? evaluate_strip(s, i + 1, std::min(static_cast<double>(num2), j + t))
                        : evaluate_strip(s, i, j);
                const std::pair<Point, Vector> item6 =
                    (i < num) ? evaluate_strip(s, i + 1, std::max(0.0, j - t))
                              : evaluate_strip(s, i, j);
                const std::pair<Point, Vector> item7 =
                    (j < num2)
                        ? evaluate_strip(s, i, std::min(static_cast<double>(num2), j + (1.0 - t)))
                        : evaluate_strip(s, i, j);

                std::vector<std::pair<Point, Vector>> cell_pts;
                bool is_bnd = true;
                if (i > 0 && j > 0 && i < num && j < num2) {
                    cell_pts = {item, item2, item4, item6, item5, item7};
                    is_bnd = false;
                } else if (i == 0 && j == 0) {
                    cell_pts = {item7, item, item6, item5};
                } else if (i == 0 && j == num2) {
                    cell_pts = {item, item4, item6, item5};
                } else if (i == num && j == 0) {
                    cell_pts = {item, item2, item6, item7};
                } else if (i == num && j == num2) {
                    cell_pts = {item, item2, item4, item7};
                } else if (i == 0 && j > 0 && j < num2) {
                    cell_pts = {item7, item4, item6, item5};
                } else if (i == num && j > 0 && j < num2) {
                    cell_pts = {item, item2, item4, item7};
                } else if (j == 0 && i > 0 && i < num) {
                    cell_pts = {item, item2, item6, item5, item7};
                } else if (j == num2 && i > 0 && i < num) {
                    cell_pts = {item, item2, item4, item6, item5};
                }

                std::vector<Point> ring;
                std::vector<Vector> rn;
                for (const std::pair<Point, Vector>& ev : cell_pts) {
                    if (ring.empty() || ring.back().distance(ev.first) > 1e-3) {
                        ring.push_back(ev.first);
                        rn.push_back(ev.second);
                    }
                }
                while (ring.size() > 1 && ring.back().distance(ring.front()) <= 1e-3) {
                    ring.pop_back();
                    rn.pop_back();
                }
                if (ring.size() >= 3) {
                    polys.push_back(ring);
                    norms.push_back(rn);
                    patch_ids.push_back(s);
                    boundary_flags.push_back(is_bnd);
                }
            }
        }
    }

    if (surfaces.size() > 1)
        merge_seam_cells(polys, norms, patch_ids, boundary_flags);

    return build_welded_mesh(polys, norms);
}

Mesh SurfaceDivision::dual_hexagons() const {

    std::vector<std::array<size_t, 3>> tris;
    for (const Cell& cell : _cells) {
        std::vector<size_t> q;
        for (const size_t v : cell.corners)
            if (q.empty() || q.back() != v)
                q.push_back(v);
        while (q.size() > 1 && q.back() == q.front())
            q.pop_back();
        if (q.size() == 3) {
            tris.push_back({q[0], q[1], q[2]});
        } else if (q.size() == 4) {
            const double d02 = _points[q[0]].distance(_points[q[2]]);
            const double d13 = _points[q[1]].distance(_points[q[3]]);
            if (d02 <= d13 + 1e-6) {
                tris.push_back({q[0], q[1], q[2]});
                tris.push_back({q[0], q[2], q[3]});
            } else {
                tris.push_back({q[0], q[1], q[3]});
                tris.push_back({q[1], q[2], q[3]});
            }
        }
    }

    std::vector<Point> centroids(tris.size());
    std::vector<Vector> tri_norms(tris.size());
    std::vector<std::map<size_t, std::pair<size_t, size_t>>> fan(_points.size());
    std::vector<std::set<size_t>> incoming(_points.size());

    for (size_t t = 0; t < tris.size(); t++) {
        const std::array<size_t, 3>& tri = tris[t];
        centroids[t] = Point(
            (_points[tri[0]][0] + _points[tri[1]][0] + _points[tri[2]][0]) / 3.0,
            (_points[tri[0]][1] + _points[tri[1]][1] + _points[tri[2]][1]) / 3.0,
            (_points[tri[0]][2] + _points[tri[1]][2] + _points[tri[2]][2]) / 3.0
        );
        Vector tn = _normals[tri[0]] + _normals[tri[1]] + _normals[tri[2]];
        if (!tn.normalize_self())
            tn = Vector(0.0, 0.0, 1.0);
        tri_norms[t] = tn;

        for (int k = 0; k < 3; k++) {
            const size_t v = tri[k];
            const size_t a = tri[(k + 1) % 3];
            const size_t b = tri[(k + 2) % 3];
            fan[v][a] = {t, b};
            incoming[v].insert(b);
        }
    }

    std::vector<std::vector<Point>> polys;
    std::vector<std::vector<Vector>> norms;
    for (size_t v = 0; v < _points.size(); v++) {
        if (fan[v].empty())
            continue;

        size_t start_nb = fan[v].begin()->first;
        bool boundary = false;
        for (const std::pair<const size_t, std::pair<size_t, size_t>>& kv : fan[v]) {
            if (!incoming[v].count(kv.first)) {
                start_nb = kv.first;
                boundary = true;
                break;
            }
        }

        std::vector<Point> ring;
        std::vector<Vector> rn;
        if (boundary) {
            ring.push_back(Point::mid_point(_points[v], _points[start_nb]));
            rn.push_back((_normals[v] + _normals[start_nb]).normalized());
        }

        size_t cur = start_nb;
        size_t end_nb = start_nb;
        for (size_t step = 0; step < fan[v].size(); step++) {
            if (!fan[v].count(cur))
                break;
            const std::pair<size_t, size_t> step_info = fan[v][cur];
            ring.push_back(centroids[step_info.first]);
            rn.push_back(tri_norms[step_info.first]);
            end_nb = step_info.second;
            cur = end_nb;
            if (!boundary && cur == start_nb)
                break;
        }

        if (boundary) {
            ring.push_back(Point::mid_point(_points[v], _points[end_nb]));
            rn.push_back((_normals[v] + _normals[end_nb]).normalized());
            const Vector d0 = (_points[v] - ring.front()).normalized();
            const Vector d1 = (ring.back() - _points[v]).normalized();
            if (d0.dot(d1) < 0.96) {
                ring.push_back(_points[v]);
                rn.push_back(_normals[v]);
            }
        }

        if (ring.size() >= 3) {
            polys.push_back(ring);
            norms.push_back(rn);
        }
    }

    return build_welded_mesh(polys, norms);
}

Mesh SurfaceDivision::bricks(double length, double stagger) const {

    if (surfaces.empty())
        return Mesh();

    // Knit-course increase/decrease topology with half-brick running-bond overlay
    // (Popescu, Rippmann, Van Mele & Block, DMSP 2017):
    // 1. Sample courses at uniform meridian surface arc-length spacing size_v.
    // 2. Sample each contour i in {0..courses} into N_i = 2 * B_i evenly-spaced half-brick knit loops.
    // 3. Connect adjacent contours with 2-stitch running-bond bricks and symmetric 5-vertex knit
    //    increase/decrease pairs (Fig. 1a/b) whose head joints preserve exact 1-stitch half-brick parity.
    const double target_len = std::max(20.0, length);
    const double shift_frac = std::clamp(stagger, 0.0, 0.9);
    std::vector<std::vector<Point>> polys;
    std::vector<std::vector<Vector>> norms;

    for (size_t s = 0; s < surfaces.size(); s++) {
        const NurbsSurface& surf = surfaces[s];
        const Grid& g = _grids[s];
        const bool closed_u = surf.is_closed(0) ||
                              (eval_unit(surf, 0.0, 0.5).distance(eval_unit(surf, 1.0, 0.5)) < 1e-1);

        // 1. Build average meridian arc-length table so courses have uniform physical height size_v.
        const int m_samples = 128;
        const int u_probes = 8;
        std::vector<double> mer_arc(static_cast<size_t>(m_samples + 1), 0.0);
        for (int k = 1; k <= m_samples; k++) {
            const double gj0 = static_cast<double>(g.v) * static_cast<double>(k - 1) / m_samples;
            const double gj1 = static_cast<double>(g.v) * static_cast<double>(k) / m_samples;
            double avg_ds = 0.0;
            for (int p = 0; p < u_probes; p++) {
                const double gu = static_cast<double>(g.u) * (static_cast<double>(p) + 0.5) / u_probes;
                const Point p0 = evaluate(s, gu, gj0).first;
                const Point p1 = evaluate(s, gu, gj1).first;
                avg_ds += p0.distance(p1);
            }
            mer_arc[static_cast<size_t>(k)] =
                mer_arc[static_cast<size_t>(k - 1)] + avg_ds / static_cast<double>(u_probes);
        }
        const double total_mer = mer_arc.back();
        const int courses = std::max(4, static_cast<int>(std::lround(total_mer / size_v)));

        std::vector<double> contour_gj(static_cast<size_t>(courses + 1), 0.0);
        for (int c = 0; c <= courses; c++) {
            const double target_s = total_mer * static_cast<double>(c) / static_cast<double>(courses);
            for (int k = 0; k < m_samples; k++) {
                if (target_s <= mer_arc[static_cast<size_t>(k + 1)] + 1e-9) {
                    const double seg = mer_arc[static_cast<size_t>(k + 1)] - mer_arc[static_cast<size_t>(k)];
                    const double alpha = seg > 1e-12 ? (target_s - mer_arc[static_cast<size_t>(k)]) / seg : 0.0;
                    contour_gj[static_cast<size_t>(c)] =
                        static_cast<double>(g.v) * (static_cast<double>(k) + std::clamp(alpha, 0.0, 1.0)) /
                        static_cast<double>(m_samples);
                    break;
                }
            }
        }
        contour_gj.front() = 0.0;
        contour_gj.back() = static_cast<double>(g.v);

        if (!closed_u) {
            // Open patch: arc-length contoured courses with half-brick stagger and clean T-joint splits
            std::vector<std::vector<double>> course_joints(static_cast<size_t>(courses));
            for (int j = 0; j < courses; j++) {
                const double gj_mid = 0.5 * (contour_gj[static_cast<size_t>(j)] + contour_gj[static_cast<size_t>(j + 1)]);
                double hoop_len = 0.0;
                const int samples = 64;
                Point prev = evaluate(s, 0.0, gj_mid).first;
                for (int k = 1; k <= samples; k++) {
                    const Point cur =
                        evaluate(s, static_cast<double>(g.u) * k / samples, gj_mid).first;
                    hoop_len += prev.distance(cur);
                    prev = cur;
                }
                const int count = std::max(3, static_cast<int>(std::lround(hoop_len / target_len)));
                const double step = static_cast<double>(g.u) / count;
                const double off = (j % 2 == 1) ? shift_frac * step : 0.0;
                if (off > 1e-6)
                    course_joints[static_cast<size_t>(j)].push_back(0.0);
                for (int k = 0; k < count; k++) {
                    const double pos = off + k * step;
                    if (pos <= g.u + 1e-6)
                        course_joints[static_cast<size_t>(j)].push_back(std::min(static_cast<double>(g.u), pos));
                }
                if (course_joints[static_cast<size_t>(j)].back() < g.u - 1e-6)
                    course_joints[static_cast<size_t>(j)].push_back(static_cast<double>(g.u));
            }

            for (int j = 0; j < courses; j++) {
                const std::vector<double>& joints = course_joints[static_cast<size_t>(j)];
                const double gj_lo = contour_gj[static_cast<size_t>(j)];
                const double gj_hi = contour_gj[static_cast<size_t>(j + 1)];
                for (size_t k = 0; k + 1 < joints.size(); k++) {
                    const double a0 = joints[k];
                    const double a1 = joints[k + 1];
                    if (a1 - a0 <= 1e-4)
                        continue;
                    std::array<std::vector<double>, 2> seam_splits;
                    const std::array<int, 2> adj_rows = {j - 1, j + 1};
                    for (int side = 0; side < 2; side++) {
                        seam_splits[static_cast<size_t>(side)] = {a0, a1};
                        const int other_j = adj_rows[static_cast<size_t>(side)];
                        if (other_j >= 0 && other_j < courses) {
                            for (const double ox : course_joints[static_cast<size_t>(other_j)]) {
                                if (ox > a0 + 1e-4 && ox < a1 - 1e-4)
                                    seam_splits[static_cast<size_t>(side)].push_back(ox);
                            }
                        }
                        std::sort(
                            seam_splits[static_cast<size_t>(side)].begin(),
                            seam_splits[static_cast<size_t>(side)].end()
                        );
                    }
                    std::vector<Point> ring;
                    std::vector<Vector> rn;
                    for (const double x : seam_splits[0]) {
                        const std::pair<Point, Vector> ev =
                            evaluate(s, std::clamp(x, 0.0, static_cast<double>(g.u)), gj_lo);
                        if (ring.empty() || ring.back().distance(ev.first) > 1e-3) {
                            ring.push_back(ev.first);
                            rn.push_back(ev.second);
                        }
                    }
                    for (int m = static_cast<int>(seam_splits[1].size()) - 1; m >= 0; m--) {
                        const std::pair<Point, Vector> ev = evaluate(
                            s, std::clamp(seam_splits[1][static_cast<size_t>(m)], 0.0, static_cast<double>(g.u)), gj_hi
                        );
                        if (ring.empty() || ring.back().distance(ev.first) > 1e-3) {
                            ring.push_back(ev.first);
                            rn.push_back(ev.second);
                        }
                    }
                    if (ring.size() >= 3) {
                        polys.push_back(ring);
                        norms.push_back(rn);
                    }
                }
            }
            continue;
        }

        // 2. Closed enlarging/narrowing funnel/tubular surface: Popescu (2017) knit-increase/decrease
        //    course branching with meridian head joints (u_top == u_bot on every course) and constrained
        //    half-brick running-bond equalization so radial joints are twist-free with zero sawteeth.
        const int c_samples = 256;
        std::vector<ArcTable> hoop_arcs(static_cast<size_t>(courses));
        std::vector<int> course_counts(static_cast<size_t>(courses), 10);

        for (int j = 0; j < courses; j++) {
            const double gj_mid =
                0.5 * (contour_gj[static_cast<size_t>(j)] + contour_gj[static_cast<size_t>(j + 1)]);
            ArcTable arc;
            arc.fractions.resize(static_cast<size_t>(c_samples + 1), 0.0);
            arc.lengths.resize(static_cast<size_t>(c_samples + 1), 0.0);
            Point prev = evaluate(s, 0.0, gj_mid).first;
            for (int k = 1; k <= c_samples; k++) {
                const double f = static_cast<double>(k) / static_cast<double>(c_samples);
                const Point cur = evaluate(s, f * static_cast<double>(g.u), gj_mid).first;
                arc.fractions[static_cast<size_t>(k)] = f;
                arc.lengths[static_cast<size_t>(k)] =
                    arc.lengths[static_cast<size_t>(k - 1)] + prev.distance(cur);
                prev = cur;
            }
            arc.total = arc.lengths.back();
            hoop_arcs[static_cast<size_t>(j)] = arc;
            course_counts[static_cast<size_t>(j)] =
                std::max(10, static_cast<int>(std::lround(arc.total / target_len)));
        }

        // Clamp adjacent course brick ratio to [1/2, 2] so knit increases/decreases branch cleanly
        for (int j = 0; j + 1 < courses; j++) {
            const int b0 = course_counts[static_cast<size_t>(j)];
            int& b1 = course_counts[static_cast<size_t>(j + 1)];
            b1 = std::clamp(b1, std::max(8, (b0 + 1) / 2), 2 * b0);
        }

        // Sorted arc-length fractions in [0, 1) of meridian head joints for each course j
        std::vector<std::vector<double>> course_fracs(static_cast<size_t>(courses));
        {
            const int b_first = course_counts[0];
            course_fracs[0].resize(static_cast<size_t>(b_first));
            for (int k = 0; k < b_first; k++) {
                course_fracs[0][static_cast<size_t>(k)] =
                    static_cast<double>(k) / static_cast<double>(b_first);
            }
        }

        double spiral_phase = 0.0;
        for (int j = 0; j + 1 < courses; j++) {
            const std::vector<double>& prev_f = course_fracs[static_cast<size_t>(j)];
            const int b0 = static_cast<int>(prev_f.size());
            const int b1 = course_counts[static_cast<size_t>(j + 1)];

            // Unwrapped monotonically increasing span boundaries [a_k, b_k] of course j
            std::vector<double> span_lo(static_cast<size_t>(b0));
            std::vector<double> span_hi(static_cast<size_t>(b0));
            for (int k = 0; k < b0; k++) {
                span_lo[static_cast<size_t>(k)] = prev_f[static_cast<size_t>(k)];
                double hi = prev_f[static_cast<size_t>((k + 1) % b0)];
                if (k + 1 == b0) {
                    hi += 1.0;
                }
                span_hi[static_cast<size_t>(k)] = hi;
            }

            std::vector<double> joints;
            std::vector<double> lo_bound;
            std::vector<double> hi_bound;
            const double min_clear = 0.22; // Minimum stagger clearance fraction from parent head joints

            if (b1 >= b0) {
                const int inc = std::min(b0, b1 - b0);
                // Select inc spans combining wide angular span + evenly distributed golden-angle phase
                std::vector<bool> split_span(static_cast<size_t>(b0), false);
                if (inc > 0) {
                    spiral_phase = std::fmod(spiral_phase + 0.38196601125, 1.0);
                    std::vector<std::pair<double, int>> ranked;
                    for (int k = 0; k < b0; k++) {
                        const double width = span_hi[static_cast<size_t>(k)] - span_lo[static_cast<size_t>(k)];
                        const double mid = std::fmod(0.5 * (span_lo[static_cast<size_t>(k)] + span_hi[static_cast<size_t>(k)]), 1.0);
                        // Distance to nearest ideal evenly-spaced gore slot
                        double best_slot_dist = 1.0;
                        for (int g_idx = 0; g_idx < inc; g_idx++) {
                            const double target_mid =
                                std::fmod(spiral_phase + static_cast<double>(g_idx) / static_cast<double>(inc), 1.0);
                            double d = std::abs(mid - target_mid);
                            d = std::min(d, 1.0 - d);
                            best_slot_dist = std::min(best_slot_dist, d);
                        }
                        const double score = width * static_cast<double>(b0) - 0.85 * best_slot_dist;
                        ranked.emplace_back(-score, k);
                    }
                    std::sort(ranked.begin(), ranked.end());
                    for (int g_idx = 0; g_idx < inc; g_idx++) {
                        split_span[static_cast<size_t>(ranked[static_cast<size_t>(g_idx)].second)] = true;
                    }
                }

                for (int k = 0; k < b0; k++) {
                    const double a = span_lo[static_cast<size_t>(k)];
                    const double b = span_hi[static_cast<size_t>(k)];
                    const double w = b - a;
                    if (!split_span[static_cast<size_t>(k)]) {
                        joints.push_back(a + 0.5 * w);
                        lo_bound.push_back(a + min_clear * w);
                        hi_bound.push_back(b - min_clear * w);
                    } else {
                        // Popescu knit-increase: 2 meridian joints inside parent span [a, b]
                        joints.push_back(a + 0.30 * w);
                        lo_bound.push_back(a + 0.18 * w);
                        hi_bound.push_back(a + 0.44 * w);

                        joints.push_back(a + 0.70 * w);
                        lo_bound.push_back(a + 0.56 * w);
                        hi_bound.push_back(b - 0.18 * w);
                    }
                }
            } else {
                const int dec = std::min(b0 / 2, b0 - b1);
                // Mark dec evenly-spaced non-adjacent parent spans to skip (knit-decrease 2->1)
                std::vector<bool> skip_span(static_cast<size_t>(b0), false);
                spiral_phase = std::fmod(spiral_phase + 0.38196601125, 1.0);
                const int start_k = static_cast<int>(std::floor(spiral_phase * static_cast<double>(b0))) % b0;
                for (int g_idx = 0; g_idx < dec; g_idx++) {
                    const int slot = (start_k + (g_idx * b0) / std::max(1, dec)) % b0;
                    skip_span[static_cast<size_t>(slot)] = true;
                }
                for (int k = 0; k < b0; k++) {
                    if (skip_span[static_cast<size_t>(k)]) {
                        continue;
                    }
                    const double a = span_lo[static_cast<size_t>(k)];
                    const double b = span_hi[static_cast<size_t>(k)];
                    const double w = b - a;
                    joints.push_back(a + 0.5 * w);
                    lo_bound.push_back(a + min_clear * w);
                    hi_bound.push_back(b - min_clear * w);
                }
            }

            // Equalize brick widths around the 360-degree ring while preserving hard half-brick stagger bounds!
            const int m = static_cast<int>(joints.size());
            for (int iter = 0; iter < 36 && m >= 3; iter++) {
                std::vector<double> next_j = joints;
                for (int i = 0; i < m; i++) {
                    const double prev_v =
                        (i == 0) ? (joints[static_cast<size_t>(m - 1)] - 1.0) : joints[static_cast<size_t>(i - 1)];
                    const double next_v =
                        (i + 1 == m) ? (joints[0] + 1.0) : joints[static_cast<size_t>(i + 1)];
                    const double target = 0.5 * (prev_v + next_v);
                    next_j[static_cast<size_t>(i)] = std::clamp(
                        joints[static_cast<size_t>(i)] + 0.45 * (target - joints[static_cast<size_t>(i)]),
                        lo_bound[static_cast<size_t>(i)],
                        hi_bound[static_cast<size_t>(i)]
                    );
                }
                joints = std::move(next_j);
            }

            // Wrap into sorted [0, 1) fractions
            std::vector<double> wrapped(static_cast<size_t>(m));
            for (int i = 0; i < m; i++) {
                double f = std::fmod(joints[static_cast<size_t>(i)], 1.0);
                if (f < 0.0) {
                    f += 1.0;
                }
                wrapped[static_cast<size_t>(i)] = f;
            }
            std::sort(wrapped.begin(), wrapped.end());
            course_fracs[static_cast<size_t>(j + 1)] = std::move(wrapped);
        }

        // Convert each course's hoop arc-length fractions to exact surface u parameters in [0, g.u)
        std::vector<std::vector<double>> course_u(static_cast<size_t>(courses));
        for (int j = 0; j < courses; j++) {
            const std::vector<double>& fr = course_fracs[static_cast<size_t>(j)];
            course_u[static_cast<size_t>(j)].resize(fr.size());
            for (size_t k = 0; k < fr.size(); k++) {
                course_u[static_cast<size_t>(j)][k] =
                    hoop_arcs[static_cast<size_t>(j)].at_fraction(fr[k]) * static_cast<double>(g.u);
            }
            std::sort(course_u[static_cast<size_t>(j)].begin(), course_u[static_cast<size_t>(j)].end());
        }

        // Emit conformal meridian-jointed bricks with exact T-joint vertices on top and bottom bed seams
        const double u_period = static_cast<double>(g.u);
        for (int j = 0; j < courses; j++) {
            const std::vector<double>& cu = course_u[static_cast<size_t>(j)];
            const size_t nb = cu.size();
            const double gj_lo = contour_gj[static_cast<size_t>(j)];
            const double gj_hi = contour_gj[static_cast<size_t>(j + 1)];

            for (size_t k = 0; k < nb; k++) {
                const double u0 = cu[k];
                const double u1 = (k + 1 < nb) ? cu[k + 1] : (cu[0] + u_period);
                if (u1 - u0 <= 1e-5) {
                    continue;
                }

                std::array<std::vector<double>, 2> seam_u;
                const std::array<int, 2> adj_j = {j - 1, j + 1};
                for (int side = 0; side < 2; side++) {
                    seam_u[static_cast<size_t>(side)] = {u0, u1};
                    const int oj = adj_j[static_cast<size_t>(side)];
                    if (oj >= 0 && oj < courses) {
                        for (const double ox_base : course_u[static_cast<size_t>(oj)]) {
                            for (int wrap = 0; wrap <= 1; wrap++) {
                                const double ox = ox_base + static_cast<double>(wrap) * u_period;
                                if (ox > u0 + 1e-4 && ox < u1 - 1e-4) {
                                    seam_u[static_cast<size_t>(side)].push_back(ox);
                                }
                            }
                        }
                    }
                    std::sort(
                        seam_u[static_cast<size_t>(side)].begin(),
                        seam_u[static_cast<size_t>(side)].end()
                    );
                }

                std::vector<Point> ring;
                std::vector<Vector> rn;
                for (const double ux : seam_u[0]) {
                    double wrapped_u = std::fmod(ux, u_period);
                    if (wrapped_u < 0.0) {
                        wrapped_u += u_period;
                    }
                    const std::pair<Point, Vector> ev = evaluate(s, wrapped_u, gj_lo);
                    if (ring.empty() || ring.back().distance(ev.first) > 1e-2) {
                        ring.push_back(ev.first);
                        rn.push_back(ev.second);
                    }
                }
                for (int m = static_cast<int>(seam_u[1].size()) - 1; m >= 0; m--) {
                    double wrapped_u = std::fmod(seam_u[1][static_cast<size_t>(m)], u_period);
                    if (wrapped_u < 0.0) {
                        wrapped_u += u_period;
                    }
                    const std::pair<Point, Vector> ev = evaluate(s, wrapped_u, gj_hi);
                    if (ring.empty() || ring.back().distance(ev.first) > 1e-2) {
                        ring.push_back(ev.first);
                        rn.push_back(ev.second);
                    }
                }
                if (ring.size() >= 4) {
                    polys.push_back(ring);
                    norms.push_back(rn);
                }
            }
        }
    }

    return build_welded_mesh(polys, norms);
}

Mesh SurfaceDivision::chebyshev(double edge, double angle_degrees) const {

    if (surfaces.empty() || edge <= 1.0)
        return Mesh();

    NurbsSurface surf = surfaces[0];
    surf.set_domain(0, 0.0, 1.0);
    surf.set_domain(1, 0.0, 1.0);
    const bool flip = _grids[0].flip_normal;

    const Plane mid_frame = surf.frame_at(0.5, 0.5);
    Vector mid_n = surf.normal_at(0.5, 0.5);
    if (!mid_n.normalize_self())
        mid_n = Vector(0.0, 0.0, 1.0);
    if (flip)
        mid_n = -mid_n;

    Vector ax0 = mid_frame.x_axis() - mid_n * mid_frame.x_axis().dot(mid_n);
    if (!ax0.normalize_self())
        ax0 = Vector(1.0, 0.0, 0.0);
    const double rad = angle_degrees * Tolerance::TO_RADIANS;
    const Vector e0 =
        (ax0 * std::cos(rad) + mid_n.cross(ax0) * std::sin(rad)).normalized();
    const Vector e1 = mid_n.cross(e0).normalized();
    const std::array<Vector, 4> dirs = {e0, e1, -e0, -e1};

    const int max_steps = 40;
    std::array<std::vector<std::pair<double, double>>, 4> axes;
    for (int d = 0; d < 4; d++) {
        axes[d].emplace_back(0.5, 0.5);
        Vector cur_dir = dirs[d];
        for (int s = 0; s < max_steps; s++) {
            double nu = 0.0;
            double nv = 0.0;
            if (!chebyshev_axis_step(
                    surf, flip, edge, axes[d].back().first, axes[d].back().second, cur_dir, nu, nv
                ))
                break;
            axes[d].emplace_back(nu, nv);
            if (nu < -0.08 || nu > 1.08 || nv < -0.08 || nv > 1.08)
                break;
        }
    }

    std::vector<std::vector<Point>> polys;
    std::vector<std::vector<Vector>> norms;

    for (int q = 0; q < 4; q++) {
        const std::vector<std::pair<double, double>>& ax_a = axes[q];
        const std::vector<std::pair<double, double>>& ax_b = axes[(q + 1) % 4];
        const size_t na = ax_a.size();
        const size_t nb = ax_b.size();
        if (na < 2 || nb < 2)
            continue;

        std::vector<std::vector<std::pair<double, double>>> uv_grid(
            na, std::vector<std::pair<double, double>>(nb, {-10.0, -10.0})
        );
        std::vector<std::vector<bool>> valid(na, std::vector<bool>(nb, false));
        for (size_t i = 0; i < na; i++) {
            uv_grid[i][0] = ax_a[i];
            valid[i][0] = true;
        }
        for (size_t j = 0; j < nb; j++) {
            uv_grid[0][j] = ax_b[j];
            valid[0][j] = true;
        }

        for (size_t i = 1; i < na; i++) {
            for (size_t j = 1; j < nb; j++) {
                if (!valid[i - 1][j] || !valid[i][j - 1] || !valid[i - 1][j - 1])
                    continue;
                double u = 0.0;
                double v = 0.0;
                if (chebyshev_rhombus_corner(
                        surf,
                        edge,
                        uv_grid[i - 1][j].first,
                        uv_grid[i - 1][j].second,
                        uv_grid[i][j - 1].first,
                        uv_grid[i][j - 1].second,
                        uv_grid[i - 1][j - 1].first,
                        uv_grid[i - 1][j - 1].second,
                        u,
                        v
                    )) {
                    uv_grid[i][j] = {u, v};
                    valid[i][j] = true;
                }
            }
        }

        for (size_t i = 1; i < na; i++) {
            for (size_t j = 1; j < nb; j++) {
                if (!valid[i - 1][j - 1] || !valid[i][j - 1] || !valid[i][j] || !valid[i - 1][j])
                    continue;

                const std::vector<std::pair<double, double>> raw_uv = {
                    uv_grid[i - 1][j - 1], uv_grid[i][j - 1], uv_grid[i][j], uv_grid[i - 1][j]
                };
                bool any_in = false;
                for (const std::pair<double, double>& uv : raw_uv) {
                    if (uv.first >= -1e-4 && uv.first <= 1.0 + 1e-4 && uv.second >= -1e-4 &&
                        uv.second <= 1.0 + 1e-4)
                        any_in = true;
                }
                if (!any_in)
                    continue;

                const std::vector<std::pair<double, double>> clipped = clip_uv_unit_square(raw_uv);
                if (clipped.size() < 3)
                    continue;

                std::vector<Point> ring;
                std::vector<Vector> rn;
                for (const std::pair<double, double>& uv : clipped) {
                    const double cu = std::clamp(uv.first, 0.0, 1.0);
                    const double cv = std::clamp(uv.second, 0.0, 1.0);
                    const Point pt = surf.point_at(cu, cv);
                    Vector nrm = surf.normal_at(cu, cv);
                    if (!nrm.normalize_self())
                        nrm = Vector(0.0, 0.0, 1.0);
                    if (flip)
                        nrm = -nrm;
                    if (ring.empty() || ring.back().distance(pt) > 6e-3 * edge) {
                        ring.push_back(pt);
                        rn.push_back(nrm);
                    }
                }
                while (ring.size() > 1 && ring.back().distance(ring.front()) <= 6e-3 * edge) {
                    ring.pop_back();
                    rn.pop_back();
                }
                if (ring.size() < 3)
                    continue;

                Point c(0.0, 0.0, 0.0);
                for (const Point& p : ring)
                    c = c + Vector(p[0], p[1], p[2]);
                c = Point(
                    c[0] / static_cast<double>(ring.size()),
                    c[1] / static_cast<double>(ring.size()),
                    c[2] / static_cast<double>(ring.size())
                );
                Vector area(0.0, 0.0, 0.0);
                for (size_t k = 0; k < ring.size(); k++)
                    area = area + (ring[k] - c).cross(ring[(k + 1) % ring.size()] - c);
                if (0.5 * area.magnitude() < 0.003 * edge * edge)
                    continue;

                polys.push_back(ring);
                norms.push_back(rn);
            }
        }
    }

    return build_welded_mesh(polys, norms, 1e-2 * edge);
}

}
