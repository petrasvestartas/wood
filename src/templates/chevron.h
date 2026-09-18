#pragma once
#include "session.h"
#include "wood_session.h"
#include "nurbssurface.h"
#include "mesh.h"
#include "polyline.h"
#include "json.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace wood_chevron {

/// Full knot vector from unique knot values and their multiplicities, with the first and last knot stripped as OpenNURBS stores it.
inline std::vector<double> expand_knots(const std::vector<int>& mults,
                                        const std::vector<double>& vals) {
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
}

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

    std::vector<session_cpp::NurbsSurface> surfaces;
    surfaces.reserve(arr.size());

    for (nlohmann::json& s : arr) {

        int deg_u = s["degree_u"];
        int deg_v = s["degree_v"];
        int n_u   = s["n_u"];
        int n_v   = s["n_v"];

        std::vector<int> u_mults = s["u_mults"].get<std::vector<int>>();
        std::vector<int> v_mults = s["v_mults"].get<std::vector<int>>();
        std::vector<double> u_vals  = s["u_nurbsknots"].get<std::vector<double>>();
        std::vector<double> v_vals  = s["v_nurbsknots"].get<std::vector<double>>();

        std::vector<double> knots_u = expand_knots(u_mults, u_vals);
        std::vector<double> knots_v = expand_knots(v_mults, v_vals);

        session_cpp::NurbsSurface srf;
        srf.create_raw(3, false, deg_u + 1, deg_v + 1, n_u, n_v);

        for (int i = 0; i < (int)knots_u.size(); i++) {
            srf.set_nurbsknot(0, i, knots_u[i]);
        }
        for (int j = 0; j < (int)knots_v.size(); j++) {
            srf.set_nurbsknot(1, j, knots_v[j]);
        }

        const nlohmann::json& pts = s["points"];
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

    std::pair<double,double> du = srf.domain(0);
    std::pair<double,double> dv = srf.domain(1);

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

/// A 3D vector as a plain array: x, y, z.
using Vector3 = std::array<double, 3>;

/// Sum of two vectors.
inline Vector3 add(const Vector3& a, const Vector3& b) {
    return {a[0]+b[0], a[1]+b[1], a[2]+b[2]};
}

/// Difference a - b of two vectors.
inline Vector3 subtract(const Vector3& a, const Vector3& b) {
    return {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
}

/// Vector scaled by a factor.
inline Vector3 scale(const Vector3& a, double s) {
    return {a[0]*s, a[1]*s, a[2]*s};
}

/// Dot product of two vectors.
inline double dot(const Vector3& a, const Vector3& b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/// Cross product a x b of two vectors.
inline Vector3 cross(const Vector3& a, const Vector3& b) {
    return { a[1]*b[2] - a[2]*b[1],
             a[2]*b[0] - a[0]*b[2],
             a[0]*b[1] - a[1]*b[0] };
}

/// Euclidean length of a vector.
inline double length(const Vector3& a) {
    return std::sqrt(dot(a, a));
}

/// Unit vector along a, or world z when a is shorter than 1e-12.
inline Vector3 normalize(const Vector3& a) {
    double n = std::sqrt(dot(a, a));
    return n > 1e-12 ? scale(a, 1.0/n) : Vector3{0, 0, 1};
}

/// Gauss-Jordan solve of the 3x3 system A x = b, A[row] being the normal rows and b the right-hand side; false when singular.
inline bool solve_3x3(const std::array<Vector3,3>& A, const Vector3& b_in, Vector3& x) {
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
}

/// A plane as origin o and unit axes x, y, z, where z = cross(x, y) is the normal.
struct Plane { Vector3 o, x, y, z; };

/// Plane from an origin and two axis directions, both normalized; z = cross(x, y).
inline Plane make_plane(const Vector3& o, const Vector3& x_in, const Vector3& y_in) {
    Vector3 x = normalize(x_in);
    Vector3 y = normalize(y_in);
    Vector3 z = normalize(cross(x, y));
    return {o, x, y, z};
}

/// Plane with its origin translated along z by d (translate_by_normal / move_along_axis axis=2).
inline Plane plane_translate(const Plane& p, double d) {
    return {add(p.o, scale(p.z, d)), p.x, p.y, p.z};
}

/// Plane with x flipped (and z, since z = cross(x,y)); y stays.  Matches Plane.flip_x().
inline Plane plane_flip_x(const Plane& p) {
    return { p.o,
             {-p.x[0], -p.x[1], -p.x[2]},
             p.y,
             {-p.z[0], -p.z[1], -p.z[2]} };
}

/// Plane rotated around its own y axis by angle in radians.  Matches Rhino Plane.Rotate(angle, YAxis).
inline Plane plane_rotate_y(const Plane& p, double angle) {
    double c = std::cos(angle), s = std::sin(angle);
    Vector3 new_x = normalize(add(scale(p.x, c), scale(p.z, -s)));
    Vector3 new_z = normalize(add(scale(p.x, s), scale(p.z,  c)));
    Vector3 new_y = cross(new_z, new_x);
    return {p.o, new_x, new_y, new_z};
}

/// Plane with z snapped to a world axis and x, y rebuilt; axis: 1=auto (dominant), 2=X, 3=Y, 4=Z.
inline Plane plane_orthogonal(const Plane& p, int axis) {
    int idx; double sign;
    if (axis >= 2 && axis <= 4) {
        idx = axis - 2;
        sign = (p.z[idx] >= 0) ? 1.0 : -1.0;
    } else {
        idx = 0; double best = 0;
        for (int i = 0; i < 3; i++) if (std::abs(p.z[i]) > best) { best = std::abs(p.z[i]); idx = i; }
        sign = (p.z[idx] >= 0) ? 1.0 : -1.0;
    }
    Vector3 new_z = {0,0,0}; new_z[idx] = sign;
    Vector3 ref = (idx != 0) ? Vector3{1,0,0} : Vector3{0,1,0};
    Vector3 new_x = normalize(cross(ref, new_z));
    Vector3 new_y = cross(new_z, new_x);
    return {p.o, new_x, new_y, new_z};
}

/// Intersection point of three planes, or nullopt when they do not meet in one point.
inline std::optional<Vector3> plane_plane_plane_intersection(const Plane& p0, const Plane& p1, const Plane& p2) {
    std::array<Vector3,3> A = {p0.z, p1.z, p2.z};
    Vector3 b = { dot(p0.z, p0.o), dot(p1.z, p1.o), dot(p2.z, p2.o) };
    Vector3 x;
    if (!solve_3x3(A, b, x)) {
        return std::nullopt;
    }
    return x;
}

/// Intersection line of two planes as (anchor, direction), both nullopt when parallel.  Matches plane_plane_line().
inline std::pair<std::optional<Vector3>, std::optional<Vector3>> plane_plane_intersection(const Plane& p0, const Plane& p1) {
    Vector3 d = cross(p0.z, p1.z);
    double dn = length(d);
    if (dn < 1e-10) return {std::nullopt, std::nullopt};
    d = scale(d, 1.0/dn);
    std::array<Vector3,3> A = {p0.z, p1.z, d};
    Vector3 b = { dot(p0.z, p0.o), dot(p1.z, p1.o), 0.0 };
    Vector3 anchor;
    if (!solve_3x3(A, b, anchor)) {
        return {std::nullopt, std::nullopt};
    }
    return {anchor, d};
}

/// Closest approach of two infinite lines: parameters t0, t1 and the closest point on line 0.  Matches line_line_closest().
inline std::tuple<double, double, Vector3> line_line_closest(const Vector3& o0, const Vector3& d0, const Vector3& o1, const Vector3& d1) {
    Vector3 w = subtract(o0, o1);
    double a = dot(d0, d0), b = dot(d0, d1), c = dot(d1, d1);
    double dv = dot(d0, w), e = dot(d1, w);
    double denom = a*c - b*b;
    if (std::abs(denom) < 1e-12) return {0, 0, o0};
    double t0 = (b*e - c*dv) / denom;
    double t1 = (a*e - b*dv) / denom;
    return {t0, t1, add(o0, scale(d0, t0))};
}

/// Dihedral bisector plane of two planes, or nullopt when they are parallel or share an origin.  Matches dihedral_plane().
inline std::optional<Plane> dihedral_plane(const Plane& p0, const Plane& p1) {
    auto [anch_opt, dir_opt] = plane_plane_intersection(p0, p1);
    if (!anch_opt) {
        return std::nullopt;
    }
    if (dot(p0.z, p1.z) > 1.0 - 0.01) {
        return std::nullopt;
    }
    if (length(subtract(p0.o, p1.o)) < 0.001) {
        return std::nullopt;
    }

    auto [t0, t1, center] = line_line_closest(p0.o, p0.z, p1.o, p1.z);
    Vector3 v0 = normalize(subtract(p0.o, center));
    Vector3 v1 = normalize(subtract(p1.o, center));
    Vector3 bis = add(v0, v1);
    double bn = length(bis);
    if (bn < 1e-12) {
        return std::nullopt;
    }
    bis = scale(bis, 1.0/bn);

    Vector3 ldir = *dir_opt;
    return Plane{ *anch_opt, ldir, bis, normalize(cross(ldir, bis)) };
}

/// Closed polygon from intersecting a base plane with n side planes in a loop; a missed corner falls back to the base origin.
inline std::vector<Vector3> polygon_from_planes(const Plane& base, const std::vector<Plane>& sides) {
    int ns = (int)sides.size();
    std::vector<Vector3> pts;
    pts.reserve(ns + 1);
    for (int i = 0; i < ns; i++) {
        std::optional<Vector3> pt = plane_plane_plane_intersection(base, sides[i], sides[(i+1)%ns]);
        pts.push_back(pt ? *pt : base.o);
    }
    pts.push_back(pts.front());
    return pts;
}

/// Position of the mesh vertex with key vk.
inline Vector3 vertex_position(const session_cpp::Mesh& mesh, size_t vk) {
    const session_cpp::VertexData& vd = mesh.vertex.at(vk);
    return {vd.x, vd.y, vd.z};
}

/// Normal of face fi from its (already-reversed) vertex list, the flipped face normal of mesh.Flip(): for a quad the cross product of its diagonals as OpenNURBS computes it, the best-fit normal of a twisted quad; a plane through three vertices would tilt by the twist; world z when degenerate.
inline Vector3 face_normal(const session_cpp::Mesh& mesh, const std::vector<std::vector<size_t>>& face_vertices, int fi) {

    const std::vector<size_t>& vertices = face_vertices[fi];
    if ((int)vertices.size() < 3)
        return {0.0, 0.0, 1.0};

    Vector3 a = vertex_position(mesh, vertices[0]), b = vertex_position(mesh, vertices[1]), c = vertex_position(mesh, vertices[2]);
    if ((int)vertices.size() < 4)
        return normalize(cross(subtract(b, a), subtract(c, a)));

    Vector3 d = vertex_position(mesh, vertices[3]);
    return normalize(cross(subtract(c, a), subtract(d, b)));
}

/// Faces adjacent to the edge vi0-vi1, looked up by the sorted vertex pair.
inline std::vector<int>& adjacent_faces(std::map<std::pair<size_t,size_t>, std::vector<int>>& edge_adjacency, size_t vi0, size_t vi1) {
    return edge_adjacency[{std::min(vi0,vi1), std::max(vi0,vi1)}];
}

/// Whether quad faces fi and fj share an odd local edge (1-2 or 3-0), which groups faces adjacent in V (same U-column, consecutive V-rows) into V-direction stripes.
inline bool faces_share_strip_edge(const std::vector<std::vector<size_t>>& face_vertices, int fi, int fj) {

    if ((int)face_vertices[fi].size() < 4 || (int)face_vertices[fj].size() < 4) {
        return false;
    }

    size_t a=face_vertices[fi][0],b=face_vertices[fi][1],c=face_vertices[fi][2],d=face_vertices[fi][3];
    size_t a1=face_vertices[fj][0],b1=face_vertices[fj][1],c1=face_vertices[fj][2],d1=face_vertices[fj][3];
    return ((b==c1 && c==b1) ||   // fi edge 1-2 == fj edge 1-2 reversed
            (d==a1 && a==d1) ||   // fi edge 3-0 == fj edge 3-0 reversed
            (b==a1 && c==d1) ||   // fi edge 1-2 == fj edge 3-0 reversed
            (d==c1 && a==b1));    // fi edge 3-0 == fj edge 1-2 reversed
}

/// Append the closed polygon cut from base by sides to plines as a polyline.
inline void add_polygon(std::vector<session_cpp::Polyline>& plines, const Plane& base, const std::vector<Plane>& sides) {

    std::vector<Vector3> pts = polygon_from_planes(base, sides);
    std::vector<session_cpp::Point> spts;
    spts.reserve(pts.size());

    for (Vector3& p : pts) {
        spts.emplace_back(p[0], p[1], p[2]);
    }

    plines.emplace_back(spts);
}

/// Centroid of a plate polyline (used for the box_insertion_line origin); zero for an empty polyline.
inline Vector3 polygon_centroid(const session_cpp::Polyline& pl) {

    Vector3 c = {0,0,0};
    int np = (int)pl.point_count();
    if (np == 0) {
        return c;
    }

    for (int k = 0; k < np; k++) {
        session_cpp::Point p = pl.get_point(k);
        c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
    }

    return scale(c, 1.0 / np);
}

/// Unit direction of the intersection line of a face plane with a corner's bisector plane; zero when the bisector is absent or the planes are parallel.
inline Vector3 bisector_direction(const Plane& face_plane, const std::optional<Plane>& bisector_plane) {

    if (!bisector_plane) {
        return {0,0,0};
    }

    auto [a, d] = plane_plane_intersection(face_plane, *bisector_plane);
    return d ? normalize(*d) : Vector3{0,0,0};
}

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

    // ── 1. Extract mesh topology ──────────────────────────────────────────
    std::vector<size_t> fkeys;
    for (auto& [fk, _] : mesh.face) {
        fkeys.push_back(fk);
    }

    int n = (int)fkeys.size();
    if (n == 0) return {};

    // Per-face vertex list — reversed to match mesh.Flip(True,True,True) from
    // the reference Grasshopper implementation, which reverses face winding and
    // flips all normals before any processing.
    std::vector<std::vector<size_t>> fv(n);
    for (int i = 0; i < n; i++) {
        std::optional<std::vector<size_t>> opt = mesh.face_vertices(fkeys[i]);
        if (opt) {
            fv[i] = *opt;
            std::reverse(fv[i].begin(), fv[i].end());
        }
    }

    // Edge → adjacent face indices
    std::map<std::pair<size_t,size_t>, std::vector<int>> edge_adj;
    for (int i = 0; i < n; i++) {
        int nv = (int)fv[i].size();
        for (int j = 0; j < nv; j++) {
            size_t u = fv[i][j], v = fv[i][(j+1)%nv];
            edge_adj[{std::min(u,v), std::max(u,v)}].push_back(i);
        }
    }

    // ── Phase 1: stripper ─────────────────────────────────────────────────
    // Determine which 2 local edges per face are chevron connector edges (f_e),
    // and whether the face is in an "even" strip (f_rf).
    // Uses BFS via _faces_share_strip_edge (edges 0 or 2 shared).
    std::vector<std::array<int,2>> f_e(n);
    std::vector<bool> f_rf(n, false);     // rotation_flip flag
    std::vector<bool> flagged(n, false);
    std::vector<int>  f_order;            // face visit order for plate output
    f_order.reserve(n);

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

                    if (faces_share_strip_edge(fv, fi, fj)) {
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

    std::vector<std::vector<Plane>> ep(n, std::vector<Plane>(4));  // edge planes
    std::vector<Plane>              fp(n);                       // face planes

    for (int fi = 0; fi < n; fi++) {

        if ((int)fv[fi].size() < 4) {
            continue;
        }

        // Face normal from the flipped vertex list
        Vector3 fn = face_normal(mesh, fv, fi);

        // Face centroid
        Vector3 fc = {0,0,0};
        for (size_t vk : fv[fi]) {
            fc = add(fc, vertex_position(mesh, vk));
        }
        fc = scale(fc, 1.0 / fv[fi].size());

        // Face plane: z = fn, x = cross(ref, fn), y = cross(fn, x)
        Vector3 ref = (std::abs(fn[0]) < 0.9) ? Vector3{1,0,0} : Vector3{0,1,0};
        Vector3 fx  = normalize(cross(ref, fn));
        Vector3 fy  = cross(fn, fx);
        fp[fi] = {fc, fx, fy, fn};

        // Edge planes
        for (int j = 0; j < 4; j++) {
            size_t vi0 = fv[fi][j], vi1 = fv[fi][(j+1)%4];
            Vector3 p0 = vertex_position(mesh, vi0), p1 = vertex_position(mesh, vi1);
            Vector3 mid = scale(add(p0, p1), 0.5);
            Vector3 ex  = subtract(p0, p1);

            // Average flipped normals of adjacent faces
            const std::vector<int>& adj = adjacent_faces(edge_adj, vi0, vi1);
            Vector3 avg_n = {0,0,0};
            for (int fi2 : adj) {
                avg_n = add(avg_n, face_normal(mesh, fv, fi2));
            }
            avg_n = normalize(avg_n);

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
            const std::vector<int>& adj = adjacent_faces(edge_adj, vi0, vi1);
            bool is_chevron = (j == f_e[fi][0] || j == f_e[fi][1]);

            if ((int)adj.size() == 2) {
                if (is_chevron) {
                    if (j % 2 == 1) {
                        // even chevron: translate
                        ep[fi][j] = plane_translate(ep[fi][j], plate_thickness * edge_offset);
                    } else {
                        // odd chevron: rotate
                        // Python: sign = -1 if flip else 1, sign *= -1 → flip? +1 : -1
                        double sign = f_rf[fi] ? 1.0 : -1.0;
                        ep[fi][j] = plane_rotate_y(ep[fi][j], angle_rad * sign);
                    }
                    // propagate to neighbor (flip x/z so it faces the other way)
                    int nb = (adj[0] != fi) ? adj[0] : adj[1];
                    for (int k = 0; k < 4; k++) {
                        size_t u = fv[nb][k], v = fv[nb][(k+1)%4];
                        if (std::min(u,v) == std::min(vi0,vi1) &&
                            std::max(u,v) == std::max(vi0,vi1))
                        {
                            ep[nb][k] = plane_flip_x(ep[fi][j]);
                            break;
                        }
                    }
                }
                // non-chevron interior edge: leave as-is
            } else if (ortho_edges[j] != 0) {
                // boundary edge: snap normal to world axis
                ep[fi][j] = plane_orthogonal(ep[fi][j], ortho_edges[j]);
            }
        }
    }

    // ── Phase 4: bisector planes ──────────────────────────────────────────
    // bi[fi][j] is the dihedral bisector plane at the vertex between
    // edge j and edge (j+1)%4.  Matches get_bisector_planes() where
    // bi[j] = dihedral(e_planes[(j+1)%4], e_planes[j]).

    std::vector<std::vector<std::optional<Plane>>> bi(n, std::vector<std::optional<Plane>>(4));
    for (int fi = 0; fi < n; fi++) {
        for (int j = 0; j < 4; j++) {
            bi[fi][j] = dihedral_plane(ep[fi][(j+1)%4], ep[fi][j]);
        }
    }

    // ── Phase 5: get plates ───────────────────────────────────────────────
    // Per face in f_order: 4 horizontal plates + 2×2 side plates = 8 polylines.

    const double H   = box_height;
    const double inp = top_plate_inlet;
    const double t   = plate_thickness;

    ChevronResult out;
    out.plines.reserve(n * 8);

    for (int fi : f_order) {
        // Edge planes with additional +t offset on chevron edges (for face plates)
        std::vector<Plane> ep_local(4);
        for (int j = 0; j < 4; j++) {
            ep_local[j] = ep[fi][j];
            if (j == f_e[fi][0] || j == f_e[fi][1]) {
                ep_local[j] = plane_translate(ep_local[j], t);
            }
        }

        const Plane& fplane = fp[fi];

        // 4 horizontal face plates: top pair then bottom pair
        add_polygon(out.plines, plane_translate(fplane,  H*0.5 - inp - t*0.5), ep_local);
        add_polygon(out.plines, plane_translate(fplane,  H*0.5 - inp + t*0.5), ep_local);
        add_polygon(out.plines, plane_translate(fplane, -H*0.5 + inp - t*0.5), ep_local);
        add_polygon(out.plines, plane_translate(fplane, -H*0.5 + inp + t*0.5), ep_local);

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

            Plane s0 = plane_translate(fplane,  H * 0.5);   // top
            Plane s2 = plane_translate(fplane, -H * 0.5);   // bottom

            Plane s1, s3;
            if (idx == 0) {
                s1 = ep[fi][prev];
                s3 = bi[fi][curr] ? *bi[fi][curr] : ep[fi][nxt];
            } else {
                s1 = bi[fi][prev] ? *bi[fi][prev] : ep[fi][prev];
                s3 = ep[fi][nxt];
            }

            std::vector<Plane> sides = {s0, s1, s2, s3};
            Plane base0 = ep[fi][curr];
            Plane base1 = plane_translate(base0, t);

            add_polygon(out.plines, base0, sides);
            add_polygon(out.plines, base1, sides);

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
        Vector3 bdir0 = bisector_direction(fp[fi], bi[fi][e_s[0]]);
        Vector3 bdir1 = bisector_direction(fp[fi], bi[fi][(e_s[1] + 1) % 4]);

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
        // type-10 at ALL side faces (2-5): build_wood_element may reverse the
        // polyline orientation, which re-numbers face indices.  Covering all
        // four side positions ensures BVH detection succeeds regardless of
        // which face index the shared edge lands on after reversal.
        out.joints_per_face[counter*4+2] = {0,0,10,10,10,10};
        out.joints_per_face[counter*4+3] = {0,0,10,10,10,10};

        // Side plates: insertion_vectors stay all-zeros (already default-initialised).

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
            const std::vector<int>& adf = adjacent_faces(edge_adj, vi0, vi1);
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
        Vector3 ctr = polygon_centroid(out.plines[counter * 8 + 1]);
        Vector3 tip = add(ctr, scale(bdir1, 300.0));
        out.box_insertion_lines.emplace_back(std::vector<session_cpp::Point>{
            session_cpp::Point(ctr[0], ctr[1], ctr[2]),
            session_cpp::Point(tip[0], tip[1], tip[2])
        });
    }

    return out;
}

} // namespace wood_chevron

using namespace session_cpp;
using namespace wood_session;

/// A chevron shell from a NURBS surface with thickness.
///
/// Subdivides a NURBS surface into a V-shaped zigzag (chevron) quad mesh,
/// then offsets each face into 4 plate-pairs per face (top, bottom, side0, side1)
/// stored as Plate objects. Joinery data is also stored for use with
/// get_connection_zones().
///
/// Usage:
///   Chevron ch;                              // built-in flat 3000×5000 surface
///   Chevron ch(my_surface, 4, 900.0);        // custom surface, 4 strips, 900mm rows
class Chevron {
public:
    Mesh mesh;
    std::vector<std::shared_ptr<Plate>> elements;

    /// Joinery solver inputs (see wood_chevron::ChevronResult)
    std::vector<std::array<double,18>> insertion_vectors;
    std::vector<std::array<int,6>>     joints_per_face;
    std::vector<std::array<int,4>>     three_valence;
    std::vector<std::pair<int,int>>    adjacency;

    Chevron(NurbsSurface surface       = default_surface(),
            int    u_divisions         = 4,
            double v_division_dist     = 900.0,
            double shift               = 0.5,
            double scale               = 0.05799,
            double box_height          = 760.0,
            double top_plate_inlet     = 80.0,
            double plate_thickness     = 40.0,
            double edge_rotation       = 1.0,
            double edge_offset         = 0.5,
            std::array<int,4> ortho_edges = {1,1,1,1})
    {

        if (u_divisions < 1) {
            throw std::invalid_argument("Chevron: u_divisions must be >= 1");
        }

        if (plate_thickness == 0.0) {
            throw std::invalid_argument("Chevron: plate_thickness must not be zero");
        }

        mesh = wood_chevron::chevron_mesh(
            surface, u_divisions, v_division_dist, shift, scale);

        wood_chevron::ChevronResult result = wood_chevron::chevron_plates(
            mesh, edge_rotation, edge_offset,
            box_height, top_plate_inlet, plate_thickness, ortho_edges);

        // 8 polylines per face → 4 plate-pairs → 4 WoodElements per face
        for (size_t i = 0; i + 1 < result.plines.size(); i += 2) {
            elements.push_back(std::make_shared<Plate>(result.plines[i], result.plines[i + 1]));
        }

        insertion_vectors = std::move(result.insertion_vectors);
        joints_per_face   = std::move(result.joints_per_face);
        three_valence     = std::move(result.three_valence);
        adjacency         = std::move(result.adjacency);
    }

    /// Degree-3 bicubic flat surface: 3000×5000 mm centred at world origin.
    /// u ∈ [−1500, 1500], v ∈ [−2500, 2500].  chevron_mesh uses V as the long axis.
    static NurbsSurface default_surface() {

        // order=4 (degree 3), 4×4 control points, centred so the mesh appears at origin.
        // Knots use physical mm half-extents so chevron_mesh can use v_division_dist
        // directly as a parametric step.
        // Clamped knot vector centred: full=[−H,−H,−H,−H,+H,+H,+H,+H],
        // OpenNURBS strips first/last → [−H,−H,−H,+H,+H,+H].
        const double hu = 1500.0, hv = 2500.0;

        NurbsSurface srf;
        srf.create_raw(3, false, 4, 4, 4, 4);

        const double ku[] = {-hu, -hu, -hu, hu, hu, hu};
        const double kv[] = {-hv, -hv, -hv, hv, hv, hv};
        for (int i = 0; i < 6; i++) {
            srf.set_nurbsknot(0, i, ku[i]);
            srf.set_nurbsknot(1, i, kv[i]);
        }

        const double us[] = {-hu, -hu/3.0, hu/3.0, hu};
        const double vs[] = {-hv, -hv/3.0, hv/3.0, hv};

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                srf.set_cv(i, j, Point(us[i], vs[j], 0.0));
            }
        }

        return srf;
    }
};
