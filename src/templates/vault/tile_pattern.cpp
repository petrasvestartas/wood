#include "pch.h"
#include "src/templates/vault/tile_pattern.h"
#include <complex>
#include "intersection.h"
#include "line.h"
#include "nurbscurve.h"
#include "plane.h"
#include "tolerance.h"
#include "wood_config.h"
#include "wood_element_geometry.h"

namespace wood_vault {

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Pure-C++ 3x3 Linear Algebra, Jacobi SVD & Sparse CSR PCG Solver
// ═══════════════════════════════════════════════════════════════════════════

struct Mat3 {
    double m[3][3] = {
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}
    };
};

Mat3 mat3_identity() {

    Mat3 out;
    out.m[0][0] = 1.0;
    out.m[1][1] = 1.0;
    out.m[2][2] = 1.0;

    return out;
}

Mat3 mat3_mul(const Mat3& a, const Mat3& b) {

    Mat3 c;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            double s = 0.0;
            for (int k = 0; k < 3; k++) {
                s += a.m[i][k] * b.m[k][j];
            }
            c.m[i][j] = s;
        }
    }

    return c;
}

Mat3 mat3_transpose(const Mat3& a) {

    Mat3 t;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            t.m[i][j] = a.m[j][i];
        }
    }

    return t;
}

double mat3_det(const Mat3& a) {

    return a.m[0][0] * (a.m[1][1] * a.m[2][2] - a.m[1][2] * a.m[2][1]) -
           a.m[0][1] * (a.m[1][0] * a.m[2][2] - a.m[1][2] * a.m[2][0]) +
           a.m[0][2] * (a.m[1][0] * a.m[2][1] - a.m[1][1] * a.m[2][0]);
}

Vector mat3_mul_vec(const Mat3& a, const Vector& v) {

    return Vector(
        a.m[0][0] * v[0] + a.m[0][1] * v[1] + a.m[0][2] * v[2],
        a.m[1][0] * v[0] + a.m[1][1] * v[1] + a.m[1][2] * v[2],
        a.m[2][0] * v[0] + a.m[2][1] * v[1] + a.m[2][2] * v[2]
    );
}

/// Computes proper rotation R = U * V^T in SO(3) maximizing trace(R^T * C) via cyclic Jacobi SVD.
Mat3 svd3x3_proper_rotation(const Mat3& corr) {

    const Mat3 ct = mat3_transpose(corr);
    Mat3 ata = mat3_mul(ct, corr);
    Mat3 v_mat = mat3_identity();

    for (int sweep = 0; sweep < 36; sweep++) {
        const int pairs[3][2] = {{0, 1}, {0, 2}, {1, 2}};
        double off_norm = 0.0;
        for (int p = 0; p < 3; p++) {
            const int r = pairs[p][0];
            const int c = pairs[p][1];
            const double apq = ata.m[r][c];
            off_norm += apq * apq;
            if (std::abs(apq) <= 1e-15) {
                continue;
            }
            const double app = ata.m[r][r];
            const double aqq = ata.m[c][c];
            const double tau = (aqq - app) / (2.0 * apq);
            const double t = (tau >= 0.0)
                                 ? (1.0 / (tau + std::sqrt(1.0 + tau * tau)))
                                 : (-1.0 / (-tau + std::sqrt(1.0 + tau * tau)));
            const double cs = 1.0 / std::sqrt(1.0 + t * t);
            const double sn = t * cs;

            for (int k = 0; k < 3; k++) {
                if (k != r && k != c) {
                    const double akr = ata.m[k][r];
                    const double akc = ata.m[k][c];
                    ata.m[k][r] = cs * akr - sn * akc;
                    ata.m[r][k] = ata.m[k][r];
                    ata.m[k][c] = sn * akr + cs * akc;
                    ata.m[c][k] = ata.m[k][c];
                }
            }
            ata.m[r][r] = cs * cs * app - 2.0 * cs * sn * apq + sn * sn * aqq;
            ata.m[c][c] = sn * sn * app + 2.0 * cs * sn * apq + cs * cs * aqq;
            ata.m[r][c] = 0.0;
            ata.m[c][r] = 0.0;

            for (int k = 0; k < 3; k++) {
                const double vkr = v_mat.m[k][r];
                const double vkc = v_mat.m[k][c];
                v_mat.m[k][r] = cs * vkr - sn * vkc;
                v_mat.m[k][c] = sn * vkr + cs * vkc;
            }
        }
        if (off_norm <= 1e-28) {
            break;
        }
    }

    std::array<double, 3> sigma = {
        std::sqrt(std::max(0.0, ata.m[0][0])),
        std::sqrt(std::max(0.0, ata.m[1][1])),
        std::sqrt(std::max(0.0, ata.m[2][2]))
    };
    std::array<int, 3> order = {0, 1, 2};
    if (sigma[order[0]] < sigma[order[1]]) {
        std::swap(order[0], order[1]);
    }
    if (sigma[order[1]] < sigma[order[2]]) {
        std::swap(order[1], order[2]);
    }
    if (sigma[order[0]] < sigma[order[1]]) {
        std::swap(order[0], order[1]);
    }

    Mat3 v_sorted;
    std::array<double, 3> sig_sorted;
    for (int col = 0; col < 3; col++) {
        sig_sorted[static_cast<size_t>(col)] = sigma[static_cast<size_t>(order[static_cast<size_t>(col)])];
        for (int row = 0; row < 3; row++) {
            v_sorted.m[row][col] = v_mat.m[row][order[static_cast<size_t>(col)]];
        }
    }
    if (mat3_det(v_sorted) < 0.0) {
        for (int row = 0; row < 3; row++) {
            v_sorted.m[row][2] = -v_sorted.m[row][2];
        }
    }

    std::array<Vector, 3> u_cols;
    std::array<bool, 3> valid_u = {false, false, false};
    for (int col = 0; col < 3; col++) {
        if (sig_sorted[static_cast<size_t>(col)] > 1e-10) {
            const Vector vc(v_sorted.m[0][col], v_sorted.m[1][col], v_sorted.m[2][col]);
            Vector uc = mat3_mul_vec(corr, vc) * (1.0 / sig_sorted[static_cast<size_t>(col)]);
            if (uc.normalize_self()) {
                u_cols[static_cast<size_t>(col)] = uc;
                valid_u[static_cast<size_t>(col)] = true;
            }
        }
    }

    if (!valid_u[0]) {
        u_cols[0] = Vector(1.0, 0.0, 0.0);
        valid_u[0] = true;
    }
    if (!valid_u[1]) {
        Vector cand = (std::abs(u_cols[0][0]) < 0.9) ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0);
        cand = cand - u_cols[0] * cand.dot(u_cols[0]);
        cand.normalize_self();
        u_cols[1] = cand;
        valid_u[1] = true;
    } else {
        u_cols[1] = u_cols[1] - u_cols[0] * u_cols[1].dot(u_cols[0]);
        if (!u_cols[1].normalize_self()) {
            Vector cand = (std::abs(u_cols[0][0]) < 0.9) ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0);
            cand = cand - u_cols[0] * cand.dot(u_cols[0]);
            cand.normalize_self();
            u_cols[1] = cand;
        }
    }
    u_cols[2] = u_cols[0].cross(u_cols[1]).normalized();

    Mat3 u_mat;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            u_mat.m[row][col] = u_cols[static_cast<size_t>(col)][row];
        }
    }

    Mat3 rot = mat3_mul(u_mat, mat3_transpose(v_sorted));
    if (mat3_det(rot) < 0.0) {
        for (int row = 0; row < 3; row++) {
            u_mat.m[row][2] = -u_mat.m[row][2];
        }
        rot = mat3_mul(u_mat, mat3_transpose(v_sorted));
    }

    return rot;
}

/// Compressed Sparse Row symmetric positive-definite matrix.
struct CsrMatrix {
    size_t n = 0;
    std::vector<size_t> row_ptr;
    std::vector<size_t> col_idx;
    std::vector<double> values;
    std::vector<double> inv_diag;

    static CsrMatrix from_maps(const std::vector<std::map<size_t, double>>& rows) {

        CsrMatrix csr;
        csr.n = rows.size();
        csr.row_ptr.resize(csr.n + 1, 0);
        csr.inv_diag.resize(csr.n, 1.0);

        for (size_t i = 0; i < csr.n; i++) {
            csr.row_ptr[i] = csr.col_idx.size();
            double diag = 0.0;
            for (const std::pair<const size_t, double>& kv : rows[i]) {
                csr.col_idx.push_back(kv.first);
                csr.values.push_back(kv.second);
                if (kv.first == i) {
                    diag = kv.second;
                }
            }
            csr.inv_diag[i] = (std::abs(diag) > 1e-15) ? (1.0 / diag) : 1.0;
        }
        csr.row_ptr[csr.n] = csr.col_idx.size();

        return csr;
    }

    void mul_scalar(const std::vector<double>& x, std::vector<double>& y) const {

        y.assign(n, 0.0);
        for (size_t i = 0; i < n; i++) {
            double s = 0.0;
            for (size_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
                s += values[k] * x[col_idx[k]];
            }
            y[i] = s;
        }
    }
};

/// Solves CsrMatrix * x = b via Jacobi-preconditioned Conjugate Gradient warm-started at x.
void solve_pcg_scalar(
    const CsrMatrix& mat,
    const std::vector<double>& b,
    std::vector<double>& x,
    int max_iters = 400,
    double rel_tol = 1e-11
) {

    if (mat.n == 0) {
        return;
    }
    if (x.size() != mat.n) {
        x.assign(mat.n, 0.0);
    }

    std::vector<double> ax(mat.n, 0.0);
    mat.mul_scalar(x, ax);

    std::vector<double> r(mat.n, 0.0);
    std::vector<double> z(mat.n, 0.0);
    std::vector<double> p(mat.n, 0.0);
    double b_norm_sq = 0.0;
    double rz = 0.0;

    for (size_t i = 0; i < mat.n; i++) {
        b_norm_sq += b[i] * b[i];
        r[i] = b[i] - ax[i];
        z[i] = mat.inv_diag[i] * r[i];
        p[i] = z[i];
        rz += r[i] * z[i];
    }
    const double stop_sq = std::max(1e-28, rel_tol * rel_tol * b_norm_sq);

    std::vector<double> ap(mat.n, 0.0);
    for (int iter = 0; iter < max_iters; iter++) {
        double r_sq = 0.0;
        for (size_t i = 0; i < mat.n; i++) {
            r_sq += r[i] * r[i];
        }
        if (r_sq <= stop_sq) {
            break;
        }

        mat.mul_scalar(p, ap);
        double pap = 0.0;
        for (size_t i = 0; i < mat.n; i++) {
            pap += p[i] * ap[i];
        }
        if (std::abs(pap) <= 1e-30) {
            break;
        }

        const double alpha = rz / pap;
        double rz_next = 0.0;
        for (size_t i = 0; i < mat.n; i++) {
            x[i] += alpha * p[i];
            r[i] -= alpha * ap[i];
            z[i] = mat.inv_diag[i] * r[i];
            rz_next += r[i] * z[i];
        }
        if (std::abs(rz) <= 1e-30) {
            break;
        }
        const double beta = rz_next / rz;
        for (size_t i = 0; i < mat.n; i++) {
            p[i] = z[i] + beta * p[i];
        }
        rz = rz_next;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Closest-Point Surface Projector (Uniform Spatial Hash + Triangle Projection)
// ═══════════════════════════════════════════════════════════════════════════

Point closest_point_on_triangle(
    const Point& p,
    const Point& a,
    const Point& b,
    const Point& c,
    double& out_w0,
    double& out_w1,
    double& out_w2
) {

    const Vector ab = b - a;
    const Vector ac = c - a;
    const Vector ap = p - a;
    const double d1 = ab.dot(ap);
    const double d2 = ac.dot(ap);
    if (d1 <= 0.0 && d2 <= 0.0) {
        out_w0 = 1.0;
        out_w1 = 0.0;
        out_w2 = 0.0;
        return a;
    }

    const Vector bp = p - b;
    const double d3 = ab.dot(bp);
    const double d4 = ac.dot(bp);
    if (d3 >= 0.0 && d4 <= d3) {
        out_w0 = 0.0;
        out_w1 = 1.0;
        out_w2 = 0.0;
        return b;
    }

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double v = d1 / (d1 - d3);
        out_w0 = 1.0 - v;
        out_w1 = v;
        out_w2 = 0.0;
        return a + ab * v;
    }

    const Vector cp = p - c;
    const double d5 = ab.dot(cp);
    const double d6 = ac.dot(cp);
    if (d6 >= 0.0 && d5 <= d6) {
        out_w0 = 0.0;
        out_w1 = 0.0;
        out_w2 = 1.0;
        return c;
    }

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double w = d2 / (d2 - d6);
        out_w0 = 1.0 - w;
        out_w1 = 0.0;
        out_w2 = w;
        return a + ac * w;
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        out_w0 = 0.0;
        out_w1 = 1.0 - w;
        out_w2 = w;
        return b + (c - b) * w;
    }

    const double denom = 1.0 / (va + vb + vc);
    const double v = vb * denom;
    const double w = vc * denom;
    out_w0 = 1.0 - v - w;
    out_w1 = v;
    out_w2 = w;

    return a + ab * v + ac * w;
}

struct SurfaceProjector {
    std::vector<Point> verts;
    std::vector<Vector> vnorms;
    std::vector<std::array<size_t, 3>> tris;
    Point min_pt = Point(0.0, 0.0, 0.0);
    double cell_size = 100.0;
    int nx = 1;
    int ny = 1;
    int nz = 1;
    std::vector<std::vector<size_t>> buckets;

    void build(
        const std::vector<Point>& in_verts,
        const std::vector<Vector>& in_norms,
        const std::vector<std::array<size_t, 3>>& in_tris
    ) {

        verts = in_verts;
        vnorms = in_norms;
        tris = in_tris;
        if (verts.empty() || tris.empty()) {
            return;
        }

        Point hi = verts[0];
        min_pt = verts[0];
        for (const Point& pt : verts) {
            for (int k = 0; k < 3; k++) {
                min_pt[k] = std::min(min_pt[k], pt[k]);
                hi[k] = std::max(hi[k], pt[k]);
            }
        }
        const double diag = std::max(1.0, min_pt.distance(hi));
        cell_size = std::max(1e-3, diag / 20.0);
        nx = std::clamp(static_cast<int>(std::ceil((hi[0] - min_pt[0]) / cell_size)) + 1, 1, 32);
        ny = std::clamp(static_cast<int>(std::ceil((hi[1] - min_pt[1]) / cell_size)) + 1, 1, 32);
        nz = std::clamp(static_cast<int>(std::ceil((hi[2] - min_pt[2]) / cell_size)) + 1, 1, 32);
        buckets.assign(static_cast<size_t>(nx * ny * nz), {});

        for (size_t t = 0; t < tris.size(); t++) {
            const Point& a = verts[tris[t][0]];
            const Point& b = verts[tris[t][1]];
            const Point& c = verts[tris[t][2]];
            int lo_i[3];
            int hi_i[3];
            const int dims[3] = {nx, ny, nz};
            for (int d = 0; d < 3; d++) {
                const double tmin = std::min({a[d], b[d], c[d]});
                const double tmax = std::max({a[d], b[d], c[d]});
                lo_i[d] = std::clamp(static_cast<int>(std::floor((tmin - min_pt[d]) / cell_size)), 0, dims[d] - 1);
                hi_i[d] = std::clamp(static_cast<int>(std::floor((tmax - min_pt[d]) / cell_size)), 0, dims[d] - 1);
            }
            for (int ix = lo_i[0]; ix <= hi_i[0]; ix++) {
                for (int iy = lo_i[1]; iy <= hi_i[1]; iy++) {
                    for (int iz = lo_i[2]; iz <= hi_i[2]; iz++) {
                        const size_t bid = static_cast<size_t>((ix * ny + iy) * nz + iz);
                        buckets[bid].push_back(t);
                    }
                }
            }
        }
    }

    std::pair<Point, Vector> project(const Point& query, double* out_dist = nullptr) const {

        if (tris.empty()) {
            if (out_dist) {
                *out_dist = 0.0;
            }
            return {query, Vector(0.0, 0.0, 1.0)};
        }

        const int qx = std::clamp(static_cast<int>(std::floor((query[0] - min_pt[0]) / cell_size)), 0, nx - 1);
        const int qy = std::clamp(static_cast<int>(std::floor((query[1] - min_pt[1]) / cell_size)), 0, ny - 1);
        const int qz = std::clamp(static_cast<int>(std::floor((query[2] - min_pt[2]) / cell_size)), 0, nz - 1);

        double best_d2 = std::numeric_limits<double>::infinity();
        Point best_pt = query;
        Vector best_nrm(0.0, 0.0, 1.0);

        for (int radius = 1; radius <= 3; radius++) {
            for (int ix = std::max(0, qx - radius); ix <= std::min(nx - 1, qx + radius); ix++) {
                for (int iy = std::max(0, qy - radius); iy <= std::min(ny - 1, qy + radius); iy++) {
                    for (int iz = std::max(0, qz - radius); iz <= std::min(nz - 1, qz + radius); iz++) {
                        const size_t bid = static_cast<size_t>((ix * ny + iy) * nz + iz);
                        for (const size_t t : buckets[bid]) {
                            double w0 = 0.0;
                            double w1 = 0.0;
                            double w2 = 0.0;
                            const Point cp = closest_point_on_triangle(
                                query,
                                verts[tris[t][0]],
                                verts[tris[t][1]],
                                verts[tris[t][2]],
                                w0,
                                w1,
                                w2
                            );
                            const double d2 = (cp - query).magnitude_squared();
                            if (d2 < best_d2) {
                                best_d2 = d2;
                                best_pt = cp;
                                Vector n = vnorms[tris[t][0]] * w0 + vnorms[tris[t][1]] * w1 +
                                           vnorms[tris[t][2]] * w2;
                                if (!n.normalize_self()) {
                                    n = Vector(0.0, 0.0, 1.0);
                                }
                                best_nrm = n;
                            }
                        }
                    }
                }
            }
            const double safe_bound = static_cast<double>(radius) * cell_size;
            if (best_d2 < safe_bound * safe_bound) {
                break;
            }
        }

        if (!std::isfinite(best_d2)) {
            for (size_t t = 0; t < tris.size(); t++) {
                double w0 = 0.0;
                double w1 = 0.0;
                double w2 = 0.0;
                const Point cp = closest_point_on_triangle(
                    query, verts[tris[t][0]], verts[tris[t][1]], verts[tris[t][2]], w0, w1, w2
                );
                const double d2 = (cp - query).magnitude_squared();
                if (d2 < best_d2) {
                    best_d2 = d2;
                    best_pt = cp;
                    Vector n = vnorms[tris[t][0]] * w0 + vnorms[tris[t][1]] * w1 +
                               vnorms[tris[t][2]] * w2;
                    if (!n.normalize_self()) {
                        n = Vector(0.0, 0.0, 1.0);
                    }
                    best_nrm = n;
                }
            }
        }

        if (out_dist) {
            *out_dist = std::sqrt(std::max(0.0, best_d2));
        }

        return {best_pt, best_nrm};
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QuadriFlow Extrinsic 4-RoSy & Position Field Primitives (Huang et al. 2018)
// ═══════════════════════════════════════════════════════════════════════════

void build_tangent_frame(const Vector& n, Vector& s, Vector& t) {

    if (std::abs(n[0]) > std::abs(n[1])) {
        const double inv = 1.0 / std::sqrt(std::max(1e-24, n[0] * n[0] + n[2] * n[2]));
        t = Vector(n[2] * inv, 0.0, -n[0] * inv);
    } else {
        const double inv = 1.0 / std::sqrt(std::max(1e-24, n[1] * n[1] + n[2] * n[2]));
        t = Vector(0.0, n[2] * inv, -n[1] * inv);
    }
    s = t.cross(n).normalized();
}

Vector rotate_around_normal(const Vector& v, const Vector& n, double angle_rad) {

    const double c = std::cos(angle_rad);
    const double s = std::sin(angle_rad);

    return v * c + n.cross(v) * s + n * (n.dot(v) * (1.0 - c));
}

Plane plane_with_normal(const Point& origin, const Vector& unit_z) {

    Vector ref = (std::abs(unit_z[0]) < 0.85) ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0);
    Vector x_ax = (ref - unit_z * ref.dot(unit_z)).normalized();
    Vector y_ax = unit_z.cross(x_ax).normalized();

    return Plane(origin, x_ax, y_ax);
}

/// Trims any residual intersecting block pairs along their shared planar bisector so all blocks meet flush without overlap.
void resolve_planar_block_overlaps(std::vector<std::shared_ptr<Element>>& elements) {

    const size_t n = elements.size();
    if (n < 2) {
        return;
    }
    std::vector<Mesh> meshes(n);
    std::vector<AABB> boxes(n);
    std::vector<Point> centres(n);
    for (size_t i = 0; i < n; i++) {
        meshes[i] = elements[i]->model_geometry_mesh();
        boxes[i] = AABB::from_mesh(meshes[i]);
        const std::vector<Point> pts = meshes[i].to_vertices_and_faces().first;
        centres[i] = pts.empty() ? Point(0.0, 0.0, 0.0) : Point::centroid(pts);
    }

    std::vector<bool> dirty(n, false);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (!boxes[i].intersects(boxes[j])) {
                continue;
            }
            Mesh common;
            try {
                common = solid_boolean(meshes[i], meshes[j], SolidOperation::intersect);
            } catch (const std::exception&) {
                continue;
            }
            if (common.number_of_faces() < 4) {
                continue;
            }
            const double vol = compute_volume(common);
            if (vol <= 0.5) {
                continue;
            }
            const std::vector<Point> cpts = common.to_vertices_and_faces().first;
            if (cpts.empty()) {
                continue;
            }
            const Point cm = Point::centroid(cpts);
            // Use the wafer-thin intersection's minimum-variance PCA axis (the true shared joint normal!)
            // rather than centres[j] - centres[i] so staggered running-bond bricks never get 45-degree corner chops!
            const Plane sliver_pca = Plane::from_points_pca(cpts);
            Vector sep = sliver_pca.z_axis();
            if (sep.dot(centres[j] - centres[i]) < 0.0) {
                sep = -sep;
            }
            if (!sep.normalize_self()) {
                continue;
            }
            const Mesh next_i = cut_mesh(meshes[i], {plane_with_normal(cm, -sep)});
            const Mesh next_j = cut_mesh(meshes[j], {plane_with_normal(cm, sep)});
            if (next_i.number_of_faces() >= 4 && manifold_solid(next_i) &&
                compute_volume(next_i) > 50.0) {
                meshes[i] = next_i;
                boxes[i] = AABB::from_mesh(meshes[i]);
                dirty[i] = true;
            }
            if (next_j.number_of_faces() >= 4 && manifold_solid(next_j) &&
                compute_volume(next_j) > 50.0) {
                meshes[j] = next_j;
                boxes[j] = AABB::from_mesh(meshes[j]);
                dirty[j] = true;
            }
        }
    }

    for (size_t i = 0; i < n; i++) {
        if (dirty[i]) {
            elements[i] = std::make_shared<NodeBlockElement>(meshes[i], elements[i]->name);
        }
    }
}



[[maybe_unused]] Point closest_point_on_segment(const Point& q, const Point& a, const Point& b) {

    const Vector ab = b - a;
    const double len2 = ab.magnitude_squared();
    if (len2 <= 1e-16) {
        return a;
    }
    const double t = std::clamp((q - a).dot(ab) / len2, 0.0, 1.0);

    return a + ab * t;
}

// ═══════════════════════════════════════════════════════════════════════════
// Pure-C++ Amir Vaxman Directional Helpers (https://github.com/avaxman/Directional)
// ═══════════════════════════════════════════════════════════════════════════

/// Solves a 5x5 symmetric positive-definite system M * x = rhs via Gaussian elimination with partial pivoting.
bool solve_5x5_linear(double mat[5][5], double rhs[5], double out[5]) {

    double aug[5][6];
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            aug[i][j] = mat[i][j];
        }
        aug[i][5] = rhs[i];
    }

    for (int col = 0; col < 5; col++) {
        int piv = col;
        double best = std::abs(aug[col][col]);
        for (int row = col + 1; row < 5; row++) {
            if (std::abs(aug[row][col]) > best) {
                best = std::abs(aug[row][col]);
                piv = row;
            }
        }
        if (best <= 1e-18) {
            return false;
        }
        if (piv != col) {
            for (int k = col; k < 6; k++) {
                std::swap(aug[col][k], aug[piv][k]);
            }
        }
        const double inv_d = 1.0 / aug[col][col];
        for (int k = col; k < 6; k++) {
            aug[col][k] *= inv_d;
        }
        for (int row = 0; row < 5; row++) {
            if (row == col) {
                continue;
            }
            const double factor = aug[row][col];
            if (std::abs(factor) <= 1e-20) {
                continue;
            }
            for (int k = col; k < 6; k++) {
                aug[row][k] -= factor * aug[col][k];
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        out[i] = aug[i][5];
    }

    return true;
}

/// 2x2 integer rotation matrix R_m = (R_1)^m where R_1 = [[0, -1], [1, 0]] (Directional set_sign_symmetry(4)).
struct IntMat2 {
    int a00 = 1;
    int a01 = 0;
    int a10 = 0;
    int a11 = 1;
};

IntMat2 rot90_int_mat(int m) {

    const int k = ((m % 4) + 4) % 4;
    if (k == 0) {
        return {1, 0, 0, 1};
    }
    if (k == 1) {
        return {0, -1, 1, 0};
    }
    if (k == 2) {
        return {-1, 0, 0, -1};
    }
    return {0, 1, -1, 0};
}

IntMat2 mul_int_mat2(const IntMat2& a, const IntMat2& b) {
    return {
        a.a00 * b.a00 + a.a01 * b.a10,
        a.a00 * b.a01 + a.a01 * b.a11,
        a.a10 * b.a00 + a.a11 * b.a10,
        a.a10 * b.a01 + a.a11 * b.a11
    };
}

/// Linear equality row: sum_j coeffs[j] * x[j] = rhs.
struct SparseLinearRow {
    std::map<size_t, double> coeffs;
    double rhs = 0.0;
};

/// Exact Gauss-Jordan row-echelon parameterization x = x0 + S * y for sparse seamless & integer constraints.
struct NullspaceParam {
    size_t num_full = 0;
    size_t num_free = 0;
    std::vector<double> x0;
    std::vector<std::vector<std::pair<size_t, double>>> s_rows; // Per full var i: list of (free_idx, coeff).

    void build(size_t n_vars, const std::vector<SparseLinearRow>& eqs) {

        num_full = n_vars;
        // Pivots stored as x[piv] = p_const[piv] + sum_{col} p_expr[piv][col] * x[col] (only non-pivot cols!)
        std::vector<bool> is_piv(n_vars, false);
        std::vector<double> p_const(n_vars, 0.0);
        std::vector<std::map<size_t, double>> p_expr(n_vars);
        std::vector<size_t> piv_list;

        for (const SparseLinearRow& raw_eq : eqs) {
            std::map<size_t, double> cur;
            double c_rhs = raw_eq.rhs;
            for (const std::pair<const size_t, double>& kv : raw_eq.coeffs) {
                const size_t col = kv.first;
                const double val = kv.second;
                if (std::abs(val) <= 1e-13) {
                    continue;
                }
                if (is_piv[col]) {
                    c_rhs -= val * p_const[col];
                    for (const std::pair<const size_t, double>& pk : p_expr[col]) {
                        cur[pk.first] += val * pk.second;
                    }
                } else {
                    cur[col] += val;
                }
            }

            size_t best_col = n_vars;
            double best_abs = 1e-9;
            for (const std::pair<const size_t, double>& kv : cur) {
                if (std::abs(kv.second) > best_abs) {
                    best_abs = std::abs(kv.second);
                    best_col = kv.first;
                }
            }
            if (best_col == n_vars) {
                continue;
            }

            const double inv_piv = 1.0 / cur[best_col];
            const double new_c = c_rhs * inv_piv;
            std::map<size_t, double> new_e;
            for (const std::pair<const size_t, double>& kv : cur) {
                if (kv.first != best_col && std::abs(kv.second * inv_piv) > 1e-13) {
                    new_e[kv.first] = -kv.second * inv_piv;
                }
            }

            // Back-substitute into all earlier pivot expressions so p_expr only references free columns
            for (const size_t prev_p : piv_list) {
                const std::map<size_t, double>::iterator it = p_expr[prev_p].find(best_col);
                if (it == p_expr[prev_p].end()) {
                    continue;
                }
                const double w = it->second;
                p_expr[prev_p].erase(it);
                p_const[prev_p] += w * new_c;
                for (const std::pair<const size_t, double>& kv : new_e) {
                    const double updated = p_expr[prev_p][kv.first] + w * kv.second;
                    if (std::abs(updated) <= 1e-13) {
                        p_expr[prev_p].erase(kv.first);
                    } else {
                        p_expr[prev_p][kv.first] = updated;
                    }
                }
            }

            is_piv[best_col] = true;
            p_const[best_col] = new_c;
            p_expr[best_col] = std::move(new_e);
            piv_list.push_back(best_col);
        }

        std::vector<size_t> full_to_free(n_vars, n_vars);
        num_free = 0;
        for (size_t i = 0; i < n_vars; i++) {
            if (!is_piv[i]) {
                full_to_free[i] = num_free++;
            }
        }

        x0.assign(n_vars, 0.0);
        s_rows.assign(n_vars, {});
        for (size_t i = 0; i < n_vars; i++) {
            if (!is_piv[i]) {
                s_rows[i].emplace_back(full_to_free[i], 1.0);
            } else {
                x0[i] = p_const[i];
                for (const std::pair<const size_t, double>& kv : p_expr[i]) {
                    if (full_to_free[kv.first] < num_free && std::abs(kv.second) > 1e-13) {
                        s_rows[i].emplace_back(full_to_free[kv.first], kv.second);
                    }
                }
            }
        }
    }

    std::vector<double> expand(const std::vector<double>& y) const {

        std::vector<double> full = x0;
        for (size_t i = 0; i < num_full; i++) {
            double s = full[i];
            for (const std::pair<size_t, double>& kv : s_rows[i]) {
                s += kv.second * y[kv.first];
            }
            full[i] = s;
        }

        return full;
    }

    double max_free_sensitivity(const std::map<size_t, double>& form) const {

        std::map<size_t, double> red;
        for (const std::pair<const size_t, double>& kv : form) {
            for (const std::pair<size_t, double>& sk : s_rows[kv.first]) {
                red[sk.first] += kv.second * sk.second;
            }
        }
        double max_c = 0.0;
        for (const std::pair<const size_t, double>& kv : red) {
            max_c = std::max(max_c, std::abs(kv.second));
        }

        return max_c;
    }
};

/// Convex polygon in canonical triangle barycentric space (alpha, beta) for exact isoline arrangement.
struct BaryVert {
    double a = 0.0;
    double b = 0.0;
};

void split_bary_poly_line(
    const std::vector<BaryVert>& poly,
    double f0,
    double df_da,
    double df_db,
    double iso_val,
    std::vector<BaryVert>& out_le,
    std::vector<BaryVert>& out_ge
) {

    out_le.clear();
    out_ge.clear();
    const size_t m = poly.size();
    if (m < 3) {
        return;
    }
    std::vector<double> vals(m, 0.0);
    double min_v = 1e30;
    double max_v = -1e30;
    for (size_t i = 0; i < m; i++) {
        vals[i] = f0 + df_da * poly[i].a + df_db * poly[i].b - iso_val;
        min_v = std::min(min_v, vals[i]);
        max_v = std::max(max_v, vals[i]);
    }
    if (max_v <= 1e-8) {
        out_le = poly;
        return;
    }
    if (min_v >= -1e-8) {
        out_ge = poly;
        return;
    }

    for (size_t i = 0; i < m; i++) {
        const size_t j = (i + 1) % m;
        const double vi = vals[i];
        const double vj = vals[j];
        if (vi <= 1e-9) {
            out_le.push_back(poly[i]);
        }
        if (vi >= -1e-9) {
            out_ge.push_back(poly[i]);
        }
        if ((vi < -1e-9 && vj > 1e-9) || (vi > 1e-9 && vj < -1e-9)) {
            const double t = std::clamp(-vi / (vj - vi), 0.0, 1.0);
            const BaryVert mid = {
                poly[i].a + t * (poly[j].a - poly[i].a),
                poly[i].b + t * (poly[j].b - poly[i].b)
            };
            out_le.push_back(mid);
            out_ge.push_back(mid);
        }
    }
}

struct DisjointSet {
    std::vector<size_t> parent;

    explicit DisjointSet(size_t n) : parent(n) {
        for (size_t i = 0; i < n; i++) {
            parent[i] = i;
        }
    }

    size_t find(size_t x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }

    void unite(size_t a, size_t b) {
        const size_t ra = find(a);
        const size_t rb = find(b);
        if (ra != rb) {
            parent[rb] = ra;
        }
    }
};

/// Triangulates an arbitrary Mesh and welds close vertices.
void extract_welded_triangles(
    const Mesh& mesh,
    std::vector<Point>& verts,
    std::vector<std::array<size_t, 3>>& tris,
    double tol = 1e-3
) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        mesh.to_vertices_and_faces();
    verts.clear();
    tris.clear();
    if (vf.first.empty() || vf.second.empty()) {
        return;
    }

    const double tol_sq = tol * tol;
    std::vector<size_t> remap(vf.first.size(), 0);
    for (size_t i = 0; i < vf.first.size(); i++) {
        size_t found = verts.size();
        for (size_t k = 0; k < verts.size(); k++) {
            if ((verts[k] - vf.first[i]).magnitude_squared() <= tol_sq) {
                found = k;
                break;
            }
        }
        if (found == verts.size()) {
            verts.push_back(vf.first[i]);
        }
        remap[i] = found;
    }

    for (const std::vector<size_t>& face : vf.second) {
        std::vector<size_t> clean;
        for (const size_t idx : face) {
            const size_t m = remap[idx];
            if (clean.empty() || clean.back() != m) {
                clean.push_back(m);
            }
        }
        while (clean.size() > 1 && clean.back() == clean.front()) {
            clean.pop_back();
        }
        if (clean.size() == 3) {
            tris.push_back({clean[0], clean[1], clean[2]});
        } else if (clean.size() == 4) {
            const double d02 = verts[clean[0]].distance(verts[clean[2]]);
            const double d13 = verts[clean[1]].distance(verts[clean[3]]);
            if (d02 <= d13) {
                tris.push_back({clean[0], clean[1], clean[2]});
                tris.push_back({clean[0], clean[2], clean[3]});
            } else {
                tris.push_back({clean[0], clean[1], clean[3]});
                tris.push_back({clean[1], clean[2], clean[3]});
            }
        } else if (clean.size() > 4) {
            for (size_t k = 1; k + 1 < clean.size(); k++) {
                tris.push_back({clean[0], clean[k], clean[k + 1]});
            }
        }
    }
}

/// Subdivides triangles with edges longer than max_edge via longest-edge midpoint splitting.
void subdivide_triangles_to_length(
    std::vector<Point>& verts,
    std::vector<std::array<size_t, 3>>& tris,
    double max_edge,
    size_t max_tris = 18000
) {

    const double limit_sq = max_edge * max_edge;
    for (int pass = 0; pass < 6; pass++) {
        if (tris.size() >= max_tris) {
            break;
        }
        std::map<std::pair<size_t, size_t>, size_t> edge_mid;
        for (const std::array<size_t, 3>& tri : tris) {
            for (int k = 0; k < 3; k++) {
                const size_t a = tri[static_cast<size_t>(k)];
                const size_t b = tri[static_cast<size_t>((k + 1) % 3)];
                const std::pair<size_t, size_t> key = (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
                if (!edge_mid.count(key) && (verts[a] - verts[b]).magnitude_squared() > limit_sq) {
                    edge_mid[key] = verts.size();
                    verts.push_back(Point::mid_point(verts[a], verts[b]));
                }
            }
        }
        if (edge_mid.empty()) {
            break;
        }

        std::vector<std::array<size_t, 3>> next_tris;
        next_tris.reserve(tris.size() * 2);
        for (const std::array<size_t, 3>& tri : tris) {
            std::array<int, 3> mids = {-1, -1, -1};
            int split_cnt = 0;
            for (int k = 0; k < 3; k++) {
                const size_t a = tri[static_cast<size_t>(k)];
                const size_t b = tri[static_cast<size_t>((k + 1) % 3)];
                const std::pair<size_t, size_t> key = (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
                const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it = edge_mid.find(key);
                if (it != edge_mid.end()) {
                    mids[static_cast<size_t>(k)] = static_cast<int>(it->second);
                    split_cnt++;
                }
            }
            if (split_cnt == 0) {
                next_tris.push_back(tri);
            } else if (split_cnt == 3) {
                const size_t m0 = static_cast<size_t>(mids[0]);
                const size_t m1 = static_cast<size_t>(mids[1]);
                const size_t m2 = static_cast<size_t>(mids[2]);
                next_tris.push_back({tri[0], m0, m2});
                next_tris.push_back({m0, tri[1], m1});
                next_tris.push_back({m2, m1, tri[2]});
                next_tris.push_back({m0, m1, m2});
            } else if (split_cnt == 1) {
                int sk = 0;
                for (int k = 0; k < 3; k++) {
                    if (mids[static_cast<size_t>(k)] >= 0) {
                        sk = k;
                    }
                }
                const size_t v0 = tri[static_cast<size_t>(sk)];
                const size_t v1 = tri[static_cast<size_t>((sk + 1) % 3)];
                const size_t v2 = tri[static_cast<size_t>((sk + 2) % 3)];
                const size_t m = static_cast<size_t>(mids[static_cast<size_t>(sk)]);
                next_tris.push_back({v0, m, v2});
                next_tris.push_back({m, v1, v2});
            } else {
                int unsplit = 0;
                for (int k = 0; k < 3; k++) {
                    if (mids[static_cast<size_t>(k)] < 0) {
                        unsplit = k;
                    }
                }
                const size_t v0 = tri[static_cast<size_t>(unsplit)];
                const size_t v1 = tri[static_cast<size_t>((unsplit + 1) % 3)];
                const size_t v2 = tri[static_cast<size_t>((unsplit + 2) % 3)];
                const size_t m1 = static_cast<size_t>(mids[static_cast<size_t>((unsplit + 1) % 3)]);
                const size_t m2 = static_cast<size_t>(mids[static_cast<size_t>((unsplit + 2) % 3)]);
                next_tris.push_back({v2, m2, m1});
                if (verts[v0].distance(verts[m1]) <= verts[v1].distance(verts[m2])) {
                    next_tris.push_back({v0, v1, m1});
                    next_tris.push_back({v0, m1, m2});
                } else {
                    next_tris.push_back({v0, v1, m2});
                    next_tris.push_back({v1, m1, m2});
                }
            }
        }
        tris = std::move(next_tris);
    }
}

/// Computes angle-weighted vertex normals and barycentric vertex areas on a triangle mesh.
void compute_normals_and_areas(
    const std::vector<Point>& verts,
    const std::vector<std::array<size_t, 3>>& tris,
    std::vector<Vector>& norms,
    std::vector<double>& areas
) {

    const size_t nv = verts.size();
    norms.assign(nv, Vector(0.0, 0.0, 0.0));
    areas.assign(nv, 0.0);

    for (const std::array<size_t, 3>& tri : tris) {
        const Point& p0 = verts[tri[0]];
        const Point& p1 = verts[tri[1]];
        const Point& p2 = verts[tri[2]];
        Vector fn = (p1 - p0).cross(p2 - p0);
        const double dbl_area = fn.magnitude();
        if (dbl_area <= 1e-14) {
            continue;
        }
        fn = fn * (1.0 / dbl_area);
        const double third_area = dbl_area / 6.0;
        for (int k = 0; k < 3; k++) {
            const size_t vid = tri[static_cast<size_t>(k)];
            const Vector e1 = verts[tri[static_cast<size_t>((k + 1) % 3)]] - verts[vid];
            const Vector e2 = verts[tri[static_cast<size_t>((k + 2) % 3)]] - verts[vid];
            const double denom = std::sqrt(std::max(1e-24, e1.magnitude_squared() * e2.magnitude_squared()));
            const double cos_a = std::clamp(e1.dot(e2) / denom, -1.0, 1.0);
            const double ang = std::acos(cos_a);
            norms[vid] = norms[vid] + fn * ang;
            areas[vid] += third_area;
        }
    }

    for (size_t i = 0; i < nv; i++) {
        if (!norms[i].normalize_self()) {
            norms[i] = Vector(0.0, 0.0, 1.0);
        }
    }
}


// ═══════════════════════════════════════════════════════════════════════════
// Dual Graph & Exact Branch-and-Bound + LNS Solver for BIP Tile Matching
// ═══════════════════════════════════════════════════════════════════════════

struct DualTilingGraph {
    size_t num_faces = 0;
    std::vector<std::pair<size_t, size_t>> edges;
    std::vector<double> linear_weights;
    std::vector<bool> is_border;
    std::vector<std::vector<size_t>> face_edges;
    std::vector<std::vector<size_t>> face_adj;
    std::vector<std::pair<size_t, size_t>> parallel_pairs;
    std::vector<std::pair<size_t, size_t>> running_pairs;
    std::vector<std::pair<size_t, size_t>> herringbone_pairs;
    std::vector<std::vector<std::pair<size_t, double>>> quad_adj; // Edge e1 -> [(e2, q_weight)].
};

std::pair<size_t, size_t> ordered_pair(size_t a, size_t b) {
    return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

DualTilingGraph build_dual_tiling_graph(
    const std::vector<std::vector<size_t>>& faces,
    int pattern,
    double penalty_weight,
    double reward_weight,
    double border_weight
) {

    DualTilingGraph g;
    g.num_faces = faces.size();
    g.is_border.assign(g.num_faces, false);
    g.face_adj.resize(g.num_faces);

    std::map<std::pair<size_t, size_t>, size_t> he_to_face;
    for (size_t fi = 0; fi < faces.size(); fi++) {
        const std::vector<size_t>& f = faces[fi];
        for (size_t k = 0; k < f.size(); k++) {
            he_to_face[{f[k], f[(k + 1) % f.size()]}] = fi;
        }
    }

    std::set<std::pair<size_t, size_t>> edge_set;
    for (size_t fi = 0; fi < faces.size(); fi++) {
        const std::vector<size_t>& f = faces[fi];
        bool bnd = false;
        for (size_t k = 0; k < f.size(); k++) {
            const size_t u = f[k];
            const size_t v = f[(k + 1) % f.size()];
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it = he_to_face.find({v, u});
            if (it == he_to_face.end()) {
                bnd = true;
            } else {
                const size_t nb = it->second;
                if (nb != fi) {
                    edge_set.insert(ordered_pair(fi, nb));
                    if (std::find(g.face_adj[fi].begin(), g.face_adj[fi].end(), nb) == g.face_adj[fi].end()) {
                        g.face_adj[fi].push_back(nb);
                    }
                }
            }
        }
        g.is_border[fi] = bnd;
    }

    g.edges.assign(edge_set.begin(), edge_set.end());
    g.linear_weights.resize(g.edges.size(), 1.0);
    g.face_edges.resize(g.num_faces);
    std::map<std::pair<size_t, size_t>, size_t> edge_idx;
    for (size_t ei = 0; ei < g.edges.size(); ei++) {
        const size_t fa = g.edges[ei].first;
        const size_t fb = g.edges[ei].second;
        edge_idx[g.edges[ei]] = ei;
        g.face_edges[fa].push_back(ei);
        g.face_edges[fb].push_back(ei);
        const double wa = g.is_border[fa] ? border_weight : 1.0;
        const double wb = g.is_border[fb] ? border_weight : 1.0;
        g.linear_weights[ei] = 0.5 * (wa + wb);
    }

    // 1. Parallel pairs in 2x2 quad cycles a-b-d-c-a (Eq. 2, optimal_bricks2.py)
    std::vector<std::set<size_t>> adj_sets(g.num_faces);
    for (size_t f = 0; f < g.num_faces; f++) {
        adj_sets[f].insert(g.face_adj[f].begin(), g.face_adj[f].end());
    }
    std::set<std::pair<size_t, size_t>> par_set;
    for (size_t a = 0; a < g.num_faces; a++) {
        for (const size_t b : g.face_adj[a]) {
            for (const size_t c : g.face_adj[a]) {
                if (b == c) {
                    continue;
                }
                for (const size_t d : g.face_adj[c]) {
                    if (d != a && adj_sets[b].count(d)) {
                        const size_t e1 = edge_idx[ordered_pair(a, b)];
                        const size_t e2 = edge_idx[ordered_pair(c, d)];
                        if (e1 != e2) {
                            par_set.insert(ordered_pair(e1, e2));
                        }
                    }
                }
            }
        }
    }
    g.parallel_pairs.assign(par_set.begin(), par_set.end());

    // 2. Running-bond shifted pairs & Herringbone L-pairs (Eq. 3 & Section 3.4, optimal_bricks3*.py)
    std::set<std::pair<size_t, size_t>> run_set;
    std::set<std::pair<size_t, size_t>> her_set;

    for (size_t a = 0; a < faces.size(); a++) {
        const std::vector<size_t>& fa = faces[a];
        if (fa.size() != 4) {
            continue;
        }
        for (size_t k = 0; k < 4; k++) {
            const size_t u = fa[k];
            const size_t v = fa[(k + 1) % 4];
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_b = he_to_face.find({v, u});
            if (it_b == he_to_face.end() || faces[it_b->second].size() != 4) {
                continue;
            }
            const size_t b = it_b->second;
            const std::vector<size_t>& fb = faces[b];
            size_t kb = 4;
            for (size_t m = 0; m < 4; m++) {
                if (fb[m] == v && fb[(m + 1) % 4] == u) {
                    kb = m;
                    break;
                }
            }
            if (kb == 4) {
                continue;
            }
            const size_t v_hat = fb[(kb + 2) % 4];
            const size_t u_hat2 = fb[(kb + 3) % 4];
            const size_t e1 = edge_idx[ordered_pair(a, b)];

            // Side 1 across (u, v_hat) of b -> c1
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_c1 =
                he_to_face.find({v_hat, u});
            if (it_c1 != he_to_face.end() && faces[it_c1->second].size() == 4) {
                const size_t c1 = it_c1->second;
                const std::vector<size_t>& fc1 = faces[c1];
                size_t kc1 = 4;
                for (size_t m = 0; m < 4; m++) {
                    if (fc1[m] == v_hat && fc1[(m + 1) % 4] == u) {
                        kc1 = m;
                        break;
                    }
                }
                if (kc1 < 4) {
                    const size_t w_hat = fc1[(kc1 + 2) % 4];
                    const size_t u_hat = fc1[(kc1 + 3) % 4];
                    const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_d1 =
                        he_to_face.find({v_hat, u_hat});
                    if (it_d1 != he_to_face.end()) {
                        const size_t e2 = edge_idx[ordered_pair(c1, it_d1->second)];
                        if (e1 != e2) {
                            run_set.insert(ordered_pair(e1, e2));
                        }
                    }
                    const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_h1 =
                        he_to_face.find({u_hat, w_hat});
                    if (it_h1 != he_to_face.end()) {
                        const size_t eh = edge_idx[ordered_pair(c1, it_h1->second)];
                        if (e1 != eh) {
                            her_set.insert(ordered_pair(e1, eh));
                        }
                    }
                }
            }

            // Side 2 across (u_hat2, v) of b -> c2
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_c2 =
                he_to_face.find({v, u_hat2});
            if (it_c2 != he_to_face.end() && faces[it_c2->second].size() == 4) {
                const size_t c2 = it_c2->second;
                const std::vector<size_t>& fc2 = faces[c2];
                size_t kc2 = 4;
                for (size_t m = 0; m < 4; m++) {
                    if (fc2[m] == v && fc2[(m + 1) % 4] == u_hat2) {
                        kc2 = m;
                        break;
                    }
                }
                if (kc2 < 4) {
                    const size_t v_hat2 = fc2[(kc2 + 2) % 4];
                    const size_t w_hat2 = fc2[(kc2 + 3) % 4];
                    const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_d2 =
                        he_to_face.find({v_hat2, u_hat2});
                    if (it_d2 != he_to_face.end()) {
                        const size_t e3 = edge_idx[ordered_pair(c2, it_d2->second)];
                        if (e1 != e3) {
                            run_set.insert(ordered_pair(e1, e3));
                        }
                    }
                    const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it_h2 =
                        he_to_face.find({w_hat2, v_hat2});
                    if (it_h2 != he_to_face.end()) {
                        const size_t eh2 = edge_idx[ordered_pair(c2, it_h2->second)];
                        if (e1 != eh2) {
                            her_set.insert(ordered_pair(e1, eh2));
                        }
                    }
                }
            }
        }
    }

    g.running_pairs.assign(run_set.begin(), run_set.end());
    g.herringbone_pairs.assign(her_set.begin(), her_set.end());

    // Build combined quadratic interaction table per edge
    std::vector<std::map<size_t, double>> q_map(g.edges.size());
    for (const std::pair<size_t, size_t>& pr : g.parallel_pairs) {
        q_map[pr.first][pr.second] -= penalty_weight;
        q_map[pr.second][pr.first] -= penalty_weight;
    }
    const std::vector<std::pair<size_t, size_t>>& rewards =
        (pattern == TileVaultGuide::RUNNING_BOND)
            ? g.running_pairs
            : ((pattern == TileVaultGuide::HERRINGBONE) ? g.herringbone_pairs
                                                         : std::vector<std::pair<size_t, size_t>>{});
    for (const std::pair<size_t, size_t>& pr : rewards) {
        q_map[pr.first][pr.second] += reward_weight;
        q_map[pr.second][pr.first] += reward_weight;
    }

    g.quad_adj.resize(g.edges.size());
    for (size_t e = 0; e < g.edges.size(); e++) {
        for (const std::pair<const size_t, double>& kv : q_map[e]) {
            if (std::abs(kv.second) > 1e-12) {
                g.quad_adj[e].emplace_back(kv.first, kv.second);
            }
        }
    }

    return g;
}

double evaluate_bip_objective(const DualTilingGraph& g, const std::vector<uint8_t>& x) {

    double obj = 0.0;
    for (size_t e = 0; e < g.edges.size(); e++) {
        if (!x[e]) {
            continue;
        }
        obj += g.linear_weights[e];
        for (const std::pair<size_t, double>& q : g.quad_adj[e]) {
            if (e < q.first && x[q.first]) {
                obj += q.second;
            }
        }
    }

    return obj;
}

/// Exact Branch-and-Bound solver over a free face subset W while keeping all outside edges fixed.
struct WindowMipSolver {
    const DualTilingGraph& g;
    std::vector<size_t> window_faces;
    std::vector<int> face_to_local; // -1 if outside window.
    std::vector<size_t> window_edges;
    std::vector<int> edge_to_local; // -1 if outside window.
    std::vector<double> base_edge_gain; // Linear + interaction with fixed outside edges.
    std::vector<std::vector<std::pair<size_t, double>>> local_quad; // Local edge -> [(local_e2, w)].
    std::vector<std::vector<size_t>> local_face_edges;
    std::vector<uint8_t> face_matched;
    std::vector<int8_t> edge_state; // -1 undecided, 0 excluded, 1 chosen.
    std::vector<uint8_t> best_edge_choice;
    double best_score = -1e30;
    size_t nodes_visited = 0;
    size_t node_limit = 65000;

    explicit WindowMipSolver(const DualTilingGraph& graph) : g(graph) {}

    double upper_bound(double current_score) const {

        double optimistic = current_score;
        for (size_t lf = 0; lf < window_faces.size(); lf++) {
            if (face_matched[lf]) {
                continue;
            }
            double best_half = 0.0;
            for (const size_t le : local_face_edges[lf]) {
                if (edge_state[le] != -1) {
                    continue;
                }
                const size_t ge = window_edges[le];
                const int lfa = face_to_local[g.edges[ge].first];
                const int lfb = face_to_local[g.edges[ge].second];
                if (face_matched[static_cast<size_t>(lfa)] || face_matched[static_cast<size_t>(lfb)]) {
                    continue;
                }
                double e_max = base_edge_gain[le];
                for (const std::pair<size_t, double>& q : local_quad[le]) {
                    if (edge_state[q.first] == 1) {
                        e_max += q.second;
                    } else if (edge_state[q.first] == -1 && q.second > 0.0) {
                        const size_t ge2 = window_edges[q.first];
                        const size_t qa = static_cast<size_t>(face_to_local[g.edges[ge2].first]);
                        const size_t qb = static_cast<size_t>(face_to_local[g.edges[ge2].second]);
                        if (!face_matched[qa] && !face_matched[qb] &&
                            qa != static_cast<size_t>(lfa) && qa != static_cast<size_t>(lfb) &&
                            qb != static_cast<size_t>(lfa) && qb != static_cast<size_t>(lfb)) {
                            e_max += 0.5 * q.second;
                        }
                    }
                }
                best_half = std::max(best_half, 0.5 * e_max);
            }
            optimistic += best_half;
        }

        return optimistic;
    }

    void dfs(double current_score) {

        if (nodes_visited >= node_limit) {
            return;
        }
        nodes_visited++;

        if (current_score > best_score + 1e-12) {
            best_score = current_score;
            for (size_t le = 0; le < window_edges.size(); le++) {
                best_edge_choice[le] = (edge_state[le] == 1) ? 1 : 0;
            }
        }

        if (upper_bound(current_score) <= best_score + 1e-9) {
            return;
        }

        // Choose most-constrained unmatched face (MRV heuristic) with >= 1 available edge
        int chosen_lf = -1;
        size_t min_deg = 999999;
        double best_face_gain = -1e30;

        for (size_t lf = 0; lf < window_faces.size(); lf++) {
            if (face_matched[lf]) {
                continue;
            }
            size_t avail = 0;
            double top_gain = -1e30;
            for (const size_t le : local_face_edges[lf]) {
                if (edge_state[le] != -1) {
                    continue;
                }
                const size_t ge = window_edges[le];
                const size_t lfa = static_cast<size_t>(face_to_local[g.edges[ge].first]);
                const size_t lfb = static_cast<size_t>(face_to_local[g.edges[ge].second]);
                if (!face_matched[lfa] && !face_matched[lfb]) {
                    avail++;
                    double g_est = base_edge_gain[le];
                    for (const std::pair<size_t, double>& q : local_quad[le]) {
                        if (edge_state[q.first] == 1) {
                            g_est += q.second;
                        } else if (edge_state[q.first] == -1 && q.second > 0.0) {
                            g_est += 0.5 * q.second;
                        }
                    }
                    top_gain = std::max(top_gain, g_est);
                }
            }
            if (avail == 0) {
                continue;
            }
            if (avail < min_deg || (avail == min_deg && top_gain > best_face_gain)) {
                min_deg = avail;
                best_face_gain = top_gain;
                chosen_lf = static_cast<int>(lf);
            }
        }

        if (chosen_lf < 0) {
            return;
        }

        const size_t lf = static_cast<size_t>(chosen_lf);
        std::vector<std::pair<double, size_t>> candidates;
        for (const size_t le : local_face_edges[lf]) {
            if (edge_state[le] != -1) {
                continue;
            }
            const size_t ge = window_edges[le];
            const size_t lfa = static_cast<size_t>(face_to_local[g.edges[ge].first]);
            const size_t lfb = static_cast<size_t>(face_to_local[g.edges[ge].second]);
            if (!face_matched[lfa] && !face_matched[lfb]) {
                double delta = base_edge_gain[le];
                for (const std::pair<size_t, double>& q : local_quad[le]) {
                    if (edge_state[q.first] == 1) {
                        delta += q.second;
                    }
                }
                double future_bonus = 0.0;
                for (const std::pair<size_t, double>& q : local_quad[le]) {
                    if (edge_state[q.first] == -1 && q.second > 0.0) {
                        future_bonus += 0.35 * q.second;
                    }
                }
                candidates.emplace_back(delta + future_bonus, le);
            }
        }
        std::sort(candidates.rbegin(), candidates.rend());

        // Branch 1..d: match lf along one of its available incident edges
        for (const std::pair<double, size_t>& cand : candidates) {
            const size_t le = cand.second;
            const size_t ge = window_edges[le];
            const size_t lfa = static_cast<size_t>(face_to_local[g.edges[ge].first]);
            const size_t lfb = static_cast<size_t>(face_to_local[g.edges[ge].second]);

            double exact_delta = base_edge_gain[le];
            for (const std::pair<size_t, double>& q : local_quad[le]) {
                if (edge_state[q.first] == 1) {
                    exact_delta += q.second;
                }
            }

            std::vector<size_t> touched;
            for (const size_t inc_e : local_face_edges[lfa]) {
                if (inc_e != le && edge_state[inc_e] == -1) {
                    edge_state[inc_e] = 0;
                    touched.push_back(inc_e);
                }
            }
            for (const size_t inc_e : local_face_edges[lfb]) {
                if (inc_e != le && edge_state[inc_e] == -1) {
                    edge_state[inc_e] = 0;
                    touched.push_back(inc_e);
                }
            }

            edge_state[le] = 1;
            face_matched[lfa] = 1;
            face_matched[lfb] = 1;

            dfs(current_score + exact_delta);

            face_matched[lfa] = 0;
            face_matched[lfb] = 0;
            edge_state[le] = -1;
            for (const size_t inc_e : touched) {
                edge_state[inc_e] = -1;
            }
        }

        // Final branch: leave lf unmatched (half-tile)
        std::vector<size_t> blocked;
        for (const size_t inc_e : local_face_edges[lf]) {
            if (edge_state[inc_e] == -1) {
                edge_state[inc_e] = 0;
                blocked.push_back(inc_e);
            }
        }
        face_matched[lf] = 1;
        dfs(current_score);
        face_matched[lf] = 0;
        for (const size_t inc_e : blocked) {
            edge_state[inc_e] = -1;
        }
    }

    void optimize_window(const std::vector<size_t>& free_faces, std::vector<uint8_t>& x, size_t max_nodes) {

        node_limit = max_nodes;
        face_to_local.assign(g.num_faces, -1);
        window_faces.clear();

        // Include only faces that are either unmatched OR matched to ANOTHER face inside free_faces
        std::set<size_t> seed_set(free_faces.begin(), free_faces.end());
        std::vector<int> partner(g.num_faces, -1);
        for (size_t e = 0; e < g.edges.size(); e++) {
            if (x[e]) {
                partner[g.edges[e].first] = static_cast<int>(g.edges[e].second);
                partner[g.edges[e].second] = static_cast<int>(g.edges[e].first);
            }
        }
        for (const size_t f : free_faces) {
            if (partner[f] >= 0) {
                seed_set.insert(static_cast<size_t>(partner[f]));
            }
        }

        for (const size_t f : seed_set) {
            face_to_local[f] = static_cast<int>(window_faces.size());
            window_faces.push_back(f);
        }
        if (window_faces.size() < 2) {
            return;
        }

        edge_to_local.assign(g.edges.size(), -1);
        window_edges.clear();
        for (size_t ge = 0; ge < g.edges.size(); ge++) {
            const int la = face_to_local[g.edges[ge].first];
            const int lb = face_to_local[g.edges[ge].second];
            if (la >= 0 && lb >= 0) {
                edge_to_local[ge] = static_cast<int>(window_edges.size());
                window_edges.push_back(ge);
            }
        }
        if (window_edges.empty()) {
            return;
        }

        const size_t m = window_edges.size();
        base_edge_gain.assign(m, 0.0);
        local_quad.assign(m, {});
        local_face_edges.assign(window_faces.size(), {});

        for (size_t le = 0; le < m; le++) {
            const size_t ge = window_edges[le];
            const size_t lfa = static_cast<size_t>(face_to_local[g.edges[ge].first]);
            const size_t lfb = static_cast<size_t>(face_to_local[g.edges[ge].second]);
            local_face_edges[lfa].push_back(le);
            local_face_edges[lfb].push_back(le);

            double gain = g.linear_weights[ge];
            for (const std::pair<size_t, double>& q : g.quad_adj[ge]) {
                const int lq = edge_to_local[q.first];
                if (lq >= 0) {
                    local_quad[le].emplace_back(static_cast<size_t>(lq), q.second);
                } else if (x[q.first]) {
                    gain += q.second;
                }
            }
            base_edge_gain[le] = gain;
        }

        // Warm-start best_score from current assignment inside window!
        best_edge_choice.assign(m, 0);
        double incumbent = 0.0;
        for (size_t le = 0; le < m; le++) {
            if (x[window_edges[le]]) {
                best_edge_choice[le] = 1;
                incumbent += base_edge_gain[le];
                for (const std::pair<size_t, double>& q : local_quad[le]) {
                    if (le < q.first && x[window_edges[q.first]]) {
                        incumbent += q.second;
                    }
                }
            }
        }
        best_score = incumbent;
        face_matched.assign(window_faces.size(), 0);
        edge_state.assign(m, -1);
        nodes_visited = 0;

        dfs(0.0);

        for (size_t le = 0; le < m; le++) {
            x[window_edges[le]] = best_edge_choice[le];
        }
    }
};

/// Collects a connected BFS patch of up to max_faces faces starting at seed.
std::vector<size_t> bfs_face_window(const DualTilingGraph& g, size_t seed, size_t max_faces) {

    std::vector<size_t> out;
    if (seed >= g.num_faces) {
        return out;
    }
    std::vector<uint8_t> seen(g.num_faces, 0);
    out.push_back(seed);
    seen[seed] = 1;
    size_t head = 0;
    while (head < out.size() && out.size() < max_faces) {
        const size_t u = out[head++];
        for (const size_t nb : g.face_adj[u]) {
            if (!seen[nb]) {
                seen[nb] = 1;
                out.push_back(nb);
                if (out.size() >= max_faces) {
                    break;
                }
            }
        }
    }

    return out;
}

/// Builds a high-cardinality initial matching via augmenting paths prioritizing interior faces.
std::vector<uint8_t> initial_augmenting_matching(const DualTilingGraph& g, size_t seed_offset) {

    std::vector<int> match_face(g.num_faces, -1);
    std::vector<size_t> order;
    order.reserve(g.num_faces);
    for (size_t i = 0; i < g.num_faces; i++) {
        const size_t f = (i + seed_offset) % g.num_faces;
        if (!g.is_border[f]) {
            order.push_back(f);
        }
    }
    for (size_t i = 0; i < g.num_faces; i++) {
        const size_t f = (i + seed_offset) % g.num_faces;
        if (g.is_border[f]) {
            order.push_back(f);
        }
    }

    // Greedy pass + shortest augmenting paths on bipartite/near-bipartite dual graph
    for (const size_t f : order) {
        if (match_face[f] >= 0) {
            continue;
        }
        for (const size_t nb : g.face_adj[f]) {
            if (match_face[nb] < 0 && !g.is_border[nb]) {
                match_face[f] = static_cast<int>(nb);
                match_face[nb] = static_cast<int>(f);
                break;
            }
        }
        if (match_face[f] < 0) {
            for (const size_t nb : g.face_adj[f]) {
                if (match_face[nb] < 0) {
                    match_face[f] = static_cast<int>(nb);
                    match_face[nb] = static_cast<int>(f);
                    break;
                }
            }
        }
    }

    // Augmenting-path pass from any remaining unmatched interior faces to free border/interior faces
    for (int pass = 0; pass < 3; pass++) {
        for (const size_t start : order) {
            if (match_face[start] >= 0) {
                continue;
            }
            // Alternating BFS: start (unmatched) -> nb (matched to m) -> m (unmatched step) -> ...
            std::vector<int> parent_u(g.num_faces, -1);
            std::vector<int> via_nb(g.num_faces, -1);
            std::vector<size_t> queue = {start};
            parent_u[start] = static_cast<int>(start);
            size_t head = 0;
            int found_free_nb = -1;
            int found_from_u = -1;

            while (head < queue.size() && found_free_nb < 0) {
                const size_t u = queue[head++];
                for (const size_t nb : g.face_adj[u]) {
                    if (nb == start) {
                        continue;
                    }
                    const int m = match_face[nb];
                    if (m < 0) {
                        // Augmenting path to free face (always accept if start is interior or nb is interior!)
                        if (!g.is_border[start] || !g.is_border[nb] || pass >= 1) {
                            found_free_nb = static_cast<int>(nb);
                            found_from_u = static_cast<int>(u);
                            break;
                        }
                    } else if (parent_u[static_cast<size_t>(m)] < 0) {
                        parent_u[static_cast<size_t>(m)] = static_cast<int>(u);
                        via_nb[static_cast<size_t>(m)] = static_cast<int>(nb);
                        queue.push_back(static_cast<size_t>(m));
                    }
                }
            }

            // Also allow shifting an unmatched interior face out to an already-matched border face!
            if (found_free_nb < 0 && !g.is_border[start]) {
                for (size_t m = 0; m < g.num_faces; m++) {
                    if (m != start && parent_u[m] >= 0 && g.is_border[m]) {
                        // Evict border face m so interior start becomes matched!
                        size_t cur = m;
                        while (cur != start) {
                            const size_t prev_u = static_cast<size_t>(parent_u[cur]);
                            const size_t mid_nb = static_cast<size_t>(via_nb[cur]);
                            match_face[prev_u] = static_cast<int>(mid_nb);
                            match_face[mid_nb] = static_cast<int>(prev_u);
                            cur = prev_u;
                        }
                        match_face[m] = -1;
                        break;
                    }
                }
            } else if (found_free_nb >= 0) {
                int cur_u = found_from_u;
                int cur_nb = found_free_nb;
                while (true) {
                    const int old_nb = match_face[static_cast<size_t>(cur_u)];
                    match_face[static_cast<size_t>(cur_u)] = cur_nb;
                    match_face[static_cast<size_t>(cur_nb)] = cur_u;
                    if (cur_u == static_cast<int>(start)) {
                        break;
                    }
                    cur_nb = via_nb[static_cast<size_t>(cur_u)];
                    cur_u = parent_u[static_cast<size_t>(cur_u)];
                    (void)old_nb;
                }
            }
        }
    }

    std::vector<uint8_t> x(g.edges.size(), 0);
    for (size_t e = 0; e < g.edges.size(); e++) {
        const size_t fa = g.edges[e].first;
        const size_t fb = g.edges[e].second;
        if (match_face[fa] == static_cast<int>(fb) && match_face[fb] == static_cast<int>(fa)) {
            x[e] = 1;
        }
    }

    return x;
}

/// Welds polygon rings with normals into a Mesh.
Mesh weld_polygon_mesh(
    const std::vector<std::vector<Point>>& polys,
    const std::vector<std::vector<Vector>>& poly_norms,
    double tol = 1e-2
) {

    std::vector<Point> verts;
    std::vector<Vector> norms;
    std::vector<std::vector<size_t>> faces;
    const double tol_sq = tol * tol;

    for (size_t p = 0; p < polys.size(); p++) {
        const std::vector<Point>& ring = polys[p];
        if (ring.size() < 3) {
            continue;
        }
        std::vector<size_t> ids;
        Vector up_ref(0.0, 0.0, 0.0);
        for (size_t k = 0; k < ring.size(); k++) {
            const Vector nrm =
                (p < poly_norms.size() && k < poly_norms[p].size())
                    ? poly_norms[p][k]
                    : Vector(0.0, 0.0, 1.0);
            size_t found = verts.size();
            for (size_t v = 0; v < verts.size(); v++) {
                if ((verts[v] - ring[k]).magnitude_squared() <= tol_sq) {
                    norms[v] = norms[v] + nrm;
                    found = v;
                    break;
                }
            }
            if (found == verts.size()) {
                verts.push_back(ring[k]);
                norms.push_back(nrm);
            }
            if (ids.empty() || ids.back() != found) {
                ids.push_back(found);
            }
            up_ref = up_ref + nrm;
        }
        while (ids.size() > 1 && ids.back() == ids.front()) {
            ids.pop_back();
        }
        std::vector<size_t> unique_ids;
        for (const size_t id : ids) {
            if (std::find(unique_ids.begin(), unique_ids.end(), id) == unique_ids.end()) {
                unique_ids.push_back(id);
            }
        }
        if (unique_ids.size() < 3) {
            continue;
        }

        Point c(0.0, 0.0, 0.0);
        for (const size_t id : unique_ids) {
            c = c + Vector(verts[id][0], verts[id][1], verts[id][2]);
        }
        c = Point(
            c[0] / static_cast<double>(unique_ids.size()),
            c[1] / static_cast<double>(unique_ids.size()),
            c[2] / static_cast<double>(unique_ids.size())
        );
        Vector area_n(0.0, 0.0, 0.0);
        for (size_t k = 0; k < unique_ids.size(); k++) {
            const Vector da = verts[unique_ids[k]] - c;
            const Vector db = verts[unique_ids[(k + 1) % unique_ids.size()]] - c;
            area_n = area_n + da.cross(db);
        }
        if (area_n.magnitude_squared() <= 1e-10) {
            continue;
        }
        if (up_ref.magnitude_squared() > 1e-10 && area_n.dot(up_ref) < 0.0) {
            std::reverse(unique_ids.begin(), unique_ids.end());
        }
        faces.push_back(unique_ids);
    }

    Mesh out = Mesh::from_vertices_and_faces(verts, faces);
    for (size_t i = 0; i < verts.size(); i++) {
        Vector n = norms[i];
        if (!n.normalize_self()) {
            n = Vector(0.0, 0.0, 1.0);
        }
        if (out.vertex.count(i)) {
            out.vertex[i].set_normal(n[0], n[1], n[2]);
        }
    }

    return out;
}

/// Builds a closed rectangular prism NodeBlockElement centered at origin with frame (ex, ey, ez).
std::shared_ptr<NodeBlockElement> make_box_tile_element(
    const Point& centre,
    const Vector& ex,
    const Vector& ey,
    const Vector& ez,
    double len_x,
    double len_y,
    double thickness,
    bool trim_base,
    double base_z,
    const std::string& name
) {

    const double hx = 0.5 * len_x;
    const double hy = 0.5 * len_y;
    const double hz = 0.5 * thickness;
    const std::array<Point, 4> bot = {
        centre - ex * hx - ey * hy - ez * hz,
        centre + ex * hx - ey * hy - ez * hz,
        centre + ex * hx + ey * hy - ez * hz,
        centre - ex * hx + ey * hy - ez * hz
    };
    const std::array<Point, 4> top = {
        centre - ex * hx - ey * hy + ez * hz,
        centre + ex * hx - ey * hy + ez * hz,
        centre + ex * hx + ey * hy + ez * hz,
        centre - ex * hx + ey * hy + ez * hz
    };

    const std::vector<std::vector<Point>> faces = {
        {bot[3], bot[2], bot[1], bot[0]},
        {top[0], top[1], top[2], top[3]},
        {bot[0], bot[1], top[1], top[0]},
        {bot[1], bot[2], top[2], top[1]},
        {bot[2], bot[3], top[3], top[2]},
        {bot[3], bot[0], top[0], top[3]}
    };
    Mesh solid = Mesh::from_polylines(faces, 1e-4);
    solid.orient_outward();
    std::shared_ptr<NodeBlockElement> elem = std::make_shared<NodeBlockElement>(solid, name);

    if (trim_base) {
        double min_z = 1e30;
        double max_z = -1e30;
        for (int k = 0; k < 4; k++) {
            min_z = std::min({min_z, bot[static_cast<size_t>(k)][2], top[static_cast<size_t>(k)][2]});
            max_z = std::max({max_z, bot[static_cast<size_t>(k)][2], top[static_cast<size_t>(k)][2]});
        }
        if (max_z <= base_z + 2.0) {
            return nullptr;
        }
        if (min_z < base_z - 0.5) {
            elem->cuts = {Plane(Point(0.0, 0.0, base_z), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0))};
            elem->invalidate_geometry();
        }
    }

    return elem;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// TileVaultGuide Shell Loaders & Presets
// ═══════════════════════════════════════════════════════════════════════════

Mesh TileVaultGuide::materialisation_vault_shell() {

    const std::filesystem::path path =
        config::session_data_dir() / "templates" / "materialisation_vault_shell.json";
    if (std::filesystem::exists(path)) {
        return Mesh::file_json_load(path.string());
    }

    const SurfaceDivision div({pillow_dome_surface(3000.0, 3000.0, 1219.0)}, 150.0, 150.0);

    return div.quads();
}

Mesh TileVaultGuide::materialisation_tripod_shell() {

    const std::filesystem::path path =
        config::session_data_dir() / "templates" / "materialisation_tripod_shell.json";
    if (std::filesystem::exists(path)) {
        return Mesh::file_json_load(path.string());
    }

    const SurfaceDivision div({saddle_vault_surface(5116.0, 6272.0, 2550.0, 520.0)}, 220.0, 220.0);

    return div.quads();
}

TileVaultGuide TileVaultGuide::funicular_running_blocks() {

    TileVaultGuide guide;
    guide.shell = materialisation_vault_shell();
    guide.pattern = RUNNING_BOND;
    guide.materialisation = MERGED_BLOCKS;
    guide.tile_width = 130.0;
    guide.thickness = 70.0;
    guide.shapeup_iterations = 50;
    guide.planar_sides = true;
    guide.trim_base = true;

    return guide;
}

TileVaultGuide TileVaultGuide::funicular_standard_tiles() {

    TileVaultGuide guide;
    guide.shell = materialisation_vault_shell();
    guide.pattern = RUNNING_BOND;
    guide.materialisation = STANDARD_TILES;
    guide.tile_width = 130.0;
    guide.thickness = 26.0;
    guide.mortar_gap = 5.0;
    guide.shapeup_iterations = 50;
    guide.trim_base = false;

    return guide;
}

TileVaultGuide TileVaultGuide::funicular_herringbone_blocks() {

    TileVaultGuide guide;
    guide.shell = materialisation_vault_shell();
    guide.pattern = HERRINGBONE;
    guide.materialisation = MERGED_BLOCKS;
    guide.tile_width = 135.0;
    guide.thickness = 70.0;
    guide.orientation_degrees = 45.0;
    guide.shapeup_iterations = 50;
    guide.planar_sides = true;
    guide.trim_base = true;

    return guide;
}

TileVaultGuide TileVaultGuide::funicular_geodesic_blocks() {

    TileVaultGuide guide;
    guide.shell = materialisation_vault_shell();
    guide.pattern = GEODESIC_COURSES;
    guide.materialisation = MERGED_BLOCKS;
    guide.tile_width = 135.0;
    guide.thickness = 70.0;
    guide.polar_geodesics = true;
    guide.planar_sides = true;
    guide.trim_base = true;

    return guide;
}

TileVaultGuide TileVaultGuide::tripod_staggered_blocks() {

    TileVaultGuide guide;
    guide.shell = materialisation_tripod_shell();
    guide.pattern = STAGGERED;
    guide.materialisation = MERGED_BLOCKS;
    guide.tile_width = 230.0;
    guide.thickness = 130.0;
    guide.shapeup_iterations = 50;
    guide.planar_sides = true;
    guide.trim_base = true;

    return guide;
}

TileVaultGuide TileVaultGuide::tripod_running_tiles() {

    TileVaultGuide guide;
    guide.shell = materialisation_tripod_shell();
    guide.pattern = RUNNING_BOND;
    guide.materialisation = STANDARD_TILES;
    guide.tile_width = 210.0;
    guide.thickness = 32.0;
    guide.mortar_gap = 8.0;
    guide.shapeup_iterations = 50;
    guide.trim_base = false;

    return guide;
}

TileVaultGuide TileVaultGuide::tripod_herringbone_tiles() {

    TileVaultGuide guide;
    guide.shell = materialisation_tripod_shell();
    guide.pattern = HERRINGBONE;
    guide.materialisation = STANDARD_TILES;
    guide.tile_width = 210.0;
    guide.thickness = 32.0;
    guide.mortar_gap = 8.0;
    guide.orientation_degrees = 45.0;
    guide.shapeup_iterations = 50;
    guide.trim_base = false;

    return guide;
}

TileVaultGuide TileVaultGuide::tripod_geodesic_blocks() {

    TileVaultGuide guide;
    guide.shell = materialisation_tripod_shell();
    guide.pattern = GEODESIC_COURSES;
    guide.materialisation = MERGED_BLOCKS;
    guide.tile_width = 215.0;
    guide.thickness = 120.0;
    guide.polar_geodesics = false;
    guide.planar_sides = true;
    guide.trim_base = true;

    return guide;
}

// ═══════════════════════════════════════════════════════════════════════════
// Distortion Evaluation
// ═══════════════════════════════════════════════════════════════════════════

void evaluate_quad_distortion(
    const Mesh& quad_mesh,
    double target_edge,
    double& mean_edge_error,
    double& max_edge_error,
    double& mean_angle_error_deg,
    double& max_angle_error_deg,
    size_t& singularity_count
) {

    mean_edge_error = 0.0;
    max_edge_error = 0.0;
    mean_angle_error_deg = 0.0;
    max_angle_error_deg = 0.0;
    singularity_count = 0;

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        quad_mesh.to_vertices_and_faces();
    const std::vector<Point>& verts = vf.first;
    const std::vector<std::vector<size_t>>& faces = vf.second;
    if (verts.empty() || faces.empty()) {
        return;
    }

    double sum_edge = 0.0;
    size_t cnt_edge = 0;
    double sum_ang = 0.0;
    size_t cnt_ang = 0;

    std::map<std::pair<size_t, size_t>, int> edge_uses;
    std::vector<size_t> valence(verts.size(), 0);

    for (const std::vector<size_t>& f : faces) {
        const size_t m = f.size();
        if (m < 3) {
            continue;
        }
        for (size_t k = 0; k < m; k++) {
            const size_t u = f[k];
            const size_t v = f[(k + 1) % m];
            valence[u]++;
            edge_uses[ordered_pair(u, v)]++;
            if (target_edge > 1e-9) {
                const double rel = std::abs(verts[u].distance(verts[v]) - target_edge) / target_edge;
                sum_edge += rel;
                max_edge_error = std::max(max_edge_error, rel);
                cnt_edge++;
            }
        }
        if (m == 4) {
            for (size_t k = 0; k < 4; k++) {
                Vector e_prev = verts[f[(k + 3) % 4]] - verts[f[k]];
                Vector e_next = verts[f[(k + 1) % 4]] - verts[f[k]];
                if (e_prev.normalize_self() && e_next.normalize_self()) {
                    const double deg = std::acos(std::clamp(e_prev.dot(e_next), -1.0, 1.0)) *
                                       Tolerance::TO_DEGREES;
                    const double err = std::abs(deg - 90.0);
                    sum_ang += err;
                    max_angle_error_deg = std::max(max_angle_error_deg, err);
                    cnt_ang++;
                }
            }
        }
    }

    std::vector<bool> on_boundary(verts.size(), false);
    for (const std::pair<const std::pair<size_t, size_t>, int>& kv : edge_uses) {
        if (kv.second == 1) {
            on_boundary[kv.first.first] = true;
            on_boundary[kv.first.second] = true;
        }
    }
    for (size_t v = 0; v < verts.size(); v++) {
        if (!on_boundary[v] && valence[v] > 0 && valence[v] != 4) {
            singularity_count++;
        }
    }

    mean_edge_error = cnt_edge > 0 ? (sum_edge / static_cast<double>(cnt_edge)) : 0.0;
    mean_angle_error_deg = cnt_ang > 0 ? (sum_ang / static_cast<double>(cnt_ang)) : 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Pure-C++ Field-Guided Seamless Quad Meshing (Amir Vaxman Directional)
// PCFaceTangentBundle + PowerField + CutMesh + Seamless MIQ + Isoline Mesher
// ═══════════════════════════════════════════════════════════════════════════

Mesh quadriflow_remesh(
    const Mesh& shell_mesh,
    double target_edge,
    double orientation_degrees,
    bool align_boundary
) {

    const double h = std::max(5.0, target_edge);
    std::vector<Point> base_verts;
    std::vector<std::array<size_t, 3>> base_tris;
    extract_welded_triangles(shell_mesh, base_verts, base_tris, 1e-2);
    if (base_verts.empty() || base_tris.empty()) {
        return Mesh();
    }

    // Orient base triangles consistently (+Z global sense for shells)
    double signed_z_sum = 0.0;
    for (const std::array<size_t, 3>& t : base_tris) {
        signed_z_sum += (base_verts[t[1]] - base_verts[t[0]]).cross(base_verts[t[2]] - base_verts[t[0]])[2];
    }
    if (signed_z_sum < 0.0) {
        for (std::array<size_t, 3>& t : base_tris) {
            std::swap(t[1], t[2]);
        }
    }

    std::vector<Vector> base_norms;
    std::vector<double> base_areas;
    compute_normals_and_areas(base_verts, base_tris, base_norms, base_areas);

    SurfaceProjector projector;
    projector.build(base_verts, base_norms, base_tris);

    // Collect exact naked boundary loops and multi-vertex sharp corners from base shell BEFORE subdivision
    std::map<std::pair<size_t, size_t>, int> base_he;
    for (size_t f = 0; f < base_tris.size(); f++) {
        for (size_t k = 0; k < 3; k++) {
            base_he[{base_tris[f][k], base_tris[f][(k + 1) % 3]}] = static_cast<int>(3 * f + k);
        }
    }
    std::map<size_t, size_t> base_bnd_next;
    std::vector<bool> base_is_bnd(base_verts.size(), false);
    for (size_t f = 0; f < base_tris.size(); f++) {
        for (size_t k = 0; k < 3; k++) {
            const size_t u = base_tris[f][k];
            const size_t w = base_tris[f][(k + 1) % 3];
            if (!base_he.count({w, u})) {
                base_is_bnd[u] = true;
                base_is_bnd[w] = true;
                base_bnd_next[u] = w;
            }
        }
    }
    std::vector<Point> sharp_corner_pts;
    {
        std::set<size_t> vis_bv;
        for (const std::pair<const size_t, size_t>& kv : base_bnd_next) {
            if (vis_bv.count(kv.first)) {
                continue;
            }
            std::vector<size_t> loop;
            size_t cur = kv.first;
            for (size_t step = 0; step <= base_bnd_next.size(); step++) {
                if (vis_bv.count(cur)) {
                    break;
                }
                vis_bv.insert(cur);
                loop.push_back(cur);
                const std::map<size_t, size_t>::const_iterator it = base_bnd_next.find(cur);
                if (it == base_bnd_next.end()) {
                    break;
                }
                cur = it->second;
            }
            const size_t m = loop.size();
            if (m < 4) {
                continue;
            }
            // Measure windowed chord turn over +-0.45*h around each boundary vertex to catch multi-vertex corners
            std::vector<double> turn_cos(m, 1.0);
            for (size_t i = 0; i < m; i++) {
                const Point& pv = base_verts[loop[i]];
                size_t i_prev = (i + m - 1) % m;
                double d_prev = pv.distance(base_verts[loop[i_prev]]);
                for (size_t s = 1; s < m / 3 && d_prev < 0.42 * h; s++) {
                    i_prev = (i + m - 1 - s) % m;
                    d_prev = pv.distance(base_verts[loop[i_prev]]);
                }
                size_t i_next = (i + 1) % m;
                double d_next = pv.distance(base_verts[loop[i_next]]);
                for (size_t s = 1; s < m / 3 && d_next < 0.42 * h; s++) {
                    i_next = (i + 1 + s) % m;
                    d_next = pv.distance(base_verts[loop[i_next]]);
                }
                Vector vin = pv - base_verts[loop[i_prev]];
                Vector vout = base_verts[loop[i_next]] - pv;
                if (vin.normalize_self() && vout.normalize_self()) {
                    turn_cos[i] = vin.dot(vout);
                }
            }
            for (size_t i = 0; i < m; i++) {
                if (turn_cos[i] < 0.72 &&
                    turn_cos[i] <= turn_cos[(i + m - 1) % m] + 1e-6 &&
                    turn_cos[i] <= turn_cos[(i + 1) % m] + 1e-6) {
                    bool near_existing = false;
                    for (const Point& sc : sharp_corner_pts) {
                        if (sc.distance(base_verts[loop[i]]) < 0.75 * h) {
                            near_existing = true;
                            break;
                        }
                    }
                    if (!near_existing) {
                        sharp_corner_pts.push_back(base_verts[loop[i]]);
                    }
                }
            }
        }
    }

    // 1. Subdivide triangles to <= 0.52 * h while keeping boundary midpoints exact on boundary segments
    std::vector<Point> verts = base_verts;
    std::vector<std::array<size_t, 3>> tris = base_tris;
    subdivide_triangles_to_length(verts, tris, 0.52 * h, 7500);

    const size_t nv = verts.size();
    const size_t nf = tris.size();
    const size_t nhe = 3 * nf;

    // Build exact DCEL halfedge tables (Directional::TriMesh)
    std::vector<int> he_twin(nhe, -1);
    std::map<std::pair<size_t, size_t>, int> dir_he_map;
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            dir_he_map[{tris[f][k], tris[f][(k + 1) % 3]}] = static_cast<int>(3 * f + k);
        }
    }
    std::vector<bool> is_bnd_v(nv, false);
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            const size_t u = tris[f][k];
            const size_t w = tris[f][(k + 1) % 3];
            const std::map<std::pair<size_t, size_t>, int>::const_iterator it = dir_he_map.find({w, u});
            if (it != dir_he_map.end()) {
                he_twin[3 * f + k] = it->second;
            } else {
                is_bnd_v[u] = true;
                is_bnd_v[w] = true;
            }
        }
    }

    // Project interior subdivided vertices onto smooth shell; keep boundary vertices exact on base boundary
    for (size_t v = 0; v < nv; v++) {
        if (!is_bnd_v[v]) {
            verts[v] = projector.project(verts[v]).first;
        }
    }

    std::vector<bool> is_sharp_v(nv, false);
    for (size_t v = 0; v < nv; v++) {
        if (!is_bnd_v[v]) {
            continue;
        }
        for (const Point& sc : sharp_corner_pts) {
            if (verts[v].distance(sc) < 1e-1) {
                is_sharp_v[v] = true;
                break;
            }
        }
    }

    // Build undirected edge tables EV, EF, EH, FE & vertex halfedge pointers VH (boundary-first!)
    std::vector<std::pair<size_t, size_t>> ev_list;
    std::vector<std::array<int, 2>> ef_list;
    std::vector<std::array<int, 2>> eh_list;
    std::vector<std::array<size_t, 3>> fe_table(nf, {0, 0, 0});
    std::vector<int> he_to_edge(nhe, -1);
    std::map<std::pair<size_t, size_t>, size_t> edge_lookup;

    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            const int h_id = static_cast<int>(3 * f + k);
            const size_t u = tris[f][k];
            const size_t w = tris[f][(k + 1) % 3];
            const std::pair<size_t, size_t> ek = ordered_pair(u, w);
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it = edge_lookup.find(ek);
            if (it == edge_lookup.end()) {
                const size_t eid = ev_list.size();
                edge_lookup[ek] = eid;
                ev_list.emplace_back(u, w);
                ef_list.push_back({static_cast<int>(f), -1});
                eh_list.push_back({h_id, -1});
                fe_table[f][k] = eid;
                he_to_edge[static_cast<size_t>(h_id)] = static_cast<int>(eid);
            } else {
                const size_t eid = it->second;
                ef_list[eid][1] = static_cast<int>(f);
                eh_list[eid][1] = h_id;
                fe_table[f][k] = eid;
                he_to_edge[static_cast<size_t>(h_id)] = static_cast<int>(eid);
            }
        }
    }
    const size_t ne = ev_list.size();

    // VH[v]: outgoing halfedge; for boundary vertices, wind CW so VH[v] is the outgoing boundary halfedge (twin == -1)
    std::vector<int> vh(nv, -1);
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        const size_t u = tris[h_id / 3][h_id % 3];
        if (vh[u] < 0) {
            vh[u] = static_cast<int>(h_id);
        }
    }
    for (size_t v = 0; v < nv; v++) {
        if (!is_bnd_v[v] || vh[v] < 0) {
            continue;
        }
        int cur = vh[v];
        const int start = cur;
        while (he_twin[static_cast<size_t>(cur)] != -1) {
            const int tw = he_twin[static_cast<size_t>(cur)];
            const int nxt = 3 * (tw / 3) + ((tw % 3) + 1) % 3;
            cur = nxt;
            if (cur == start) {
                break;
            }
        }
        vh[v] = cur;
    }

    // 2. PCFaceTangentBundle: FBx, FBy, faceNormals, faceAreas, Levi-Civita connection, & Brandt harmonic mass
    std::vector<Vector> fbx(nf);
    std::vector<Vector> fby(nf);
    std::vector<Vector> fnrm(nf);
    std::vector<double> farea(nf, 0.0);
    double total_area = 0.0;
    for (size_t f = 0; f < nf; f++) {
        const Vector lx = verts[tris[f][1]] - verts[tris[f][0]];
        const Vector ly = verts[tris[f][2]] - verts[tris[f][0]];
        const Vector lz = lx.cross(ly);
        const double area = 0.5 * lz.magnitude();
        farea[f] = std::max(1e-12, area);
        total_area += farea[f];
        fnrm[f] = (area > 1e-14) ? lz.normalized() : Vector(0.0, 0.0, 1.0);
        fbx[f] = (lx.magnitude_squared() > 1e-16) ? lx.normalized() : Vector(1.0, 0.0, 0.0);
        fby[f] = fnrm[f].cross(fbx[f]).normalized();
    }

    std::vector<std::complex<double>> conn(ne, std::complex<double>(1.0, 0.0));
    std::vector<double> conn_mass(ne, 0.0);
    double total_smooth_weight = 0.0;
    for (size_t e = 0; e < ne; e++) {
        if (ef_list[e][0] < 0 || ef_list[e][1] < 0) {
            continue;
        }
        const size_t f0 = static_cast<size_t>(ef_list[e][0]);
        const size_t f1 = static_cast<size_t>(ef_list[e][1]);
        const Vector raw_ev = verts[ev_list[e].second] - verts[ev_list[e].first];
        const double len2 = raw_ev.magnitude_squared();
        if (len2 <= 1e-18) {
            continue;
        }
        const Vector ev = raw_ev * (1.0 / std::sqrt(len2));
        const std::complex<double> ef(ev.dot(fbx[f0]), ev.dot(fby[f0]));
        const std::complex<double> eg(ev.dot(fbx[f1]), ev.dot(fby[f1]));
        if (std::abs(ef) > 1e-14 && std::abs(eg) > 1e-14) {
            conn[e] = (eg / ef) / std::abs(eg / ef);
        }
        conn_mass[e] = 3.0 * len2 / (farea[f0] + farea[f1]);
        total_smooth_weight += conn_mass[e];
    }
    total_smooth_weight = std::max(1e-12, total_smooth_weight);

    // Hildebrandt et al. (2005) quadratic height-function shape operator (shape_operator.h) over 2-ring
    std::vector<Vector> vnrm(nv, Vector(0.0, 0.0, 0.0));
    std::vector<std::set<size_t>> v_adj(nv);
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            vnrm[tris[f][k]] = vnrm[tris[f][k]] + fnrm[f] * farea[f];
            v_adj[tris[f][k]].insert(tris[f][(k + 1) % 3]);
            v_adj[tris[f][(k + 1) % 3]].insert(tris[f][k]);
        }
    }
    for (size_t v = 0; v < nv; v++) {
        if (!vnrm[v].normalize_self()) {
            vnrm[v] = Vector(0.0, 0.0, 1.0);
        }
    }
    for (int pass = 0; pass < 3; pass++) {
        std::vector<Vector> next_vn = vnrm;
        for (size_t v = 0; v < nv; v++) {
            Vector acc = vnrm[v] * 2.0;
            for (const size_t nb : v_adj[v]) {
                acc = acc + vnrm[nb];
            }
            if (acc.normalize_self()) {
                next_vn[v] = acc;
            }
        }
        vnrm = std::move(next_vn);
    }

    const double rot_rad = orientation_degrees * Tolerance::TO_RADIANS;
    std::vector<Vector> v_pdir(nv, Vector(1.0, 0.0, 0.0));
    std::vector<double> v_aniso(nv, 0.0);
    for (size_t v = 0; v < nv; v++) {
        Vector bx;
        Vector by;
        build_tangent_frame(vnrm[v], bx, by);

        std::set<size_t> ring2 = v_adj[v];
        for (const size_t nb1 : v_adj[v]) {
            for (const size_t nb2 : v_adj[nb1]) {
                if (nb2 != v) {
                    ring2.insert(nb2);
                }
            }
        }

        double ata[5][5] = {};
        double atr[5] = {};
        for (const size_t nj : ring2) {
            const Vector dp = verts[nj] - verts[v];
            const double x = dp.dot(bx) / h;
            const double y = dp.dot(by) / h;
            const double z = dp.dot(vnrm[v]) / h;
            const double row[5] = {x * x, x * y, y * y, x, y};
            for (int r = 0; r < 5; r++) {
                atr[r] += row[r] * z;
                for (int c = 0; c < 5; c++) {
                    ata[r][c] += row[r] * row[c];
                }
            }
        }
        for (int d = 0; d < 5; d++) {
            ata[d][d] += 1e-9;
        }
        double coeffs[5] = {};
        if (ring2.size() >= 5 && solve_5x5_linear(ata, atr, coeffs)) {
            const double s00 = -2.0 * coeffs[0];
            const double s01 = -coeffs[1];
            const double s11 = -2.0 * coeffs[2];
            const double diff = std::sqrt((s00 - s11) * (s00 - s11) + 4.0 * s01 * s01);
            if (diff > 1e-4) {
                const double theta = 0.5 * std::atan2(2.0 * s01, s00 - s11) + rot_rad;
                v_pdir[v] = (bx * std::cos(theta) + by * std::sin(theta)).normalized();
                v_aniso[v] = std::min(1.0, diff * 2.5);
            }
        }
    }

    // Smooth boundary tangent along smooth boundary chains (between sharp corners)
    std::vector<int> bnd_next_he(nhe, -1);
    std::vector<int> bnd_prev_he(nhe, -1);
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        if (he_twin[h_id] != -1) {
            continue;
        }
        const size_t w = tris[h_id / 3][(h_id % 3 + 1) % 3];
        if (vh[w] >= 0 && he_twin[static_cast<size_t>(vh[w])] == -1) {
            bnd_next_he[h_id] = vh[w];
            bnd_prev_he[static_cast<size_t>(vh[w])] = static_cast<int>(h_id);
        }
    }
    std::vector<Vector> he_bnd_dir(nhe, Vector(0.0, 0.0, 0.0));
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        if (he_twin[h_id] != -1) {
            continue;
        }
        const size_t u = tris[h_id / 3][h_id % 3];
        const size_t w = tris[h_id / 3][(h_id % 3 + 1) % 3];
        Vector acc = (verts[w] - verts[u]).normalized() * 2.0;
        if (!is_sharp_v[u] && bnd_prev_he[h_id] >= 0) {
            const size_t ph = static_cast<size_t>(bnd_prev_he[h_id]);
            acc = acc + (verts[tris[ph / 3][(ph % 3 + 1) % 3]] - verts[tris[ph / 3][ph % 3]]).normalized();
        }
        if (!is_sharp_v[w] && bnd_next_he[h_id] >= 0) {
            const size_t nh = static_cast<size_t>(bnd_next_he[h_id]);
            acc = acc + (verts[tris[nh / 3][(nh % 3 + 1) % 3]] - verts[tris[nh / 3][nh % 3]]).normalized();
        }
        if (acc.normalize_self()) {
            he_bnd_dir[h_id] = acc;
        }
    }

    // 3. Directional::power_field (N = 4 intrinsic connection Laplacian + boundary & curvature alignment)
    std::vector<std::map<size_t, double>> pf_rows(2 * nf);
    std::vector<double> pf_rhs(2 * nf, 0.0);

    for (size_t e = 0; e < ne; e++) {
        if (ef_list[e][0] < 0 || ef_list[e][1] < 0) {
            continue;
        }
        const size_t f0 = static_cast<size_t>(ef_list[e][0]);
        const size_t f1 = static_cast<size_t>(ef_list[e][1]);
        const std::complex<double> c2 = conn[e] * conn[e];
        const std::complex<double> c4 = c2 * c2;
        const double cr = c4.real();
        const double ci = c4.imag();
        const double w = conn_mass[e] / total_smooth_weight;

        pf_rows[2 * f0][2 * f0] += w;
        pf_rows[2 * f0 + 1][2 * f0 + 1] += w;
        pf_rows[2 * f1][2 * f1] += w;
        pf_rows[2 * f1 + 1][2 * f1 + 1] += w;

        pf_rows[2 * f0][2 * f1] -= w * cr;
        pf_rows[2 * f0][2 * f1 + 1] -= w * ci;
        pf_rows[2 * f0 + 1][2 * f1] += w * ci;
        pf_rows[2 * f0 + 1][2 * f1 + 1] -= w * cr;

        pf_rows[2 * f1][2 * f0] -= w * cr;
        pf_rows[2 * f1][2 * f0 + 1] += w * ci;
        pf_rows[2 * f1 + 1][2 * f0] -= w * ci;
        pf_rows[2 * f1 + 1][2 * f0 + 1] -= w * cr;
    }

    double total_bnd_len = 0.0;
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        if (he_twin[h_id] == -1) {
            total_bnd_len += verts[tris[h_id / 3][h_id % 3]].distance(
                verts[tris[h_id / 3][(h_id % 3 + 1) % 3]]
            );
        }
    }
    total_bnd_len = std::max(1e-9, total_bnd_len);

    double constrained_count = 0.0;
    for (size_t f = 0; f < nf; f++) {
        bool has_bnd_align = false;
        if (align_boundary) {
            std::complex<double> bnd_z_sum(0.0, 0.0);
            double bnd_len_f = 0.0;
            for (size_t k = 0; k < 3; k++) {
                const size_t h_id = 3 * f + k;
                if (he_twin[h_id] == -1 && he_bnd_dir[h_id].magnitude_squared() > 0.5) {
                    const double elen =
                        verts[tris[f][k]].distance(verts[tris[f][(k + 1) % 3]]);
                    Vector td = he_bnd_dir[h_id] - fnrm[f] * he_bnd_dir[h_id].dot(fnrm[f]);
                    if (td.normalize_self()) {
                        td = rotate_around_normal(td, fnrm[f], rot_rad).normalized();
                        const std::complex<double> u_c(td.dot(fbx[f]), td.dot(fby[f]));
                        const std::complex<double> u2 = u_c * u_c;
                        bnd_z_sum += -(u2 * u2) * elen;
                        bnd_len_f += elen;
                    }
                }
            }
            if (bnd_len_f > 1e-9 && std::abs(bnd_z_sum) > 1e-6) {
                const std::complex<double> z_tgt = bnd_z_sum / std::abs(bnd_z_sum);
                const Point fc(
                    (verts[tris[f][0]][0] + verts[tris[f][1]][0] + verts[tris[f][2]][0]) / 3.0,
                    (verts[tris[f][0]][1] + verts[tris[f][1]][1] + verts[tris[f][2]][1]) / 3.0,
                    (verts[tris[f][0]][2] + verts[tris[f][1]][2] + verts[tris[f][2]][2]) / 3.0
                );
                double w_b = 55.0 * (bnd_len_f / total_bnd_len);
                for (const Point& sc : sharp_corner_pts) {
                    const double d = fc.distance(sc);
                    if (d < 0.45 * h) {
                        w_b *= 0.15 + 0.85 * (d / (0.45 * h));
                    }
                }
                pf_rows[2 * f][2 * f] += w_b;
                pf_rows[2 * f + 1][2 * f + 1] += w_b;
                pf_rhs[2 * f] += w_b * z_tgt.real();
                pf_rhs[2 * f + 1] += w_b * z_tgt.imag();
                has_bnd_align = true;
                constrained_count += 1.0;
            }
        }

        if (!has_bnd_align) {
            std::complex<double> z_curv(0.0, 0.0);
            double a_sum = 0.0;
            for (size_t k = 0; k < 3; k++) {
                const size_t vk = tris[f][k];
                if (v_aniso[vk] > 1e-4) {
                    Vector pd = v_pdir[vk] - fnrm[f] * v_pdir[vk].dot(fnrm[f]);
                    if (pd.normalize_self()) {
                        const std::complex<double> u_c(pd.dot(fbx[f]), pd.dot(fby[f]));
                        const std::complex<double> u2 = u_c * u_c;
                        z_curv += -(u2 * u2) * v_aniso[vk];
                        a_sum += v_aniso[vk];
                    }
                }
            }
            if (a_sum > 1e-4 && std::abs(z_curv) > 1e-6) {
                const std::complex<double> z_tgt = z_curv / std::abs(z_curv);
                const double w_c = 0.35 * (a_sum / 3.0) * (farea[f] / total_area);
                pf_rows[2 * f][2 * f] += w_c;
                pf_rows[2 * f + 1][2 * f + 1] += w_c;
                pf_rhs[2 * f] += w_c * z_tgt.real();
                pf_rhs[2 * f + 1] += w_c * z_tgt.imag();
                constrained_count += 1.0;
            }
        }
    }

    if (constrained_count < 0.5) {
        pf_rows[0][0] += 1.0;
        pf_rows[1][1] += 1.0;
        pf_rhs[0] += -1.0;
    } else {
        for (size_t i = 0; i < 2 * nf; i++) {
            pf_rows[i][i] += 1e-12;
        }
    }

    const CsrMatrix pf_csr = CsrMatrix::from_maps(pf_rows);
    std::vector<double> pf_sol(2 * nf, 0.0);
    if (constrained_count < 0.5) {
        pf_sol[0] = -1.0;
    }
    solve_pcg_scalar(pf_csr, pf_rhs, pf_sol, 5000, 1e-13);

    // power_to_raw (power_to_raw.h): 4 CCW unit vectors per face
    std::vector<std::array<std::complex<double>, 4>> raw_int(nf);
    for (size_t f = 0; f < nf; f++) {
        std::complex<double> neg_z(-pf_sol[2 * f], -pf_sol[2 * f + 1]);
        if (std::abs(neg_z) <= 1e-14) {
            neg_z = std::complex<double>(1.0, 0.0);
        } else {
            neg_z /= std::abs(neg_z);
        }
        const double ang0 = 0.25 * std::arg(neg_z);
        for (int j = 0; j < 4; j++) {
            const double ang = ang0 + 0.5 * Tolerance::PI * static_cast<double>(j);
            raw_int[f][static_cast<size_t>(j)] = std::complex<double>(std::cos(ang), std::sin(ang));
        }
    }

    // principal_matching (principal_matching.h lines 43-79)
    std::vector<int> raw_matching(ne, -1);
    std::vector<double> raw_effort(ne, 0.0);
    for (size_t e = 0; e < ne; e++) {
        if (ef_list[e][0] < 0 || ef_list[e][1] < 0) {
            continue;
        }
        const size_t f0 = static_cast<size_t>(ef_list[e][0]);
        const size_t f1 = static_cast<size_t>(ef_list[e][1]);
        double min_rot = 1e9;
        int idx_min = 0;
        std::complex<double> free_coeff(1.0, 0.0);
        const std::complex<double> trans0 = raw_int[f0][0] * conn[e];
        for (int j = 0; j < 4; j++) {
            const std::complex<double> vjf = raw_int[f0][static_cast<size_t>(j)];
            const std::complex<double> vjg = raw_int[f1][static_cast<size_t>(j)];
            free_coeff *= (vjg / (vjf * conn[e]));
            const double cur_rot = std::arg(vjg / trans0);
            if (std::abs(cur_rot) < std::abs(min_rot)) {
                min_rot = cur_rot;
                idx_min = j;
            }
        }
        raw_effort[e] = std::arg(free_coeff);
        double cur_effort = 0.0;
        for (int j = 0; j < 4; j++) {
            const std::complex<double> vjf = raw_int[f0][static_cast<size_t>(j)];
            const std::complex<double> vjg = raw_int[f1][static_cast<size_t>((j + idx_min + 4) % 4)];
            cur_effort += std::arg(vjg / (vjf * conn[e]));
        }
        const int m_adj =
            idx_min - static_cast<int>(std::llround((cur_effort - raw_effort[e]) / Tolerance::TWO_PI));
        raw_matching[e] = ((m_adj % 4) + 4) % 4;
    }

    // effort_to_indices (effort_to_indices.h + dual_cycles.h): exact singularity indices on interior vertices
    std::vector<int> sing_indices(nv, 0);
    std::vector<size_t> sing_verts;
    for (size_t v = 0; v < nv; v++) {
        if (is_bnd_v[v] || vh[v] < 0) {
            continue;
        }
        double angle_sum = 0.0;
        double effort_sum = 0.0;
        int cur = vh[v];
        const int start = cur;
        for (int step = 0; step < 256; step++) {
            const size_t f = static_cast<size_t>(cur / 3);
            const size_t k = static_cast<size_t>(cur % 3);
            const Vector e_out = (verts[tris[f][(k + 1) % 3]] - verts[v]).normalized();
            const Vector e_in_rev = (verts[tris[f][(k + 2) % 3]] - verts[v]).normalized();
            angle_sum += std::acos(std::clamp(e_out.dot(e_in_rev), -1.0, 1.0));

            const int prev_h = static_cast<int>(3 * f + (k + 2) % 3);
            const int next_h = he_twin[static_cast<size_t>(prev_h)];
            if (next_h < 0) {
                break;
            }
            const size_t eid = static_cast<size_t>(he_to_edge[static_cast<size_t>(prev_h)]);
            const double sgn = (ef_list[eid][0] == static_cast<int>(f)) ? 1.0 : -1.0;
            effort_sum += sgn * raw_effort[eid];
            cur = next_h;
            if (cur == start) {
                break;
            }
        }
        const double defect = Tolerance::TWO_PI - angle_sum;
        const int idx4 = static_cast<int>(std::llround((effort_sum + 4.0 * defect) / Tolerance::TWO_PI));
        if (idx4 != 0) {
            sing_indices[v] = idx4;
            sing_verts.push_back(v);
        }
    }

    // 4. cut_mesh_with_singularities (cut_mesh_with_singularities.h) + combing (combing.h)
    std::vector<Point> f_bary(nf);
    for (size_t f = 0; f < nf; f++) {
        f_bary[f] = Point(
            (verts[tris[f][0]][0] + verts[tris[f][1]][0] + verts[tris[f][2]][0]) / 3.0,
            (verts[tris[f][0]][1] + verts[tris[f][1]][1] + verts[tris[f][2]][1]) / 3.0,
            (verts[tris[f][0]][2] + verts[tris[f][1]][2] + verts[tris[f][2]][2]) / 3.0
        );
    }

    std::vector<int> is_he_cut(nhe, 1);
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        if (he_twin[h_id] == -1) {
            is_he_cut[h_id] = 0;
        }
    }
    {
        std::vector<bool> f_vis(nf, false);
        using DualQItem = std::tuple<double, size_t, int>;
        std::priority_queue<DualQItem, std::vector<DualQItem>, std::greater<DualQItem>> pq;
        pq.emplace(0.0, 0, -1);
        while (!pq.empty()) {
            const DualQItem top = pq.top();
            pq.pop();
            const double d_cur = std::get<0>(top);
            const size_t f_cur = std::get<1>(top);
            const int prev_he = std::get<2>(top);
            if (f_vis[f_cur]) {
                continue;
            }
            f_vis[f_cur] = true;
            if (prev_he >= 0) {
                is_he_cut[static_cast<size_t>(prev_he)] = 0;
                is_he_cut[static_cast<size_t>(he_twin[static_cast<size_t>(prev_he)])] = 0;
            }
            for (size_t k = 0; k < 3; k++) {
                const int cur_he = static_cast<int>(3 * f_cur + k);
                const int tw = he_twin[static_cast<size_t>(cur_he)];
                if (tw < 0) {
                    continue;
                }
                const size_t fn = static_cast<size_t>(tw / 3);
                if (!f_vis[fn]) {
                    pq.emplace(d_cur + f_bary[f_cur].distance(f_bary[fn]), fn, cur_he);
                }
            }
        }
        for (size_t f_seed = 0; f_seed < nf; f_seed++) {
            if (f_vis[f_seed]) {
                continue;
            }
            pq.emplace(0.0, f_seed, -1);
            while (!pq.empty()) {
                const DualQItem top = pq.top();
                pq.pop();
                const size_t fc = std::get<1>(top);
                const int ph = std::get<2>(top);
                if (f_vis[fc]) {
                    continue;
                }
                f_vis[fc] = true;
                if (ph >= 0) {
                    is_he_cut[static_cast<size_t>(ph)] = 0;
                    is_he_cut[static_cast<size_t>(he_twin[static_cast<size_t>(ph)])] = 0;
                }
                for (size_t k = 0; k < 3; k++) {
                    const int ch = static_cast<int>(3 * fc + k);
                    const int tw = he_twin[static_cast<size_t>(ch)];
                    if (tw >= 0 && !f_vis[static_cast<size_t>(tw / 3)]) {
                        pq.emplace(std::get<0>(top) + 1.0, static_cast<size_t>(tw / 3), ch);
                    }
                }
            }
        }
    }

    // Retract valence-1 non-singular non-boundary cut leaves
    std::vector<int> cut_valences(nv, 0);
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        if (is_he_cut[h_id] || he_twin[h_id] == -1) {
            cut_valences[tris[h_id / 3][h_id % 3]]++;
        }
    }
    std::queue<int> cut_q;
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        const size_t u = tris[h_id / 3][h_id % 3];
        if (is_he_cut[h_id] && cut_valences[u] == 1 && sing_indices[u] == 0 && !is_bnd_v[u]) {
            cut_q.push(static_cast<int>(h_id));
        }
    }
    while (!cut_q.empty()) {
        const int ch = cut_q.front();
        cut_q.pop();
        if (!is_he_cut[static_cast<size_t>(ch)]) {
            continue;
        }
        const size_t u = tris[static_cast<size_t>(ch) / 3][static_cast<size_t>(ch) % 3];
        if (cut_valences[u] != 1 || sing_indices[u] != 0 || is_bnd_v[u]) {
            continue;
        }
        const int tw = he_twin[static_cast<size_t>(ch)];
        is_he_cut[static_cast<size_t>(ch)] = 0;
        if (tw >= 0) {
            is_he_cut[static_cast<size_t>(tw)] = 0;
        }
        const size_t nxt_v = tris[static_cast<size_t>(ch) / 3][(static_cast<size_t>(ch) % 3 + 1) % 3];
        cut_valences[u]--;
        if (is_bnd_v[nxt_v]) {
            continue;
        }
        cut_valences[nxt_v]--;
        if (cut_valences[nxt_v] == 1 && sing_indices[nxt_v] == 0) {
            int hb = vh[nxt_v];
            int hc = hb;
            do {
                if (is_he_cut[static_cast<size_t>(hc)]) {
                    cut_q.push(hc);
                    break;
                }
                const int prev_h = 3 * (hc / 3) + ((hc % 3) + 2) % 3;
                hc = he_twin[static_cast<size_t>(prev_h)];
            } while (hc >= 0 && hc != hb);
        }
    }

    // Combing (combing.h lines 47-89)
    std::vector<int> space_turns(nf, 0);
    std::vector<bool> comb_vis(nf, false);
    std::queue<std::pair<size_t, int>> comb_q;
    for (size_t f_seed = 0; f_seed < nf; f_seed++) {
        if (comb_vis[f_seed]) {
            continue;
        }
        comb_q.emplace(f_seed, 0);
        while (!comb_q.empty()) {
            const std::pair<size_t, int> cur = comb_q.front();
            comb_q.pop();
            const size_t f = cur.first;
            const int turn = cur.second;
            if (comb_vis[f]) {
                continue;
            }
            comb_vis[f] = true;
            space_turns[f] = turn;
            for (size_t k = 0; k < 3; k++) {
                const size_t h_id = 3 * f + k;
                if (is_he_cut[h_id] || he_twin[h_id] < 0) {
                    continue;
                }
                const size_t eid = fe_table[f][k];
                const size_t fn = static_cast<size_t>(
                    (ef_list[eid][0] == static_cast<int>(f)) ? ef_list[eid][1] : ef_list[eid][0]
                );
                if (comb_vis[fn]) {
                    continue;
                }
                int dm = raw_matching[eid];
                if (ef_list[eid][0] != static_cast<int>(f)) {
                    dm = -dm;
                }
                const int next_turn = ((dm + turn) % 4 + 4) % 4;
                comb_q.emplace(fn, next_turn);
            }
        }
    }

    std::vector<std::array<Vector, 4>> combed_ext(nf);
    for (size_t f = 0; f < nf; f++) {
        const int st = space_turns[f];
        for (int j = 0; j < 4; j++) {
            const std::complex<double> cj = raw_int[f][static_cast<size_t>((j + st) % 4)];
            combed_ext[f][static_cast<size_t>(j)] =
                (fbx[f] * cj.real() + fby[f] * cj.imag()).normalized();
        }
    }
    std::vector<int> combed_matching(ne, -1);
    for (size_t e = 0; e < ne; e++) {
        if (ef_list[e][0] < 0 || ef_list[e][1] < 0) {
            continue;
        }
        const size_t f0 = static_cast<size_t>(ef_list[e][0]);
        const size_t f1 = static_cast<size_t>(ef_list[e][1]);
        combed_matching[e] = ((space_turns[f0] - space_turns[f1] + raw_matching[e]) % 4 + 4) % 4;
    }

    // 5. setup_integration (setup_integration.h lines 193-490) in exact 2x2 reduced form (n = 2)
    std::vector<int> seam_valence(nv, 0);
    for (size_t e = 0; e < ne; e++) {
        if (eh_list[e][0] >= 0 && is_he_cut[static_cast<size_t>(eh_list[e][0])]) {
            seam_valence[ev_list[e].first]++;
            seam_valence[ev_list[e].second]++;
        }
    }

    std::vector<int> he_to_matching(nhe, 0);
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        const size_t eid = static_cast<size_t>(he_to_edge[h_id]);
        if (combed_matching[eid] < 0) {
            continue;
        }
        const int m = (eh_list[eid][0] == static_cast<int>(h_id)) ? -combed_matching[eid]
                                                                  : combed_matching[eid];
        he_to_matching[h_id] = ((m % 4) + 4) % 4;
    }

    std::vector<size_t> cut2whole;
    std::vector<Point> cut_verts;
    std::vector<std::array<size_t, 3>> cut_tris(nf, {0, 0, 0});

    for (size_t i = 0; i < nv; i++) {
        int begin_h = vh[i];
        if (begin_h < 0) {
            continue;
        }
        int cur_h = begin_h;
        if (!is_bnd_v[i]) {
            do {
                if (is_he_cut[static_cast<size_t>(cur_h)] != 0) {
                    break;
                }
                const int tw = he_twin[static_cast<size_t>(cur_h)];
                cur_h = 3 * (tw / 3) + ((tw % 3) + 1) % 3;
            } while (cur_h != begin_h);
        }
        begin_h = cur_h;
        do {
            if (is_he_cut[static_cast<size_t>(cur_h)] != 0 || cur_h == begin_h) {
                cut2whole.push_back(i);
                cut_verts.push_back(verts[i]);
            }
            const size_t cf = static_cast<size_t>(cur_h / 3);
            for (size_t j = 0; j < 3; j++) {
                if (tris[cf][j] == i) {
                    cut_tris[cf][j] = cut2whole.size() - 1;
                }
            }
            const int prev_h = 3 * (cur_h / 3) + ((cur_h % 3) + 2) % 3;
            cur_h = he_twin[static_cast<size_t>(prev_h)];
        } while (cur_h >= 0 && cur_h != begin_h);
    }
    const size_t ncv = cut_verts.size();

    // Trace maximal seam curves to assign transition variables 1..num_transitions
    std::vector<int> he_to_trans(nhe, 0);
    std::vector<bool> he_claimed(nhe, false);
    int cur_trans = 1;

    for (size_t i = 0; i < nv; i++) {
        if ((seam_valence[i] == 2 && sing_indices[i] == 0 && !is_bnd_v[i]) || seam_valence[i] == 0) {
            continue;
        }
        int begin_h = vh[i];
        if (begin_h < 0) {
            continue;
        }
        int cur_h = begin_h;
        do {
            if (is_he_cut[static_cast<size_t>(cur_h)] != 0 &&
                !he_claimed[static_cast<size_t>(cur_h)] &&
                he_twin[static_cast<size_t>(cur_h)] != -1) {
                int step_h = cur_h;
                const int step_tw = he_twin[static_cast<size_t>(step_h)];
                he_to_trans[static_cast<size_t>(step_h)] = cur_trans;
                he_to_trans[static_cast<size_t>(step_tw)] = -cur_trans;
                he_claimed[static_cast<size_t>(step_h)] = true;
                he_claimed[static_cast<size_t>(step_tw)] = true;

                size_t nxt_v = tris[static_cast<size_t>(step_h) / 3]
                                   [(static_cast<size_t>(step_h) % 3 + 1) % 3];
                while (seam_valence[nxt_v] == 2 && sing_indices[nxt_v] == 0 && !is_bnd_v[nxt_v]) {
                    const int hb = vh[nxt_v];
                    int hc = hb;
                    int found_h = -1;
                    do {
                        if (is_he_cut[static_cast<size_t>(hc)] != 0 &&
                            !he_claimed[static_cast<size_t>(hc)]) {
                            found_h = hc;
                            break;
                        }
                        const int ph = 3 * (hc / 3) + ((hc % 3) + 2) % 3;
                        hc = he_twin[static_cast<size_t>(ph)];
                    } while (hc >= 0 && hc != hb);
                    if (found_h < 0) {
                        break;
                    }
                    const int ftw = he_twin[static_cast<size_t>(found_h)];
                    he_to_trans[static_cast<size_t>(found_h)] = cur_trans;
                    he_to_trans[static_cast<size_t>(ftw)] = -cur_trans;
                    he_claimed[static_cast<size_t>(found_h)] = true;
                    he_claimed[static_cast<size_t>(ftw)] = true;
                    nxt_v = tris[static_cast<size_t>(found_h) / 3]
                                [(static_cast<size_t>(found_h) % 3 + 1) % 3];
                }
                cur_trans++;
            }
            const int prev_h = 3 * (cur_h / 3) + ((cur_h % 3) + 2) % 3;
            cur_h = he_twin[static_cast<size_t>(prev_h)];
        } while (cur_h >= 0 && cur_h != begin_h);
    }
    for (size_t h_id = 0; h_id < nhe; h_id++) {
        if (is_he_cut[h_id] != 0 && !he_claimed[h_id] && he_twin[h_id] != -1) {
            int step_h = static_cast<int>(h_id);
            while (step_h >= 0 && !he_claimed[static_cast<size_t>(step_h)]) {
                const int stw = he_twin[static_cast<size_t>(step_h)];
                he_to_trans[static_cast<size_t>(step_h)] = cur_trans;
                he_to_trans[static_cast<size_t>(stw)] = -cur_trans;
                he_claimed[static_cast<size_t>(step_h)] = true;
                he_claimed[static_cast<size_t>(stw)] = true;
                const size_t nxt_v = tris[static_cast<size_t>(step_h) / 3]
                                         [(static_cast<size_t>(step_h) % 3 + 1) % 3];
                const int hb = vh[nxt_v];
                int hc = hb;
                int next_found = -1;
                do {
                    if (is_he_cut[static_cast<size_t>(hc)] != 0 && !he_claimed[static_cast<size_t>(hc)]) {
                        next_found = hc;
                        break;
                    }
                    const int ph = 3 * (hc / 3) + ((hc % 3) + 2) % 3;
                    hc = he_twin[static_cast<size_t>(ph)];
                } while (hc >= 0 && hc != hb);
                step_h = next_found;
            }
            cur_trans++;
        }
    }

    const size_t num_trans = static_cast<size_t>(cur_trans - 1);
    const size_t num_vars = 2 * (nv + num_trans);

    // Build exact integer map v2cut_int[2 * cv] and interior holonomy constraints
    std::vector<std::map<size_t, int>> v2cut_int(2 * ncv);
    std::vector<SparseLinearRow> base_eqs;

    for (size_t i = 0; i < nv; i++) {
        int begin_h = vh[i];
        if (begin_h < 0) {
            continue;
        }
        int cur_h = begin_h;
        if (!is_bnd_v[i]) {
            do {
                if (is_he_cut[static_cast<size_t>(cur_h)] != 0) {
                    break;
                }
                const int tw = he_twin[static_cast<size_t>(cur_h)];
                cur_h = 3 * (tw / 3) + ((tw % 3) + 1) % 3;
            } while (cur_h != begin_h);
        }
        begin_h = cur_h;

        std::vector<IntMat2> p_mats = {{1, 0, 0, 1}};
        std::vector<size_t> p_idx = {i};
        int last_cv = -1;

        do {
            const size_t cf = static_cast<size_t>(cur_h / 3);
            int new_cv = -1;
            for (size_t j = 0; j < 3; j++) {
                if (tris[cf][j] == i) {
                    new_cv = static_cast<int>(cut_tris[cf][j]);
                }
            }
            if (new_cv >= 0 && new_cv != last_cv) {
                last_cv = new_cv;
                const size_t u_row = 2 * static_cast<size_t>(last_cv);
                const size_t v_row = u_row + 1;
                v2cut_int[u_row].clear();
                v2cut_int[v_row].clear();
                for (size_t m = 0; m < p_idx.size(); m++) {
                    const size_t base_col = 2 * p_idx[m];
                    v2cut_int[u_row][base_col] += p_mats[m].a00;
                    v2cut_int[u_row][base_col + 1] += p_mats[m].a01;
                    v2cut_int[v_row][base_col] += p_mats[m].a10;
                    v2cut_int[v_row][base_col + 1] += p_mats[m].a11;
                }
            }

            const int prev_h = 3 * (cur_h / 3) + ((cur_h % 3) + 2) % 3;
            const int next_h = he_twin[static_cast<size_t>(prev_h)];
            if (next_h < 0) {
                cur_h = -1;
                continue;
            }
            if (is_he_cut[static_cast<size_t>(next_h)] == 0) {
                cur_h = next_h;
                continue;
            }

            const IntMat2 step_r = rot90_int_mat(he_to_matching[static_cast<size_t>(next_h)]);
            const int tr = he_to_trans[static_cast<size_t>(next_h)];
            if (tr > 0) {
                for (size_t m = 0; m < p_mats.size(); m++) {
                    p_mats[m] = mul_int_mat2(step_r, p_mats[m]);
                }
                p_mats.push_back({1, 0, 0, 1});
                p_idx.push_back(nv + static_cast<size_t>(tr - 1));
            } else if (tr < 0) {
                p_mats.push_back({-1, 0, 0, -1});
                p_idx.push_back(nv + static_cast<size_t>(-tr - 1));
                for (size_t m = 0; m < p_mats.size(); m++) {
                    p_mats[m] = mul_int_mat2(step_r, p_mats[m]);
                }
            }
            cur_h = next_h;
        } while (cur_h >= 0 && cur_h != begin_h);

        if (!is_bnd_v[i]) {
            std::map<size_t, std::array<int, 4>> clean;
            for (size_t m = 0; m < p_idx.size(); m++) {
                std::array<int, 4>& cm = clean[p_idx[m]];
                cm[0] += p_mats[m].a00;
                cm[1] += p_mats[m].a01;
                cm[2] += p_mats[m].a10;
                cm[3] += p_mats[m].a11;
            }
            clean[i][0] -= 1;
            clean[i][3] -= 1;

            SparseLinearRow r0;
            SparseLinearRow r1;
            for (const std::pair<const size_t, std::array<int, 4>>& kv : clean) {
                const size_t col = 2 * kv.first;
                if (kv.second[0] != 0) {
                    r0.coeffs[col] = static_cast<double>(kv.second[0]);
                }
                if (kv.second[1] != 0) {
                    r0.coeffs[col + 1] = static_cast<double>(kv.second[1]);
                }
                if (kv.second[2] != 0) {
                    r1.coeffs[col] = static_cast<double>(kv.second[2]);
                }
                if (kv.second[3] != 0) {
                    r1.coeffs[col + 1] = static_cast<double>(kv.second[3]);
                }
            }
            if (!r0.coeffs.empty()) {
                base_eqs.push_back(std::move(r0));
            }
            if (!r1.coeffs.empty()) {
                base_eqs.push_back(std::move(r1));
            }
        }
    }

    for (size_t r = 0; r < 2 * ncv; r++) {
        for (std::map<size_t, int>::iterator it = v2cut_int[r].begin(); it != v2cut_int[r].end();) {
            if (it->second == 0) {
                it = v2cut_int[r].erase(it);
            } else {
                ++it;
            }
        }
    }

    // 6. Singularity & Seam-Jump Integer Targets (Directional::integrate roundSeams = false)
    struct IntegerTarget {
        std::map<size_t, double> form;
        int phase = 0;
    };
    std::vector<IntegerTarget> int_targets;

    for (const size_t sv : sing_verts) {
        IntegerTarget t0;
        t0.form[2 * sv] = 1.0;
        t0.phase = 0;
        int_targets.push_back(std::move(t0));

        IntegerTarget t1;
        t1.form[2 * sv + 1] = 1.0;
        t1.phase = 0;
        int_targets.push_back(std::move(t1));
    }

    for (size_t t = 0; t < num_trans; t++) {
        IntegerTarget j0;
        j0.form[2 * (nv + t)] = 1.0;
        j0.phase = 1;
        int_targets.push_back(std::move(j0));

        IntegerTarget j1;
        j1.form[2 * (nv + t) + 1] = 1.0;
        j1.phase = 1;
        int_targets.push_back(std::move(j1));
    }

    size_t gauge_v = 0;
    if (!sing_verts.empty()) {
        gauge_v = sing_verts[0];
    } else {
        for (size_t v = 0; v < nv; v++) {
            if (is_sharp_v[v]) {
                gauge_v = v;
                break;
            }
        }
    }
    {
        SparseLinearRow g0;
        g0.coeffs[2 * gauge_v] = 1.0;
        g0.rhs = 0.0;
        base_eqs.push_back(std::move(g0));

        SparseLinearRow g1;
        g1.coeffs[2 * gauge_v + 1] = 1.0;
        g1.rhs = 0.0;
        base_eqs.push_back(std::move(g1));
    }

    // 7. Assemble exact FEM triangle-gradient Poisson energy + soft boundary isoline co-alignment
    std::vector<std::map<size_t, double>> ete_rows(num_vars);
    std::vector<double> ete_rhs(num_vars, 0.0);

    for (size_t f = 0; f < nf; f++) {
        const Point& p0 = cut_verts[cut_tris[f][0]];
        const Point& p1 = cut_verts[cut_tris[f][1]];
        const Point& p2 = cut_verts[cut_tris[f][2]];
        const double inv_2a = 1.0 / (2.0 * farea[f]);
        const Vector b_grad[3] = {
            fnrm[f].cross(p1 - p2) * inv_2a,
            fnrm[f].cross(p2 - p0) * inv_2a,
            fnrm[f].cross(p0 - p1) * inv_2a
        };
        const double w_f = farea[f];

        for (size_t axis = 0; axis < 2; axis++) {
            const Vector target_g = combed_ext[f][axis] * (1.045 / h);
            std::map<size_t, Vector> grad_coeffs;
            for (size_t c = 0; c < 3; c++) {
                const size_t cut_row = 2 * cut_tris[f][c] + axis;
                for (const std::pair<const size_t, int>& kv : v2cut_int[cut_row]) {
                    grad_coeffs[kv.first] =
                        grad_coeffs[kv.first] + b_grad[c] * static_cast<double>(kv.second);
                }
            }
            for (const std::pair<const size_t, Vector>& ka : grad_coeffs) {
                ete_rhs[ka.first] += w_f * ka.second.dot(target_g);
                for (const std::pair<const size_t, Vector>& kb : grad_coeffs) {
                    const double val = w_f * ka.second.dot(kb.second);
                    if (std::abs(val) > 1e-18) {
                        ete_rows[ka.first][kb.first] += val;
                    }
                }
            }
        }

        // Soft tangential isoline co-alignment along aligned boundary edges (prevents sawteeth without collapsing piers)
        if (align_boundary) {
            for (size_t k = 0; k < 3; k++) {
                const size_t h_id = 3 * f + k;
                if (he_twin[h_id] != -1) {
                    continue;
                }
                const size_t cv_u = cut_tris[f][k];
                const size_t cv_w = cut_tris[f][(k + 1) % 3];
                if (is_sharp_v[cut2whole[cv_u]] || is_sharp_v[cut2whole[cv_w]]) {
                    continue;
                }
                Vector bdir = cut_verts[cv_w] - cut_verts[cv_u];
                const double blen = bdir.magnitude();
                if (blen <= 1e-9) {
                    continue;
                }
                bdir = bdir * (1.0 / blen);
                for (size_t axis = 0; axis < 2; axis++) {
                    const double align_dot = std::abs(combed_ext[f][axis].dot(bdir));
                    if (align_dot < 0.24) {
                        std::map<size_t, double> diff_form;
                        for (const std::pair<const size_t, int>& kv : v2cut_int[2 * cv_w + axis]) {
                            diff_form[kv.first] += static_cast<double>(kv.second);
                        }
                        for (const std::pair<const size_t, int>& kv : v2cut_int[2 * cv_u + axis]) {
                            diff_form[kv.first] -= static_cast<double>(kv.second);
                        }
                        const double w_b_iso = 0.55 * (h / blen);
                        for (const std::pair<const size_t, double>& ka : diff_form) {
                            for (const std::pair<const size_t, double>& kb : diff_form) {
                                ete_rows[ka.first][kb.first] += w_b_iso * ka.second * kb.second;
                            }
                        }
                    }
                }
            }
        }
    }

    // Iterative greedy MIQ rounding with warm-started PCG (Directional::integrate, Phase 0 then Phase 1)
    std::vector<SparseLinearRow> active_eqs = base_eqs;
    std::vector<bool> target_done(int_targets.size(), false);
    std::vector<double> full_x(num_vars, 0.0);

    for (size_t round_step = 0; round_step <= int_targets.size(); round_step++) {
        NullspaceParam ns;
        ns.build(num_vars, active_eqs);

        if (ns.num_free > 0) {
            std::vector<double> ete_x0(num_vars, 0.0);
            for (size_t i = 0; i < num_vars; i++) {
                double s = 0.0;
                for (const std::pair<const size_t, double>& kv : ete_rows[i]) {
                    s += kv.second * ns.x0[kv.first];
                }
                ete_x0[i] = s;
            }

            std::vector<std::map<size_t, double>> red_rows(ns.num_free);
            std::vector<double> red_rhs(ns.num_free, 0.0);
            std::vector<double> y_sol(ns.num_free, 0.0);
            for (size_t i = 0; i < num_vars; i++) {
                if (ns.s_rows[i].size() == 1 && std::abs(ns.s_rows[i][0].second - 1.0) < 1e-12 &&
                    std::abs(ns.x0[i]) < 1e-12) {
                    y_sol[ns.s_rows[i][0].first] = full_x[i];
                }
                const double r_i = ete_rhs[i] - ete_x0[i];
                for (const std::pair<size_t, double>& si : ns.s_rows[i]) {
                    red_rhs[si.first] += si.second * r_i;
                }
                for (const std::pair<const size_t, double>& kv : ete_rows[i]) {
                    const size_t j = kv.first;
                    const double a_ij = kv.second;
                    for (const std::pair<size_t, double>& si : ns.s_rows[i]) {
                        for (const std::pair<size_t, double>& sj : ns.s_rows[j]) {
                            red_rows[si.first][sj.first] += si.second * a_ij * sj.second;
                        }
                    }
                }
            }
            for (size_t f_id = 0; f_id < ns.num_free; f_id++) {
                red_rows[f_id][f_id] += 1e-12;
            }
            const CsrMatrix red_csr = CsrMatrix::from_maps(red_rows);
            const int max_pcg = (round_step == 0 || round_step == int_targets.size()) ? 4000 : 1200;
            solve_pcg_scalar(red_csr, red_rhs, y_sol, max_pcg, 1e-12);
            full_x = ns.expand(y_sol);
        } else {
            full_x = ns.x0;
        }

        int best_t = -1;
        int best_phase = 2;
        double best_frac = 1e30;
        double best_val = 0.0;

        for (size_t ti = 0; ti < int_targets.size(); ti++) {
            if (target_done[ti]) {
                continue;
            }
            const double sens = ns.max_free_sensitivity(int_targets[ti].form);
            if (sens <= 1e-7) {
                target_done[ti] = true;
                continue;
            }
            double val = 0.0;
            for (const std::pair<const size_t, double>& kv : int_targets[ti].form) {
                val += kv.second * full_x[kv.first];
            }
            const double frac = std::abs(val - std::round(val));
            const int ph = int_targets[ti].phase;
            if (ph < best_phase || (ph == best_phase && frac < best_frac)) {
                best_phase = ph;
                best_frac = frac;
                best_t = static_cast<int>(ti);
                best_val = val;
            }
        }

        if (best_t < 0) {
            // Final polish solve when all integer targets are fixed
            if (round_step > 0 && ns.num_free > 0) {
                break;
            }
            break;
        }
        target_done[static_cast<size_t>(best_t)] = true;
        SparseLinearRow fixed_eq;
        fixed_eq.coeffs = int_targets[static_cast<size_t>(best_t)].form;
        fixed_eq.rhs = std::round(best_val);
        active_eqs.push_back(std::move(fixed_eq));
    }

    // Ensure every transition variable is exact integer, solve final Poisson polish, and apply Simulation of Simplicity!
    {
        for (size_t t = 0; t < num_trans; t++) {
            for (size_t c = 0; c < 2; c++) {
                SparseLinearRow teq;
                teq.coeffs[2 * (nv + t) + c] = 1.0;
                teq.rhs = std::round(full_x[2 * (nv + t) + c]);
                active_eqs.push_back(std::move(teq));
            }
        }
        NullspaceParam ns_final;
        ns_final.build(num_vars, active_eqs);
        if (ns_final.num_free > 0) {
            std::vector<double> ete_x0(num_vars, 0.0);
            for (size_t i = 0; i < num_vars; i++) {
                double s = 0.0;
                for (const std::pair<const size_t, double>& kv : ete_rows[i]) {
                    s += kv.second * ns_final.x0[kv.first];
                }
                ete_x0[i] = s;
            }
            std::vector<std::map<size_t, double>> red_rows(ns_final.num_free);
            std::vector<double> red_rhs(ns_final.num_free, 0.0);
            std::vector<double> y_fin(ns_final.num_free, 0.0);
            for (size_t i = 0; i < num_vars; i++) {
                if (ns_final.s_rows[i].size() == 1 &&
                    std::abs(ns_final.s_rows[i][0].second - 1.0) < 1e-12 &&
                    std::abs(ns_final.x0[i]) < 1e-12) {
                    y_fin[ns_final.s_rows[i][0].first] = full_x[i];
                }
                const double r_i = ete_rhs[i] - ete_x0[i];
                for (const std::pair<size_t, double>& si : ns_final.s_rows[i]) {
                    red_rhs[si.first] += si.second * r_i;
                }
                for (const std::pair<const size_t, double>& kv : ete_rows[i]) {
                    const size_t j = kv.first;
                    const double a_ij = kv.second;
                    for (const std::pair<size_t, double>& si : ns_final.s_rows[i]) {
                        for (const std::pair<size_t, double>& sj : ns_final.s_rows[j]) {
                            red_rows[si.first][sj.first] += si.second * a_ij * sj.second;
                        }
                    }
                }
            }
            for (size_t f_id = 0; f_id < ns_final.num_free; f_id++) {
                red_rows[f_id][f_id] += 1e-12;
            }
            const CsrMatrix red_csr = CsrMatrix::from_maps(red_rows);
            solve_pcg_scalar(red_csr, red_rhs, y_fin, 4000, 1e-13);
            full_x = ns_final.expand(y_fin);

            // Simulation of Simplicity (Edelsbrunner & Mucke 1990): nudge free DOFs so no non-singular vertex lands on Z
            for (int sos_pass = 0; sos_pass < 4; sos_pass++) {
                bool nudged = false;
                for (size_t v = 0; v < nv; v++) {
                    if (sing_indices[v] != 0) {
                        continue;
                    }
                    for (size_t c = 0; c < 2; c++) {
                        const size_t idx = 2 * v + c;
                        const double val = full_x[idx];
                        if (std::abs(val - std::round(val)) < 3e-4 && !ns_final.s_rows[idx].empty()) {
                            const size_t fid = ns_final.s_rows[idx][0].first;
                            const double sgn = (ns_final.s_rows[idx][0].second >= 0.0) ? 1.0 : -1.0;
                            const double dir = (val >= std::round(val)) ? 1.0 : -1.0;
                            y_fin[fid] += sgn * dir * 4.5e-4;
                            full_x = ns_final.expand(y_fin);
                            nudged = true;
                        }
                    }
                }
                if (!nudged) {
                    break;
                }
            }
        } else {
            full_x = ns_final.x0;
        }
    }

    std::vector<std::pair<double, double>> cut_uv(ncv, {0.0, 0.0});
    for (size_t cv = 0; cv < ncv; cv++) {
        double u_val = 0.0;
        double v_val = 0.0;
        for (const std::pair<const size_t, int>& kv : v2cut_int[2 * cv]) {
            u_val += static_cast<double>(kv.second) * full_x[kv.first];
        }
        for (const std::pair<const size_t, int>& kv : v2cut_int[2 * cv + 1]) {
            v_val += static_cast<double>(kv.second) * full_x[kv.first];
        }
        if (sing_indices[cut2whole[cv]] != 0) {
            if (std::abs(u_val - std::round(u_val)) < 1e-4) {
                u_val = std::round(u_val);
            }
            if (std::abs(v_val - std::round(v_val)) < 1e-4) {
                v_val = std::round(v_val);
            }
        }
        cut_uv[cv] = {u_val, v_val};
    }

    // Precompute whether any original undirected edge connects two exact-integer singular vertices along the same isoline
    std::vector<bool> edge_is_exact_iso(ne, false);
    for (size_t eid = 0; eid < ne; eid++) {
        const size_t va = ev_list[eid].first;
        const size_t vb = ev_list[eid].second;
        if (sing_indices[va] != 0 && sing_indices[vb] != 0 && ef_list[eid][0] >= 0) {
            const size_t f0 = static_cast<size_t>(ef_list[eid][0]);
            size_t cva = 0;
            size_t cvb = 0;
            for (size_t k = 0; k < 3; k++) {
                if (tris[f0][k] == va) {
                    cva = cut_tris[f0][k];
                }
                if (tris[f0][k] == vb) {
                    cvb = cut_tris[f0][k];
                }
            }
            const bool same_u = (std::abs(cut_uv[cva].first - std::round(cut_uv[cva].first)) < 1e-6) &&
                                (std::abs(cut_uv[cvb].first - std::round(cut_uv[cvb].first)) < 1e-6) &&
                                (std::llround(cut_uv[cva].first) == std::llround(cut_uv[cvb].first));
            const bool same_v = (std::abs(cut_uv[cva].second - std::round(cut_uv[cva].second)) < 1e-6) &&
                                (std::abs(cut_uv[cvb].second - std::round(cut_uv[cvb].second)) < 1e-6) &&
                                (std::llround(cut_uv[cva].second) == std::llround(cut_uv[cvb].second));
            edge_is_exact_iso[eid] = (same_u || same_v);
        }
    }

    // 8. Seam-Exact Canonical-Triangle Isoline Arrangement & Simplification (generate_mesh.h + NFunctionMesher.h)
    std::vector<int> orig_v_to_arr(nv, -1);
    std::vector<std::vector<std::pair<double, size_t>>> edge_crossings(ne);
    std::vector<Point> arr_pts;
    std::vector<bool> arr_is_orig_sharp;
    std::vector<bool> arr_is_bnd;

    std::vector<std::vector<size_t>> sub_polys;
    std::vector<std::vector<bool>> sub_edge_is_iso;

    for (size_t f = 0; f < nf; f++) {
        const size_t cv0 = cut_tris[f][0];
        const size_t cv1 = cut_tris[f][1];
        const size_t cv2 = cut_tris[f][2];
        const double u0 = cut_uv[cv0].first;
        const double u1 = cut_uv[cv1].first;
        const double u2 = cut_uv[cv2].first;
        const double v0 = cut_uv[cv0].second;
        const double v1 = cut_uv[cv1].second;
        const double v2 = cut_uv[cv2].second;

        const double du_da = u1 - u0;
        const double du_db = u2 - u0;
        const double dv_da = v1 - v0;
        const double dv_db = v2 - v0;

        const int u_min = static_cast<int>(std::floor(std::min({u0, u1, u2}) + 1e-9)) + 1;
        const int u_max = static_cast<int>(std::ceil(std::max({u0, u1, u2}) - 1e-9)) - 1;
        const int v_min = static_cast<int>(std::floor(std::min({v0, v1, v2}) + 1e-9)) + 1;
        const int v_max = static_cast<int>(std::ceil(std::max({v0, v1, v2}) - 1e-9)) - 1;

        std::vector<std::vector<BaryVert>> pieces = {{{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}}};

        if (u_max - u_min <= 64) {
            for (int m = u_min; m <= u_max; m++) {
                std::vector<std::vector<BaryVert>> next_p;
                for (const std::vector<BaryVert>& poly : pieces) {
                    std::vector<BaryVert> le;
                    std::vector<BaryVert> ge;
                    split_bary_poly_line(poly, u0, du_da, du_db, static_cast<double>(m), le, ge);
                    if (le.size() >= 3) {
                        next_p.push_back(std::move(le));
                    }
                    if (ge.size() >= 3) {
                        next_p.push_back(std::move(ge));
                    }
                }
                pieces = std::move(next_p);
            }
        }
        if (v_max - v_min <= 64) {
            for (int n = v_min; n <= v_max; n++) {
                std::vector<std::vector<BaryVert>> next_p;
                for (const std::vector<BaryVert>& poly : pieces) {
                    std::vector<BaryVert> le;
                    std::vector<BaryVert> ge;
                    split_bary_poly_line(poly, v0, dv_da, dv_db, static_cast<double>(n), le, ge);
                    if (le.size() >= 3) {
                        next_p.push_back(std::move(le));
                    }
                    if (ge.size() >= 3) {
                        next_p.push_back(std::move(ge));
                    }
                }
                pieces = std::move(next_p);
            }
        }

        const Point& p0 = verts[tris[f][0]];
        const Point& p1 = verts[tris[f][1]];
        const Point& p2 = verts[tris[f][2]];
        std::vector<std::tuple<double, double, size_t>> face_crossings;

        for (const std::vector<BaryVert>& poly : pieces) {
            std::vector<size_t> vids;
            std::vector<BaryVert> bvs;
            for (const BaryVert& bv : poly) {
                const double l1 = bv.a;
                const double l2 = bv.b;
                const double l0 = 1.0 - l1 - l2;

                size_t vid = arr_pts.size();
                if (l0 > 1.0 - 1e-5 || l1 > 1.0 - 1e-5 || l2 > 1.0 - 1e-5) {
                    size_t ov = tris[f][0];
                    if (l1 > l0 && l1 >= l2) {
                        ov = tris[f][1];
                    } else if (l2 > l0 && l2 > l1) {
                        ov = tris[f][2];
                    }
                    if (orig_v_to_arr[ov] >= 0) {
                        vid = static_cast<size_t>(orig_v_to_arr[ov]);
                    } else {
                        vid = arr_pts.size();
                        orig_v_to_arr[ov] = static_cast<int>(vid);
                        arr_pts.push_back(verts[ov]);
                        arr_is_orig_sharp.push_back(is_sharp_v[ov]);
                        arr_is_bnd.push_back(is_bnd_v[ov]);
                    }
                } else if (l2 < 1e-5 || l0 < 1e-5 || l1 < 1e-5) {
                    size_t k_edge = 0;
                    double t_local = l1;
                    if (l0 <= l1 && l0 <= l2) {
                        k_edge = 1;
                        t_local = l2 / std::max(1e-12, l1 + l2);
                    } else if (l1 <= l0 && l1 <= l2) {
                        k_edge = 2;
                        t_local = l0 / std::max(1e-12, l2 + l0);
                    } else {
                        k_edge = 0;
                        t_local = l1 / std::max(1e-12, l0 + l1);
                    }
                    const size_t eid = fe_table[f][k_edge];
                    const size_t va = tris[f][k_edge];
                    const double s_canon =
                        (ev_list[eid].first == va) ? t_local : (1.0 - t_local);
                    const bool on_bnd = (he_twin[3 * f + k_edge] == -1);

                    if (s_canon <= 1e-5) {
                        const size_t ov = ev_list[eid].first;
                        if (orig_v_to_arr[ov] < 0) {
                            orig_v_to_arr[ov] = static_cast<int>(arr_pts.size());
                            arr_pts.push_back(verts[ov]);
                            arr_is_orig_sharp.push_back(is_sharp_v[ov]);
                            arr_is_bnd.push_back(is_bnd_v[ov]);
                        }
                        vid = static_cast<size_t>(orig_v_to_arr[ov]);
                    } else if (s_canon >= 1.0 - 1e-5) {
                        const size_t ov = ev_list[eid].second;
                        if (orig_v_to_arr[ov] < 0) {
                            orig_v_to_arr[ov] = static_cast<int>(arr_pts.size());
                            arr_pts.push_back(verts[ov]);
                            arr_is_orig_sharp.push_back(is_sharp_v[ov]);
                            arr_is_bnd.push_back(is_bnd_v[ov]);
                        }
                        vid = static_cast<size_t>(orig_v_to_arr[ov]);
                    } else {
                        bool found_ec = false;
                        for (const std::pair<double, size_t>& ec : edge_crossings[eid]) {
                            if (std::abs(ec.first - s_canon) < 1e-3) {
                                vid = ec.second;
                                found_ec = true;
                                break;
                            }
                        }
                        if (!found_ec) {
                            vid = arr_pts.size();
                            edge_crossings[eid].emplace_back(s_canon, vid);
                            const Point& ea = verts[ev_list[eid].first];
                            const Point& eb = verts[ev_list[eid].second];
                            arr_pts.emplace_back(
                                (1.0 - s_canon) * ea[0] + s_canon * eb[0],
                                (1.0 - s_canon) * ea[1] + s_canon * eb[1],
                                (1.0 - s_canon) * ea[2] + s_canon * eb[2]
                            );
                            arr_is_orig_sharp.push_back(false);
                            arr_is_bnd.push_back(on_bnd);
                        } else if (on_bnd) {
                            arr_is_bnd[vid] = true;
                        }
                    }
                } else {
                    bool found_fc = false;
                    for (const std::tuple<double, double, size_t>& fc : face_crossings) {
                        if (std::abs(std::get<0>(fc) - l1) + std::abs(std::get<1>(fc) - l2) < 5e-4) {
                            vid = std::get<2>(fc);
                            found_fc = true;
                            break;
                        }
                    }
                    if (!found_fc) {
                        vid = arr_pts.size();
                        face_crossings.emplace_back(l1, l2, vid);
                        arr_pts.emplace_back(
                            l0 * p0[0] + l1 * p1[0] + l2 * p2[0],
                            l0 * p0[1] + l1 * p1[1] + l2 * p2[1],
                            l0 * p0[2] + l1 * p1[2] + l2 * p2[2]
                        );
                        arr_is_orig_sharp.push_back(false);
                        arr_is_bnd.push_back(false);
                    }
                }

                if (vids.empty() || vids.back() != vid) {
                    vids.push_back(vid);
                    bvs.push_back(bv);
                }
            }
            if (vids.size() >= 2 && vids.front() == vids.back()) {
                vids.pop_back();
                bvs.pop_back();
            }
            if (vids.size() < 3) {
                continue;
            }

            // Exact topological isoline classification: internal chords of T are ALWAYS isolines;
            // boundary sub-edges of T dissolve unless the original mesh edge connects two singular integer vertices!
            const size_t pm = vids.size();
            std::vector<bool> e_iso(pm, false);
            for (size_t k = 0; k < pm; k++) {
                const BaryVert& ba = bvs[k];
                const BaryVert& bb = bvs[(k + 1) % pm];
                const double ml1 = 0.5 * (ba.a + bb.a);
                const double ml2 = 0.5 * (ba.b + bb.b);
                const double ml0 = 1.0 - ml1 - ml2;
                if (ml0 > 1e-5 && ml1 > 1e-5 && ml2 > 1e-5) {
                    e_iso[k] = true;
                } else {
                    size_t k_edge = 0;
                    if (ml0 <= ml1 && ml0 <= ml2) {
                        k_edge = 1;
                    } else if (ml1 <= ml0 && ml1 <= ml2) {
                        k_edge = 2;
                    }
                    e_iso[k] = edge_is_exact_iso[fe_table[f][k_edge]];
                }
            }
            sub_polys.push_back(std::move(vids));
            sub_edge_is_iso.push_back(std::move(e_iso));
        }
    }

    // Dissolve all non-isoline internal sub-edges shared by two sub-polygons
    const size_t n_sub = sub_polys.size();
    std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>> undir_sub_edges;
    for (size_t sp = 0; sp < n_sub; sp++) {
        const size_t pm = sub_polys[sp].size();
        for (size_t k = 0; k < pm; k++) {
            const std::pair<size_t, size_t> ek =
                ordered_pair(sub_polys[sp][k], sub_polys[sp][(k + 1) % pm]);
            undir_sub_edges[ek].emplace_back(sp, k);
        }
    }

    DisjointSet dsu(n_sub);
    for (const std::pair<const std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>>& kv :
         undir_sub_edges) {
        if (kv.second.size() == 2) {
            const std::pair<size_t, size_t>& a = kv.second[0];
            const std::pair<size_t, size_t>& b = kv.second[1];
            const bool keep_iso = sub_edge_is_iso[a.first][a.second] ||
                                  sub_edge_is_iso[b.first][b.second];
            if (!keep_iso) {
                dsu.unite(a.first, b.first);
            }
        }
    }

    std::map<size_t, std::vector<size_t>> comp_members;
    for (size_t sp = 0; sp < n_sub; sp++) {
        comp_members[dsu.find(sp)].push_back(sp);
    }

    std::vector<std::vector<size_t>> raw_cells;
    for (const std::pair<const size_t, std::vector<size_t>>& kv : comp_members) {
        std::set<std::pair<size_t, size_t>> hes;
        for (const size_t sp : kv.second) {
            const std::vector<size_t>& loop = sub_polys[sp];
            const size_t pm = loop.size();
            for (size_t k = 0; k < pm; k++) {
                const std::pair<size_t, size_t> opp = {loop[(k + 1) % pm], loop[k]};
                if (hes.count(opp)) {
                    hes.erase(opp);
                } else {
                    hes.insert({loop[k], loop[(k + 1) % pm]});
                }
            }
        }
        std::map<size_t, std::vector<size_t>> out_adj;
        for (const std::pair<size_t, size_t>& he : hes) {
            out_adj[he.first].push_back(he.second);
        }
        std::set<std::pair<size_t, size_t>> used_he;
        std::vector<size_t> best_cycle;
        for (const std::pair<size_t, size_t>& start_he : hes) {
            if (used_he.count(start_he)) {
                continue;
            }
            std::vector<size_t> cyc;
            std::pair<size_t, size_t> cur = start_he;
            for (size_t step = 0; step <= hes.size(); step++) {
                used_he.insert(cur);
                cyc.push_back(cur.first);
                const std::vector<size_t>& nbrs = out_adj[cur.second];
                if (nbrs.empty()) {
                    cyc.clear();
                    break;
                }
                size_t nxt_v = nbrs[0];
                for (const size_t cand : nbrs) {
                    if (!used_he.count({cur.second, cand})) {
                        nxt_v = cand;
                        break;
                    }
                }
                cur = {cur.second, nxt_v};
                if (cur == start_he) {
                    break;
                }
            }
            if (cyc.size() > best_cycle.size()) {
                best_cycle = std::move(cyc);
            }
        }
        if (best_cycle.size() >= 3) {
            raw_cells.push_back(std::move(best_cycle));
        }
    }

    // First pass: remove pass-through degree-2 vertices on raw_cells before boundary strip cleanup
    {
        std::vector<std::set<size_t>> v_inc0(arr_pts.size());
        std::vector<std::set<size_t>> v_nbr0(arr_pts.size());
        for (size_t ci = 0; ci < raw_cells.size(); ci++) {
            const std::vector<size_t>& c = raw_cells[ci];
            for (size_t k = 0; k < c.size(); k++) {
                v_inc0[c[k]].insert(ci);
                v_nbr0[c[k]].insert(c[(k + 1) % c.size()]);
                v_nbr0[c[(k + 1) % c.size()]].insert(c[k]);
            }
        }
        std::vector<std::vector<size_t>> simp_cells;
        for (const std::vector<size_t>& c : raw_cells) {
            std::vector<size_t> sc;
            for (const size_t vid : c) {
                const size_t deg = v_nbr0[vid].size();
                const size_t n_inc = v_inc0[vid].size();
                const bool is_pt =
                    (deg == 2 && !arr_is_orig_sharp[vid]) &&
                    ((!arr_is_bnd[vid] && n_inc == 2) || (arr_is_bnd[vid] && n_inc == 1));
                if (!is_pt) {
                    sc.push_back(vid);
                }
            }
            if (sc.size() >= 3) {
                simp_cells.push_back(std::move(sc));
            }
        }
        raw_cells = std::move(simp_cells);
    }

    // Collapse short transverse edges (u_int -> w_bnd, length < 0.38 * h) so thin boundary strips snap cleanly onto
    // the exact boundary curve WITHOUT introducing p5+ step-jump pentagons or valence-3 interior vertices!
    for (int snap_pass = 0; snap_pass < 2; snap_pass++) {
        std::vector<size_t> v_remap(arr_pts.size());
        for (size_t i = 0; i < arr_pts.size(); i++) {
            v_remap[i] = i;
        }
        std::vector<double> best_bnd_dist(arr_pts.size(), 0.31 * h);
        bool any_snapped = false;
        for (const std::vector<size_t>& c : raw_cells) {
            const size_t m = c.size();
            for (size_t k = 0; k < m; k++) {
                size_t u = c[k];
                size_t w = c[(k + 1) % m];
                if (arr_is_bnd[u] == arr_is_bnd[w]) {
                    continue;
                }
                if (arr_is_bnd[u]) {
                    std::swap(u, w);
                }
                const double d = arr_pts[u].distance(arr_pts[w]);
                if (d < best_bnd_dist[u]) {
                    best_bnd_dist[u] = d;
                    v_remap[u] = w;
                    any_snapped = true;
                }
            }
        }
        if (!any_snapped) {
            break;
        }
        std::vector<std::vector<size_t>> next_cells;
        for (const std::vector<size_t>& c : raw_cells) {
            std::vector<size_t> rc;
            for (const size_t vid : c) {
                const size_t mv = v_remap[vid];
                if (rc.empty() || rc.back() != mv) {
                    rc.push_back(mv);
                }
            }
            while (rc.size() >= 2 && rc.front() == rc.back()) {
                rc.pop_back();
            }
            // Strip any hairpin ears (a -> b -> a)
            bool changed = true;
            while (changed && rc.size() >= 3) {
                changed = false;
                for (size_t k = 0; k < rc.size(); k++) {
                    if (rc[k] == rc[(k + 2) % rc.size()]) {
                        rc.erase(rc.begin() + static_cast<std::ptrdiff_t>((k + 1) % rc.size()));
                        if (k < rc.size()) {
                            rc.erase(rc.begin() + static_cast<std::ptrdiff_t>(k));
                        }
                        changed = true;
                        break;
                    }
                }
            }
            // Require distinct vertices and non-zero polygon area
            std::set<size_t> uniq(rc.begin(), rc.end());
            if (rc.size() >= 3 && uniq.size() == rc.size()) {
                Point ctr(0.0, 0.0, 0.0);
                for (const size_t vid : rc) {
                    ctr = ctr + Vector(arr_pts[vid][0], arr_pts[vid][1], arr_pts[vid][2]);
                }
                ctr = Point(
                    ctr[0] / static_cast<double>(rc.size()),
                    ctr[1] / static_cast<double>(rc.size()),
                    ctr[2] / static_cast<double>(rc.size())
                );
                Vector avec(0.0, 0.0, 0.0);
                for (size_t k = 0; k < rc.size(); k++) {
                    avec = avec + (arr_pts[rc[k]] - ctr).cross(arr_pts[rc[(k + 1) % rc.size()]] - ctr);
                }
                if (0.5 * avec.magnitude() > 0.02 * h * h) {
                    next_cells.push_back(std::move(rc));
                }
            }
        }
        raw_cells = std::move(next_cells);
    }

    // Merge 3-vertex boundary transition triangles (b0, b1, u_int) across their shorter interior edge (b1, u_int)
    // into the adjacent boundary cell so b1 simplifies out as degree-2 pass-through and T U Q becomes a pure 4-gon!
    for (int tri_pass = 0; tri_pass < 3; tri_pass++) {
        // First simplify pass-through degree-2 vertices on raw_cells
        {
            std::vector<std::set<size_t>> vi(arr_pts.size());
            std::vector<std::set<size_t>> vn(arr_pts.size());
            for (size_t ci = 0; ci < raw_cells.size(); ci++) {
                const std::vector<size_t>& c = raw_cells[ci];
                for (size_t k = 0; k < c.size(); k++) {
                    vi[c[k]].insert(ci);
                    vn[c[k]].insert(c[(k + 1) % c.size()]);
                    vn[c[(k + 1) % c.size()]].insert(c[k]);
                }
            }
            for (std::vector<size_t>& c : raw_cells) {
                std::vector<size_t> sc;
                for (const size_t vid : c) {
                    const bool is_pt =
                        (vn[vid].size() == 2 && !arr_is_orig_sharp[vid]) &&
                        ((!arr_is_bnd[vid] && vi[vid].size() == 2) ||
                         (arr_is_bnd[vid] && vi[vid].size() == 1));
                    if (!is_pt) {
                        sc.push_back(vid);
                    }
                }
                if (sc.size() >= 3) {
                    c = std::move(sc);
                }
            }
        }

        std::map<std::pair<size_t, size_t>, std::vector<size_t>> edge_cells;
        for (size_t ci = 0; ci < raw_cells.size(); ci++) {
            const std::vector<size_t>& c = raw_cells[ci];
            for (size_t k = 0; k < c.size(); k++) {
                edge_cells[ordered_pair(c[k], c[(k + 1) % c.size()])].push_back(ci);
            }
        }

        std::vector<bool> dead(raw_cells.size(), false);
        bool any_merged = false;
        for (size_t ci = 0; ci < raw_cells.size(); ci++) {
            if (dead[ci] || raw_cells[ci].size() != 3) {
                continue;
            }
            const std::vector<size_t>& c = raw_cells[ci];
            bool has_bnd_edge = false;
            for (size_t k = 0; k < 3; k++) {
                if (edge_cells[ordered_pair(c[k], c[(k + 1) % 3])].size() == 1) {
                    has_bnd_edge = true;
                }
            }
            if (!has_bnd_edge) {
                continue;
            }
            const double tri_area =
                0.5 * (arr_pts[c[1]] - arr_pts[c[0]]).cross(arr_pts[c[2]] - arr_pts[c[0]]).magnitude();
            if (tri_area > 0.34 * h * h) {
                continue;
            }

            // Pick the shortest shared interior edge that has a non-sharp boundary endpoint (so merging turns T U Q into a quad!)
            size_t best_nb = raw_cells.size();
            double best_elen = 1e30;
            for (size_t k = 0; k < 3; k++) {
                const size_t va = c[k];
                const size_t vb = c[(k + 1) % 3];
                const std::pair<size_t, size_t> ek = ordered_pair(va, vb);
                if (edge_cells[ek].size() != 2) {
                    continue;
                }
                const bool has_smooth_bnd_end =
                    (arr_is_bnd[va] && !arr_is_orig_sharp[va]) ||
                    (arr_is_bnd[vb] && !arr_is_orig_sharp[vb]);
                if (!has_smooth_bnd_end) {
                    continue;
                }
                const size_t oc = (edge_cells[ek][0] == ci) ? edge_cells[ek][1] : edge_cells[ek][0];
                if (dead[oc] || raw_cells[oc].size() > 4) {
                    continue;
                }
                const double elen = arr_pts[va].distance(arr_pts[vb]);
                if (elen < best_elen) {
                    best_elen = elen;
                    best_nb = oc;
                }
            }
            if (best_nb >= raw_cells.size()) {
                continue;
            }

            std::set<std::pair<size_t, size_t>> m_hes;
            for (const size_t idx_c : {ci, best_nb}) {
                const std::vector<size_t>& lc = raw_cells[idx_c];
                for (size_t k = 0; k < lc.size(); k++) {
                    const std::pair<size_t, size_t> opp = {lc[(k + 1) % lc.size()], lc[k]};
                    if (m_hes.count(opp)) {
                        m_hes.erase(opp);
                    } else {
                        m_hes.insert({lc[k], lc[(k + 1) % lc.size()]});
                    }
                }
            }
            std::map<size_t, size_t> nxt;
            for (const std::pair<size_t, size_t>& he : m_hes) {
                nxt[he.first] = he.second;
            }
            if (nxt.size() >= 3) {
                std::vector<size_t> ring;
                size_t cur = nxt.begin()->first;
                for (size_t s = 0; s < nxt.size(); s++) {
                    ring.push_back(cur);
                    const std::map<size_t, size_t>::const_iterator it = nxt.find(cur);
                    if (it == nxt.end()) {
                        ring.clear();
                        break;
                    }
                    cur = it->second;
                    if (cur == ring.front()) {
                        break;
                    }
                }
                if (ring.size() == nxt.size()) {
                    raw_cells[best_nb] = std::move(ring);
                    dead[ci] = true;
                    any_merged = true;
                }
            }
        }
        if (!any_merged) {
            break;
        }
        std::vector<std::vector<size_t>> kept;
        for (size_t ci = 0; ci < raw_cells.size(); ci++) {
            if (!dead[ci]) {
                kept.push_back(std::move(raw_cells[ci]));
            }
        }
        raw_cells = std::move(kept);
    }

    // Simplify pass-through degree-2 vertices (NFunctionMesher.h lines 567-573)
    std::vector<std::set<size_t>> v_inc_cells(arr_pts.size());
    std::vector<std::set<size_t>> v_cell_nbrs(arr_pts.size());
    for (size_t ci = 0; ci < raw_cells.size(); ci++) {
        const std::vector<size_t>& c = raw_cells[ci];
        for (size_t k = 0; k < c.size(); k++) {
            v_inc_cells[c[k]].insert(ci);
            v_cell_nbrs[c[k]].insert(c[(k + 1) % c.size()]);
            v_cell_nbrs[c[(k + 1) % c.size()]].insert(c[k]);
        }
    }

    std::vector<std::vector<Point>> out_rings;
    std::vector<std::vector<Vector>> out_norms;
    for (const std::vector<size_t>& c : raw_cells) {
        std::vector<size_t> simp;
        for (const size_t vid : c) {
            const size_t deg = v_cell_nbrs[vid].size();
            const size_t n_inc = v_inc_cells[vid].size();
            const bool is_pass_through =
                (deg == 2 && !arr_is_orig_sharp[vid]) &&
                ((!arr_is_bnd[vid] && n_inc == 2) || (arr_is_bnd[vid] && n_inc == 1));
            if (!is_pass_through) {
                simp.push_back(vid);
            }
        }
        if (simp.size() < 3) {
            continue;
        }
        std::vector<Point> r;
        std::vector<Vector> rn;
        for (const size_t vid : simp) {
            const std::pair<Point, Vector> pr = projector.project(arr_pts[vid]);
            r.push_back(arr_is_bnd[vid] ? arr_pts[vid] : pr.first);
            rn.push_back(pr.second);
        }
        out_rings.push_back(std::move(r));
        out_norms.push_back(std::move(rn));
    }

    return weld_polygon_mesh(out_rings, out_norms, 1e-2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Pure-C++ ShapeUp Regularization (Bouaziz et al. 2012 / igl::shapeup)
// ═══════════════════════════════════════════════════════════════════════════

Mesh shapeup_regularize(
    const Mesh& quad_mesh,
    double target_edge,
    int iterations,
    double shape_weight,
    double close_weight,
    double smooth_weight
) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        quad_mesh.to_vertices_and_faces();
    const std::vector<Point>& p0 = vf.first;
    const std::vector<std::vector<size_t>>& faces = vf.second;
    if (p0.empty() || faces.empty() || iterations <= 0) {
        return quad_mesh;
    }

    const size_t nv = p0.size();
    std::map<std::pair<size_t, size_t>, int> edge_uses;
    for (const std::vector<size_t>& f : faces) {
        const size_t m = f.size();
        for (size_t k = 0; k < m; k++) {
            edge_uses[ordered_pair(f[k], f[(k + 1) % m])]++;
        }
    }

    std::vector<bool> bnd_v(nv, false);
    for (const std::pair<const std::pair<size_t, size_t>, int>& kv : edge_uses) {
        if (kv.second == 1) {
            bnd_v[kv.first.first] = true;
            bnd_v[kv.first.second] = true;
        }
    }

    // Assemble constant SPD matrix Q = A^T * W * A directly
    std::vector<std::map<size_t, double>> q_rows(nv);
    for (const std::vector<size_t>& f : faces) {
        const size_t m = f.size();
        if (m < 3) {
            continue;
        }
        const double inv_m = 1.0 / static_cast<double>(m);
        for (size_t j = 0; j < m; j++) {
            for (size_t k = 0; k < m; k++) {
                const double val = (j == k) ? (1.0 - inv_m) : (-inv_m);
                q_rows[f[j]][f[k]] += shape_weight * val;
            }
        }
    }

    std::vector<double> close_w(nv, close_weight);
    for (size_t i = 0; i < nv; i++) {
        close_w[i] = close_weight * (bnd_v[i] ? 2.5 : 1.0);
        q_rows[i][i] += close_w[i];
    }

    for (const std::pair<const std::pair<size_t, size_t>, int>& kv : edge_uses) {
        const size_t u = kv.first.first;
        const size_t v = kv.first.second;
        q_rows[u][u] += smooth_weight;
        q_rows[v][v] += smooth_weight;
        q_rows[u][v] -= smooth_weight;
        q_rows[v][u] -= smooth_weight;
    }

    const CsrMatrix q_mat = CsrMatrix::from_maps(q_rows);

    std::vector<double> px(nv);
    std::vector<double> py(nv);
    std::vector<double> pz(nv);
    for (size_t i = 0; i < nv; i++) {
        px[i] = p0[i][0];
        py[i] = p0[i][1];
        pz[i] = p0[i][2];
    }

    for (int iter = 0; iter < iterations; iter++) {
        std::vector<double> bx(nv, 0.0);
        std::vector<double> by(nv, 0.0);
        std::vector<double> bz(nv, 0.0);
        for (size_t i = 0; i < nv; i++) {
            bx[i] = close_w[i] * p0[i][0];
            by[i] = close_w[i] * p0[i][1];
            bz[i] = close_w[i] * p0[i][2];
        }

        // Local step: best-fitting 3D rotated regular polygon of edge target_edge (igl::shapeup_regular_face_projection)
        for (const std::vector<size_t>& f : faces) {
            const size_t m = f.size();
            if (m < 3) {
                continue;
            }
            Vector avg(0.0, 0.0, 0.0);
            for (const size_t vid : f) {
                avg = avg + Vector(px[vid], py[vid], pz[vid]);
            }
            avg = avg * (1.0 / static_cast<double>(m));

            std::vector<Vector> source(m);
            std::vector<Vector> target(m);
            Mat3 corr;
            for (size_t j = 0; j < m; j++) {
                const double ang = Tolerance::TWO_PI * static_cast<double>(j) / static_cast<double>(m);
                source[j] = Vector(std::cos(ang), std::sin(ang), 0.0);
                target[j] = Vector(px[f[j]] - avg[0], py[f[j]] - avg[1], pz[f[j]] - avg[2]);
                for (int r = 0; r < 3; r++) {
                    for (int c = 0; c < 3; c++) {
                        corr.m[r][c] += source[j][r] * target[j][c];
                    }
                }
            }

            const Mat3 rot = svd3x3_proper_rotation(corr);
            const double unit_chord = 2.0 * std::sin(Tolerance::PI / static_cast<double>(m));
            double scale = 1.0;
            if (target_edge > 0.0) {
                scale = target_edge / unit_chord;
            } else {
                double ratio_sum = 0.0;
                for (size_t j = 0; j < m; j++) {
                    ratio_sum += (target[(j + 1) % m] - target[j]).magnitude() / unit_chord;
                }
                scale = ratio_sum / static_cast<double>(m);
            }

            // Note: proj = (source * rot) * scale = (rot^T * source^T)^T * scale, centered so C_m^T * proj = proj
            const Mat3 rot_t = mat3_transpose(rot);
            for (size_t j = 0; j < m; j++) {
                const Vector p_rot = mat3_mul_vec(rot_t, source[j]) * scale;
                bx[f[j]] += shape_weight * p_rot[0];
                by[f[j]] += shape_weight * p_rot[1];
                bz[f[j]] += shape_weight * p_rot[2];
            }
        }

        const std::vector<double> prev_x = px;
        const std::vector<double> prev_y = py;
        const std::vector<double> prev_z = pz;

        solve_pcg_scalar(q_mat, bx, px, 180, 1e-11);
        solve_pcg_scalar(q_mat, by, py, 180, 1e-11);
        solve_pcg_scalar(q_mat, bz, pz, 180, 1e-11);

        double max_step = 0.0;
        for (size_t i = 0; i < nv; i++) {
            max_step = std::max({
                max_step,
                std::abs(px[i] - prev_x[i]),
                std::abs(py[i] - prev_y[i]),
                std::abs(pz[i] - prev_z[i])
            });
        }
        if (max_step < 1e-5) {
            break;
        }
    }

    std::vector<Point> out_verts(nv);
    for (size_t i = 0; i < nv; i++) {
        out_verts[i] = Point(px[i], py[i], pz[i]);
    }

    // Recompute smooth vertex normals on the regularized polygon mesh
    std::vector<Vector> out_norms(nv, Vector(0.0, 0.0, 0.0));
    for (const std::vector<size_t>& f : faces) {
        if (f.size() < 3) {
            continue;
        }
        Point c(0.0, 0.0, 0.0);
        for (const size_t vid : f) {
            c = c + Vector(out_verts[vid][0], out_verts[vid][1], out_verts[vid][2]);
        }
        c = Point(
            c[0] / static_cast<double>(f.size()),
            c[1] / static_cast<double>(f.size()),
            c[2] / static_cast<double>(f.size())
        );
        Vector fn(0.0, 0.0, 0.0);
        for (size_t k = 0; k < f.size(); k++) {
            fn = fn + (out_verts[f[k]] - c).cross(out_verts[f[(k + 1) % f.size()]] - c);
        }
        for (const size_t vid : f) {
            out_norms[vid] = out_norms[vid] + fn;
        }
    }

    Mesh result = Mesh::from_vertices_and_faces(out_verts, faces);
    for (size_t i = 0; i < nv; i++) {
        Vector n = out_norms[i];
        if (!n.normalize_self()) {
            n = Vector(0.0, 0.0, 1.0);
        }
        if (result.vertex.count(i)) {
            result.vertex[i].set_normal(n[0], n[1], n[2]);
        }
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Pure-C++ Binary Integer Program Tile Matcher (Du, Van Mele & Block 2023)
// ═══════════════════════════════════════════════════════════════════════════

BrickMatchingResult match_brick_tiles(
    const Mesh& quad_mesh,
    int pattern,
    double penalty_weight,
    double reward_weight,
    double border_weight
) {

    BrickMatchingResult out;
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        quad_mesh.to_vertices_and_faces();
    const std::vector<std::vector<size_t>>& faces = vf.second;
    if (faces.empty()) {
        return out;
    }

    const DualTilingGraph g =
        build_dual_tiling_graph(faces, pattern, penalty_weight, reward_weight, border_weight);
    if (g.edges.empty()) {
        for (size_t f = 0; f < g.num_faces; f++) {
            out.unmatched_faces.push_back(f);
        }
        return out;
    }

    // Multi-start augmenting-path initializations + exact window B&B / Large-Neighborhood Search
    std::vector<uint8_t> best_x(g.edges.size(), 0);
    double best_obj = -1e30;

    WindowMipSolver solver(g);
    if (g.num_faces <= 64) {
        best_x = initial_augmenting_matching(g, 0);
        std::vector<size_t> all_faces(g.num_faces);
        for (size_t f = 0; f < g.num_faces; f++) {
            all_faces[f] = f;
        }
        solver.optimize_window(all_faces, best_x, 220000);
        best_obj = evaluate_bip_objective(g, best_x);
    } else {
        const size_t num_starts = 2;
        for (size_t s = 0; s < num_starts; s++) {
            std::vector<uint8_t> x =
                initial_augmenting_matching(g, (s * g.num_faces) / num_starts);

            const size_t win_size = 46;
            const size_t stride = std::max<size_t>(1, g.num_faces / 18);
            for (int sweep = 0; sweep < 3; sweep++) {
                std::vector<size_t> hot_seeds;
                std::vector<uint8_t> matched(g.num_faces, 0);
                for (size_t e = 0; e < g.edges.size(); e++) {
                    if (x[e]) {
                        matched[g.edges[e].first] = 1;
                        matched[g.edges[e].second] = 1;
                    }
                }
                for (size_t f = 0; f < g.num_faces; f++) {
                    if (!matched[f] && !g.is_border[f]) {
                        hot_seeds.push_back(f);
                    }
                }
                for (const std::pair<size_t, size_t>& pp : g.parallel_pairs) {
                    if (x[pp.first] && x[pp.second]) {
                        hot_seeds.push_back(g.edges[pp.first].first);
                    }
                }
                for (const size_t seed : hot_seeds) {
                    const std::vector<size_t> win = bfs_face_window(g, seed, win_size);
                    solver.optimize_window(win, x, 14000);
                }

                for (size_t idx = 0; idx < g.num_faces; idx += stride) {
                    const size_t seed = (idx + sweep * 7 + s * 13) % g.num_faces;
                    const std::vector<size_t> win = bfs_face_window(g, seed, win_size);
                    solver.optimize_window(win, x, 12000);
                }
            }

            const double obj = evaluate_bip_objective(g, x);
            if (obj > best_obj) {
                best_obj = obj;
                best_x = x;
            }
        }

        // Fast 2x2 alternating 4-cycle flip pass (flips parallel stack-bond pairs (ab,cd) -> (ac,bd) when improving obj)
        std::map<std::pair<size_t, size_t>, size_t> fast_eidx;
        for (size_t e = 0; e < g.edges.size(); e++) {
            fast_eidx[g.edges[e]] = e;
        }
        for (int flip_pass = 0; flip_pass < 4; flip_pass++) {
            bool any_flip = false;
            for (const std::pair<size_t, size_t>& pp : g.parallel_pairs) {
                if (!best_x[pp.first] || !best_x[pp.second]) {
                    continue;
                }
                const size_t a = g.edges[pp.first].first;
                const size_t b = g.edges[pp.first].second;
                const size_t c = g.edges[pp.second].first;
                const size_t d = g.edges[pp.second].second;
                const std::array<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>, 2> alt = {{
                    {ordered_pair(a, c), ordered_pair(b, d)},
                    {ordered_pair(a, d), ordered_pair(b, c)}
                }};
                for (const std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>& cand : alt) {
                    const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it1 =
                        fast_eidx.find(cand.first);
                    const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it2 =
                        fast_eidx.find(cand.second);
                    if (it1 == fast_eidx.end() || it2 == fast_eidx.end()) {
                        continue;
                    }
                    const size_t e_alt1 = it1->second;
                    const size_t e_alt2 = it2->second;
                    if (best_x[e_alt1] || best_x[e_alt2]) {
                        continue;
                    }
                    const double cur_score = evaluate_bip_objective(g, best_x);
                    best_x[pp.first] = 0;
                    best_x[pp.second] = 0;
                    best_x[e_alt1] = 1;
                    best_x[e_alt2] = 1;
                    const double new_score = evaluate_bip_objective(g, best_x);
                    if (new_score > cur_score + 1e-9) {
                        any_flip = true;
                        break;
                    }
                    best_x[e_alt1] = 0;
                    best_x[e_alt2] = 0;
                    best_x[pp.first] = 1;
                    best_x[pp.second] = 1;
                }
            }
            if (!any_flip) {
                break;
            }
        }

        // Targeted elimination of any remaining parallel stack-bond pairs + final polish sweep
        for (int pass = 0; pass < 2; pass++) {
            for (const std::pair<size_t, size_t>& pp : g.parallel_pairs) {
                if (best_x[pp.first] && best_x[pp.second]) {
                    const std::vector<size_t> win = bfs_face_window(g, g.edges[pp.first].first, 56);
                    solver.optimize_window(win, best_x, 28000);
                }
            }
        }
        const size_t polish_stride = std::max<size_t>(1, g.num_faces / 20);
        for (size_t seed = 0; seed < g.num_faces; seed += polish_stride) {
            const std::vector<size_t> win = bfs_face_window(g, seed, 50);
            solver.optimize_window(win, best_x, 22000);
        }
        best_obj = evaluate_bip_objective(g, best_x);
    }

    std::vector<uint8_t> face_covered(g.num_faces, 0);
    for (size_t e = 0; e < g.edges.size(); e++) {
        if (best_x[e]) {
            out.matched_pairs.push_back(g.edges[e]);
            face_covered[g.edges[e].first] = 1;
            face_covered[g.edges[e].second] = 1;
        }
    }
    for (size_t f = 0; f < g.num_faces; f++) {
        if (!face_covered[f]) {
            out.unmatched_faces.push_back(f);
            if (!g.is_border[f]) {
                out.unmatched_interior++;
            }
        }
    }
    for (const std::pair<size_t, size_t>& pr : g.parallel_pairs) {
        if (best_x[pr.first] && best_x[pr.second]) {
            out.parallel_pairs++;
        }
    }
    for (const std::pair<size_t, size_t>& pr : g.running_pairs) {
        if (best_x[pr.first] && best_x[pr.second]) {
            out.running_pairs++;
        }
    }
    for (const std::pair<size_t, size_t>& pr : g.herringbone_pairs) {
        if (best_x[pr.first] && best_x[pr.second]) {
            out.herringbone_pairs++;
        }
    }
    out.objective = best_obj;

    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// Conformal Brick Mesh Merging & 3D Tile / Stereotomic Block Materialisation
// ═══════════════════════════════════════════════════════════════════════════

Mesh merge_matched_brick_mesh(const Mesh& quad_mesh, const BrickMatchingResult& matching) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        quad_mesh.to_vertices_and_faces();
    const std::vector<Point>& verts = vf.first;
    const std::vector<std::vector<size_t>>& faces = vf.second;
    const std::vector<size_t> keys = quad_mesh.vertices();
    if (verts.empty() || faces.empty()) {
        return quad_mesh;
    }

    std::vector<Vector> vnorms(verts.size(), Vector(0.0, 0.0, 1.0));
    for (size_t i = 0; i < verts.size(); i++) {
        const std::optional<std::array<double, 3>> attr = quad_mesh.vertex.at(keys[i]).normal();
        if (attr) {
            Vector sn((*attr)[0], (*attr)[1], (*attr)[2]);
            if (sn.normalize_self()) {
                vnorms[i] = sn;
            }
        }
    }

    std::vector<bool> used(faces.size(), false);
    std::vector<std::vector<size_t>> merged_faces;

    for (const std::pair<size_t, size_t>& pr : matching.matched_pairs) {
        const size_t fa = pr.first;
        const size_t fb = pr.second;
        if (fa >= faces.size() || fb >= faces.size() || used[fa] || used[fb]) {
            continue;
        }
        const std::vector<size_t>& la = faces[fa];
        const std::vector<size_t>& lb = faces[fb];

        // Dissolve shared halfedge pair (u -> v in fa, v -> u in fb), keeping all 6 boundary vertices intact!
        std::set<std::pair<size_t, size_t>> halfedges;
        for (size_t k = 0; k < la.size(); k++) {
            halfedges.insert({la[k], la[(k + 1) % la.size()]});
        }
        for (size_t k = 0; k < lb.size(); k++) {
            const std::pair<size_t, size_t> opp = {lb[(k + 1) % lb.size()], lb[k]};
            if (halfedges.count(opp)) {
                halfedges.erase(opp);
            } else {
                halfedges.insert({lb[k], lb[(k + 1) % lb.size()]});
            }
        }

        std::map<size_t, size_t> next_v;
        for (const std::pair<size_t, size_t>& he : halfedges) {
            next_v[he.first] = he.second;
        }
        if (next_v.size() >= 3) {
            std::vector<size_t> ring;
            size_t cur = next_v.begin()->first;
            for (size_t step = 0; step < next_v.size(); step++) {
                ring.push_back(cur);
                const std::map<size_t, size_t>::const_iterator it = next_v.find(cur);
                if (it == next_v.end()) {
                    ring.clear();
                    break;
                }
                cur = it->second;
                if (cur == ring.front()) {
                    break;
                }
            }
            if (ring.size() == next_v.size()) {
                merged_faces.push_back(ring);
                used[fa] = true;
                used[fb] = true;
            }
        }
    }

    for (size_t f = 0; f < faces.size(); f++) {
        if (!used[f] && faces[f].size() >= 3) {
            merged_faces.push_back(faces[f]);
        }
    }

    Mesh out = Mesh::from_vertices_and_faces(verts, merged_faces);
    for (size_t i = 0; i < verts.size(); i++) {
        if (out.vertex.count(i)) {
            out.vertex[i].set_normal(vnorms[i][0], vnorms[i][1], vnorms[i][2]);
        }
    }

    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// Chalmers Geodesic Coordinate Brick Courses (Adiels, Ander & Williams, IASS 2017)
// Geodesics-in-Heat Bed Trajectories (a22 = const) + Orthogonal Running Bond (a12 = 0)
// ═══════════════════════════════════════════════════════════════════════════

Mesh geodesic_brick_pattern(
    const Mesh& shell_mesh,
    double bed_height,
    double brick_length,
    bool polar_from_holes
) {

    const double h_bed = std::max(10.0, bed_height);
    const double l_brick = std::max(h_bed, brick_length);

    std::vector<Point> base_verts;
    std::vector<std::array<size_t, 3>> base_tris;
    extract_welded_triangles(shell_mesh, base_verts, base_tris, 1e-2);
    if (base_verts.empty() || base_tris.empty()) {
        return Mesh();
    }

    double signed_z = 0.0;
    for (const std::array<size_t, 3>& t : base_tris) {
        signed_z += (base_verts[t[1]] - base_verts[t[0]]).cross(base_verts[t[2]] - base_verts[t[0]])[2];
    }
    if (signed_z < 0.0) {
        for (std::array<size_t, 3>& t : base_tris) {
            std::swap(t[1], t[2]);
        }
    }

    std::vector<Vector> base_norms;
    std::vector<double> base_areas;
    compute_normals_and_areas(base_verts, base_tris, base_norms, base_areas);

    SurfaceProjector projector;
    projector.build(base_verts, base_norms, base_tris);

    // Subdivide mesh to <= 0.36 * h_bed and project interior vertices onto smooth shell
    std::vector<Point> verts = base_verts;
    std::vector<std::array<size_t, 3>> tris = base_tris;
    subdivide_triangles_to_length(verts, tris, 0.36 * h_bed, 8500);

    const size_t nv = verts.size();
    const size_t nf = tris.size();
    std::map<std::pair<size_t, size_t>, int> dir_he;
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            dir_he[{tris[f][k], tris[f][(k + 1) % 3]}] = static_cast<int>(3 * f + k);
        }
    }

    std::vector<bool> is_bnd_v(nv, false);
    std::map<size_t, size_t> bnd_next;
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            const size_t u = tris[f][k];
            const size_t w = tris[f][(k + 1) % 3];
            if (!dir_he.count({w, u})) {
                is_bnd_v[u] = true;
                is_bnd_v[w] = true;
                bnd_next[u] = w;
            }
        }
    }
    for (size_t v = 0; v < nv; v++) {
        if (!is_bnd_v[v]) {
            verts[v] = projector.project(verts[v]).first;
        }
    }

    // Extract closed boundary loops, sharp corners, and select geodesic starting curve Gamma_0
    std::vector<std::vector<size_t>> bnd_loops;
    std::vector<double> loop_perims;
    {
        std::set<size_t> vis;
        for (const std::pair<const size_t, size_t>& kv : bnd_next) {
            if (vis.count(kv.first)) {
                continue;
            }
            std::vector<size_t> lp;
            double perim = 0.0;
            size_t cur = kv.first;
            for (size_t step = 0; step <= bnd_next.size(); step++) {
                if (vis.count(cur)) {
                    break;
                }
                vis.insert(cur);
                lp.push_back(cur);
                const std::map<size_t, size_t>::const_iterator it = bnd_next.find(cur);
                if (it == bnd_next.end()) {
                    break;
                }
                perim += verts[cur].distance(verts[it->second]);
                cur = it->second;
            }
            if (lp.size() >= 3) {
                bnd_loops.push_back(std::move(lp));
                loop_perims.push_back(perim);
            }
        }
    }

    std::vector<bool> is_sharp_v(nv, false);
    for (const std::vector<size_t>& lp : bnd_loops) {
        const size_t m = lp.size();
        if (m < 4) {
            continue;
        }
        std::vector<double> tcos(m, 1.0);
        for (size_t i = 0; i < m; i++) {
            const Point& pv = verts[lp[i]];
            size_t ip = (i + m - 1) % m;
            for (size_t s = 1; s < m / 3 && pv.distance(verts[lp[ip]]) < 0.42 * h_bed; s++) {
                ip = (i + m - 1 - s) % m;
            }
            size_t in = (i + 1) % m;
            for (size_t s = 1; s < m / 3 && pv.distance(verts[lp[in]]) < 0.42 * h_bed; s++) {
                in = (i + 1 + s) % m;
            }
            Vector vin = pv - verts[lp[ip]];
            Vector vout = verts[lp[in]] - pv;
            if (vin.normalize_self() && vout.normalize_self()) {
                tcos[i] = vin.dot(vout);
            }
        }
        for (size_t i = 0; i < m; i++) {
            if (tcos[i] < 0.72 && tcos[i] <= tcos[(i + m - 1) % m] + 1e-6 &&
                tcos[i] <= tcos[(i + 1) % m] + 1e-6) {
                is_sharp_v[lp[i]] = true;
            }
        }
    }

    std::vector<bool> is_seed(nv, false);
    if (polar_from_holes && bnd_loops.size() >= 2) {
        // Polar geodesic coordinates from the inner oculus loop (Adiels et al. 2017, Fig. 6 top)
        size_t min_li = 0;
        for (size_t li = 1; li < bnd_loops.size(); li++) {
            if (loop_perims[li] < loop_perims[min_li]) {
                min_li = li;
            }
        }
        for (const size_t vid : bnd_loops[min_li]) {
            is_seed[vid] = true;
        }
    } else if (!bnd_loops.empty()) {
        // Select longest smooth arch chain of outer loop as starting curve Gamma_0 (Adiels et al. 2017, Section 5)
        size_t max_li = 0;
        for (size_t li = 1; li < bnd_loops.size(); li++) {
            if (loop_perims[li] > loop_perims[max_li]) {
                max_li = li;
            }
        }
        const std::vector<size_t>& outer = bnd_loops[max_li];
        std::vector<size_t> corners;
        for (size_t i = 0; i < outer.size(); i++) {
            if (is_sharp_v[outer[i]]) {
                corners.push_back(i);
            }
        }
        if (corners.size() >= 2) {
            double best_chain_len = -1.0;
            std::vector<size_t> best_chain;
            for (size_t ci = 0; ci < corners.size(); ci++) {
                const size_t i0 = corners[ci];
                const size_t i1 = corners[(ci + 1) % corners.size()];
                std::vector<size_t> ch;
                double clen = 0.0;
                size_t cur = i0;
                while (true) {
                    ch.push_back(outer[cur]);
                    if (cur == i1) {
                        break;
                    }
                    const size_t nxt = (cur + 1) % outer.size();
                    clen += verts[outer[cur]].distance(verts[outer[nxt]]);
                    cur = nxt;
                }
                if (clen > best_chain_len) {
                    best_chain_len = clen;
                    best_chain = std::move(ch);
                }
            }
            for (const size_t vid : best_chain) {
                is_seed[vid] = true;
            }
        } else {
            Point ctr = Point::centroid(verts);
            size_t best_v = 0;
            double best_d = 1e30;
            for (size_t v = 0; v < nv; v++) {
                const double d = verts[v].distance(ctr);
                if (d < best_d) {
                    best_d = d;
                    best_v = v;
                }
            }
            is_seed[best_v] = true;
        }
    }

    // Build undirected edge lookup + Geodesics-in-Heat operators (Crane, Weischedel & Wardetzky 2013)
    std::vector<std::pair<size_t, size_t>> ev_list;
    std::vector<std::array<size_t, 3>> fe_table(nf, {0, 0, 0});
    std::map<std::pair<size_t, size_t>, size_t> edge_lookup;
    double edge_len_sum = 0.0;
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            const size_t u = tris[f][k];
            const size_t w = tris[f][(k + 1) % 3];
            const std::pair<size_t, size_t> ek = ordered_pair(u, w);
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it = edge_lookup.find(ek);
            if (it == edge_lookup.end()) {
                const size_t eid = ev_list.size();
                edge_lookup[ek] = eid;
                ev_list.push_back(ek);
                fe_table[f][k] = eid;
                edge_len_sum += verts[u].distance(verts[w]);
            } else {
                fe_table[f][k] = it->second;
            }
        }
    }
    const size_t ne = ev_list.size();
    const double mean_elen = ne > 0 ? (edge_len_sum / static_cast<double>(ne)) : (0.3 * h_bed);

    std::vector<Vector> fnrm(nf);
    std::vector<double> farea(nf, 0.0);
    std::vector<std::array<Vector, 3>> b_grad(nf);
    std::vector<std::map<size_t, double>> lap_rows(nv);
    std::vector<double> v_mass(nv, 0.0);

    for (size_t f = 0; f < nf; f++) {
        const Point& p0 = verts[tris[f][0]];
        const Point& p1 = verts[tris[f][1]];
        const Point& p2 = verts[tris[f][2]];
        const Vector cr = (p1 - p0).cross(p2 - p0);
        const double area = std::max(1e-12, 0.5 * cr.magnitude());
        farea[f] = area;
        fnrm[f] = (cr.magnitude_squared() > 1e-20) ? cr.normalized() : Vector(0.0, 0.0, 1.0);
        const double inv_2a = 1.0 / (2.0 * area);
        b_grad[f][0] = fnrm[f].cross(p1 - p2) * inv_2a;
        b_grad[f][1] = fnrm[f].cross(p2 - p0) * inv_2a;
        b_grad[f][2] = fnrm[f].cross(p0 - p1) * inv_2a;
        for (size_t a = 0; a < 3; a++) {
            v_mass[tris[f][a]] += area / 3.0;
            for (size_t b = 0; b < 3; b++) {
                lap_rows[tris[f][a]][tris[f][b]] += area * b_grad[f][a].dot(b_grad[f][b]);
            }
        }
    }

    // Step I: short-time heat diffusion (M + t * L) u_heat = delta_seed
    const double heat_t = 6.0 * mean_elen * mean_elen;
    std::vector<std::map<size_t, double>> heat_rows = lap_rows;
    std::vector<double> heat_rhs(nv, 0.0);
    for (size_t i = 0; i < nv; i++) {
        for (std::pair<const size_t, double>& kv : heat_rows[i]) {
            kv.second *= heat_t;
        }
        heat_rows[i][i] += v_mass[i];
        if (is_seed[i]) {
            heat_rows[i][i] += 1e8;
            heat_rhs[i] = 1e8;
        }
    }
    const CsrMatrix heat_csr = CsrMatrix::from_maps(heat_rows);
    std::vector<double> u_heat(nv, 0.0);
    solve_pcg_scalar(heat_csr, heat_rhs, u_heat, 1500, 1e-13);

    // Step II: normalized unit geodesic gradient X_f and orthogonal bed-course tangent course_dir[f]
    std::vector<Vector> geod_dir(nf, Vector(1.0, 0.0, 0.0));
    std::vector<Vector> course_dir(nf, Vector(0.0, 1.0, 0.0));
    std::vector<double> div_rhs(nv, 0.0);
    for (size_t f = 0; f < nf; f++) {
        Vector g = b_grad[f][0] * u_heat[tris[f][0]] + b_grad[f][1] * u_heat[tris[f][1]] +
                   b_grad[f][2] * u_heat[tris[f][2]];
        Vector xf = -g;
        if (!xf.normalize_self()) {
            xf = (verts[tris[f][1]] - verts[tris[f][0]]).normalized();
        }
        geod_dir[f] = xf;
        course_dir[f] = fnrm[f].cross(xf).normalized();
        for (size_t c = 0; c < 3; c++) {
            div_rhs[tris[f][c]] += farea[f] * b_grad[f][c].dot(xf);
        }
    }

    // Step III: Poisson distance recovery L * phi = div(X), phi|_seed = 0
    std::vector<std::map<size_t, double>> pois_rows = lap_rows;
    for (size_t i = 0; i < nv; i++) {
        pois_rows[i][i] += 1e-12;
        if (is_seed[i]) {
            pois_rows[i][i] += 1e9;
            div_rhs[i] = 0.0;
        }
    }
    const CsrMatrix pois_csr = CsrMatrix::from_maps(pois_rows);
    std::vector<double> phi(nv, 0.0);
    solve_pcg_scalar(pois_csr, div_rhs, phi, 3500, 1e-13);

    for (size_t v = 0; v < nv; v++) {
        phi[v] = is_seed[v] ? 0.0 : std::max(0.0, phi[v]);
    }
    double grad_norm_area = 0.0;
    double area_tot = 0.0;
    for (size_t f = 0; f < nf; f++) {
        const Vector gp = b_grad[f][0] * phi[tris[f][0]] + b_grad[f][1] * phi[tris[f][1]] +
                          b_grad[f][2] * phi[tris[f][2]];
        const double gn = gp.magnitude();
        if (gn > 1e-6) {
            grad_norm_area += farea[f] * gn;
            area_tot += farea[f];
        }
    }
    const double mean_gn = (area_tot > 0.0) ? (grad_norm_area / area_tot) : 1.0;
    std::vector<double> tau(nv, 0.0);
    for (size_t v = 0; v < nv; v++) {
        tau[v] = (phi[v] / std::max(0.2, mean_gn)) / h_bed;
        if (!is_seed[v] && std::abs(tau[v] - std::round(tau[v])) < 3e-4) {
            tau[v] += 4.5e-4;
        }
    }

    // Slice mesh along exact integer geodesic bed isolines tau = 1, 2, ...
    std::vector<int> orig_to_sv(nv, -1);
    std::vector<std::vector<std::pair<double, size_t>>> edge_iso_pts(ne);
    std::vector<Point> strip_pts;
    std::vector<bool> strip_v_bnd;
    std::vector<bool> strip_v_sharp;

    struct SubPiece {
        std::vector<size_t> vids;
        int course_k = 0;
        size_t parent_f = 0;
        double area = 0.0;
    };
    std::vector<SubPiece> sub_pieces;

    for (size_t f = 0; f < nf; f++) {
        const double t0 = tau[tris[f][0]];
        const double t1 = tau[tris[f][1]];
        const double t2 = tau[tris[f][2]];
        const double dt_da = t1 - t0;
        const double dt_db = t2 - t0;

        const int m_min = static_cast<int>(std::floor(std::min({t0, t1, t2}) + 1e-9)) + 1;
        const int m_max = static_cast<int>(std::ceil(std::max({t0, t1, t2}) - 1e-9)) - 1;

        std::vector<std::vector<BaryVert>> pieces = {{{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}}};
        if (m_max - m_min <= 64) {
            for (int m = m_min; m <= m_max; m++) {
                std::vector<std::vector<BaryVert>> next_p;
                for (const std::vector<BaryVert>& poly : pieces) {
                    std::vector<BaryVert> le;
                    std::vector<BaryVert> ge;
                    split_bary_poly_line(poly, t0, dt_da, dt_db, static_cast<double>(m), le, ge);
                    if (le.size() >= 3) {
                        next_p.push_back(std::move(le));
                    }
                    if (ge.size() >= 3) {
                        next_p.push_back(std::move(ge));
                    }
                }
                pieces = std::move(next_p);
            }
        }

        for (const std::vector<BaryVert>& poly : pieces) {
            std::vector<size_t> vids;
            double tau_sum = 0.0;
            for (const BaryVert& bv : poly) {
                const double l1 = bv.a;
                const double l2 = bv.b;
                const double l0 = 1.0 - l1 - l2;
                tau_sum += t0 + dt_da * l1 + dt_db * l2;

                size_t vid = strip_pts.size();
                if (l0 > 1.0 - 1e-5 || l1 > 1.0 - 1e-5 || l2 > 1.0 - 1e-5) {
                    size_t ov = tris[f][0];
                    if (l1 > l0 && l1 >= l2) {
                        ov = tris[f][1];
                    } else if (l2 > l0 && l2 > l1) {
                        ov = tris[f][2];
                    }
                    if (orig_to_sv[ov] < 0) {
                        orig_to_sv[ov] = static_cast<int>(strip_pts.size());
                        strip_pts.push_back(verts[ov]);
                        strip_v_bnd.push_back(is_bnd_v[ov]);
                        strip_v_sharp.push_back(is_sharp_v[ov]);
                    }
                    vid = static_cast<size_t>(orig_to_sv[ov]);
                } else {
                    size_t k_edge = 0;
                    double t_loc = l1;
                    if (l0 <= l1 && l0 <= l2) {
                        k_edge = 1;
                        t_loc = l2 / std::max(1e-12, l1 + l2);
                    } else if (l1 <= l0 && l1 <= l2) {
                        k_edge = 2;
                        t_loc = l0 / std::max(1e-12, l2 + l0);
                    } else {
                        k_edge = 0;
                        t_loc = l1 / std::max(1e-12, l0 + l1);
                    }
                    const size_t eid = fe_table[f][k_edge];
                    const double s_can =
                        (ev_list[eid].first == tris[f][k_edge]) ? t_loc : (1.0 - t_loc);
                    const bool on_bnd = !dir_he.count(
                        {tris[f][(k_edge + 1) % 3], tris[f][k_edge]}
                    );
                    bool found = false;
                    for (const std::pair<double, size_t>& ec : edge_iso_pts[eid]) {
                        if (std::abs(ec.first - s_can) < 1e-3) {
                            vid = ec.second;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        vid = strip_pts.size();
                        edge_iso_pts[eid].emplace_back(s_can, vid);
                        const Point& ea = verts[ev_list[eid].first];
                        const Point& eb = verts[ev_list[eid].second];
                        strip_pts.emplace_back(
                            (1.0 - s_can) * ea[0] + s_can * eb[0],
                            (1.0 - s_can) * ea[1] + s_can * eb[1],
                            (1.0 - s_can) * ea[2] + s_can * eb[2]
                        );
                        strip_v_bnd.push_back(on_bnd);
                        strip_v_sharp.push_back(false);
                    } else if (on_bnd) {
                        strip_v_bnd[vid] = true;
                    }
                }
                if (vids.empty() || vids.back() != vid) {
                    vids.push_back(vid);
                }
            }
            if (vids.size() >= 2 && vids.front() == vids.back()) {
                vids.pop_back();
            }
            if (vids.size() < 3) {
                continue;
            }
            Vector avec(0.0, 0.0, 0.0);
            for (size_t k = 1; k + 1 < vids.size(); k++) {
                avec = avec + (strip_pts[vids[k]] - strip_pts[vids[0]])
                                  .cross(strip_pts[vids[k + 1]] - strip_pts[vids[0]]);
            }
            SubPiece sp;
            sp.vids = std::move(vids);
            sp.course_k = std::max(0, static_cast<int>(std::floor(tau_sum / static_cast<double>(poly.size()))));
            sp.parent_f = f;
            sp.area = 0.5 * avec.magnitude();
            sub_pieces.push_back(std::move(sp));
        }
    }

    // Group sub_pieces into connected geodesic strip components within each course_k
    const size_t n_sp = sub_pieces.size();
    DisjointSet strip_dsu(n_sp);
    {
        std::map<std::pair<size_t, size_t>, std::vector<size_t>> sp_edges;
        for (size_t i = 0; i < n_sp; i++) {
            const std::vector<size_t>& v = sub_pieces[i].vids;
            for (size_t k = 0; k < v.size(); k++) {
                sp_edges[ordered_pair(v[k], v[(k + 1) % v.size()])].push_back(i);
            }
        }
        for (const std::pair<const std::pair<size_t, size_t>, std::vector<size_t>>& kv : sp_edges) {
            if (kv.second.size() == 2) {
                const size_t a = kv.second[0];
                const size_t b = kv.second[1];
                if (sub_pieces[a].course_k == sub_pieces[b].course_k) {
                    strip_dsu.unite(a, b);
                }
            }
        }
    }

    std::map<int, std::vector<std::vector<size_t>>> strips_by_course;
    {
        std::map<size_t, std::vector<size_t>> comp_map;
        for (size_t i = 0; i < n_sp; i++) {
            comp_map[strip_dsu.find(i)].push_back(i);
        }
        for (std::pair<const size_t, std::vector<size_t>>& kv : comp_map) {
            const int ck = sub_pieces[kv.second[0]].course_k;
            strips_by_course[ck].push_back(std::move(kv.second));
        }
    }

    // Slice each geodesic strip orthogonally into constant-length running-bond bricks
    std::vector<Point> brick_pts = strip_pts;
    std::vector<bool> brick_v_bnd = strip_v_bnd;
    std::vector<bool> brick_v_sharp = strip_v_sharp;
    std::vector<std::vector<size_t>> raw_bricks;
    std::map<std::pair<size_t, size_t>, std::vector<std::pair<double, size_t>>> head_edge_pts;

    std::vector<double> prev_ring_fracs;
    double ring_spiral_phase = 0.0;
    Vector ref_ring_tx(1.0, 0.0, 0.0);
    Vector ref_ring_ty(0.0, 1.0, 0.0);
    bool has_ref_ring_frame = false;

    for (const std::pair<const int, std::vector<std::vector<size_t>>>& c_entry : strips_by_course) {
        const int course_k = c_entry.first;
        for (const std::vector<size_t>& comp : c_entry.second) {
            double strip_area = 0.0;
            std::map<size_t, size_t> g2l;
            std::vector<size_t> l2g;
            std::set<std::pair<size_t, size_t>> comp_edges;
            for (const size_t spi : comp) {
                strip_area += sub_pieces[spi].area;
                const std::vector<size_t>& lv = sub_pieces[spi].vids;
                for (size_t k = 0; k < lv.size(); k++) {
                    if (!g2l.count(lv[k])) {
                        g2l[lv[k]] = l2g.size();
                        l2g.push_back(lv[k]);
                    }
                    comp_edges.insert(ordered_pair(lv[k], lv[(k + 1) % lv.size()]));
                }
            }
            if (strip_area < 0.08 * h_bed * h_bed || l2g.size() < 3) {
                continue;
            }

            const int chi = static_cast<int>(l2g.size()) - static_cast<int>(comp_edges.size()) +
                            static_cast<int>(comp.size());

            // Sub-polygon triangles inside this strip component with scalar coordinate s_val at corners
            struct StripTri {
                std::array<size_t, 3> g_vids;
                std::array<double, 3> s_vals;
            };
            std::vector<StripTri> s_tris;
            std::vector<double> cut_stations;
            std::vector<double> ring_fracs;
            bool is_closed_ring = (chi <= 0);
            int n_ring = 4;

            if (is_closed_ring) {
                // Closed 360-degree polar geodesic ring around oculus with Popescu half-brick stagger equalization
                Point r_ctr(0.0, 0.0, 0.0);
                Vector r_nrm(0.0, 0.0, 0.0);
                for (const size_t gv : l2g) {
                    r_ctr = r_ctr + Vector(strip_pts[gv][0], strip_pts[gv][1], strip_pts[gv][2]);
                }
                r_ctr = Point(
                    r_ctr[0] / static_cast<double>(l2g.size()),
                    r_ctr[1] / static_cast<double>(l2g.size()),
                    r_ctr[2] / static_cast<double>(l2g.size())
                );
                for (const size_t spi : comp) {
                    r_nrm = r_nrm + fnrm[sub_pieces[spi].parent_f] * sub_pieces[spi].area;
                }
                if (!r_nrm.normalize_self()) {
                    r_nrm = Vector(0.0, 0.0, 1.0);
                }
                if (!has_ref_ring_frame) {
                    build_tangent_frame(r_nrm, ref_ring_tx, ref_ring_ty);
                    has_ref_ring_frame = true;
                }
                Vector tx = ref_ring_tx - r_nrm * ref_ring_tx.dot(r_nrm);
                if (!tx.normalize_self()) {
                    build_tangent_frame(r_nrm, tx, ref_ring_ty);
                }
                const Vector ty = r_nrm.cross(tx).normalized();

                const double perim = strip_area / h_bed;
                n_ring = std::max(6, static_cast<int>(std::llround(perim / l_brick)));
                if (!prev_ring_fracs.empty()) {
                    const int b0 = static_cast<int>(prev_ring_fracs.size());
                    n_ring = std::clamp(n_ring, std::max(6, (b0 + 1) / 2), 2 * b0);
                    std::vector<double> span_lo(static_cast<size_t>(b0));
                    std::vector<double> span_hi(static_cast<size_t>(b0));
                    for (int k = 0; k < b0; k++) {
                        span_lo[static_cast<size_t>(k)] = prev_ring_fracs[static_cast<size_t>(k)];
                        double hi = prev_ring_fracs[static_cast<size_t>((k + 1) % b0)];
                        if (k + 1 == b0) {
                            hi += 1.0;
                        }
                        span_hi[static_cast<size_t>(k)] = hi;
                    }
                    std::vector<double> joints;
                    std::vector<double> lo_b;
                    std::vector<double> hi_b;
                    const double min_clear = 0.22;
                    if (n_ring >= b0) {
                        const int inc = std::min(b0, n_ring - b0);
                        std::vector<bool> split_sp(static_cast<size_t>(b0), false);
                        if (inc > 0) {
                            ring_spiral_phase = std::fmod(ring_spiral_phase + 0.38196601125, 1.0);
                            std::vector<std::pair<double, int>> ranked;
                            for (int k = 0; k < b0; k++) {
                                const double w = span_hi[static_cast<size_t>(k)] - span_lo[static_cast<size_t>(k)];
                                const double mid = std::fmod(0.5 * (span_lo[static_cast<size_t>(k)] + span_hi[static_cast<size_t>(k)]), 1.0);
                                double best_d = 1.0;
                                for (int g_idx = 0; g_idx < inc; g_idx++) {
                                    const double tgt = std::fmod(ring_spiral_phase + static_cast<double>(g_idx) / static_cast<double>(inc), 1.0);
                                    const double d = std::abs(mid - tgt);
                                    best_d = std::min(best_d, std::min(d, 1.0 - d));
                                }
                                ranked.emplace_back(-(w * static_cast<double>(b0) - 0.85 * best_d), k);
                            }
                            std::sort(ranked.begin(), ranked.end());
                            for (int g_idx = 0; g_idx < inc; g_idx++) {
                                split_sp[static_cast<size_t>(ranked[static_cast<size_t>(g_idx)].second)] = true;
                            }
                        }
                        for (int k = 0; k < b0; k++) {
                            const double a = span_lo[static_cast<size_t>(k)];
                            const double b = span_hi[static_cast<size_t>(k)];
                            const double w = b - a;
                            if (!split_sp[static_cast<size_t>(k)]) {
                                joints.push_back(a + 0.5 * w);
                                lo_b.push_back(a + min_clear * w);
                                hi_b.push_back(b - min_clear * w);
                            } else {
                                joints.push_back(a + 0.30 * w);
                                lo_b.push_back(a + 0.18 * w);
                                hi_b.push_back(a + 0.44 * w);
                                joints.push_back(a + 0.70 * w);
                                lo_b.push_back(a + 0.56 * w);
                                hi_b.push_back(b - 0.18 * w);
                            }
                        }
                    } else {
                        const int dec = std::min(b0 / 2, b0 - n_ring);
                        std::vector<bool> skip_sp(static_cast<size_t>(b0), false);
                        ring_spiral_phase = std::fmod(ring_spiral_phase + 0.38196601125, 1.0);
                        const int sk = static_cast<int>(std::floor(ring_spiral_phase * static_cast<double>(b0))) % b0;
                        for (int g_idx = 0; g_idx < dec; g_idx++) {
                            skip_sp[static_cast<size_t>((sk + (g_idx * b0) / std::max(1, dec)) % b0)] = true;
                        }
                        for (int k = 0; k < b0; k++) {
                            if (skip_sp[static_cast<size_t>(k)]) {
                                continue;
                            }
                            const double a = span_lo[static_cast<size_t>(k)];
                            const double b = span_hi[static_cast<size_t>(k)];
                            const double w = b - a;
                            joints.push_back(a + 0.5 * w);
                            lo_b.push_back(a + min_clear * w);
                            hi_b.push_back(b - min_clear * w);
                        }
                    }
                    const int mj = static_cast<int>(joints.size());
                    for (int iter = 0; iter < 36 && mj >= 3; iter++) {
                        std::vector<double> nxt = joints;
                        for (int i = 0; i < mj; i++) {
                            const double pv = (i == 0) ? (joints[static_cast<size_t>(mj - 1)] - 1.0) : joints[static_cast<size_t>(i - 1)];
                            const double nv_j = (i + 1 == mj) ? (joints[0] + 1.0) : joints[static_cast<size_t>(i + 1)];
                            nxt[static_cast<size_t>(i)] = std::clamp(
                                joints[static_cast<size_t>(i)] + 0.45 * (0.5 * (pv + nv_j) - joints[static_cast<size_t>(i)]),
                                lo_b[static_cast<size_t>(i)],
                                hi_b[static_cast<size_t>(i)]
                            );
                        }
                        joints = std::move(nxt);
                    }
                    for (double jf : joints) {
                        double wf = std::fmod(jf, 1.0);
                        if (wf < 0.0) {
                            wf += 1.0;
                        }
                        ring_fracs.push_back(wf);
                    }
                    std::sort(ring_fracs.begin(), ring_fracs.end());
                    n_ring = static_cast<int>(ring_fracs.size());
                } else {
                    for (int k = 0; k < n_ring; k++) {
                        ring_fracs.push_back(
                            (static_cast<double>(k) + 0.125) / static_cast<double>(n_ring)
                        );
                    }
                }
                prev_ring_fracs = ring_fracs;

                for (int wrap = -1; wrap <= 1; wrap++) {
                    for (const double rf : ring_fracs) {
                        cut_stations.push_back(rf + static_cast<double>(wrap));
                    }
                }
                std::sort(cut_stations.begin(), cut_stations.end());

                std::vector<double> v_ang(l2g.size(), 0.0);
                for (size_t li = 0; li < l2g.size(); li++) {
                    const Vector dp = strip_pts[l2g[li]] - r_ctr;
                    double frac = std::atan2(dp.dot(ty), dp.dot(tx)) / Tolerance::TWO_PI;
                    frac = frac - std::floor(frac);
                    for (const double rf : ring_fracs) {
                        if (std::abs(frac - rf) < 4e-4) {
                            frac = std::fmod(frac + 6e-4, 1.0);
                        }
                    }
                    v_ang[li] = frac;
                }

                for (const size_t spi : comp) {
                    const std::vector<size_t>& lv = sub_pieces[spi].vids;
                    const double base_u = v_ang[g2l[lv[0]]];
                    std::vector<double> uw(lv.size(), base_u);
                    for (size_t k = 1; k < lv.size(); k++) {
                        double u = v_ang[g2l[lv[k]]];
                        while (u - base_u > 0.5) {
                            u -= 1.0;
                        }
                        while (u - base_u < -0.5) {
                            u += 1.0;
                        }
                        uw[k] = u;
                    }
                    for (size_t k = 1; k + 1 < lv.size(); k++) {
                        s_tris.push_back({{lv[0], lv[k], lv[k + 1]}, {uw[0], uw[k], uw[k + 1]}});
                    }
                }
            } else {
                // Open arch-to-arch geodesic strip: integrate orthogonal arc length along course_dir (a12 = 0)
                const size_t n_lv = l2g.size();
                std::vector<std::map<size_t, double>> s_rows(n_lv);
                std::vector<double> s_rhs(n_lv, 0.0);
                for (const size_t spi : comp) {
                    const Vector& cdir = course_dir[sub_pieces[spi].parent_f];
                    const std::vector<size_t>& lv = sub_pieces[spi].vids;
                    for (size_t k = 0; k < lv.size(); k++) {
                        const size_t la = g2l[lv[k]];
                        const size_t lb = g2l[lv[(k + 1) % lv.size()]];
                        const Vector dp = strip_pts[lv[(k + 1) % lv.size()]] - strip_pts[lv[k]];
                        const double w = 1.0 / std::max(1.0, dp.magnitude());
                        const double ds = cdir.dot(dp);
                        s_rows[la][la] += w;
                        s_rows[lb][lb] += w;
                        s_rows[la][lb] -= w;
                        s_rows[lb][la] -= w;
                        s_rhs[la] -= w * ds;
                        s_rhs[lb] += w * ds;
                    }
                }
                s_rows[0][0] += 1e6;
                const CsrMatrix s_csr = CsrMatrix::from_maps(s_rows);
                std::vector<double> s_loc(n_lv, 0.0);
                solve_pcg_scalar(s_csr, s_rhs, s_loc, 600, 1e-12);

                double s_min = s_loc[0];
                double s_max = s_loc[0];
                for (const double val : s_loc) {
                    s_min = std::min(s_min, val);
                    s_max = std::max(s_max, val);
                }
                for (double& val : s_loc) {
                    val -= s_min;
                }
                const double s_span = s_max - s_min;
                if (s_span > 1.28 * l_brick) {
                    const double stag = (course_k % 2 == 1) ? (0.5 * l_brick) : 0.0;
                    const int m_lo = -static_cast<int>(std::ceil(s_span / l_brick)) - 1;
                    const int m_hi = static_cast<int>(std::ceil(s_span / l_brick)) + 1;
                    for (int m = m_lo; m <= m_hi; m++) {
                        const double sc = 0.5 * s_span + stag + static_cast<double>(m) * l_brick;
                        if (sc >= 0.34 * l_brick && sc <= s_span - 0.34 * l_brick) {
                            cut_stations.push_back(sc);
                        }
                    }
                    std::sort(cut_stations.begin(), cut_stations.end());
                }
                if (cut_stations.empty() && (course_k % 2 == 1) && s_span >= 0.88 * l_brick) {
                    cut_stations.push_back(0.5 * s_span);
                }
                if (!cut_stations.empty()) {
                    for (double& val : s_loc) {
                        for (const double sc : cut_stations) {
                            if (std::abs(val - sc) < 3e-3) {
                                val += 5e-3;
                            }
                        }
                    }
                }
                for (const size_t spi : comp) {
                    const std::vector<size_t>& lv = sub_pieces[spi].vids;
                    for (size_t k = 1; k + 1 < lv.size(); k++) {
                        s_tris.push_back({
                            {lv[0], lv[k], lv[k + 1]},
                            {s_loc[g2l[lv[0]]], s_loc[g2l[lv[k]]], s_loc[g2l[lv[k + 1]]]}
                        });
                    }
                }
            }

            // Slice s_tris along head-joint cut_stations and group by brick slot
            std::map<int, std::vector<std::vector<size_t>>> slot_polys;

            for (const StripTri& st_tri : s_tris) {
                const double s0 = st_tri.s_vals[0];
                const double ds_da = st_tri.s_vals[1] - s0;
                const double ds_db = st_tri.s_vals[2] - s0;
                const double tri_s_lo = std::min({s0, st_tri.s_vals[1], st_tri.s_vals[2]});
                const double tri_s_hi = std::max({s0, st_tri.s_vals[1], st_tri.s_vals[2]});

                std::vector<std::vector<BaryVert>> h_pieces = {{{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}}};
                for (const double sc : cut_stations) {
                    if (sc <= tri_s_lo + 1e-7 || sc >= tri_s_hi - 1e-7) {
                        continue;
                    }
                    std::vector<std::vector<BaryVert>> next_hp;
                    for (const std::vector<BaryVert>& poly : h_pieces) {
                        std::vector<BaryVert> le;
                        std::vector<BaryVert> ge;
                        split_bary_poly_line(poly, s0, ds_da, ds_db, sc, le, ge);
                        if (le.size() >= 3) {
                            next_hp.push_back(std::move(le));
                        }
                        if (ge.size() >= 3) {
                            next_hp.push_back(std::move(ge));
                        }
                    }
                    h_pieces = std::move(next_hp);
                }

                for (const std::vector<BaryVert>& poly : h_pieces) {
                    std::vector<size_t> vids;
                    double s_sum = 0.0;
                    for (const BaryVert& bv : poly) {
                        const double l1 = bv.a;
                        const double l2 = bv.b;
                        const double l0 = 1.0 - l1 - l2;
                        s_sum += s0 + ds_da * l1 + ds_db * l2;

                        size_t vid = brick_pts.size();
                        if (l0 > 1.0 - 1e-5 || l1 > 1.0 - 1e-5 || l2 > 1.0 - 1e-5) {
                            if (l0 >= l1 && l0 >= l2) {
                                vid = st_tri.g_vids[0];
                            } else if (l1 >= l0 && l1 >= l2) {
                                vid = st_tri.g_vids[1];
                            } else {
                                vid = st_tri.g_vids[2];
                            }
                        } else {
                            size_t ke = 0;
                            double tl = l1;
                            if (l0 <= l1 && l0 <= l2) {
                                ke = 1;
                                tl = l2 / std::max(1e-12, l1 + l2);
                            } else if (l1 <= l0 && l1 <= l2) {
                                ke = 2;
                                tl = l0 / std::max(1e-12, l2 + l0);
                            } else {
                                ke = 0;
                                tl = l1 / std::max(1e-12, l0 + l1);
                            }
                            const size_t va = st_tri.g_vids[ke];
                            const size_t vb = st_tri.g_vids[(ke + 1) % 3];
                            const std::pair<size_t, size_t> ek = ordered_pair(va, vb);
                            const double sc_param = (ek.first == va) ? tl : (1.0 - tl);
                            const bool on_bnd = brick_v_bnd[va] && brick_v_bnd[vb];
                            bool found = false;
                            for (const std::pair<double, size_t>& ep : head_edge_pts[ek]) {
                                if (std::abs(ep.first - sc_param) < 1e-3) {
                                    vid = ep.second;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                vid = brick_pts.size();
                                head_edge_pts[ek].emplace_back(sc_param, vid);
                                const Point& ea = brick_pts[ek.first];
                                const Point& eb = brick_pts[ek.second];
                                brick_pts.emplace_back(
                                    (1.0 - sc_param) * ea[0] + sc_param * eb[0],
                                    (1.0 - sc_param) * ea[1] + sc_param * eb[1],
                                    (1.0 - sc_param) * ea[2] + sc_param * eb[2]
                                );
                                brick_v_bnd.push_back(on_bnd);
                                brick_v_sharp.push_back(false);
                            }
                        }
                        if (vids.empty() || vids.back() != vid) {
                            vids.push_back(vid);
                        }
                    }
                    if (vids.size() >= 2 && vids.front() == vids.back()) {
                        vids.pop_back();
                    }
                    if (vids.size() < 3) {
                        continue;
                    }
                    const double s_avg = s_sum / static_cast<double>(poly.size());
                    int slot = 0;
                    if (is_closed_ring) {
                        const double w_s = s_avg - std::floor(s_avg);
                        for (size_t ci = 0; ci < ring_fracs.size(); ci++) {
                            if (w_s >= ring_fracs[ci]) {
                                slot = static_cast<int>((ci + 1) % ring_fracs.size());
                            }
                        }
                    } else {
                        for (size_t ci = 0; ci < cut_stations.size(); ci++) {
                            if (s_avg > cut_stations[ci]) {
                                slot = static_cast<int>(ci + 1);
                            }
                        }
                    }
                    slot_polys[slot].push_back(std::move(vids));
                }
            }

            // Dissolve internal sub-edges within each brick slot to extract outer brick boundary loops
            for (const std::pair<const int, std::vector<std::vector<size_t>>>& skv : slot_polys) {
                std::set<std::pair<size_t, size_t>> hes;
                for (const std::vector<size_t>& lp : skv.second) {
                    const size_t pm = lp.size();
                    for (size_t k = 0; k < pm; k++) {
                        const std::pair<size_t, size_t> opp = {lp[(k + 1) % pm], lp[k]};
                        if (hes.count(opp)) {
                            hes.erase(opp);
                        } else {
                            hes.insert({lp[k], lp[(k + 1) % pm]});
                        }
                    }
                }
                std::map<size_t, std::vector<size_t>> out_adj;
                for (const std::pair<size_t, size_t>& he : hes) {
                    out_adj[he.first].push_back(he.second);
                }
                std::set<std::pair<size_t, size_t>> used_he;
                for (const std::pair<size_t, size_t>& start_he : hes) {
                    if (used_he.count(start_he)) {
                        continue;
                    }
                    std::vector<size_t> cyc;
                    std::pair<size_t, size_t> cur = start_he;
                    for (size_t step = 0; step <= hes.size(); step++) {
                        used_he.insert(cur);
                        cyc.push_back(cur.first);
                        const std::vector<size_t>& nbrs = out_adj[cur.second];
                        if (nbrs.empty()) {
                            cyc.clear();
                            break;
                        }
                        size_t nxt_v = nbrs[0];
                        for (const size_t cand : nbrs) {
                            if (!used_he.count({cur.second, cand})) {
                                nxt_v = cand;
                                break;
                            }
                        }
                        cur = {cur.second, nxt_v};
                        if (cur == start_he) {
                            break;
                        }
                    }
                    if (cyc.size() >= 3) {
                        raw_bricks.push_back(std::move(cyc));
                    }
                }
            }
        }
    }

    // Subdivide every uncut bed-isoline edge (u, w) in raw_bricks by all opposite-course head-joint splits!
    for (std::pair<const std::pair<size_t, size_t>, std::vector<std::pair<double, size_t>>>& kv : head_edge_pts) {
        std::sort(kv.second.begin(), kv.second.end());
    }
    for (std::vector<size_t>& rb : raw_bricks) {
        std::vector<size_t> sub_rb;
        for (size_t k = 0; k < rb.size(); k++) {
            const size_t u = rb[k];
            const size_t w = rb[(k + 1) % rb.size()];
            sub_rb.push_back(u);
            const std::pair<size_t, size_t> ek = ordered_pair(u, w);
            const std::map<std::pair<size_t, size_t>, std::vector<std::pair<double, size_t>>>::const_iterator it =
                head_edge_pts.find(ek);
            if (it != head_edge_pts.end() && !it->second.empty()) {
                if (u == ek.first) {
                    for (const std::pair<double, size_t>& sp : it->second) {
                        if (sp.second != u && sp.second != w && sub_rb.back() != sp.second) {
                            sub_rb.push_back(sp.second);
                        }
                    }
                } else {
                    for (int si = static_cast<int>(it->second.size()) - 1; si >= 0; si--) {
                        const size_t sv = it->second[static_cast<size_t>(si)].second;
                        if (sv != u && sv != w && sub_rb.back() != sv) {
                            sub_rb.push_back(sv);
                        }
                    }
                }
            }
        }
        rb = std::move(sub_rb);
    }

    // Weld close vertices across shared bed courses so head-joint T-vertices match identically across courses!
    std::vector<Point> w_pts;
    std::vector<bool> w_bnd;
    std::vector<bool> w_sharp;
    std::vector<size_t> pt_remap(brick_pts.size(), 0);
    const double weld_tol = std::max(2.0, 0.095 * h_bed);
    const double weld_sq = weld_tol * weld_tol;
    for (size_t i = 0; i < brick_pts.size(); i++) {
        size_t found = w_pts.size();
        for (size_t j = 0; j < w_pts.size(); j++) {
            if ((w_pts[j] - brick_pts[i]).magnitude_squared() <= weld_sq) {
                found = j;
                break;
            }
        }
        if (found == w_pts.size()) {
            w_pts.push_back(brick_pts[i]);
            w_bnd.push_back(brick_v_bnd[i]);
            w_sharp.push_back(brick_v_sharp[i]);
        } else {
            if (brick_v_bnd[i]) {
                w_bnd[found] = true;
            }
            if (brick_v_sharp[i]) {
                w_sharp[found] = true;
            }
        }
        pt_remap[i] = found;
    }

    std::vector<std::vector<size_t>> welded_bricks;
    for (const std::vector<size_t>& rb : raw_bricks) {
        std::vector<size_t> wb;
        for (const size_t idx : rb) {
            const size_t m = pt_remap[idx];
            if (wb.empty() || wb.back() != m) {
                wb.push_back(m);
            }
        }
        while (wb.size() >= 2 && wb.front() == wb.back()) {
            wb.pop_back();
        }
        if (wb.size() >= 3) {
            welded_bricks.push_back(std::move(wb));
        }
    }

    // Simplify degree-2 pass-through vertices along straight/smooth shared edges so each shared brick overlap
    // is a single clean planar side face while ALL T-joint & corner vertices (inc >= 3) stay 100% conformal!
    std::vector<std::set<size_t>> v_brick_inc(w_pts.size());
    std::vector<std::set<size_t>> v_edge_nbrs(w_pts.size());
    for (size_t bi = 0; bi < welded_bricks.size(); bi++) {
        const std::vector<size_t>& b = welded_bricks[bi];
        for (size_t k = 0; k < b.size(); k++) {
            v_brick_inc[b[k]].insert(bi);
            v_edge_nbrs[b[k]].insert(b[(k + 1) % b.size()]);
            v_edge_nbrs[b[(k + 1) % b.size()]].insert(b[k]);
        }
    }

    std::vector<std::vector<size_t>> clean_bricks;
    for (const std::vector<size_t>& b : welded_bricks) {
        std::vector<size_t> cb;
        for (size_t k = 0; k < b.size(); k++) {
            const size_t vid = b[k];
            const size_t n_inc = v_brick_inc[vid].size();
            const size_t deg = v_edge_nbrs[vid].size();
            if (w_sharp[vid] || n_inc >= 3 || deg != 2) {
                cb.push_back(vid);
                continue;
            }
            if (!w_bnd[vid] && n_inc == 2) {
                continue; // Interior pass-through vertex on shared bed/head joint -> simplify!
            }
            if (w_bnd[vid] && n_inc == 1) {
                const Point& prev_p = w_pts[b[(k + b.size() - 1) % b.size()]];
                const Point& next_p = w_pts[b[(k + 1) % b.size()]];
                Vector d0 = w_pts[vid] - prev_p;
                Vector d1 = next_p - w_pts[vid];
                if (d0.normalize_self() && d1.normalize_self() && d0.dot(d1) > 0.94) {
                    continue;
                }
            }
            cb.push_back(vid);
        }
        if (cb.size() >= 3) {
            clean_bricks.push_back(std::move(cb));
        }
    }

    // Insert any T-joint corner vertex that lies within 2.0 mm of another brick's simplified bed-joint chord
    std::set<size_t> active_corners;
    for (const std::vector<size_t>& b : clean_bricks) {
        for (const size_t vid : b) {
            active_corners.insert(vid);
        }
    }
    for (std::vector<size_t>& b : clean_bricks) {
        std::vector<size_t> expanded;
        for (size_t k = 0; k < b.size(); k++) {
            const size_t u = b[k];
            const size_t w = b[(k + 1) % b.size()];
            expanded.push_back(u);
            const Vector ab = w_pts[w] - w_pts[u];
            const double len2 = ab.magnitude_squared();
            if (len2 <= 4.0) {
                continue;
            }
            std::vector<std::pair<double, size_t>> mids;
            for (const size_t cv : active_corners) {
                if (std::find(b.begin(), b.end(), cv) != b.end() ||
                    std::find(expanded.begin(), expanded.end(), cv) != expanded.end()) {
                    continue;
                }
                const double t = (w_pts[cv] - w_pts[u]).dot(ab) / len2;
                if (t > 0.04 && t < 0.96) {
                    const Point proj_p = w_pts[u] + ab * t;
                    if ((w_pts[cv] - proj_p).magnitude_squared() < 9.0) {
                        mids.emplace_back(t, cv);
                    }
                }
            }
            std::sort(mids.begin(), mids.end());
            for (const std::pair<double, size_t>& mp : mids) {
                if (expanded.back() != mp.second) {
                    expanded.push_back(mp.second);
                }
            }
        }
        b = std::move(expanded);
    }

    // Merge any tiny sliver polygons (< 0.16 * h_bed^2) into their longest-shared-edge neighbor so zero holes appear!
    for (int merge_pass = 0; merge_pass < 16; merge_pass++) {
        bool merged_any = false;
        for (size_t bi = 0; bi < clean_bricks.size(); bi++) {
            const std::vector<size_t>& b = clean_bricks[bi];
            if (b.size() < 3) {
                continue;
            }
            Point ctr(0.0, 0.0, 0.0);
            for (const size_t vid : b) {
                ctr = ctr + Vector(w_pts[vid][0], w_pts[vid][1], w_pts[vid][2]);
            }
            ctr = Point(
                ctr[0] / static_cast<double>(b.size()),
                ctr[1] / static_cast<double>(b.size()),
                ctr[2] / static_cast<double>(b.size())
            );
            Vector avec(0.0, 0.0, 0.0);
            for (size_t k = 0; k < b.size(); k++) {
                avec = avec + (w_pts[b[k]] - ctr).cross(w_pts[b[(k + 1) % b.size()]] - ctr);
            }
            const double area_b = 0.5 * avec.magnitude();
            if (area_b >= 0.16 * h_bed * h_bed) {
                continue;
            }

            // Find neighbor brick bj sharing the longest edge with bi
            std::set<std::pair<size_t, size_t>> bi_edges;
            for (size_t k = 0; k < b.size(); k++) {
                bi_edges.insert(ordered_pair(b[k], b[(k + 1) % b.size()]));
            }
            size_t best_bj = clean_bricks.size();
            double best_shared_len = -1.0;
            for (size_t bj = 0; bj < clean_bricks.size(); bj++) {
                if (bj == bi || clean_bricks[bj].size() < 3) {
                    continue;
                }
                const std::vector<size_t>& nb = clean_bricks[bj];
                double sh_len = 0.0;
                for (size_t k = 0; k < nb.size(); k++) {
                    const std::pair<size_t, size_t> ek = ordered_pair(nb[k], nb[(k + 1) % nb.size()]);
                    if (bi_edges.count(ek)) {
                        sh_len += w_pts[ek.first].distance(w_pts[ek.second]);
                    }
                }
                if (sh_len > best_shared_len && sh_len > 1e-3) {
                    best_shared_len = sh_len;
                    best_bj = bj;
                }
            }
            if (best_bj == clean_bricks.size()) {
                continue;
            }

            // Dissolve shared half-edges between bi and best_bj
            std::set<std::pair<size_t, size_t>> hes;
            for (const size_t src : {bi, best_bj}) {
                const std::vector<size_t>& lp = clean_bricks[src];
                for (size_t k = 0; k < lp.size(); k++) {
                    const std::pair<size_t, size_t> opp = {lp[(k + 1) % lp.size()], lp[k]};
                    if (hes.count(opp)) {
                        hes.erase(opp);
                    } else {
                        hes.insert({lp[k], lp[(k + 1) % lp.size()]});
                    }
                }
            }
            std::map<size_t, size_t> nxt;
            bool simple_loop = (hes.size() >= 3);
            for (const std::pair<size_t, size_t>& he : hes) {
                if (nxt.count(he.first)) {
                    simple_loop = false;
                    break;
                }
                nxt[he.first] = he.second;
            }
            if (!simple_loop) {
                continue;
            }
            std::vector<size_t> cyc;
            size_t cur_v = hes.begin()->first;
            for (size_t s = 0; s < hes.size(); s++) {
                cyc.push_back(cur_v);
                if (!nxt.count(cur_v)) {
                    simple_loop = false;
                    break;
                }
                cur_v = nxt[cur_v];
            }
            if (simple_loop && cur_v == hes.begin()->first && cyc.size() == hes.size()) {
                clean_bricks[best_bj] = std::move(cyc);
                clean_bricks[bi].clear();
                merged_any = true;
                break;
            }
        }
        if (!merged_any) {
            break;
        }
    }

    std::vector<std::vector<Point>> out_rings;
    std::vector<std::vector<Vector>> out_norms;
    for (const std::vector<size_t>& b : clean_bricks) {
        if (b.size() < 3) {
            continue;
        }
        Point ctr(0.0, 0.0, 0.0);
        for (const size_t vid : b) {
            ctr = ctr + Vector(w_pts[vid][0], w_pts[vid][1], w_pts[vid][2]);
        }
        ctr = Point(
            ctr[0] / static_cast<double>(b.size()),
            ctr[1] / static_cast<double>(b.size()),
            ctr[2] / static_cast<double>(b.size())
        );
        Vector avec(0.0, 0.0, 0.0);
        for (size_t k = 0; k < b.size(); k++) {
            avec = avec + (w_pts[b[k]] - ctr).cross(w_pts[b[(k + 1) % b.size()]] - ctr);
        }
        if (0.5 * avec.magnitude() < 0.01 * h_bed * h_bed) {
            continue;
        }
        std::vector<Point> r;
        std::vector<Vector> rn;
        for (const size_t vid : b) {
            const std::pair<Point, Vector> pr = projector.project(w_pts[vid]);
            r.push_back(w_bnd[vid] ? w_pts[vid] : pr.first);
            rn.push_back(pr.second);
        }
        out_rings.push_back(std::move(r));
        out_norms.push_back(std::move(rn));
    }

    return weld_polygon_mesh(out_rings, out_norms, 1e-2);
}

std::vector<std::shared_ptr<Element>> tile_vault(
    const TileVaultGuide& guide,
    TilePatternMetrics* metrics,
    Mesh* out_quad_mesh,
    Mesh* out_brick_mesh
) {

    TilePatternMetrics local_m;
    Mesh reg_quads;
    BrickMatchingResult matching;
    Mesh brick_mesh;

    if (guide.pattern == TileVaultGuide::GEODESIC_COURSES) {
        brick_mesh = geodesic_brick_pattern(
            guide.shell,
            guide.tile_width,
            2.0 * guide.tile_width,
            guide.polar_geodesics
        );
        reg_quads = brick_mesh;
        const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> gvf =
            brick_mesh.to_vertices_and_faces();
        local_m.quad_faces = gvf.second.size();
        for (size_t fi = 0; fi < gvf.second.size(); fi++) {
            matching.unmatched_faces.push_back(fi);
            double max_diag = 0.0;
            for (size_t a = 0; a < gvf.second[fi].size(); a++) {
                for (size_t b = a + 1; b < gvf.second[fi].size(); b++) {
                    max_diag = std::max(
                        max_diag,
                        gvf.first[gvf.second[fi][a]].distance(gvf.first[gvf.second[fi][b]])
                    );
                }
            }
            if (max_diag >= 1.55 * guide.tile_width) {
                local_m.full_bricks++;
            } else {
                local_m.half_tiles++;
            }
        }
        // Count staggered T-joint running-bond contacts (shared edges at degree-3 T-vertices)
        std::vector<size_t> v_deg(gvf.first.size(), 0);
        for (const std::vector<size_t>& f : gvf.second) {
            for (const size_t vid : f) {
                v_deg[vid]++;
            }
        }
        for (const size_t d : v_deg) {
            if (d == 3) {
                local_m.running_pairs++;
            }
        }
    } else {
        const Mesh raw_quads = quadriflow_remesh(
            guide.shell,
            guide.tile_width,
            guide.orientation_degrees,
            guide.align_boundary
        );

        double dummy_max_e = 0.0;
        double dummy_max_a = 0.0;
        evaluate_quad_distortion(
            raw_quads,
            guide.tile_width,
            local_m.raw_edge_error,
            dummy_max_e,
            local_m.raw_angle_error_deg,
            dummy_max_a,
            local_m.singularities
        );

        reg_quads = raw_quads;

        // Two-stage surface-guided ShapeUp regularization keeping squares tight to the curved shell (< 12 mm drift)
        if (guide.shapeup_iterations > 0) {
            std::vector<Point> sv;
            std::vector<std::array<size_t, 3>> st;
            extract_welded_triangles(guide.shell, sv, st, 1e-2);
            if (!sv.empty() && !st.empty()) {
                std::vector<Vector> sn;
                std::vector<double> sa;
                compute_normals_and_areas(sv, st, sn, sa);
                SurfaceProjector sproj;
                sproj.build(sv, sn, st);

                std::map<std::pair<size_t, size_t>, int> sh_uses;
                for (const std::array<size_t, 3>& t : st) {
                    for (size_t k = 0; k < 3; k++) {
                        sh_uses[ordered_pair(t[k], t[(k + 1) % 3])]++;
                    }
                }
                std::vector<std::pair<Point, Point>> bnd_segs;
                for (const std::pair<const std::pair<size_t, size_t>, int>& kv : sh_uses) {
                    if (kv.second == 1) {
                        bnd_segs.emplace_back(sv[kv.first.first], sv[kv.first.second]);
                    }
                }

                const int half_iters = std::max(10, guide.shapeup_iterations / 2);
                for (int stage = 0; stage < 3; stage++) {
                    const int iters_s = (stage < 2) ? half_iters : 10;
                    const double cw_s = (stage == 0)
                                            ? guide.shapeup_closeness
                                            : ((stage == 1) ? std::max(0.16, guide.shapeup_closeness * 2.5)
                                                            : std::max(0.35, guide.shapeup_closeness * 6.0));
                    reg_quads = shapeup_regularize(
                        reg_quads,
                        guide.tile_width,
                        iters_s,
                        1.0,
                        cw_s,
                        guide.shapeup_smoothness
                    );
                    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> rvf =
                        reg_quads.to_vertices_and_faces();
                    std::map<std::pair<size_t, size_t>, int> q_uses;
                    for (const std::vector<size_t>& f : rvf.second) {
                        for (size_t k = 0; k < f.size(); k++) {
                            q_uses[ordered_pair(f[k], f[(k + 1) % f.size()])]++;
                        }
                    }
                    std::vector<bool> q_bnd(rvf.first.size(), false);
                    for (const std::pair<const std::pair<size_t, size_t>, int>& kv : q_uses) {
                        if (kv.second == 1) {
                            q_bnd[kv.first.first] = true;
                            q_bnd[kv.first.second] = true;
                        }
                    }
                    const double pull_alpha = (stage < 2) ? 0.85 : 0.65;
                    std::vector<Point> pulled = rvf.first;
                    for (size_t vi = 0; vi < pulled.size(); vi++) {
                        if (q_bnd[vi] && !bnd_segs.empty()) {
                            Point best_bp = pulled[vi];
                            double best_d2 = 1e30;
                            for (const std::pair<Point, Point>& seg : bnd_segs) {
                                const Point cp = closest_point_on_segment(pulled[vi], seg.first, seg.second);
                                const double d2 = (cp - pulled[vi]).magnitude_squared();
                                if (d2 < best_d2) {
                                    best_d2 = d2;
                                    best_bp = cp;
                                }
                            }
                            if (best_d2 < (0.45 * guide.tile_width) * (0.45 * guide.tile_width)) {
                                pulled[vi] = pulled[vi] + (best_bp - pulled[vi]) * (0.75 * pull_alpha);
                            }
                        } else {
                            const std::pair<Point, Vector> pr = sproj.project(pulled[vi]);
                            const double normal_dist = (pr.first - pulled[vi]).dot(pr.second);
                            pulled[vi] = pulled[vi] + pr.second * (normal_dist * pull_alpha);
                        }
                    }
                    std::vector<std::vector<Vector>> pnorms(rvf.second.size());
                    std::vector<std::vector<Point>> prings(rvf.second.size());
                    for (size_t fi = 0; fi < rvf.second.size(); fi++) {
                        for (const size_t vid : rvf.second[fi]) {
                            prings[fi].push_back(pulled[vid]);
                            pnorms[fi].push_back(sproj.project(pulled[vid]).second);
                        }
                    }
                    reg_quads = weld_polygon_mesh(prings, pnorms, 1e-2);
                }
            } else {
                reg_quads = shapeup_regularize(
                    raw_quads,
                    guide.tile_width,
                    guide.shapeup_iterations,
                    1.0,
                    guide.shapeup_closeness,
                    guide.shapeup_smoothness
                );
            }
        }

        evaluate_quad_distortion(
            reg_quads,
            guide.tile_width,
            local_m.mean_edge_error,
            local_m.max_edge_error,
            local_m.mean_angle_error_deg,
            local_m.max_angle_error_deg,
            local_m.singularities
        );

        matching = match_brick_tiles(
            reg_quads,
            guide.pattern,
            guide.penalty_weight,
            guide.reward_weight,
            guide.border_weight
        );
        local_m.full_bricks = matching.matched_pairs.size();
        local_m.half_tiles = matching.unmatched_faces.size();
        local_m.unmatched_interior = matching.unmatched_interior;
        local_m.parallel_pairs = matching.parallel_pairs;
        local_m.running_pairs = matching.running_pairs;
        local_m.herringbone_pairs = matching.herringbone_pairs;
        local_m.bip_objective = matching.objective;

        brick_mesh = merge_matched_brick_mesh(reg_quads, matching);
    }

    // Measure surface drift from vertices to input shell
    {
        std::vector<Point> sv;
        std::vector<std::array<size_t, 3>> st;
        extract_welded_triangles(guide.shell, sv, st, 1e-2);
        std::vector<Vector> sn;
        std::vector<double> sa;
        compute_normals_and_areas(sv, st, sn, sa);
        SurfaceProjector proj;
        proj.build(sv, sn, st);

        const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> qvf =
            reg_quads.to_vertices_and_faces();
        local_m.quad_faces = qvf.second.size();
        double sum_drift = 0.0;
        for (const Point& pt : qvf.first) {
            double d = 0.0;
            proj.project(pt, &d);
            sum_drift += d;
            local_m.max_surface_drift = std::max(local_m.max_surface_drift, d);
        }
        if (!qvf.first.empty()) {
            local_m.mean_surface_drift = sum_drift / static_cast<double>(qvf.first.size());
        }
    }

    if (out_quad_mesh) {
        *out_quad_mesh = reg_quads;
    }
    if (out_brick_mesh) {
        *out_brick_mesh = brick_mesh;
    }

    std::vector<std::shared_ptr<Element>> elements;

    if (guide.materialisation == TileVaultGuide::STANDARD_TILES) {
        const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> qvf =
            reg_quads.to_vertices_and_faces();
        const std::vector<Point>& verts = qvf.first;
        const std::vector<std::vector<size_t>>& faces = qvf.second;
        const std::vector<size_t> keys = reg_quads.vertices();

        std::vector<Vector> vnorms(verts.size(), Vector(0.0, 0.0, 1.0));
        for (size_t i = 0; i < verts.size(); i++) {
            const std::optional<std::array<double, 3>> attr = reg_quads.vertex.at(keys[i]).normal();
            if (attr) {
                Vector sn((*attr)[0], (*attr)[1], (*attr)[2]);
                if (sn.normalize_self()) {
                    vnorms[i] = sn;
                }
            }
        }

        const double gap = std::clamp(guide.mortar_gap, 0.0, 0.35 * guide.tile_width);
        const double full_len = 2.0 * guide.tile_width - gap;
        const double full_wid = guide.tile_width - gap;

        for (size_t bi = 0; bi < matching.matched_pairs.size(); bi++) {
            const size_t fa = matching.matched_pairs[bi].first;
            const size_t fb = matching.matched_pairs[bi].second;
            std::vector<Point> ring_pts;
            Vector up_ref(0.0, 0.0, 0.0);
            Point ca(0.0, 0.0, 0.0);
            Point cb(0.0, 0.0, 0.0);
            for (const size_t vid : faces[fa]) {
                ring_pts.push_back(verts[vid]);
                up_ref = up_ref + vnorms[vid];
                ca = ca + Vector(verts[vid][0], verts[vid][1], verts[vid][2]);
            }
            ca = Point(
                ca[0] / static_cast<double>(faces[fa].size()),
                ca[1] / static_cast<double>(faces[fa].size()),
                ca[2] / static_cast<double>(faces[fa].size())
            );
            for (const size_t vid : faces[fb]) {
                ring_pts.push_back(verts[vid]);
                up_ref = up_ref + vnorms[vid];
                cb = cb + Vector(verts[vid][0], verts[vid][1], verts[vid][2]);
            }
            cb = Point(
                cb[0] / static_cast<double>(faces[fb].size()),
                cb[1] / static_cast<double>(faces[fb].size()),
                cb[2] / static_cast<double>(faces[fb].size())
            );

            const Plane pca = Plane::from_points_pca(ring_pts);
            Vector ez = pca.z_axis();
            if (ez.dot(up_ref) < 0.0) {
                ez = -ez;
            }
            Vector ex = (cb - ca) - ez * (cb - ca).dot(ez);
            if (!ex.normalize_self()) {
                ex = pca.x_axis();
            }
            const Vector ey = ez.cross(ex).normalized();
            const Point mid = Point::mid_point(ca, cb);

            const std::shared_ptr<NodeBlockElement> tile = make_box_tile_element(
                mid,
                ex,
                ey,
                ez,
                full_len,
                full_wid,
                guide.thickness,
                guide.trim_base,
                guide.base_z,
                fmt::format("tile_{}", elements.size())
            );
            if (tile) {
                elements.push_back(tile);
            }
        }

        // Trimmed boundary / singularity half-tiles following their quad footprint shrunk by 0.5 * gap
        for (const size_t fu : matching.unmatched_faces) {
            const std::vector<size_t>& f = faces[fu];
            if (f.size() < 3) {
                continue;
            }
            std::vector<Point> raw_corners;
            Vector up_ref(0.0, 0.0, 0.0);
            for (const size_t vid : f) {
                raw_corners.push_back(verts[vid]);
                up_ref = up_ref + vnorms[vid];
            }
            const Point centre = Point::centroid(raw_corners);
            const Plane pca = Plane::from_points_pca(raw_corners);
            Vector ez = pca.z_axis();
            if (ez.dot(up_ref) < 0.0) {
                ez = -ez;
            }

            std::vector<Point> bot;
            std::vector<Point> top;
            for (const Point& pt : raw_corners) {
                Vector radial = (pt - centre) - ez * (pt - centre).dot(ez);
                const double rlen = radial.magnitude();
                const double scale = (rlen > gap) ? ((rlen - 0.5 * gap) / rlen) : 0.85;
                const Point p_plane = centre + radial * scale;
                bot.push_back(p_plane - ez * (0.5 * guide.thickness));
                top.push_back(p_plane + ez * (0.5 * guide.thickness));
            }
            std::vector<Point> hull_pts = bot;
            hull_pts.insert(hull_pts.end(), top.begin(), top.end());
            Mesh solid = solid_hull(hull_pts);
            if (guide.trim_base) {
                solid = cut_mesh(
                    solid,
                    {Plane(Point(0.0, 0.0, guide.base_z), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0))}
                );
            }
            if (solid.number_of_faces() >= 4 && manifold_solid(solid)) {
                elements.push_back(
                    std::make_shared<NodeBlockElement>(
                        solid, fmt::format("half_tile_{}", elements.size())
                    )
                );
            }
        }
    } else if (guide.pattern == TileVaultGuide::GEODESIC_COURSES) {
        // Extrude conformal T-jointed geodesic-coordinate brick courses via exact planarized ngon_solids
        NgonSolidMetrics ngon_m;
        const std::vector<std::shared_ptr<NodeBlockElement>> solids = ngon_solids(
            brick_mesh,
            guide.thickness,
            guide.planar_sides,
            1.0,
            Vector(0.0, 0.0, 0.0),
            0.0,
            guide.trim_base,
            guide.base_z,
            &ngon_m
        );
        for (const std::shared_ptr<NodeBlockElement>& blk : solids) {
            if (!blk) {
                continue;
            }
            Mesh gm = blk->model_geometry_mesh();
            if (manifold_solid(gm)) {
                elements.push_back(blk);
                continue;
            }
            Mesh wm = gm.weld(0.1);
            if (wm.number_of_faces() >= 4 && manifold_solid(wm)) {
                elements.push_back(std::make_shared<NodeBlockElement>(wm, blk->name));
            }
        }
        resolve_planar_block_overlaps(elements);
        local_m.side_planarity = ngon_m.side_planarity;
        local_m.cap_planarity = ngon_m.cap_planarity;
    } else {
        // Exact shared-plane halfspace stereotomic bricks: zero overlaps, watertight Manifold solids, exact planar faces
        const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> qvf =
            reg_quads.to_vertices_and_faces();
        const std::vector<Point>& verts = qvf.first;
        std::vector<std::vector<size_t>> faces = qvf.second;
        const std::vector<size_t> keys = reg_quads.vertices();
        const size_t nv = verts.size();

        std::vector<Vector> vnorms(nv, Vector(0.0, 0.0, 1.0));
        for (size_t i = 0; i < nv; i++) {
            const std::optional<std::array<double, 3>> attr = reg_quads.vertex.at(keys[i]).normal();
            if (attr) {
                Vector sn((*attr)[0], (*attr)[1], (*attr)[2]);
                if (sn.normalize_self()) {
                    vnorms[i] = sn;
                }
            }
        }

        // Ensure every quad is wound CCW with respect to vnorms
        std::vector<std::set<size_t>> v_adj(nv);
        for (std::vector<size_t>& f : faces) {
            if (f.size() < 3) {
                continue;
            }
            Vector avg_n(0.0, 0.0, 0.0);
            Point fc(0.0, 0.0, 0.0);
            for (const size_t vid : f) {
                avg_n = avg_n + vnorms[vid];
                fc = fc + Vector(verts[vid][0], verts[vid][1], verts[vid][2]);
            }
            fc = Point(
                fc[0] / static_cast<double>(f.size()),
                fc[1] / static_cast<double>(f.size()),
                fc[2] / static_cast<double>(f.size())
            );
            Vector area_n(0.0, 0.0, 0.0);
            for (size_t k = 0; k < f.size(); k++) {
                area_n = area_n + (verts[f[k]] - fc).cross(verts[f[(k + 1) % f.size()]] - fc);
                v_adj[f[k]].insert(f[(k + 1) % f.size()]);
                v_adj[f[(k + 1) % f.size()]].insert(f[k]);
            }
            if (area_n.dot(avg_n) < 0.0) {
                std::reverse(f.begin(), f.end());
            }
        }

        // Planarize vertex rulings on the square quad mesh (pinning degree-2 sharp corner tips) so all edge planes concur along vertex rulings!
        std::vector<Point> ruling_pts(2 * nv);
        std::vector<bool> pinned(2 * nv, false);
        const double ht = 0.5 * guide.thickness;
        for (size_t i = 0; i < nv; i++) {
            ruling_pts[i] = verts[i] - vnorms[i] * ht;
            ruling_pts[nv + i] = verts[i] + vnorms[i] * ht;
            if (v_adj[i].size() <= 2) {
                pinned[i] = true;
                pinned[nv + i] = true;
            }
        }
        std::set<std::pair<size_t, size_t>> seen_qedges;
        std::vector<std::vector<size_t>> qside_quads;
        for (const std::vector<size_t>& f : faces) {
            for (size_t k = 0; k < f.size(); k++) {
                const std::pair<size_t, size_t> ek = ordered_pair(f[k], f[(k + 1) % f.size()]);
                if (seen_qedges.insert(ek).second) {
                    qside_quads.push_back({ek.first, ek.second, nv + ek.second, nv + ek.first});
                }
            }
        }
        if (guide.planar_sides && !qside_quads.empty()) {
            ruling_pts = planarized(ruling_pts, qside_quads, 1e-8, pinned);
        }

        // Canonical shared separating plane per undirected quad edge (ek.first -> ek.second outward normal)
        std::map<std::pair<size_t, size_t>, std::pair<Point, Vector>> edge_canon;
        for (const std::pair<size_t, size_t>& ek : seen_qedges) {
            const size_t u = ek.first;
            const size_t w = ek.second;
            const std::vector<Point> sq = {
                ruling_pts[u], ruling_pts[w], ruling_pts[nv + w], ruling_pts[nv + u]
            };
            const Plane sp = Plane::from_points_pca(sq);
            const Vector ref_out = (verts[w] - verts[u]).cross(vnorms[u] + vnorms[w]);
            Vector out_uw = sp.z_axis();
            if (out_uw.dot(ref_out) < 0.0) {
                out_uw = -out_uw;
            }
            edge_canon[ek] = {sp.origin(), out_uw};
        }

        const Plane base_cut_plane(
            Point(0.0, 0.0, guide.base_z), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)
        );
        const double box_span = 6.0 * guide.tile_width;
        const double box_thick = 3.5 * guide.thickness;

        // 1. Matched 2-quad full bricks: cut convex half-bricks Ha and Hb against shared outer planes & union seamlessly
        for (size_t bi = 0; bi < matching.matched_pairs.size(); bi++) {
            const size_t fa = matching.matched_pairs[bi].first;
            const size_t fb = matching.matched_pairs[bi].second;
            if (faces[fa].size() < 3 || faces[fb].size() < 3) {
                continue;
            }

            std::vector<Point> brick_pts;
            Vector up_ref(0.0, 0.0, 0.0);
            Point ca(0.0, 0.0, 0.0);
            Point cb(0.0, 0.0, 0.0);
            for (const size_t vid : faces[fa]) {
                brick_pts.push_back(verts[vid]);
                up_ref = up_ref + vnorms[vid];
                ca = ca + Vector(verts[vid][0], verts[vid][1], verts[vid][2]);
            }
            ca = Point(
                ca[0] / static_cast<double>(faces[fa].size()),
                ca[1] / static_cast<double>(faces[fa].size()),
                ca[2] / static_cast<double>(faces[fa].size())
            );
            for (const size_t vid : faces[fb]) {
                brick_pts.push_back(verts[vid]);
                up_ref = up_ref + vnorms[vid];
                cb = cb + Vector(verts[vid][0], verts[vid][1], verts[vid][2]);
            }
            cb = Point(
                cb[0] / static_cast<double>(faces[fb].size()),
                cb[1] / static_cast<double>(faces[fb].size()),
                cb[2] / static_cast<double>(faces[fb].size())
            );

            const Plane pca = Plane::from_points_pca(brick_pts);
            Vector ez = pca.z_axis();
            if (ez.dot(up_ref) < 0.0) {
                ez = -ez;
            }
            const Vector ex = (ez.dot(pca.x_axis()) < 0.9)
                                  ? (pca.x_axis() - ez * pca.x_axis().dot(ez)).normalized()
                                  : pca.y_axis();
            const Vector ey = ez.cross(ex).normalized();
            const Point mid = Point::centroid(brick_pts);
            const Plane bot_cap = plane_with_normal(mid - ez * ht, ez);
            const Plane top_cap = plane_with_normal(mid + ez * ht, -ez);

            // Find the dissolved shared edge between fa and fb
            std::set<std::pair<size_t, size_t>> fa_edges;
            for (size_t k = 0; k < faces[fa].size(); k++) {
                fa_edges.insert(ordered_pair(faces[fa][k], faces[fa][(k + 1) % faces[fa].size()]));
            }
            std::pair<size_t, size_t> shared_ek = {0, 0};
            bool found_shared = false;
            for (size_t k = 0; k < faces[fb].size(); k++) {
                const std::pair<size_t, size_t> ek =
                    ordered_pair(faces[fb][k], faces[fb][(k + 1) % faces[fb].size()]);
                if (fa_edges.count(ek)) {
                    shared_ek = ek;
                    found_shared = true;
                    break;
                }
            }
            if (!found_shared) {
                continue;
            }

            std::vector<Plane> outer_a;
            std::vector<Plane> outer_b;
            std::vector<Plane> cuts_a = {bot_cap, top_cap};
            std::vector<Plane> cuts_b = {bot_cap, top_cap};
            if (guide.trim_base) {
                cuts_a.push_back(base_cut_plane);
                cuts_b.push_back(base_cut_plane);
            }

            for (size_t k = 0; k < faces[fa].size(); k++) {
                const size_t u = faces[fa][k];
                const size_t w = faces[fa][(k + 1) % faces[fa].size()];
                const std::pair<size_t, size_t> ek = ordered_pair(u, w);
                const std::pair<Point, Vector>& ec = edge_canon[ek];
                const Vector out_n = (u < w) ? ec.second : -ec.second;
                const Vector in_n = -out_n;
                if (ek == shared_ek) {
                    cuts_a.push_back(plane_with_normal(ec.first + out_n * 0.02, in_n));
                } else {
                    const Plane pl = plane_with_normal(ec.first, in_n);
                    cuts_a.push_back(pl);
                    outer_a.push_back(pl);
                }
            }
            for (size_t k = 0; k < faces[fb].size(); k++) {
                const size_t u = faces[fb][k];
                const size_t w = faces[fb][(k + 1) % faces[fb].size()];
                const std::pair<size_t, size_t> ek = ordered_pair(u, w);
                const std::pair<Point, Vector>& ec = edge_canon[ek];
                const Vector out_n = (u < w) ? ec.second : -ec.second;
                const Vector in_n = -out_n;
                if (ek == shared_ek) {
                    cuts_b.push_back(plane_with_normal(ec.first + out_n * 0.02, in_n));
                } else {
                    const Plane pl = plane_with_normal(ec.first, in_n);
                    cuts_b.push_back(pl);
                    outer_b.push_back(pl);
                }
            }
            // Clip the 0.02 mm seam extension flush against convex outer neighbor planes
            for (const Plane& pb : outer_b) {
                if ((ca - pb.origin()).dot(pb.z_axis()) > 0.08 * guide.tile_width) {
                    cuts_a.push_back(pb);
                }
            }
            for (const Plane& pa : outer_a) {
                if ((cb - pa.origin()).dot(pa.z_axis()) > 0.08 * guide.tile_width) {
                    cuts_b.push_back(pa);
                }
            }

            const std::shared_ptr<NodeBlockElement> box_a = make_box_tile_element(
                ca, ex, ey, ez, box_span, box_span, box_thick, false, 0.0, "tmp_a"
            );
            const std::shared_ptr<NodeBlockElement> box_b = make_box_tile_element(
                cb, ex, ey, ez, box_span, box_span, box_thick, false, 0.0, "tmp_b"
            );
            if (!box_a || !box_b) {
                continue;
            }
            const Mesh ha = cut_mesh(box_a->element_geometry_mesh(), cuts_a);
            const Mesh hb = cut_mesh(box_b->element_geometry_mesh(), cuts_b);
            Mesh solid;
            bool union_ok = false;
            if (ha.number_of_faces() >= 4 && hb.number_of_faces() >= 4 &&
                manifold_solid(ha) && manifold_solid(hb)) {
                try {
                    solid = solid_boolean(ha, hb, SolidOperation::add);
                    union_ok = (solid.number_of_faces() >= 4 && manifold_solid(solid));
                } catch (const std::exception&) {
                    union_ok = false;
                }
            }
            if (!union_ok) {
                std::vector<Plane> cuts_union = {bot_cap, top_cap};
                if (guide.trim_base) {
                    cuts_union.push_back(base_cut_plane);
                }
                cuts_union.insert(cuts_union.end(), outer_a.begin(), outer_a.end());
                cuts_union.insert(cuts_union.end(), outer_b.begin(), outer_b.end());
                const std::shared_ptr<NodeBlockElement> box_m = make_box_tile_element(
                    mid, ex, ey, ez, box_span, box_span, box_thick, false, 0.0, "tmp_m"
                );
                if (box_m) {
                    solid = cut_mesh(box_m->element_geometry_mesh(), cuts_union);
                }
            }
            if (solid.number_of_faces() >= 4 && manifold_solid(solid) &&
                compute_volume(solid) > 100.0) {
                elements.push_back(
                    std::make_shared<NodeBlockElement>(
                        solid, fmt::format("voussoir_{}", elements.size())
                    )
                );
            }
        }

        // 2. Unmatched single-quad half-tiles bounded by the exact same shared edge planes
        for (const size_t fu : matching.unmatched_faces) {
            if (faces[fu].size() < 3) {
                continue;
            }
            std::vector<Point> q_pts_u;
            Vector up_ref(0.0, 0.0, 0.0);
            for (const size_t vid : faces[fu]) {
                q_pts_u.push_back(verts[vid]);
                up_ref = up_ref + vnorms[vid];
            }
            const Point cu = Point::centroid(q_pts_u);
            const Plane pca = Plane::from_points_pca(q_pts_u);
            Vector ez = pca.z_axis();
            if (ez.dot(up_ref) < 0.0) {
                ez = -ez;
            }
            const Vector ex = (ez.dot(pca.x_axis()) < 0.9)
                                  ? (pca.x_axis() - ez * pca.x_axis().dot(ez)).normalized()
                                  : pca.y_axis();
            const Vector ey = ez.cross(ex).normalized();
            const Plane bot_cap = plane_with_normal(cu - ez * ht, ez);
            const Plane top_cap = plane_with_normal(cu + ez * ht, -ez);

            std::vector<Plane> cuts_u = {bot_cap, top_cap};
            if (guide.trim_base) {
                cuts_u.push_back(base_cut_plane);
            }
            for (size_t k = 0; k < faces[fu].size(); k++) {
                const size_t u = faces[fu][k];
                const size_t w = faces[fu][(k + 1) % faces[fu].size()];
                const std::pair<size_t, size_t> ek = ordered_pair(u, w);
                const std::pair<Point, Vector>& ec = edge_canon[ek];
                const Vector out_n = (u < w) ? ec.second : -ec.second;
                cuts_u.push_back(plane_with_normal(ec.first, -out_n));
            }
            const std::shared_ptr<NodeBlockElement> box_u = make_box_tile_element(
                cu, ex, ey, ez, box_span, box_span, box_thick, false, 0.0, "tmp_u"
            );
            if (!box_u) {
                continue;
            }
            const Mesh solid = cut_mesh(box_u->element_geometry_mesh(), cuts_u);
            if (solid.number_of_faces() >= 4 && manifold_solid(solid) &&
                compute_volume(solid) > 100.0) {
                elements.push_back(
                    std::make_shared<NodeBlockElement>(
                        solid, fmt::format("voussoir_{}", elements.size())
                    )
                );
            }
        }

        resolve_planar_block_overlaps(elements);

        // Measure exact face planarity across all generated block outlines
        for (const std::shared_ptr<Element>& el : elements) {
            for (const Polyline& outline : el->model_geometry_mesh().face_outlines()) {
                std::vector<Point> pts_o = outline.get_points();
                if (pts_o.size() >= 5) {
                    pts_o.pop_back();
                    const Plane fp = Plane::from_points_pca(pts_o);
                    for (const Point& qp : pts_o) {
                        const double res = std::abs((qp - fp.origin()).dot(fp.z_axis()));
                        local_m.side_planarity = std::max(local_m.side_planarity, res);
                        local_m.cap_planarity = std::max(local_m.cap_planarity, res);
                    }
                }
            }
        }
    }

    if (metrics) {
        *metrics = local_m;
    }

    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// TileVault Session Constructor & Accessors
// ═══════════════════════════════════════════════════════════════════════════

TileVault::TileVault(const TileVaultGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    const std::vector<std::shared_ptr<Element>> pieces =
        tile_vault(guide, &_metrics, &_quad_mesh, &_brick_mesh);
    for (const std::shared_ptr<Element>& piece : pieces) {
        add(piece);
    }

    compute_face_contacts(0);
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

const Mesh& TileVault::quad_mesh() const {
    return _quad_mesh;
}

const Mesh& TileVault::brick_mesh() const {
    return _brick_mesh;
}

const TilePatternMetrics& TileVault::metrics() const {
    return _metrics;
}

// ═══════════════════════════════════════════════════════════════════════════
// Calgary 2025 Dual & Variable-Thickness Shell Materialisation Helpers
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// Interpolates scalar values at (qx, qy) over 2D control points via exact Delaunay empty-circumcircle triangulation.
double interpolate_delaunay_xy(
    const std::vector<std::array<double, 3>>& ctrl_xyv,
    double qx,
    double qy
) {

    if (ctrl_xyv.empty()) {
        return 150.0;
    }
    if (ctrl_xyv.size() < 3) {
        return ctrl_xyv[0][2];
    }

    // Deduplicate close 2D control sites
    std::vector<std::array<double, 3>> sites;
    for (const std::array<double, 3>& c : ctrl_xyv) {
        bool dup = false;
        for (std::array<double, 3>& s : sites) {
            const double dx = c[0] - s[0];
            const double dy = c[1] - s[1];
            if (dx * dx + dy * dy < 25.0) {
                s[2] = 0.5 * (s[2] + c[2]);
                dup = true;
                break;
            }
        }
        if (!dup) {
            sites.push_back(c);
        }
    }

    const size_t ns = sites.size();
    for (size_t i = 0; i < ns; i++) {
        for (size_t j = i + 1; j < ns; j++) {
            for (size_t k = j + 1; k < ns; k++) {
                const double x0 = sites[i][0];
                const double y0 = sites[i][1];
                const double x1 = sites[j][0];
                const double y1 = sites[j][1];
                const double x2 = sites[k][0];
                const double y2 = sites[k][1];
                const double area2 = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
                if (std::abs(area2) <= 1e-3) {
                    continue;
                }

                // Barycentric coordinates of (qx, qy)
                const double inv = 1.0 / area2;
                const double b1 = ((qx - x0) * (y2 - y0) - (x2 - x0) * (qy - y0)) * inv;
                const double b2 = ((x1 - x0) * (qy - y0) - (qx - x0) * (y1 - y0)) * inv;
                const double b0 = 1.0 - b1 - b2;
                if (b0 < -1e-6 || b1 < -1e-6 || b2 < -1e-6) {
                    continue;
                }

                // Check empty circumcircle property for exact Delaunay triangle (i, j, k)
                const double d = 2.0 * (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1));
                if (std::abs(d) <= 1e-6) {
                    continue;
                }
                const double r0 = x0 * x0 + y0 * y0;
                const double r1 = x1 * x1 + y1 * y1;
                const double r2 = x2 * x2 + y2 * y2;
                const double ux = (r0 * (y1 - y2) + r1 * (y2 - y0) + r2 * (y0 - y1)) / d;
                const double uy = (r0 * (x2 - x1) + r1 * (x0 - x2) + r2 * (x1 - x0)) / d;
                const double rad2 = (x0 - ux) * (x0 - ux) + (y0 - uy) * (y0 - uy);

                bool empty_circle = true;
                for (size_t m = 0; m < ns; m++) {
                    if (m == i || m == j || m == k) {
                        continue;
                    }
                    const double dm2 =
                        (sites[m][0] - ux) * (sites[m][0] - ux) + (sites[m][1] - uy) * (sites[m][1] - uy);
                    if (dm2 < rad2 - 1e-3) {
                        empty_circle = false;
                        break;
                    }
                }
                if (empty_circle) {
                    return b0 * sites[i][2] + b1 * sites[j][2] + b2 * sites[k][2];
                }
            }
        }
    }

    // Smooth inverse-distance fallback for boundary points slightly outside the 2D convex hull
    std::vector<std::pair<double, double>> near_vals;
    for (const std::array<double, 3>& s : sites) {
        const double dx = qx - s[0];
        const double dy = qy - s[1];
        const double d2 = dx * dx + dy * dy;
        if (d2 < 1e-6) {
            return s[2];
        }
        near_vals.emplace_back(d2, s[2]);
    }
    std::sort(near_vals.begin(), near_vals.end());
    const size_t k_use = std::min<size_t>(3, near_vals.size());
    double wsum = 0.0;
    double vsum = 0.0;
    for (size_t i = 0; i < k_use; i++) {
        const double w = 1.0 / near_vals[i].first;
        wsum += w;
        vsum += w * near_vals[i].second;
    }

    return (wsum > 0.0) ? (vsum / wsum) : sites[0][2];
}

/// Boundary-preserving isotropic triangle remeshing (Calgary2025 002_remesh_compas.py).
void remesh_preserve_boundary(
    std::vector<Point>& verts,
    std::vector<std::array<size_t, 3>>& tris,
    const std::vector<bool>& fixed_bnd,
    const SurfaceProjector& projector,
    double target_edge,
    int iterations
) {

    std::vector<bool> is_bnd = fixed_bnd;
    const double split_sq = (1.33 * target_edge) * (1.33 * target_edge);
    const double col_sq = (0.76 * target_edge) * (0.76 * target_edge);
    const double max_after_col_sq = (1.55 * target_edge) * (1.55 * target_edge);

    for (int iter = 0; iter < iterations; iter++) {
        // 1. Split interior edges longer than 4/3 * target_edge
        {
            std::map<std::pair<size_t, size_t>, int> uses;
            for (const std::array<size_t, 3>& t : tris) {
                for (size_t k = 0; k < 3; k++) {
                    uses[ordered_pair(t[k], t[(k + 1) % 3])]++;
                }
            }
            std::map<std::pair<size_t, size_t>, size_t> edge_mid;
            for (const std::pair<const std::pair<size_t, size_t>, int>& kv : uses) {
                if (kv.second != 2) {
                    continue;
                }
                const size_t u = kv.first.first;
                const size_t v = kv.first.second;
                if ((verts[u] - verts[v]).magnitude_squared() > split_sq) {
                    const Point mid = projector.project(Point::mid_point(verts[u], verts[v])).first;
                    edge_mid[kv.first] = verts.size();
                    verts.push_back(mid);
                    is_bnd.push_back(false);
                }
            }
            if (!edge_mid.empty()) {
                std::vector<std::array<size_t, 3>> next_tris;
                next_tris.reserve(tris.size() * 2);
                for (const std::array<size_t, 3>& t : tris) {
                    std::array<int, 3> mids = {-1, -1, -1};
                    int sc = 0;
                    for (size_t k = 0; k < 3; k++) {
                        const std::map<std::pair<size_t, size_t>, size_t>::const_iterator it =
                            edge_mid.find(ordered_pair(t[k], t[(k + 1) % 3]));
                        if (it != edge_mid.end()) {
                            mids[k] = static_cast<int>(it->second);
                            sc++;
                        }
                    }
                    if (sc == 0) {
                        next_tris.push_back(t);
                    } else if (sc == 3) {
                        const size_t m0 = static_cast<size_t>(mids[0]);
                        const size_t m1 = static_cast<size_t>(mids[1]);
                        const size_t m2 = static_cast<size_t>(mids[2]);
                        next_tris.push_back({t[0], m0, m2});
                        next_tris.push_back({m0, t[1], m1});
                        next_tris.push_back({m2, m1, t[2]});
                        next_tris.push_back({m0, m1, m2});
                    } else if (sc == 1) {
                        size_t sk = 0;
                        for (size_t k = 0; k < 3; k++) {
                            if (mids[k] >= 0) {
                                sk = k;
                            }
                        }
                        const size_t m = static_cast<size_t>(mids[sk]);
                        next_tris.push_back({t[sk], m, t[(sk + 2) % 3]});
                        next_tris.push_back({m, t[(sk + 1) % 3], t[(sk + 2) % 3]});
                    } else {
                        size_t uk = 0;
                        for (size_t k = 0; k < 3; k++) {
                            if (mids[k] < 0) {
                                uk = k;
                            }
                        }
                        const size_t v0 = t[uk];
                        const size_t v1 = t[(uk + 1) % 3];
                        const size_t v2 = t[(uk + 2) % 3];
                        const size_t m1 = static_cast<size_t>(mids[(uk + 1) % 3]);
                        const size_t m2 = static_cast<size_t>(mids[(uk + 2) % 3]);
                        next_tris.push_back({v2, m2, m1});
                        if (verts[v0].distance(verts[m1]) <= verts[v1].distance(verts[m2])) {
                            next_tris.push_back({v0, v1, m1});
                            next_tris.push_back({v0, m1, m2});
                        } else {
                            next_tris.push_back({v0, v1, m2});
                            next_tris.push_back({v1, m1, m2});
                        }
                    }
                }
                tris = std::move(next_tris);
            }
        }

        // 2. Collapse interior edges shorter than 0.76 * target_edge (preserving fixed boundary vertices)
        {
            std::vector<std::set<size_t>> v_adj(verts.size());
            std::vector<std::vector<size_t>> v_tris(verts.size());
            for (size_t ti = 0; ti < tris.size(); ti++) {
                for (size_t k = 0; k < 3; k++) {
                    const size_t a = tris[ti][k];
                    const size_t b = tris[ti][(k + 1) % 3];
                    v_adj[a].insert(b);
                    v_adj[b].insert(a);
                    v_tris[a].push_back(ti);
                }
            }
            std::vector<bool> v_locked(verts.size(), false);
            std::vector<bool> t_dead(tris.size(), false);

            for (size_t ti = 0; ti < tris.size(); ti++) {
                if (t_dead[ti]) {
                    continue;
                }
                for (size_t k = 0; k < 3; k++) {
                    size_t u = tris[ti][k];
                    size_t v = tris[ti][(k + 1) % 3];
                    if (v_locked[u] || v_locked[v] || (is_bnd[u] && is_bnd[v])) {
                        continue;
                    }
                    if ((verts[u] - verts[v]).magnitude_squared() >= col_sq) {
                        continue;
                    }
                    if (is_bnd[u]) {
                        std::swap(u, v);
                    }
                    // Check link condition: common neighbors of u and v == 2
                    size_t common = 0;
                    for (const size_t nb : v_adj[u]) {
                        if (v_adj[v].count(nb)) {
                            common++;
                        }
                    }
                    if (common != 2) {
                        continue;
                    }
                    const Point target_p =
                        is_bnd[v] ? verts[v] : projector.project(Point::mid_point(verts[u], verts[v])).first;

                    bool valid_col = true;
                    for (const size_t inc_t : v_tris[u]) {
                        if (t_dead[inc_t]) {
                            continue;
                        }
                        const std::array<size_t, 3>& ct = tris[inc_t];
                        if (ct[0] == v || ct[1] == v || ct[2] == v) {
                            continue;
                        }
                        std::array<Point, 3> np = {verts[ct[0]], verts[ct[1]], verts[ct[2]]};
                        for (size_t m = 0; m < 3; m++) {
                            if (ct[m] == u) {
                                np[m] = target_p;
                            }
                        }
                        const Vector old_n =
                            (verts[ct[1]] - verts[ct[0]]).cross(verts[ct[2]] - verts[ct[0]]).normalized();
                        Vector new_n = (np[1] - np[0]).cross(np[2] - np[0]);
                        if (!new_n.normalize_self() || new_n.dot(old_n) < 0.45) {
                            valid_col = false;
                            break;
                        }
                        for (size_t m = 0; m < 3; m++) {
                            if ((np[m] - np[(m + 1) % 3]).magnitude_squared() > max_after_col_sq) {
                                valid_col = false;
                                break;
                            }
                        }
                        if (!valid_col) {
                            break;
                        }
                    }
                    if (!valid_col) {
                        continue;
                    }

                    verts[v] = target_p;
                    v_locked[u] = true;
                    v_locked[v] = true;
                    for (const size_t nb : v_adj[u]) {
                        v_locked[nb] = true;
                    }
                    for (const size_t inc_t : v_tris[u]) {
                        if (t_dead[inc_t]) {
                            continue;
                        }
                        std::array<size_t, 3>& ct = tris[inc_t];
                        if (ct[0] == v || ct[1] == v || ct[2] == v) {
                            t_dead[inc_t] = true;
                        } else {
                            for (size_t m = 0; m < 3; m++) {
                                if (ct[m] == u) {
                                    ct[m] = v;
                                }
                            }
                        }
                    }
                    break;
                }
            }

            std::vector<std::array<size_t, 3>> kept_tris;
            for (size_t ti = 0; ti < tris.size(); ti++) {
                if (!t_dead[ti]) {
                    kept_tris.push_back(tris[ti]);
                }
            }
            tris = std::move(kept_tris);
        }

        // 3. Valence-regularizing edge flips (push interior valences toward 6, boundary toward 4)
        for (int fpass = 0; fpass < 2; fpass++) {
            std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>> edge_inc;
            std::vector<std::set<size_t>> v_adj(verts.size());
            for (size_t ti = 0; ti < tris.size(); ti++) {
                for (size_t k = 0; k < 3; k++) {
                    const size_t a = tris[ti][k];
                    const size_t b = tris[ti][(k + 1) % 3];
                    edge_inc[ordered_pair(a, b)].emplace_back(ti, k);
                    v_adj[a].insert(b);
                    v_adj[b].insert(a);
                }
            }
            std::vector<bool> t_touched(tris.size(), false);
            for (const std::pair<const std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>>& kv :
                 edge_inc) {
                if (kv.second.size() != 2) {
                    continue;
                }
                const size_t t0 = kv.second[0].first;
                const size_t k0 = kv.second[0].second;
                const size_t t1 = kv.second[1].first;
                const size_t k1 = kv.second[1].second;
                if (t_touched[t0] || t_touched[t1]) {
                    continue;
                }
                const size_t u = tris[t0][k0];
                const size_t v = tris[t0][(k0 + 1) % 3];
                const size_t a = tris[t0][(k0 + 2) % 3];
                const size_t b = tris[t1][(k1 + 2) % 3];
                if (a == b || tris[t1][k1] != v || tris[t1][(k1 + 1) % 3] != u || v_adj[a].count(b)) {
                    continue;
                }

                const int du = static_cast<int>(v_adj[u].size()) - (is_bnd[u] ? 4 : 6);
                const int dv = static_cast<int>(v_adj[v].size()) - (is_bnd[v] ? 4 : 6);
                const int da = static_cast<int>(v_adj[a].size()) - (is_bnd[a] ? 4 : 6);
                const int db = static_cast<int>(v_adj[b].size()) - (is_bnd[b] ? 4 : 6);
                const int before = du * du + dv * dv + da * da + db * db;
                const int after =
                    (du - 1) * (du - 1) + (dv - 1) * (dv - 1) + (da + 1) * (da + 1) + (db + 1) * (db + 1);
                if (after >= before) {
                    continue;
                }

                const Vector n0_old = (verts[v] - verts[u]).cross(verts[a] - verts[u]).normalized();
                const Vector n1_old = (verts[u] - verts[v]).cross(verts[b] - verts[v]).normalized();
                const Vector n_ref = (n0_old + n1_old).normalized();
                Vector n0_new = (verts[b] - verts[a]).cross(verts[u] - verts[a]);
                Vector n1_new = (verts[a] - verts[b]).cross(verts[v] - verts[b]);
                if (!n0_new.normalize_self() || !n1_new.normalize_self()) {
                    continue;
                }
                if (n0_new.dot(n_ref) < 0.65 || n1_new.dot(n_ref) < 0.65) {
                    continue;
                }

                tris[t0] = {a, b, u};
                tris[t1] = {b, a, v};
                t_touched[t0] = true;
                t_touched[t1] = true;
                v_adj[u].erase(v);
                v_adj[v].erase(u);
                v_adj[a].insert(b);
                v_adj[b].insert(a);
            }
        }

        // 4. Tangential smoothing + exact projection onto base thrust shell
        {
            std::vector<std::set<size_t>> v_adj(verts.size());
            for (const std::array<size_t, 3>& t : tris) {
                for (size_t k = 0; k < 3; k++) {
                    v_adj[t[k]].insert(t[(k + 1) % 3]);
                    v_adj[t[(k + 1) % 3]].insert(t[k]);
                }
            }
            std::vector<Point> next_v = verts;
            for (size_t i = 0; i < verts.size(); i++) {
                if (is_bnd[i] || v_adj[i].empty()) {
                    continue;
                }
                Vector avg(0.0, 0.0, 0.0);
                for (const size_t nb : v_adj[i]) {
                    avg = avg + Vector(verts[nb][0], verts[nb][1], verts[nb][2]);
                }
                avg = avg * (1.0 / static_cast<double>(v_adj[i].size()));
                const Point c(avg[0], avg[1], avg[2]);
                const Vector nrm = projector.project(verts[i]).second;
                Vector tang = (c - verts[i]) - nrm * (c - verts[i]).dot(nrm);
                next_v[i] = projector.project(verts[i] + tang * 0.45).first;
            }
            verts = std::move(next_v);
        }
    }

    // Compact referenced vertices only
    std::vector<int> remap(verts.size(), -1);
    std::vector<Point> compact_v;
    for (const std::array<size_t, 3>& t : tris) {
        for (const size_t vid : t) {
            if (remap[vid] < 0) {
                remap[vid] = static_cast<int>(compact_v.size());
                compact_v.push_back(verts[vid]);
            }
        }
    }
    for (std::array<size_t, 3>& t : tris) {
        for (size_t k = 0; k < 3; k++) {
            t[k] = static_cast<size_t>(remap[t[k]]);
        }
    }
    verts = std::move(compact_v);
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// CalgaryVaultGuide Presets
// ═══════════════════════════════════════════════════════════════════════════

Mesh CalgaryVaultGuide::calgary_shell() {

    const std::filesystem::path path =
        config::session_data_dir() / "templates" / "calgary_shell_mesh.json";
    if (std::filesystem::exists(path)) {
        return Mesh::file_json_load(path.string());
    }

    const SurfaceDivision div({saddle_vault_surface(9455.0, 11174.0, 3000.0, 1500.0)}, 900.0, 900.0);

    return div.quads();
}

CalgaryVaultGuide CalgaryVaultGuide::variable_thickness_blocks() {

    CalgaryVaultGuide guide;
    guide.shell = calgary_shell();
    guide.remesh_iterations = 35;
    guide.lower_pier_thickness = 300.0;
    guide.upper_pier_thickness = 200.0;
    guide.arch_midspan_thickness = 150.0;
    guide.crown_thickness = 100.0;
    guide.chamfer_offset = 0.0;
    guide.bevel_degrees = 0.0;
    guide.smooth_arch_borders = true;
    guide.planar_sides = true;
    guide.trim_supports = true;

    return guide;
}

CalgaryVaultGuide CalgaryVaultGuide::chamfered_blocks() {

    CalgaryVaultGuide guide = variable_thickness_blocks();
    guide.chamfer_max_angle_deg = 145.0;
    guide.chamfer_offset = 22.0;

    return guide;
}

CalgaryVaultGuide CalgaryVaultGuide::interlocking_bevel_blocks() {

    CalgaryVaultGuide guide = variable_thickness_blocks();
    guide.bevel_degrees = 14.0;
    guide.flatten_intrados = true;

    return guide;
}

// ═══════════════════════════════════════════════════════════════════════════
// Calgary 2025 Dualization, NURBS Arch Smoothing & Variable-Thickness Blocks
// ═══════════════════════════════════════════════════════════════════════════

Mesh calgary_dual_mesh(
    const CalgaryVaultGuide& guide,
    std::vector<double>* out_vertex_thickness,
    std::set<std::pair<size_t, size_t>>* out_support_edges
) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> raw_vf =
        guide.shell.to_vertices_and_faces();
    if (raw_vf.first.empty() || raw_vf.second.empty()) {
        return Mesh();
    }

    // Compute average edge length of input shell (002_remesh_compas.py line 42)
    std::set<std::pair<size_t, size_t>> raw_edges;
    double edge_len_sum = 0.0;
    for (const std::vector<size_t>& f : raw_vf.second) {
        for (size_t k = 0; k < f.size(); k++) {
            const std::pair<size_t, size_t> e = ordered_pair(f[k], f[(k + 1) % f.size()]);
            if (raw_edges.insert(e).second) {
                edge_len_sum += raw_vf.first[e.first].distance(raw_vf.first[e.second]);
            }
        }
    }
    const double mean_input_edge =
        raw_edges.empty() ? 880.0 : (edge_len_sum / static_cast<double>(raw_edges.size()));
    const double target_len = (guide.target_edge > 1.0) ? guide.target_edge : mean_input_edge;

    std::vector<Point> orig_verts;
    std::vector<std::array<size_t, 3>> orig_tris;
    extract_welded_triangles(guide.shell, orig_verts, orig_tris, 1e-2);
    if (orig_verts.empty() || orig_tris.empty()) {
        return Mesh();
    }

    // Orient all triangles upward (+Z) consistently
    for (std::array<size_t, 3>& t : orig_tris) {
        const Vector fn = (orig_verts[t[1]] - orig_verts[t[0]]).cross(orig_verts[t[2]] - orig_verts[t[0]]);
        if (fn[2] < 0.0) {
            std::swap(t[1], t[2]);
        }
    }
    std::vector<Vector> orig_norms;
    std::vector<double> orig_areas;
    compute_normals_and_areas(orig_verts, orig_tris, orig_norms, orig_areas);

    SurfaceProjector projector;
    projector.build(orig_verts, orig_norms, orig_tris);

    // Identify naked boundary vertices and the 8 sharp pier corners on the original boundary
    std::map<std::pair<size_t, size_t>, int> o_he;
    for (size_t ti = 0; ti < orig_tris.size(); ti++) {
        for (size_t k = 0; k < 3; k++) {
            o_he[{orig_tris[ti][k], orig_tris[ti][(k + 1) % 3]}] = static_cast<int>(ti * 3 + k);
        }
    }
    std::vector<bool> orig_bnd(orig_verts.size(), false);
    std::vector<Vector> b_in(orig_verts.size(), Vector(0.0, 0.0, 0.0));
    std::vector<Vector> b_out(orig_verts.size(), Vector(0.0, 0.0, 0.0));
    for (size_t ti = 0; ti < orig_tris.size(); ti++) {
        for (size_t k = 0; k < 3; k++) {
            const size_t u = orig_tris[ti][k];
            const size_t v = orig_tris[ti][(k + 1) % 3];
            if (!o_he.count({v, u})) {
                orig_bnd[u] = true;
                orig_bnd[v] = true;
                Vector d = orig_verts[v] - orig_verts[u];
                if (d.normalize_self()) {
                    b_out[u] = d;
                    b_in[v] = d;
                }
            }
        }
    }

    std::vector<Point> sharp_corner_pts;
    for (size_t v = 0; v < orig_verts.size(); v++) {
        if (orig_bnd[v] && b_in[v].magnitude_squared() > 0.5 && b_out[v].magnitude_squared() > 0.5) {
            if (b_in[v].dot(b_out[v]) < 0.80) {
                sharp_corner_pts.push_back(orig_verts[v]);
            }
        }
    }

    // 1. Boundary-preserving isotropic triangle remeshing (002_remesh_compas.py)
    std::vector<Point> r_verts = orig_verts;
    std::vector<std::array<size_t, 3>> r_tris = orig_tris;
    remesh_preserve_boundary(r_verts, r_tris, orig_bnd, projector, target_len, guide.remesh_iterations);

    // Build halfedge table on remeshed triangle mesh
    const size_t nv = r_verts.size();
    const size_t nf = r_tris.size();
    std::map<std::pair<size_t, size_t>, int> he_map;
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            he_map[{r_tris[f][k], r_tris[f][(k + 1) % 3]}] = static_cast<int>(f * 3 + k);
        }
    }
    std::vector<int> e2e(nf * 3, -1);
    std::vector<int> bnd_out_he(nv, -1);
    std::vector<bool> r_bnd_v(nv, false);
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            const size_t u = r_tris[f][k];
            const size_t v = r_tris[f][(k + 1) % 3];
            const std::map<std::pair<size_t, size_t>, int>::const_iterator it = he_map.find({v, u});
            if (it != he_map.end()) {
                e2e[f * 3 + k] = it->second;
            } else {
                r_bnd_v[u] = true;
                r_bnd_v[v] = true;
                bnd_out_he[u] = static_cast<int>(f * 3 + k);
            }
        }
    }

    std::vector<bool> r_is_corner(nv, false);
    for (size_t v = 0; v < nv; v++) {
        if (!r_bnd_v[v]) {
            continue;
        }
        for (const Point& cp : sharp_corner_pts) {
            if (r_verts[v].distance(cp) < 1e-1) {
                r_is_corner[v] = true;
                break;
            }
        }
    }

    // 2. Boundary-inclusive trivalent dual mesh with 2-valent arch vertices collapsed (003 + 004 + 005)
    std::vector<Point> dual_pts(nf);
    std::vector<bool> dual_is_corner(nf, false);
    for (size_t f = 0; f < nf; f++) {
        const Point c(
            (r_verts[r_tris[f][0]][0] + r_verts[r_tris[f][1]][0] + r_verts[r_tris[f][2]][0]) / 3.0,
            (r_verts[r_tris[f][0]][1] + r_verts[r_tris[f][1]][1] + r_verts[r_tris[f][2]][1]) / 3.0,
            (r_verts[r_tris[f][0]][2] + r_verts[r_tris[f][1]][2] + r_verts[r_tris[f][2]][2]) / 3.0
        );
        dual_pts[f] = projector.project(c).first;
    }

    std::map<int, size_t> he_to_dual_v;
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            const int he = static_cast<int>(f * 3 + k);
            if (e2e[static_cast<size_t>(he)] < 0) {
                he_to_dual_v[he] = dual_pts.size();
                dual_pts.push_back(Point::mid_point(r_verts[r_tris[f][k]], r_verts[r_tris[f][(k + 1) % 3]]));
                dual_is_corner.push_back(false);
            }
        }
    }

    std::map<size_t, size_t> corner_to_dual_v;
    for (size_t v = 0; v < nv; v++) {
        if (r_is_corner[v]) {
            corner_to_dual_v[v] = dual_pts.size();
            dual_pts.push_back(r_verts[v]);
            dual_is_corner.push_back(true);
        }
    }

    std::vector<int> sample_corner(nv, -1);
    for (size_t f = 0; f < nf; f++) {
        for (size_t k = 0; k < 3; k++) {
            sample_corner[r_tris[f][k]] = static_cast<int>(f * 3 + k);
        }
    }

    std::vector<std::vector<size_t>> dual_faces;
    for (size_t v = 0; v < nv; v++) {
        if (sample_corner[v] < 0) {
            continue;
        }
        if (!r_bnd_v[v]) {
            const int start_c = sample_corner[v];
            int cur_c = start_c;
            std::vector<size_t> ring;
            bool closed = false;
            for (int step = 0; step < 64; step++) {
                const size_t f = static_cast<size_t>(cur_c / 3);
                const size_t k = static_cast<size_t>(cur_c % 3);
                ring.push_back(f);
                const int twin = e2e[f * 3 + (k + 2) % 3];
                if (twin < 0) {
                    break;
                }
                cur_c = twin;
                if (cur_c == start_c) {
                    closed = true;
                    break;
                }
            }
            if (closed && ring.size() >= 3) {
                dual_faces.push_back(ring);
            }
        } else if (bnd_out_he[v] >= 0) {
            const int start_he = bnd_out_he[v];
            std::vector<size_t> ring;
            if (r_is_corner[v]) {
                ring.push_back(corner_to_dual_v[v]);
            }
            ring.push_back(he_to_dual_v[start_he]);

            int cur_c = start_he;
            int end_in_he = -1;
            for (int step = 0; step < 64; step++) {
                const size_t f = static_cast<size_t>(cur_c / 3);
                const size_t k = static_cast<size_t>(cur_c % 3);
                ring.push_back(f);
                const int he_in = static_cast<int>(f * 3 + (k + 2) % 3);
                const int twin = e2e[static_cast<size_t>(he_in)];
                if (twin < 0) {
                    end_in_he = he_in;
                    break;
                }
                cur_c = twin;
            }
            if (end_in_he >= 0 && he_to_dual_v.count(end_in_he)) {
                ring.push_back(he_to_dual_v[end_in_he]);
            }
            if (ring.size() >= 3) {
                dual_faces.push_back(ring);
            }
        }
    }

    // 3. Break dual boundary at the 8 corner vertices, smooth arch borders via NurbsCurve (006), & tag supports (008)
    std::map<std::pair<size_t, size_t>, int> d_he_uses;
    std::vector<std::set<size_t>> d_adj(dual_pts.size());
    for (const std::vector<size_t>& df : dual_faces) {
        const size_t m = df.size();
        for (size_t k = 0; k < m; k++) {
            const size_t u = df[k];
            const size_t w = df[(k + 1) % m];
            d_he_uses[{u, w}]++;
            d_adj[u].insert(w);
            d_adj[w].insert(u);
        }
    }
    std::map<size_t, size_t> d_bnd_next;
    std::set<size_t> d_bnd_set;
    for (const std::pair<const std::pair<size_t, size_t>, int>& kv : d_he_uses) {
        const size_t u = kv.first.first;
        const size_t w = kv.first.second;
        if (!d_he_uses.count({w, u})) {
            d_bnd_next[u] = w;
            d_bnd_set.insert(u);
            d_bnd_set.insert(w);
        }
    }

    std::vector<std::vector<size_t>> borders;
    if (!d_bnd_next.empty()) {
        size_t start_bv = d_bnd_next.begin()->first;
        for (const std::pair<const size_t, size_t>& kv : d_bnd_next) {
            if (dual_is_corner[kv.first]) {
                start_bv = kv.first;
                break;
            }
        }
        std::vector<size_t> loop;
        size_t cur = start_bv;
        for (size_t step = 0; step < d_bnd_next.size() + 1; step++) {
            loop.push_back(cur);
            const std::map<size_t, size_t>::const_iterator it = d_bnd_next.find(cur);
            if (it == d_bnd_next.end()) {
                break;
            }
            cur = it->second;
            if (cur == start_bv) {
                break;
            }
        }

        std::vector<size_t> c_indices;
        for (size_t idx = 0; idx < loop.size(); idx++) {
            if (dual_is_corner[loop[idx]]) {
                c_indices.push_back(idx);
            }
        }
        if (c_indices.size() >= 2) {
            for (size_t ci = 0; ci < c_indices.size(); ci++) {
                const size_t s_idx = c_indices[ci];
                const size_t e_idx = c_indices[(ci + 1) % c_indices.size()];
                std::vector<size_t> chain;
                size_t k = s_idx;
                while (true) {
                    chain.push_back(loop[k]);
                    if (k == e_idx) {
                        break;
                    }
                    k = (k + 1) % loop.size();
                }
                borders.push_back(chain);
            }
        }
    }

    std::set<std::pair<size_t, size_t>> support_edges;
    std::vector<Point> arch_midspans;
    for (const std::vector<size_t>& border : borders) {
        if (border.size() < 2) {
            continue;
        }
        const double chord_len = dual_pts[border.front()].distance(dual_pts[border.back()]);
        if (chord_len < 2600.0 || border.size() <= 4) {
            for (size_t k = 0; k + 1 < border.size(); k++) {
                support_edges.insert(ordered_pair(border[k], border[k + 1]));
            }
        } else {
            arch_midspans.push_back(dual_pts[border[border.size() / 2]]);
            if (guide.smooth_arch_borders && border.size() > 4) {
                std::vector<Point> sample_pts;
                for (size_t k = 0; k < border.size(); k += 2) {
                    sample_pts.push_back(dual_pts[border[k]]);
                }
                if (sample_pts.back().distance(dual_pts[border.back()]) > 1e-3) {
                    sample_pts.push_back(dual_pts[border.back()]);
                }
                if (sample_pts.size() >= 3) {
                    const NurbsCurve curve = NurbsCurve::create_interpolated(sample_pts);
                    if (curve.is_valid()) {
                        for (size_t k = 1; k + 1 < border.size(); k++) {
                            const size_t bv = border[k];
                            for (const size_t nbr : d_adj[bv]) {
                                if (!d_bnd_set.count(nbr)) {
                                    dual_pts[bv] = curve.closest_point(dual_pts[nbr]);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (out_support_edges) {
        *out_support_edges = support_edges;
    }

    // 4. Interpolate variable shell thickness in XY (009_dual_thickness_data.py: 300 -> 200 -> 150 -> 100 mm)
    std::vector<std::array<double, 3>> ctrl_xyv;
    std::vector<std::pair<double, size_t>> corner_by_z;
    for (size_t i = 0; i < sharp_corner_pts.size(); i++) {
        corner_by_z.emplace_back(sharp_corner_pts[i][2], i);
    }
    std::sort(corner_by_z.begin(), corner_by_z.end());
    const size_t half_c = corner_by_z.size() / 2;
    for (size_t i = 0; i < corner_by_z.size(); i++) {
        const Point& cp = sharp_corner_pts[corner_by_z[i].second];
        const double t_val = (i < half_c) ? guide.lower_pier_thickness : guide.upper_pier_thickness;
        ctrl_xyv.push_back({cp[0], cp[1], t_val});
    }
    for (const Point& mp : arch_midspans) {
        ctrl_xyv.push_back({mp[0], mp[1], guide.arch_midspan_thickness});
    }

    std::vector<std::pair<double, size_t>> verts_by_z;
    for (size_t i = 0; i < orig_verts.size(); i++) {
        verts_by_z.emplace_back(orig_verts[i][2], i);
    }
    std::sort(verts_by_z.begin(), verts_by_z.end());
    const size_t crown_cnt = std::min<size_t>(5, verts_by_z.size());
    for (size_t i = 0; i < crown_cnt; i++) {
        const Point& cp = orig_verts[verts_by_z[verts_by_z.size() - 1 - i].second];
        ctrl_xyv.push_back({cp[0], cp[1], guide.crown_thickness});
    }

    const double t_lo = std::min({
        guide.lower_pier_thickness,
        guide.upper_pier_thickness,
        guide.arch_midspan_thickness,
        guide.crown_thickness
    });
    const double t_hi = std::max({
        guide.lower_pier_thickness,
        guide.upper_pier_thickness,
        guide.arch_midspan_thickness,
        guide.crown_thickness
    });

    std::vector<double> dual_thick(dual_pts.size(), guide.arch_midspan_thickness);
    for (size_t v = 0; v < dual_pts.size(); v++) {
        dual_thick[v] = std::clamp(
            interpolate_delaunay_xy(ctrl_xyv, dual_pts[v][0], dual_pts[v][1]), t_lo, t_hi
        );
    }
    if (out_vertex_thickness) {
        *out_vertex_thickness = dual_thick;
    }

    Mesh dual_mesh = Mesh::from_vertices_and_faces(dual_pts, dual_faces);
    for (size_t v = 0; v < dual_pts.size(); v++) {
        Vector n = projector.project(dual_pts[v]).second;
        if (n[2] < 0.0) {
            n = -n;
        }
        if (dual_mesh.vertex.count(v)) {
            dual_mesh.vertex[v].set_normal(n[0], n[1], n[2]);
        }
    }

    return dual_mesh;
}

std::vector<std::shared_ptr<Element>> calgary_vault(
    const CalgaryVaultGuide& guide,
    CalgaryDualMetrics* metrics,
    Mesh* out_dual_mesh
) {

    CalgaryDualMetrics local_m;
    std::vector<double> v_thick;
    std::set<std::pair<size_t, size_t>> support_edges;
    const Mesh dual = calgary_dual_mesh(guide, &v_thick, &support_edges);
    if (out_dual_mesh) {
        *out_dual_mesh = dual;
    }

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        dual.to_vertices_and_faces();
    const std::vector<Point>& pts = vf.first;
    const std::vector<std::vector<size_t>>& faces = vf.second;
    const std::vector<size_t> keys = dual.vertices();
    if (pts.empty() || faces.empty()) {
        if (metrics) {
            *metrics = local_m;
        }
        return {};
    }

    local_m.dual_faces = faces.size();
    local_m.min_thickness = *std::min_element(v_thick.begin(), v_thick.end());
    local_m.max_thickness = *std::max_element(v_thick.begin(), v_thick.end());

    const size_t nv = pts.size();
    std::vector<Vector> nrms(nv, Vector(0.0, 0.0, 1.0));
    for (size_t i = 0; i < nv; i++) {
        const std::optional<std::array<double, 3>> attr = dual.vertex.at(keys[i]).normal();
        if (attr) {
            Vector sn((*attr)[0], (*attr)[1], (*attr)[2]);
            if (sn.normalize_self()) {
                nrms[i] = sn;
            }
        }
    }

    // Identify boundary vertices and ordered halfedge-to-neighbor map for chamfering (011_chamfer.py)
    std::map<std::pair<size_t, size_t>, int> edge_uses;
    for (const std::vector<size_t>& f : faces) {
        for (size_t k = 0; k < f.size(); k++) {
            edge_uses[ordered_pair(f[k], f[(k + 1) % f.size()])]++;
        }
    }
    std::vector<bool> is_bnd_v(nv, false);
    for (const std::pair<const std::pair<size_t, size_t>, int>& kv : edge_uses) {
        if (kv.second == 1) {
            is_bnd_v[kv.first.first] = true;
            is_bnd_v[kv.first.second] = true;
        }
    }

    // Build initial variable-thickness intrados & extrados rulings (010_blocks.py lines 38-39)
    std::vector<Point> ruling_pts(2 * nv);
    for (size_t i = 0; i < nv; i++) {
        const double ht = 0.5 * v_thick[i];
        ruling_pts[i] = pts[i] - nrms[i] * ht;
        ruling_pts[nv + i] = pts[i] + nrms[i] * ht;
    }

    // Planarize shared side rulings [bot_u, bot_v, top_v, top_u] so every shared side interface is exact planar!
    if (guide.planar_sides) {
        std::vector<std::vector<size_t>> side_quads;
        for (const std::pair<const std::pair<size_t, size_t>, int>& kv : edge_uses) {
            const size_t u = kv.first.first;
            const size_t w = kv.first.second;
            side_quads.push_back({u, w, nv + w, nv + u});
        }
        ruling_pts = planarized(ruling_pts, side_quads);
    }

    // Optional alternating bevel signs when guide.bevel_degrees != 0
    std::map<std::pair<size_t, size_t>, double> bevel_signs;
    if (std::abs(guide.bevel_degrees) > 1e-6) {
        std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>> inc;
        for (size_t fi = 0; fi < faces.size(); fi++) {
            const size_t m = faces[fi].size();
            for (size_t k = 0; k < m; k++) {
                inc[ordered_pair(faces[fi][k], faces[fi][(k + 1) % m])].emplace_back(fi, k);
            }
        }
        std::vector<bool> vis(faces.size(), false);
        for (size_t s = 0; s < faces.size(); s++) {
            if (vis[s]) {
                continue;
            }
            vis[s] = true;
            std::vector<std::pair<size_t, int>> q = {{s, 0}};
            size_t head = 0;
            while (head < q.size()) {
                const size_t f = q[head].first;
                const int ph = q[head].second;
                head++;
                const size_t m = faces[f].size();
                for (size_t k = 0; k < m; k++) {
                    const size_t a = faces[f][k];
                    const size_t b = faces[f][(k + 1) % m];
                    const std::pair<size_t, size_t> key = ordered_pair(a, b);
                    const double loc_out = ((static_cast<int>(k) + ph) % 2 == 0) ? 1.0 : -1.0;
                    const double canon = (a < b) ? loc_out : -loc_out;
                    if (!bevel_signs.count(key)) {
                        bevel_signs[key] = canon;
                    }
                    for (const std::pair<size_t, size_t>& u : inc[key]) {
                        if (!vis[u.first]) {
                            const size_t ua = faces[u.first][u.second];
                            const size_t ub = faces[u.first][(u.second + 1) % faces[u.first].size()];
                            const double des = (ua < ub) ? bevel_signs[key] : -bevel_signs[key];
                            const int nph =
                                ((static_cast<int>(u.second) % 2 == 0) == (des > 0.0)) ? 0 : 1;
                            vis[u.first] = true;
                            q.emplace_back(u.first, nph);
                        }
                    }
                }
            }
        }
    }

    const double bevel_rad = guide.bevel_degrees * Tolerance::TO_RADIANS;
    std::vector<std::shared_ptr<Element>> elements;

    for (size_t fi = 0; fi < faces.size(); fi++) {
        const std::vector<size_t>& f = faces[fi];
        const size_t m = f.size();
        if (m < 3) {
            continue;
        }

        std::vector<Point> raw_bot(m);
        std::vector<Point> raw_top(m);
        Vector up_ref(0.0, 0.0, 0.0);
        bool touches_support = false;
        double min_mid_z = 1e30;

        for (size_t k = 0; k < m; k++) {
            const size_t vid = f[k];
            const size_t nvid = f[(k + 1) % m];
            raw_bot[k] = ruling_pts[vid];
            raw_top[k] = ruling_pts[nv + vid];
            up_ref = up_ref + nrms[vid];
            if (support_edges.count(ordered_pair(vid, nvid))) {
                touches_support = true;
                min_mid_z = std::min({min_mid_z, pts[vid][2], pts[nvid][2]});
            }
        }
        up_ref.normalize_self();

        const Plane pca_top = Plane::from_points_pca(raw_top);
        const Plane top_plane = (pca_top.z_axis().dot(up_ref) >= 0.0)
                                    ? pca_top
                                    : Plane(pca_top.origin(), pca_top.y_axis(), pca_top.x_axis());
        const Plane pca_bot = Plane::from_points_pca(raw_bot);
        const Plane bot_plane = (pca_bot.z_axis().dot(up_ref) >= 0.0)
                                    ? pca_bot
                                    : Plane(pca_bot.origin(), pca_bot.y_axis(), pca_bot.x_axis());

        std::vector<Point> bot_ring(m);
        std::vector<Point> top_ring(m);

        if (std::abs(guide.bevel_degrees) > 1e-6) {
            // Intersect tilted shared edge planes with flat top_plane and flat bot_plane!
            std::vector<Plane> side_planes(m);
            for (size_t k = 0; k < m; k++) {
                const size_t a = f[k];
                const size_t b = f[(k + 1) % m];
                const Point mid = Point::mid_point(pts[a], pts[b]);
                Vector axis = (pts[b] - pts[a]).normalized();
                Vector avg_n = (nrms[a] + nrms[b]).normalized();
                Vector out_n = axis.cross(avg_n).normalized(); // Outward side normal for CCW face
                const std::pair<size_t, size_t> key = ordered_pair(a, b);
                const double local_sgn = (a < b) ? bevel_signs[key] : -bevel_signs[key];
                const Vector tilted_n =
                    rotate_around_normal(out_n, axis, local_sgn * bevel_rad).normalized();
                side_planes[k] = plane_with_normal(mid, tilted_n);
            }
            bool bevel_ok = true;
            for (size_t k = 0; k < m; k++) {
                const size_t kp = (k + m - 1) % m;
                const std::optional<Point> pt_top =
                    Intersection::plane_plane_plane(side_planes[kp], side_planes[k], top_plane);
                const std::optional<Point> pt_bot =
                    Intersection::plane_plane_plane(side_planes[kp], side_planes[k], bot_plane);
                if (!pt_top || !pt_bot || pt_top->distance(raw_top[k]) > 1800.0 ||
                    pt_bot->distance(raw_bot[k]) > 1800.0) {
                    bevel_ok = false;
                    break;
                }
                top_ring[k] = *pt_top;
                bot_ring[k] = *pt_bot;
            }
            if (!bevel_ok) {
                for (size_t k = 0; k < m; k++) {
                    const Line rul = Line::from_points(raw_bot[k], raw_top[k]);
                    const std::optional<Point> ht = Intersection::line_plane(rul, top_plane, false);
                    const std::optional<Point> hb = Intersection::line_plane(rul, bot_plane, false);
                    top_ring[k] = ht ? *ht : raw_top[k];
                    bot_ring[k] = hb ? *hb : raw_bot[k];
                }
            }
        } else {
            // 010_blocks.py: intersect corner rulings Line(bottom, top) with best-fit flat extrados plane!
            for (size_t k = 0; k < m; k++) {
                const Line rul = Line::from_points(raw_bot[k], raw_top[k]);
                const std::optional<Point> ht = Intersection::line_plane(rul, top_plane, false);
                top_ring[k] = (ht && ht->distance(raw_top[k]) < 600.0) ? *ht : raw_top[k];
                if (guide.flatten_intrados) {
                    const std::optional<Point> hb = Intersection::line_plane(rul, bot_plane, false);
                    bot_ring[k] = (hb && hb->distance(raw_bot[k]) < 600.0) ? *hb : raw_bot[k];
                } else {
                    bot_ring[k] = raw_bot[k];
                }
            }
        }

        // Measure side & top cap planarity residuals
        {
            const Plane check_top = Plane::from_points_pca(top_ring);
            for (const Point& tp : top_ring) {
                local_m.top_cap_planarity = std::max(
                    local_m.top_cap_planarity,
                    std::abs((tp - check_top.origin()).dot(check_top.z_axis()))
                );
            }
            for (size_t k = 0; k < m; k++) {
                const size_t nk = (k + 1) % m;
                const std::vector<Point> sq = {bot_ring[k], bot_ring[nk], top_ring[nk], top_ring[k]};
                const Plane sp = Plane::from_points_pca(sq);
                for (const Point& qp : sq) {
                    local_m.side_planarity = std::max(
                        local_m.side_planarity,
                        std::abs((qp - sp.origin()).dot(sp.z_axis()))
                    );
                }
            }
        }

        // Assemble closed block polyhedron (planar extrados cap + flush intrados + planar side quads)
        std::vector<std::vector<Point>> pfaces;
        if (m == 3) {
            pfaces.emplace_back(bot_ring.rbegin(), bot_ring.rend());
            pfaces.push_back(top_ring);
        } else {
            const Point bot_c = Point::centroid(bot_ring);
            const Point top_c = Point::centroid(top_ring);
            for (size_t k = 0; k < m; k++) {
                const size_t nk = (k + 1) % m;
                pfaces.push_back({bot_c, bot_ring[nk], bot_ring[k]});
                pfaces.push_back({top_c, top_ring[k], top_ring[nk]});
            }
        }
        for (size_t k = 0; k < m; k++) {
            const size_t nk = (k + 1) % m;
            pfaces.push_back({bot_ring[k], bot_ring[nk], top_ring[nk], top_ring[k]});
        }

        Mesh solid = Mesh::from_polylines(pfaces, 1e-4);
        solid.orient_outward();
        std::shared_ptr<NodeBlockElement> block =
            std::make_shared<NodeBlockElement>(solid, fmt::format("calgary_block_{}", fi));

        std::vector<Plane> cuts;
        if (guide.trim_supports && touches_support) {
            local_m.support_blocks++;
            const double support_z = (min_mid_z < 750.0) ? 0.0 : 1500.0;
            cuts.emplace_back(Point(0.0, 0.0, support_z), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0));
        }

        // 011_chamfer.py: slice sharp trivalent interior corners (angle <= chamfer_max_angle_deg)
        if (guide.chamfer_offset > 1e-3) {
            for (size_t k = 0; k < m; k++) {
                const size_t vid = f[k];
                if (is_bnd_v[vid]) {
                    continue;
                }
                const size_t v_prev = f[(k + m - 1) % m];
                const size_t v_next = f[(k + 1) % m];
                const Vector& nv_k = nrms[vid];
                Vector v1 = (pts[v_prev] - pts[vid]) - nv_k * (pts[v_prev] - pts[vid]).dot(nv_k);
                Vector v2 = (pts[v_next] - pts[vid]) - nv_k * (pts[v_next] - pts[vid]).dot(nv_k);
                if (!v1.normalize_self() || !v2.normalize_self()) {
                    continue;
                }
                const double ang_deg =
                    std::acos(std::clamp(v1.dot(v2), -1.0, 1.0)) * Tolerance::TO_DEGREES;
                if (ang_deg > guide.chamfer_max_angle_deg) {
                    continue;
                }
                Vector inward = v1 + v2;
                if (!inward.normalize_self()) {
                    continue;
                }
                const Point c_orig = pts[vid] + inward * guide.chamfer_offset;
                cuts.push_back(plane_with_normal(c_orig, inward));
                local_m.chamfer_cuts++;
            }
        }

        if (!cuts.empty()) {
            block->cuts = cuts;
            block->invalidate_geometry();
        }
        elements.push_back(block);
    }

    resolve_planar_block_overlaps(elements);

    if (metrics) {
        *metrics = local_m;
    }

    return elements;
}

CalgaryVault::CalgaryVault(const CalgaryVaultGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    const std::vector<std::shared_ptr<Element>> pieces =
        calgary_vault(guide, &_metrics, &_dual_mesh);
    for (const std::shared_ptr<Element>& piece : pieces) {
        add(piece);
    }

    compute_face_contacts(0);
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

const Mesh& CalgaryVault::dual_mesh() const {
    return _dual_mesh;
}

const CalgaryDualMetrics& CalgaryVault::metrics() const {
    return _metrics;
}

}
