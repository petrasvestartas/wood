#pragma once
#include "session.h"
#include "wood_session.h"
#include "chevron_result.h"
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

    srf.transpose();

    std::pair<double,double> du = srf.domain(0);
    std::pair<double,double> dv = srf.domain(1);

    double half_v    = (dv.first + dv.second) * 0.5;  // domain midpoint
    double StepU     = (du.second - du.first) / u_divisions;
    double totalV    = dv.second - dv.first;
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

/// A plane from an origin and two directions, each normalized on its own, z = x × y; not the kernel constructor, which would re-orthogonalize y.
inline session_cpp::Plane frame_plane(const session_cpp::Point& origin, const session_cpp::Vector& x_in, const session_cpp::Vector& y_in) {
    const session_cpp::Vector x = x_in.normalized();
    const session_cpp::Vector y = y_in.normalized();
    return session_cpp::Plane::from_frame(origin, x, y, x.cross(y).normalized());
}

/// The plane with x and z flipped, y kept: the same plane seen from the other side.
inline session_cpp::Plane flipped_x(const session_cpp::Plane& p) {
    return session_cpp::Plane::from_frame(p.origin(), -p.x_axis(), p.y_axis(), -p.z_axis());
}

/// The plane rotated about its own y axis by angle in radians.
inline session_cpp::Plane rotated_y(const session_cpp::Plane& p, double angle) {
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const session_cpp::Vector new_x = (p.x_axis() * c + p.z_axis() * -s).normalized();
    const session_cpp::Vector new_z = (p.x_axis() * s + p.z_axis() * c).normalized();
    return session_cpp::Plane::from_frame(p.origin(), new_x, new_z.cross(new_x), new_z);
}

/// The plane with z snapped to a world axis and x, y rebuilt; axis: 1 = the dominant one, 2 = X, 3 = Y, 4 = Z.
inline session_cpp::Plane snapped_to_axis(const session_cpp::Plane& p, int axis) {
    const session_cpp::Vector z = p.z_axis();
    int idx = 0;
    if (axis >= 2 && axis <= 4) {
        idx = axis - 2;
    } else {
        double best = 0.0;
        for (int i = 0; i < 3; i++) {
            if (std::abs(z[i]) > best) {
                best = std::abs(z[i]);
                idx = i;
            }
        }
    }
    session_cpp::Vector new_z(0.0, 0.0, 0.0);
    new_z[idx] = z[idx] >= 0.0 ? 1.0 : -1.0;
    const session_cpp::Vector ref = idx != 0 ? session_cpp::Vector(1.0, 0.0, 0.0) : session_cpp::Vector(0.0, 1.0, 0.0);
    const session_cpp::Vector new_x = ref.cross(new_z).normalized();
    return session_cpp::Plane::from_frame(p.origin(), new_x, new_z.cross(new_x), new_z);
}

/// The line two planes meet on, directed along p0.z × p1.z; none when they are parallel.
inline std::optional<session_cpp::Line> plane_pair_line(const session_cpp::Plane& p0, const session_cpp::Plane& p1) {
    session_cpp::Line line;
    if (!session_cpp::Intersection::plane_plane(p1, p0, line))
        return std::nullopt;
    return line;
}

/// Dihedral bisector plane of two planes, or nullopt when they are parallel or share an origin.
inline std::optional<session_cpp::Plane> dihedral_plane(const session_cpp::Plane& p0, const session_cpp::Plane& p1) {
    const std::optional<session_cpp::Line> seam = plane_pair_line(p0, p1);
    if (!seam)
        return std::nullopt;
    if (p0.z_axis().dot(p1.z_axis()) > 1.0 - 0.01)
        return std::nullopt;
    if ((p0.origin() - p1.origin()).magnitude() < 0.001)
        return std::nullopt;

    double t0 = 0.0;
    double t1 = 0.0;
    const session_cpp::Line axis0 = session_cpp::Line::from_points(p0.origin(), p0.origin() + p0.z_axis());
    const session_cpp::Line axis1 = session_cpp::Line::from_points(p1.origin(), p1.origin() + p1.z_axis());
    session_cpp::Point center = p0.origin();
    if (session_cpp::Intersection::line_line_parameters(axis0, axis1, t0, t1, 0.0, false, false))
        center = axis0.point_at(t0);

    const session_cpp::Vector v0 = (p0.origin() - center).normalized();
    const session_cpp::Vector v1 = (p1.origin() - center).normalized();
    session_cpp::Vector bis = v0 + v1;
    const double bn = bis.magnitude();
    if (bn < 1e-12)
        return std::nullopt;
    bis = bis * (1.0 / bn);

    const session_cpp::Vector ldir = seam->to_vector().normalized();
    return session_cpp::Plane::from_frame(seam->start(), ldir, bis, ldir.cross(bis).normalized());
}

/// Closed polygon from intersecting a base plane with n side planes in a loop; a missed corner falls back to the base origin.
inline session_cpp::Polyline polygon_from_planes(const session_cpp::Plane& base, const std::vector<session_cpp::Plane>& sides) {
    const int ns = (int)sides.size();
    std::vector<session_cpp::Point> pts;
    pts.reserve(ns + 1);
    for (int i = 0; i < ns; i++) {
        session_cpp::Point pt;
        pts.push_back(session_cpp::Intersection::plane_plane_plane(base, sides[i], sides[(i + 1) % ns], pt) ? pt : base.origin());
    }
    pts.push_back(pts.front());
    return session_cpp::Polyline(pts);
}

/// Position of the mesh vertex with key vk.
inline session_cpp::Point vertex_position(const session_cpp::Mesh& mesh, size_t vk) {
    const session_cpp::VertexData& vd = mesh.vertex.at(vk);
    return session_cpp::Point(vd.x, vd.y, vd.z);
}

/// Normal of face fi from its (already-reversed) vertex list, the flipped face normal of mesh.Flip(): for a quad the cross product of its diagonals as OpenNURBS computes it, the best-fit normal of a twisted quad; a plane through three vertices would tilt by the twist; world z when degenerate.
inline session_cpp::Vector face_normal(const session_cpp::Mesh& mesh, const std::vector<std::vector<size_t>>& face_vertices, int fi) {

    const std::vector<size_t>& vertices = face_vertices[fi];
    if ((int)vertices.size() < 3)
        return session_cpp::Vector(0.0, 0.0, 1.0);

    const session_cpp::Point a = vertex_position(mesh, vertices[0]);
    const session_cpp::Point b = vertex_position(mesh, vertices[1]);
    const session_cpp::Point c = vertex_position(mesh, vertices[2]);
    if ((int)vertices.size() < 4)
        return (b - a).cross(c - a).normalized();

    const session_cpp::Point d = vertex_position(mesh, vertices[3]);
    return (c - a).cross(d - b).normalized();
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

/// Unit direction of the intersection line of a face plane with a corner's bisector plane; zero when the bisector is absent or the planes are parallel.
inline session_cpp::Vector bisector_direction(const session_cpp::Plane& face_plane, const std::optional<session_cpp::Plane>& bisector_plane) {

    if (!bisector_plane)
        return session_cpp::Vector(0.0, 0.0, 0.0);

    const std::optional<session_cpp::Line> seam = plane_pair_line(face_plane, *bisector_plane);
    return seam ? seam->to_vector().normalized() : session_cpp::Vector(0.0, 0.0, 0.0);
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
    edge_rotation   = std::clamp(edge_rotation,  -30.0, 30.0);
    edge_offset     = std::clamp(edge_offset,     -2.0,  2.0);
    box_height      = std::max(box_height, 1.0);
    top_plate_inlet = std::clamp(top_plate_inlet, 1.0, box_height * 0.333);
    plate_thickness = std::clamp(plate_thickness, 1.0, box_height * 0.333);
    std::vector<size_t> fkeys;
    for (auto& [fk, _] : mesh.face) {
        fkeys.push_back(fk);
    }

    int n = (int)fkeys.size();
    if (n == 0)
        return {};

    std::vector<std::vector<size_t>> fv(n);
    for (int i = 0; i < n; i++) {
        std::optional<std::vector<size_t>> opt = mesh.face_vertices(fkeys[i]);
        if (opt) {
            fv[i] = *opt;
            std::reverse(fv[i].begin(), fv[i].end());
        }
    }

    std::map<std::pair<size_t,size_t>, std::vector<int>> edge_adj;
    for (int i = 0; i < n; i++) {
        int nv = (int)fv[i].size();
        for (int j = 0; j < nv; j++) {
            size_t u = fv[i][j], v = fv[i][(j+1)%nv];
            edge_adj[{std::min(u,v), std::max(u,v)}].push_back(i);
        }
    }
    std::vector<std::array<int,2>> f_e(n);
    std::vector<bool> f_rf(n, false);     // rotation_flip flag
    std::vector<bool> flagged(n, false);
    std::vector<int>  f_order;            // face visit order for plate output
    f_order.reserve(n);

    int strip_idx = 0;
    while (true) {

        int seed = -1;
        for (int i = 0; i < n && seed < 0; i++)
            if (!flagged[i])
                seed = i;

        if (seed < 0)
            break;

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

        for (int fi : strip) {
            f_order.push_back(fi);
        }
        strip_idx++;
    }

    std::vector<std::vector<session_cpp::Plane>> ep(n, std::vector<session_cpp::Plane>(4));  // edge planes
    std::vector<session_cpp::Plane>              fp(n);                                    // face planes

    for (int fi = 0; fi < n; fi++) {

        if ((int)fv[fi].size() < 4) {
            continue;
        }

        const session_cpp::Vector fn = face_normal(mesh, fv, fi);

        std::vector<session_cpp::Point> corners;
        for (size_t vk : fv[fi]) {
            corners.push_back(vertex_position(mesh, vk));
        }
        const session_cpp::Point fc = session_cpp::Point::centroid(corners);

        const session_cpp::Vector ref = std::abs(fn[0]) < 0.9 ? session_cpp::Vector(1.0, 0.0, 0.0) : session_cpp::Vector(0.0, 1.0, 0.0);
        const session_cpp::Vector fx  = ref.cross(fn).normalized();
        fp[fi] = session_cpp::Plane::from_frame(fc, fx, fn.cross(fx), fn);

        for (int j = 0; j < 4; j++) {
            size_t vi0 = fv[fi][j], vi1 = fv[fi][(j+1)%4];
            const session_cpp::Point p0 = vertex_position(mesh, vi0);
            const session_cpp::Point p1 = vertex_position(mesh, vi1);
            const session_cpp::Point mid = session_cpp::Point::mid_point(p0, p1);
            const session_cpp::Vector ex = p0 - p1;

            const std::vector<int>& adj = adjacent_faces(edge_adj, vi0, vi1);
            session_cpp::Vector avg_n(0.0, 0.0, 0.0);
            for (int fi2 : adj) {
                avg_n = avg_n + face_normal(mesh, fv, fi2);
            }

            ep[fi][j] = frame_plane(mid, ex, avg_n);
        }
    }

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
                        ep[fi][j] = ep[fi][j].translate_by_normal(plate_thickness * edge_offset);
                    } else {
                        double sign = f_rf[fi] ? 1.0 : -1.0;
                        ep[fi][j] = rotated_y(ep[fi][j], angle_rad * sign);
                    }
                    int nb = (adj[0] != fi) ? adj[0] : adj[1];
                    for (int k = 0; k < 4; k++) {
                        size_t u = fv[nb][k], v = fv[nb][(k+1)%4];
                        if (std::min(u,v) == std::min(vi0,vi1) &&
                            std::max(u,v) == std::max(vi0,vi1))
                        {
                            ep[nb][k] = flipped_x(ep[fi][j]);
                            break;
                        }
                    }
                }
            } else if (ortho_edges[j] != 0) {
                ep[fi][j] = snapped_to_axis(ep[fi][j], ortho_edges[j]);
            }
        }
    }

    std::vector<std::vector<std::optional<session_cpp::Plane>>> bi(n, std::vector<std::optional<session_cpp::Plane>>(4));
    for (int fi = 0; fi < n; fi++) {
        for (int j = 0; j < 4; j++) {
            bi[fi][j] = dihedral_plane(ep[fi][(j+1)%4], ep[fi][j]);
        }
    }

    const double H   = box_height;
    const double inp = top_plate_inlet;
    const double t   = plate_thickness;

    ChevronResult out;
    out.plines.reserve(n * 8);

    for (int fi : f_order) {
        std::vector<session_cpp::Plane> ep_local(4);
        for (int j = 0; j < 4; j++) {
            ep_local[j] = ep[fi][j];
            if (j == f_e[fi][0] || j == f_e[fi][1]) {
                ep_local[j] = ep_local[j].translate_by_normal(t);
            }
        }

        const session_cpp::Plane& fplane = fp[fi];

        out.plines.push_back(polygon_from_planes(fplane.translate_by_normal( H*0.5 - inp - t*0.5), ep_local));
        out.plines.push_back(polygon_from_planes(fplane.translate_by_normal( H*0.5 - inp + t*0.5), ep_local));
        out.plines.push_back(polygon_from_planes(fplane.translate_by_normal(-H*0.5 + inp - t*0.5), ep_local));
        out.plines.push_back(polygon_from_planes(fplane.translate_by_normal(-H*0.5 + inp + t*0.5), ep_local));

        std::array<int,2> e_sorted = f_e[fi];
        if (e_sorted[0] > e_sorted[1]) {
            std::swap(e_sorted[0], e_sorted[1]);
        }
        if (e_sorted[0] == 0 && e_sorted[1] == 3) {
            std::swap(e_sorted[0], e_sorted[1]);
        }

        for (int idx = 0; idx < 2; idx++) {
            int curr = e_sorted[idx];
            int prev = (curr - 1 + 4) % 4;
            int nxt  = (curr + 1) % 4;

            const session_cpp::Plane s0 = fplane.translate_by_normal( H * 0.5);   // top
            const session_cpp::Plane s2 = fplane.translate_by_normal(-H * 0.5);   // bottom

            session_cpp::Plane s1;
            session_cpp::Plane s3;
            if (idx == 0) {
                s1 = ep[fi][prev];
                s3 = bi[fi][curr] ? *bi[fi][curr] : ep[fi][nxt];
            } else {
                s1 = bi[fi][prev] ? *bi[fi][prev] : ep[fi][prev];
                s3 = ep[fi][nxt];
            }

            const std::vector<session_cpp::Plane> sides = {s0, s1, s2, s3};
            const session_cpp::Plane base0 = ep[fi][curr];
            const session_cpp::Plane base1 = base0.translate_by_normal(t);

            out.plines.push_back(polygon_from_planes(base0, sides));
            out.plines.push_back(polygon_from_planes(base1, sides));

        }
    }

    int n_ordered = (int)f_order.size();
    int n_pairs   = n_ordered * 4;   // 4 plate-pairs per mesh face

    out.insertion_vectors.assign(n_pairs, {});
    out.joints_per_face.assign(n_pairs, {0,0,0,0,0,0});

    std::vector<int> face_to_counter(n, -1);
    for (int c = 0; c < n_ordered; c++) {
        face_to_counter[f_order[c]] = c;
    }

    for (int counter = 0; counter < n_ordered; counter++) {
        int fi = f_order[counter];

        std::array<int,2> e_s = f_e[fi];
        if (e_s[0] > e_s[1]) {
            std::swap(e_s[0], e_s[1]);
        }
        if (e_s[0] == 0 && e_s[1] == 3) {
            std::swap(e_s[0], e_s[1]);
        }

        const session_cpp::Vector bdir0 = bisector_direction(fp[fi], bi[fi][e_s[0]]);
        const session_cpp::Vector bdir1 = bisector_direction(fp[fi], bi[fi][(e_s[1] + 1) % 4]);
        {
            std::array<double,18> ins = {};
            for (int s = 2; s < 6; s++) {
                ins[s*3+0] = bdir1[0]; ins[s*3+1] = bdir1[1]; ins[s*3+2] = bdir1[2];
            }
            for (int off : {2, 3}) {
                int idx = 2 + (e_s[1] + off) % 4;
                ins[idx*3+0] = bdir0[0]; ins[idx*3+1] = bdir0[1]; ins[idx*3+2] = bdir0[2];
            }
            out.insertion_vectors[counter*4+0] = ins;
            out.insertion_vectors[counter*4+1] = ins;
        }
        out.joints_per_face[counter*4+0] = {0,0,20,20,20,20};
        out.joints_per_face[counter*4+1] = {0,0,20,20,20,20};
        out.joints_per_face[counter*4+2] = {0,0,10,10,10,10};
        out.joints_per_face[counter*4+3] = {0,0,10,10,10,10};

        for (int role_a : {0, 1}) {
            for (int role_b : {2, 3}) {
                out.adjacency.emplace_back(counter*4+role_a, counter*4+role_b);
            }
        }
        out.adjacency.emplace_back(counter*4+2, counter*4+3);
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

            out.adjacency.emplace_back(nei*4+0, counter*4+2+ei);
            out.adjacency.emplace_back(nei*4+1, counter*4+2+ei);

            out.three_valence.push_back({counter*4+0, counter*4+2+ei, nei*4+0, counter*4+2+ei});
            out.three_valence.push_back({counter*4+1, counter*4+2+ei, nei*4+1, counter*4+2+ei});
        }
        const session_cpp::Point ctr = out.plines[counter * 8 + 1].center();
        out.box_insertion_lines.emplace_back(std::vector<session_cpp::Point>{ctr, ctr + bdir1 * 300.0});
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
