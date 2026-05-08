#pragma once

#include "nurbssurface.h"
#include "mesh.h"
#include "polyline.h"
#include "json.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>
#include <tuple>

namespace wood_chevron {

/// Load the 23 Annen building NURBS surfaces from annen_surfaces.json.
/// json_path must point to annen_surfaces.json.
/// Format per entry: degree_u/v, n_u/v, u_mults/v_mults (multiplicities),
/// u_nurbsknots/v_nurbsknots (unique values), points[n_u][n_v][xyz].
inline std::vector<session_cpp::NurbsSurface> annen_surfaces(const std::string& json_path) {
    std::ifstream f(json_path);
    if (!f) {
        return {};
    }

    nlohmann::json arr;
    f >> arr;

    auto expand_knots = [](const std::vector<int>& mults,
                           const std::vector<double>& vals) -> std::vector<double> {
        std::vector<double> full;
        full.reserve(mults.size() * 4);
        for (size_t i = 0; i < mults.size(); i++) {
            for (int k = 0; k < mults[i]; k++) {
                full.push_back(vals[i]);
            }
        }
        // strip first and last to get OpenNURBS nurbsknot vector
        if (full.size() >= 2) {
            return std::vector<double>(full.begin() + 1, full.end() - 1);
        }
        return full;
    };

    std::vector<session_cpp::NurbsSurface> surfaces;
    surfaces.reserve(arr.size());

    for (auto& s : arr) {
        int deg_u = s["degree_u"];
        int deg_v = s["degree_v"];
        int n_u   = s["n_u"];
        int n_v   = s["n_v"];

        auto u_mults = s["u_mults"].get<std::vector<int>>();
        auto v_mults = s["v_mults"].get<std::vector<int>>();
        auto u_vals  = s["u_nurbsknots"].get<std::vector<double>>();
        auto v_vals  = s["v_nurbsknots"].get<std::vector<double>>();

        auto knots_u = expand_knots(u_mults, u_vals);
        auto knots_v = expand_knots(v_mults, v_vals);

        session_cpp::NurbsSurface srf;
        srf.create_raw(3, false, deg_u + 1, deg_v + 1, n_u, n_v);

        for (int i = 0; i < (int)knots_u.size(); i++) {
            srf.set_nurbsknot(0, i, knots_u[i]);
        }
        for (int j = 0; j < (int)knots_v.size(); j++) {
            srf.set_nurbsknot(1, j, knots_v[j]);
        }

        const auto& pts = s["points"];
        for (int i = 0; i < n_u; i++) {
            for (int j = 0; j < n_v; j++) {
                srf.set_cv(i, j, session_cpp::Point(
                    pts[i][j][0].get<double>(),
                    pts[i][j][1].get<double>(),
                    pts[i][j][2].get<double>()));
            }
        }

        if (srf.is_valid()) {
            surfaces.push_back(std::move(srf));
        }
    }

    return surfaces;
}

/// Generate a chevron (V-shaped zigzag) quad mesh on a NURBS surface.
/// u_divisions  : number of strips across the short axis.
/// v_division_dist : target panel height in model units along the long axis.
/// shift  : fraction of v-step by which the middle peak point is offset.
/// scale  : growth factor per row (adaptive spacing for curvature change).
inline session_cpp::Mesh chevron_mesh(const session_cpp::NurbsSurface& surface,
                                      int    u_divisions    = 4,
                                      double v_division_dist = 900.0,
                                      double shift           = 0.5,
                                      double scale           = 0.05799) {
    session_cpp::NurbsSurface srf = surface;

    // Always transpose so u becomes the march direction and v the row direction.
    // This matches the original Python: s = s.Transpose() (unconditional).
    // The caller is responsible for orienting the surface so that after Transpose
    // the v domain spans the desired row-height direction.
    srf.transpose();

    auto du = srf.domain(0);
    auto dv = srf.domain(1);

    double half_v    = (dv.first + dv.second) * 0.5;  // domain midpoint
    double StepU     = (du.second - du.first) / u_divisions;
    double totalV    = dv.second - dv.first;
    // Direct parametric step — matches Python: baseStepV = v_division_dist.
    // Requires surfaces whose parametric domain is in the same units as
    // v_division_dist (mm). default_surface() is centred, Annen surfaces start at 0.
    double baseStepV = v_division_dist;

    std::vector<std::vector<session_cpp::Point>> polygons;

    double ctU = du.first;
    for (int j = 0; j < u_divisions; j++) {
        double ctV    = dv.first;
        double thresh = totalV / 2.0;
        double StepV1 = baseStepV;
        bool   running = true;
        std::vector<double> ListV;

        session_cpp::Point p0, p1, p2, p6, p7, p8;
        session_cpp::Point savept6, savept7, savept8;
        int iterations = 0;

        while (running && iterations < 1000) {
            iterations++;
            ListV.push_back(StepV1);

            if (iterations == 1) {
                p0 = srf.point_at(ctU,             ctV);
                p1 = srf.point_at(ctU + StepU*0.5, ctV);
                p2 = srf.point_at(ctU + StepU,     ctV);
                p6 = srf.point_at(ctU,             ctV + StepV1*(1.0 - shift/2.0));
                p7 = srf.point_at(ctU + StepU*0.5, ctV + StepV1*(1.0 + shift/2.0));
                p8 = srf.point_at(ctU + StepU,     ctV + StepV1*(1.0 - shift/2.0));
                savept6 = p6; savept7 = p7; savept8 = p8;
            } else {
                p0 = savept6; p1 = savept7; p2 = savept8;
                p6 = srf.point_at(ctU,             ctV + StepV1*(1.0 - shift/2.0));
                p7 = srf.point_at(ctU + StepU*0.5, ctV + StepV1*(1.0 + shift/2.0));
                p8 = srf.point_at(ctU + StepU,     ctV + StepV1*(1.0 - shift/2.0));
                savept6 = p6; savept7 = p7; savept8 = p8;
            }

            polygons.push_back({p0, p6, p7, p1});
            polygons.push_back({p1, p7, p8, p2});

            ctV    += StepV1;
            thresh -= StepV1;
            StepV1 += StepV1 * scale;

            if (ctV + StepV1 > half_v) {
                ListV.push_back(thresh);
                std::reverse(ListV.begin(), ListV.end());
                double revCt = (dv.first + dv.second) * 0.5;

                for (size_t i = 0; i < ListV.size() - 1; i++) {
                    revCt += ListV[i];

                    if (i == 0) {
                        p0 = srf.point_at(ctU,             revCt - ListV[i+1]*shift/2.0);
                        p1 = srf.point_at(ctU + StepU*0.5, revCt + ListV[i+1]*shift/2.0);
                        p2 = srf.point_at(ctU + StepU,     revCt - ListV[i+1]*shift/2.0);

                        polygons.push_back({p6, p0, p1, p7});
                        polygons.push_back({p7, p1, p2, p8});

                        if (i == ListV.size() - 2) {
                            // Only 1 reverse step: close flat at surface boundary
                            // (no zigzag peak, avoids evaluation beyond dv.second)
                            p6 = srf.point_at(ctU,             revCt + ListV[i+1]);
                            p7 = srf.point_at(ctU + StepU*0.5, revCt + ListV[i+1]);
                            p8 = srf.point_at(ctU + StepU,     revCt + ListV[i+1]);
                        } else {
                            p6 = srf.point_at(ctU,             revCt + ListV[i+1]*(1.0 - shift/2.0));
                            p7 = srf.point_at(ctU + StepU*0.5, revCt + ListV[i+1]*(1.0 + shift/2.0));
                            p8 = srf.point_at(ctU + StepU,     revCt + ListV[i+1]*(1.0 - shift/2.0));
                            savept6 = p6; savept7 = p7; savept8 = p8;
                        }
                    } else if (i == ListV.size() - 2) {
                        p0 = savept6; p1 = savept7; p2 = savept8;
                        p6 = srf.point_at(ctU,             revCt + ListV[i+1]);
                        p7 = srf.point_at(ctU + StepU*0.5, revCt + ListV[i+1]);
                        p8 = srf.point_at(ctU + StepU,     revCt + ListV[i+1]);
                    } else {
                        p0 = savept6; p1 = savept7; p2 = savept8;
                        p6 = srf.point_at(ctU,             revCt + ListV[i+1]*(1.0 - shift/2.0));
                        p7 = srf.point_at(ctU + StepU*0.5, revCt + ListV[i+1]*(1.0 + shift/2.0));
                        p8 = srf.point_at(ctU + StepU,     revCt + ListV[i+1]*(1.0 - shift/2.0));
                        savept6 = p6; savept7 = p7; savept8 = p8;
                    }

                    polygons.push_back({p1, p7, p8, p2});
                    polygons.push_back({p0, p6, p7, p1});
                }

                running = false;
            }
        }

        ctU += StepU;
    }

    return session_cpp::Mesh::from_polylines(polygons, 0.01);
}

/// Output of chevron_plates() — plate geometry + full joinery solver data.
/// Matches the Grasshopper get_joinery_solver_output() from code.py.
///
/// Plate ordering: 8 polylines per mesh face (in f_order), grouped as 4
/// plate-pairs (index k = counter*4 + role):
///   role 0 = top face plate  (plines counter*8+0/1)
///   role 1 = bottom face plate (plines counter*8+2/3)
///   role 2 = side plate at chevron edge 0 (plines counter*8+4/5)
///   role 3 = side plate at chevron edge 1 (plines counter*8+6/7)
struct ChevronResult {
    /// 8 polylines per face (all faces in f_order, consecutively).
    std::vector<session_cpp::Polyline> plines;

    /// Insertion vector per plate-pair.  One line = one plate-pair = 6 Vec3
    /// packed as 18 doubles (x0 y0 z0  x1 y1 z1  ...  x5 y5 z5).
    /// Positions 0-1 are zero (top/bottom faces of the plate).
    /// Positions 2-5 are the bisector directions for the T-joint tenons.
    std::vector<std::array<double,18>> insertion_vectors;

    /// Joint type per plate-pair.  One entry = 6 ints (one per plate face).
    ///   0  = no joint
    ///  10  = tenon (male)
    ///  20  = mortise (female)
    std::vector<std::array<int,6>> joints_per_face;

    /// Three-valence alignment groups (Annen method, type 0).
    /// Each row [s0, s1, e20, e31]: plate-pair s0 connects to s1,
    /// and plate-pair e20 also connects to s1 — the joint on s1 must be
    /// trimmed so both tenons fit without colliding.
    /// Written to <dataset>_three_valence.txt for get_connection_zones().
    std::vector<std::array<int,4>> three_valence;

    /// Adjacency pairs (plate-pair index pairs that share a joint).
    /// Written to <dataset>_adjacency.txt for get_connection_zones().
    std::vector<std::pair<int,int>> adjacency;

    /// One 2-point polyline per mesh face (box bisector visualization line).
    std::vector<session_cpp::Polyline> box_insertion_lines;
};

/// Generate top/bottom/side plate polylines for each chevron mesh face.
///
/// Implements the full 6-phase algorithm from code.py:
///   1. stripper()              — assign chevron-connector edges per face
///   2. get_edge_planes()       — build cutting planes at each face edge
///   3. rotate_planes()         — offset/rotate chevron edges, snap boundaries
///   4. get_bisector_planes()   — dihedral bisector at each corner
///   5. get_plates()            — intersect planes to get plate polylines
///   6. get_joinery_solver_output() — insertion vectors, joint types,
///                                    three_valence, adjacency
///
/// Returns ChevronResult with 8 polylines per face (in f_order) plus
/// the joinery data needed to drive get_connection_zones().
inline ChevronResult chevron_plates(
    const session_cpp::Mesh& mesh,
    double edge_rotation   = 1.0,
    double edge_offset     = 0.5,
    double box_height      = 760.0,
    double top_plate_inlet = 80.0,
    double plate_thickness = 40.0,
    std::array<int,4> ortho_edges = {1,1,1,1})
{
    // ── parameter clamping ────────────────────────────────────────────────
    edge_rotation   = std::clamp(edge_rotation,  -30.0, 30.0);
    edge_offset     = std::clamp(edge_offset,     -2.0,  2.0);
    box_height      = std::max(box_height, 1.0);
    top_plate_inlet = std::clamp(top_plate_inlet, 1.0, box_height * 0.333);
    plate_thickness = std::clamp(plate_thickness, 1.0, box_height * 0.333);

    // ── vector math helpers ───────────────────────────────────────────────
    using V3 = std::array<double, 3>;

    auto add3  = [](const V3& a, const V3& b) -> V3 { return {a[0]+b[0], a[1]+b[1], a[2]+b[2]}; };
    auto sub3  = [](const V3& a, const V3& b) -> V3 { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; };
    auto sc3   = [](const V3& a, double s)    -> V3 { return {a[0]*s,    a[1]*s,    a[2]*s   }; };
    auto dot3  = [](const V3& a, const V3& b) -> double {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };
    auto cross3 = [](const V3& a, const V3& b) -> V3 {
        return { a[1]*b[2] - a[2]*b[1],
                 a[2]*b[0] - a[0]*b[2],
                 a[0]*b[1] - a[1]*b[0] };
    };
    auto len3  = [&](const V3& a) { return std::sqrt(dot3(a, a)); };
    auto norm3 = [&](const V3& a) -> V3 {
        double n = std::sqrt(dot3(a, a));
        return n > 1e-12 ? sc3(a, 1.0/n) : V3{0, 0, 1};
    };

    // Gauss-Jordan solve for 3×3: A[row] are the normal rows, b is the rhs
    auto solve3 = [](const std::array<V3,3>& A, const V3& b_in, V3& x) -> bool {
        double M[3][4];
        for (int i = 0; i < 3; i++) {
            M[i][0] = A[i][0]; M[i][1] = A[i][1]; M[i][2] = A[i][2]; M[i][3] = b_in[i];
        }
        for (int col = 0; col < 3; col++) {
            int pivot = col;
            double best = std::abs(M[col][col]);
            for (int row = col+1; row < 3; row++) {
                if (std::abs(M[row][col]) > best) { best = std::abs(M[row][col]); pivot = row; }
            }
            if (best < 1e-12) {
                return false;
            }
            if (pivot != col) {
                for (int k = 0; k < 4; k++) {
                    std::swap(M[col][k], M[pivot][k]);
                }
            }
            double inv = 1.0 / M[col][col];
            for (int row = 0; row < 3; row++) {
                if (row == col) {
                    continue;
                }
                double f = M[row][col] * inv;
                for (int k = col; k < 4; k++) {
                    M[row][k] -= f * M[col][k];
                }
            }
        }
        x = { M[0][3]/M[0][0], M[1][3]/M[1][1], M[2][3]/M[2][2] };
        return true;
    };

    // ── Plane: origin o, unit axes x y z (z = cross(x,y) = the normal) ──
    struct Pl { V3 o, x, y, z; };

    auto make_plane = [&](const V3& o, const V3& x_in, const V3& y_in) -> Pl {
        V3 x = norm3(x_in);
        V3 y = norm3(y_in);
        V3 z = norm3(cross3(x, y));
        return {o, x, y, z};
    };

    // Translate origin along z by d
    auto pl_translate = [&](const Pl& p, double d) -> Pl {
        return {add3(p.o, sc3(p.z, d)), p.x, p.y, p.z};
    };

    // Move along z (= translate_by_normal / move_along_axis axis=2)
    auto pl_move_z = [&](const Pl& p, double d) -> Pl {
        return pl_translate(p, d);
    };

    // Flip x (and z, since z = cross(x,y)); y stays.  Matches Plane.flip_x().
    auto pl_flip_x = [](const Pl& p) -> Pl {
        return { p.o,
                 {-p.x[0], -p.x[1], -p.x[2]},
                 p.y,
                 {-p.z[0], -p.z[1], -p.z[2]} };
    };

    // Rotate around own Y-axis by angle_rad.  Matches Rhino Plane.Rotate(angle, YAxis).
    auto pl_rotate_y = [&](const Pl& p, double angle) -> Pl {
        double c = std::cos(angle), s = std::sin(angle);
        V3 new_x = norm3(add3(sc3(p.x, c), sc3(p.z, -s)));
        V3 new_z = norm3(add3(sc3(p.x, s), sc3(p.z,  c)));
        V3 new_y = cross3(new_z, new_x);
        return {p.o, new_x, new_y, new_z};
    };

    // Snap z to a world axis, rebuild x/y.
    // axis: 1=auto (dominant), 2=X, 3=Y, 4=Z
    auto pl_ortho = [&](const Pl& p, int axis) -> Pl {
        int idx; double sign;
        if (axis >= 2 && axis <= 4) {
            idx = axis - 2;
            sign = (p.z[idx] >= 0) ? 1.0 : -1.0;
        } else {
            idx = 0; double best = 0;
            for (int i = 0; i < 3; i++) if (std::abs(p.z[i]) > best) { best = std::abs(p.z[i]); idx = i; }
            sign = (p.z[idx] >= 0) ? 1.0 : -1.0;
        }
        V3 new_z = {0,0,0}; new_z[idx] = sign;
        V3 ref = (idx != 0) ? V3{1,0,0} : V3{0,1,0};
        V3 new_x = norm3(cross3(ref, new_z));
        V3 new_y = cross3(new_z, new_x);
        return {p.o, new_x, new_y, new_z};
    };

    // Intersect 3 planes → point
    auto ppp = [&](const Pl& p0, const Pl& p1, const Pl& p2) -> std::optional<V3> {
        std::array<V3,3> A = {p0.z, p1.z, p2.z};
        V3 b = { dot3(p0.z, p0.o), dot3(p1.z, p1.o), dot3(p2.z, p2.o) };
        V3 x;
        if (!solve3(A, b, x)) {
            return std::nullopt;
        }
        return x;
    };

    // Intersect 2 planes → (anchor, direction)  Matches plane_plane_line().
    auto ppl = [&](const Pl& p0, const Pl& p1)
               -> std::pair<std::optional<V3>, std::optional<V3>> {
        V3 d = cross3(p0.z, p1.z);
        double dn = len3(d);
        if (dn < 1e-10) return {std::nullopt, std::nullopt};
        d = sc3(d, 1.0/dn);
        std::array<V3,3> A = {p0.z, p1.z, d};
        V3 b = { dot3(p0.z, p0.o), dot3(p1.z, p1.o), 0.0 };
        V3 anchor;
        if (!solve3(A, b, anchor)) {
            return {std::nullopt, std::nullopt};
        }
        return {anchor, d};
    };

    // Closest point on line0 given two infinite lines.  Matches line_line_closest().
    auto llc = [&](const V3& o0, const V3& d0, const V3& o1, const V3& d1)
               -> std::tuple<double, double, V3> {
        V3 w = sub3(o0, o1);
        double a = dot3(d0, d0), b = dot3(d0, d1), c = dot3(d1, d1);
        double dv = dot3(d0, w), e = dot3(d1, w);
        double denom = a*c - b*b;
        if (std::abs(denom) < 1e-12) return {0, 0, o0};
        double t0 = (b*e - c*dv) / denom;
        double t1 = (a*e - b*dv) / denom;
        return {t0, t1, add3(o0, sc3(d0, t0))};
    };

    // Dihedral bisector plane.  Matches dihedral_plane().
    auto dihedral = [&](const Pl& p0, const Pl& p1) -> std::optional<Pl> {
        auto [anch_opt, dir_opt] = ppl(p0, p1);
        if (!anch_opt) {
            return std::nullopt;
        }
        if (dot3(p0.z, p1.z) > 1.0 - 0.01) {
            return std::nullopt;
        }
        if (len3(sub3(p0.o, p1.o)) < 0.001) {
            return std::nullopt;
        }

        auto [t0, t1, center] = llc(p0.o, p0.z, p1.o, p1.z);
        V3 v0 = norm3(sub3(p0.o, center));
        V3 v1 = norm3(sub3(p1.o, center));
        V3 bis = add3(v0, v1);
        double bn = len3(bis);
        if (bn < 1e-12) {
            return std::nullopt;
        }
        bis = sc3(bis, 1.0/bn);

        V3 ldir = *dir_opt;
        return Pl{ *anch_opt, ldir, bis, norm3(cross3(ldir, bis)) };
    };

    // Closed polyline from intersecting base plane with n side planes in a loop.
    auto poly_from_planes = [&](const Pl& base, const std::vector<Pl>& sides) -> std::vector<V3> {
        int ns = (int)sides.size();
        std::vector<V3> pts;
        pts.reserve(ns + 1);
        for (int i = 0; i < ns; i++) {
            auto pt = ppp(base, sides[i], sides[(i+1)%ns]);
            pts.push_back(pt ? *pt : base.o);
        }
        pts.push_back(pts.front());
        return pts;
    };

    // ── 1. Extract mesh topology ──────────────────────────────────────────
    std::vector<size_t> fkeys;
    for (auto& [fk, _] : mesh.face) {
        fkeys.push_back(fk);
    }
    int n = (int)fkeys.size();
    if (n == 0) return {};

    // Vertex position lookup
    auto vpos = [&](size_t vk) -> V3 {
        const auto& vd = mesh.vertex.at(vk);
        return {vd.x, vd.y, vd.z};
    };

    // Per-face vertex list — reversed to match mesh.Flip(True,True,True) from
    // the reference Grasshopper implementation, which reverses face winding and
    // flips all normals before any processing.
    std::vector<std::vector<size_t>> fv(n);
    for (int i = 0; i < n; i++) {
        auto opt = mesh.face_vertices(fkeys[i]);
        if (opt) {
            fv[i] = *opt;
            std::reverse(fv[i].begin(), fv[i].end());
        }
    }

    // Face normal from the (already-reversed) vertex list.
    // Equivalent to the flipped face normals after mesh.Flip().
    auto face_norm = [&](int fi) -> V3 {
        if ((int)fv[fi].size() < 3) return {0.0, 0.0, 1.0};
        V3 a = vpos(fv[fi][0]), b = vpos(fv[fi][1]), c = vpos(fv[fi][2]);
        return norm3(cross3(sub3(b, a), sub3(c, a)));
    };

    // Edge → adjacent face indices
    std::map<std::pair<size_t,size_t>, std::vector<int>> edge_adj;
    for (int i = 0; i < n; i++) {
        int nv = (int)fv[i].size();
        for (int j = 0; j < nv; j++) {
            size_t u = fv[i][j], v = fv[i][(j+1)%nv];
            edge_adj[{std::min(u,v), std::max(u,v)}].push_back(i);
        }
    }
    auto adj_faces = [&](size_t vi0, size_t vi1) -> std::vector<int>& {
        return edge_adj[{std::min(vi0,vi1), std::max(vi0,vi1)}];
    };

    // ── Phase 1: stripper ─────────────────────────────────────────────────
    // Determine which 2 local edges per face are chevron connector edges (f_e),
    // and whether the face is in an "even" strip (f_rf).
    // Uses BFS via _faces_share_strip_edge (edges 0 or 2 shared).
    std::vector<std::array<int,2>> f_e(n);
    std::vector<bool> f_rf(n, false);     // rotation_flip flag
    std::vector<bool> flagged(n, false);
    std::vector<int>  f_order;            // face visit order for plate output
    f_order.reserve(n);

    auto share_strip_edge = [&](int fi, int fj) -> bool {
        if ((int)fv[fi].size() < 4 || (int)fv[fj].size() < 4) {
            return false;
        }
        size_t a=fv[fi][0],b=fv[fi][1],c=fv[fi][2],d=fv[fi][3];
        size_t a1=fv[fj][0],b1=fv[fj][1],c1=fv[fj][2],d1=fv[fj][3];
        // Check odd local edges (1–2 and 3–0): groups faces adjacent in V
        // (same U-column, consecutive V-rows) → V-direction stripes.
        return ((b==c1 && c==b1) ||   // fi edge 1-2 == fj edge 1-2 reversed
                (d==a1 && a==d1) ||   // fi edge 3-0 == fj edge 3-0 reversed
                (b==a1 && c==d1) ||   // fi edge 1-2 == fj edge 3-0 reversed
                (d==c1 && a==b1));    // fi edge 3-0 == fj edge 1-2 reversed
    };

    int strip_idx = 0;
    while (true) {
        int seed = -1;
        for (int i = 0; i < n; i++) if (!flagged[i]) { seed = i; break; }
        if (seed < 0) {
            break;
        }

        bool flag = (strip_idx % 2 == 0);
        std::array<int,2> ce = flag ? std::array<int,2>{3, 0} : std::array<int,2>{0, 1}; // 3, 0

        std::vector<int>  strip;
        std::vector<bool> done;
        strip.push_back(seed);
        done.push_back(false);
        flagged[seed] = true;
        f_e[seed] = ce; f_rf[seed] = flag;

        bool changed = true;
        while (changed) {
            changed = false;
            for (int qi = 0; qi < (int)strip.size(); qi++) {
                if (done[qi]) {
                    continue;
                }
                int fi = strip[qi];
                for (int fj = 0; fj < n; fj++) {
                    if (flagged[fj]) {
                        continue;
                    }
                    if (share_strip_edge(fi, fj)) {
                        f_e[fj] = ce; f_rf[fj] = flag;
                        flagged[fj] = true;
                        strip.push_back(fj);
                        done.push_back(false);
                        changed = true;
                    }
                }
                done[qi] = true;
            }
        }

        // if (strip_idx % 2 == 0)
        //     std::reverse(strip.begin(), strip.end());
        for (int fi : strip) {
            f_order.push_back(fi);
        }
        strip_idx++;
    }

    // ── Phase 2: edge planes ──────────────────────────────────────────────
    // For each face and each local edge, build a cutting plane:
    //   origin = edge midpoint
    //   x      = edge direction (vi0 → vi1)
    //   y      = average face normal of edge's adjacent faces
    //   z      = cross(x, y) — the plane normal

    std::vector<std::vector<Pl>> ep(n, std::vector<Pl>(4));  // edge planes
    std::vector<Pl>              fp(n);                       // face planes

    for (int fi = 0; fi < n; fi++) {
        if ((int)fv[fi].size() < 4) {
            continue;
        }

        // Face normal from the flipped vertex list
        V3 fn = face_norm(fi);

        // Face centroid
        V3 fc = {0,0,0};
        for (size_t vk : fv[fi]) {
            fc = add3(fc, vpos(vk));
        }
        fc = sc3(fc, 1.0 / fv[fi].size());

        // Face plane: z = fn, x = cross(ref, fn), y = cross(fn, x)
        V3 ref = (std::abs(fn[0]) < 0.9) ? V3{1,0,0} : V3{0,1,0};
        V3 fx  = norm3(cross3(ref, fn));
        V3 fy  = cross3(fn, fx);
        fp[fi] = {fc, fx, fy, fn};

        // Edge planes
        for (int j = 0; j < 4; j++) {
            size_t vi0 = fv[fi][j], vi1 = fv[fi][(j+1)%4];
            V3 p0 = vpos(vi0), p1 = vpos(vi1);
            V3 mid = sc3(add3(p0, p1), 0.5);
            V3 ex  = sub3(p0, p1);

            // Average flipped normals of adjacent faces
            const auto& adj = adj_faces(vi0, vi1);
            V3 avg_n = {0,0,0};
            for (int fi2 : adj) {
                avg_n = add3(avg_n, face_norm(fi2));
            }
            avg_n = norm3(avg_n);

            ep[fi][j] = make_plane(mid, ex, avg_n);
        }
    }

    // ── Phase 3: rotate / offset edge planes ─────────────────────────────
    // For chevron interior edges:
    //   even local index → translate along z by plate_thickness * edge_offset
    //   odd  local index → rotate around Y by ±edge_rotation degrees
    //   In both cases, update the neighbor face's corresponding edge plane.
    // Boundary edges (only 1 adjacent face): snap to world axis if ortho=true.

    double angle_rad = edge_rotation * (3.14159265358979323846 / 180.0);

    for (int fi = 0; fi < n; fi++) {
        if ((int)fv[fi].size() < 4) {
            continue;
        }
        for (int j = 0; j < 4; j++) {
            size_t vi0 = fv[fi][j], vi1 = fv[fi][(j+1)%4];
            const auto& adj = adj_faces(vi0, vi1);
            bool is_chevron = (j == f_e[fi][0] || j == f_e[fi][1]);

            if ((int)adj.size() == 2) {
                if (is_chevron) {
                    if (j % 2 == 1) {
                        // even chevron: translate
                        ep[fi][j] = pl_move_z(ep[fi][j], plate_thickness * edge_offset);
                    } else {
                        // odd chevron: rotate
                        // Python: sign = -1 if flip else 1, sign *= -1 → flip? +1 : -1
                        double sign = f_rf[fi] ? 1.0 : -1.0;
                        ep[fi][j] = pl_rotate_y(ep[fi][j], angle_rad * sign);
                    }
                    // propagate to neighbor (flip x/z so it faces the other way)
                    int nb = (adj[0] != fi) ? adj[0] : adj[1];
                    for (int k = 0; k < 4; k++) {
                        size_t u = fv[nb][k], v = fv[nb][(k+1)%4];
                        if (std::min(u,v) == std::min(vi0,vi1) &&
                            std::max(u,v) == std::max(vi0,vi1))
                        {
                            ep[nb][k] = pl_flip_x(ep[fi][j]);
                            break;
                        }
                    }
                }
                // non-chevron interior edge: leave as-is
            } else if (ortho_edges[j] != 0) {
                // boundary edge: snap normal to world axis
                ep[fi][j] = pl_ortho(ep[fi][j], ortho_edges[j]);
            }
        }
    }

    // ── Phase 4: bisector planes ──────────────────────────────────────────
    // bi[fi][j] is the dihedral bisector plane at the vertex between
    // edge j and edge (j+1)%4.  Matches get_bisector_planes() where
    // bi[j] = dihedral(e_planes[(j+1)%4], e_planes[j]).

    std::vector<std::vector<std::optional<Pl>>> bi(n, std::vector<std::optional<Pl>>(4));
    for (int fi = 0; fi < n; fi++) {
        for (int j = 0; j < 4; j++) {
            bi[fi][j] = dihedral(ep[fi][(j+1)%4], ep[fi][j]);
        }
    }

    // ── Phase 5: get plates ───────────────────────────────────────────────
    // Per face in f_order: 4 horizontal plates + 2×2 side plates = 8 polylines.

    const double H   = box_height;
    const double inp = top_plate_inlet;
    const double t   = plate_thickness;

    ChevronResult out;
    out.plines.reserve(n * 8);

    auto add_poly = [&](const Pl& base, const std::vector<Pl>& sides) {
        auto pts = poly_from_planes(base, sides);
        std::vector<session_cpp::Point> spts;
        spts.reserve(pts.size());
        for (auto& p : pts) {
            spts.emplace_back(p[0], p[1], p[2]);
        }
        out.plines.emplace_back(spts);
    };

    for (int fi : f_order) {
        // Edge planes with additional +t offset on chevron edges (for face plates)
        std::vector<Pl> ep_local(4);
        for (int j = 0; j < 4; j++) {
            ep_local[j] = ep[fi][j];
            if (j == f_e[fi][0] || j == f_e[fi][1]) {
                ep_local[j] = pl_move_z(ep_local[j], t);
            }
        }

        const Pl& fplane = fp[fi];

        // 4 horizontal face plates: top pair then bottom pair
        add_poly(pl_translate(fplane,  H*0.5 - inp - t*0.5), ep_local);
        add_poly(pl_translate(fplane,  H*0.5 - inp + t*0.5), ep_local);
        add_poly(pl_translate(fplane, -H*0.5 + inp - t*0.5), ep_local);
        add_poly(pl_translate(fplane, -H*0.5 + inp + t*0.5), ep_local);

        // Sort chevron edge indices; special-case [0,3] → reverse to [3,0]
        std::array<int,2> e_sorted = f_e[fi];
        if (e_sorted[0] > e_sorted[1]) {
            std::swap(e_sorted[0], e_sorted[1]);
        }
        if (e_sorted[0] == 0 && e_sorted[1] == 3) {
            std::swap(e_sorted[0], e_sorted[1]);
        }

        // 2 side plates per chevron edge (base plane + offset by t)
        for (int idx = 0; idx < 2; idx++) {
            int curr = e_sorted[idx];
            int prev = (curr - 1 + 4) % 4;
            int nxt  = (curr + 1) % 4;

            Pl s0 = pl_translate(fplane,  H * 0.5);   // top
            Pl s2 = pl_translate(fplane, -H * 0.5);   // bottom

            Pl s1, s3;
            if (idx == 0) {
                s1 = ep[fi][prev];
                s3 = bi[fi][curr] ? *bi[fi][curr] : ep[fi][nxt];
            } else {
                s1 = bi[fi][prev] ? *bi[fi][prev] : ep[fi][prev];
                s3 = ep[fi][nxt];
            }

            std::vector<Pl> sides = {s0, s1, s2, s3};
            Pl base0 = ep[fi][curr];
            Pl base1 = pl_move_z(base0, t);

            add_poly(base0, sides);
            add_poly(base1, sides);

        }
    }

    // ── Phase 6: joinery solver output ───────────────────────────────────────
    // Implements get_joinery_solver_output() from code.py.
    // Produces insertion_vectors, joints_per_face, three_valence, adjacency.

    int n_ordered = (int)f_order.size();
    int n_pairs   = n_ordered * 4;   // 4 plate-pairs per mesh face

    out.insertion_vectors.assign(n_pairs, {});
    out.joints_per_face.assign(n_pairs, {0,0,0,0,0,0});

    // Map fkeys[] array-index → counter position in f_order
    std::vector<int> face_to_counter(n, -1);
    for (int c = 0; c < n_ordered; c++) {
        face_to_counter[f_order[c]] = c;
    }

    // Centroid of a plate polyline (used for box_insertion_line origin)
    auto poly_centroid = [&](const session_cpp::Polyline& pl) -> V3 {
        V3 c = {0,0,0};
        int np = (int)pl.point_count();
        if (np == 0) {
            return c;
        }
        for (int k = 0; k < np; k++) {
            auto p = pl.get_point(k);
            c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
        }
        return sc3(c, 1.0 / np);
    };

    for (int counter = 0; counter < n_ordered; counter++) {
        int fi = f_order[counter];

        // Sort chevron edges (same logic as Phase 5)
        std::array<int,2> e_s = f_e[fi];
        if (e_s[0] > e_s[1]) {
            std::swap(e_s[0], e_s[1]);
        }
        if (e_s[0] == 0 && e_s[1] == 3) {
            std::swap(e_s[0], e_s[1]);
        }

        // Bisector directions: intersection line of face-plane with bisector-plane.
        // bi[fi][j] = dihedral(e_planes[(j+1)%4], e_planes[j])
        //   → bisector at corner j, between edge j and edge (j+1)%4.
        // bisector_dir0: at corner e_s[0] (start of chevron edge 0)
        // bisector_dir1: at corner (e_s[1]+1)%4 (end of chevron edge 1)
        auto bisector_dir = [&](int corner) -> V3 {
            if (!bi[fi][corner]) {
                return {0,0,0};
            }
            auto [a, d] = ppl(fp[fi], *bi[fi][corner]);
            return d ? norm3(*d) : V3{0,0,0};
        };
        V3 bdir0 = bisector_dir(e_s[0]);
        V3 bdir1 = bisector_dir((e_s[1] + 1) % 4);

        // ── Insertion vectors ─────────────────────────────────────────────
        // Top and bottom face plates: positions 0,1 = zero; 2-5 = bdir1,
        // then override positions (2+(e_s[1]+2)%4) and (2+(e_s[1]+3)%4) with bdir0.
        {
            std::array<double,18> ins = {};
            // Set positions 2..5 to bdir1
            for (int s = 2; s < 6; s++) {
                ins[s*3+0] = bdir1[0]; ins[s*3+1] = bdir1[1]; ins[s*3+2] = bdir1[2];
            }
            // Override two positions with bdir0
            for (int off : {2, 3}) {
                int idx = 2 + (e_s[1] + off) % 4;
                ins[idx*3+0] = bdir0[0]; ins[idx*3+1] = bdir0[1]; ins[idx*3+2] = bdir0[2];
            }
            out.insertion_vectors[counter*4+0] = ins;
            out.insertion_vectors[counter*4+1] = ins;
            // Side plates: all zeros (already default-initialised)
        }

        // ── Joint types ───────────────────────────────────────────────────
        // Top/bottom face plates: positions 2-5 are mortises (20).
        out.joints_per_face[counter*4+0] = {0,0,20,20,20,20};
        out.joints_per_face[counter*4+1] = {0,0,20,20,20,20};
        // Side plates: one tenon at position 4, the other at position 2.
        // The wrap-around case [3,0] swaps which side gets which.
        int a_idx = (e_s[0] == 3 && e_s[1] == 0) ? 1 : 0;
        int b_idx = 1 - a_idx;
        out.joints_per_face[counter*4+2+a_idx] = {0,0,0,0,10,0};  // tenon at pos 4
        out.joints_per_face[counter*4+2+b_idx] = {0,0,10,0,0,0};  // tenon at pos 2

        // ── Within-box adjacency ──────────────────────────────────────────
        // top↔side0, top↔side1, bot↔side0, bot↔side1, side0↔side1
        for (int role_a : {0, 1}) {
            for (int role_b : {2, 3}) {
                out.adjacency.emplace_back(counter*4+role_a, counter*4+role_b);
            }
        }
        out.adjacency.emplace_back(counter*4+2, counter*4+3);

        // ── Cross-box adjacency + three_valence ───────────────────────────
        // For each chevron edge (ei=0 → side2, ei=1 → side3):
        // find the adjacent mesh face; if it exists, wire up adjacency and
        // add two three_valence rows (top and bottom) for alignment.
        for (int ei = 0; ei < 2; ei++) {
            int cedge = e_s[ei];
            size_t vi0 = fv[fi][cedge], vi1 = fv[fi][(cedge+1)%4];
            const auto& adf = adj_faces(vi0, vi1);
            if ((int)adf.size() != 2) {
                continue;
            }
            int nb_fi = (adf[0] != fi) ? adf[0] : adf[1];
            int nei = face_to_counter[nb_fi];
            if (nei < 0) {
                continue;
            }

            // Neighbor's top and bottom face plates connect to current side plate
            out.adjacency.emplace_back(nei*4+0, counter*4+2+ei);
            out.adjacency.emplace_back(nei*4+1, counter*4+2+ei);

            // Three-valence: current top/bot + current side + neighbor top/bot + same side
            out.three_valence.push_back({counter*4+0, counter*4+2+ei, nei*4+0, counter*4+2+ei});
            out.three_valence.push_back({counter*4+1, counter*4+2+ei, nei*4+1, counter*4+2+ei});
        }

        // ── Box insertion line (visualization) ────────────────────────────
        // From centroid of pline[counter*8+1] in bdir1 direction × 300.
        V3 ctr = poly_centroid(out.plines[counter * 8 + 1]);
        V3 tip = add3(ctr, sc3(bdir1, 300.0));
        out.box_insertion_lines.emplace_back(std::vector<session_cpp::Point>{
            session_cpp::Point(ctr[0], ctr[1], ctr[2]),
            session_cpp::Point(tip[0], tip[1], tip[2])
        });
    }

    return out;
}

} // namespace wood_chevron
