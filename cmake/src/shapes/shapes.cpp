#include "shapes.h"
#include "nurbssurface.h"
#include "mesh.h"
#include "polyline.h"
#include "line.h"
#include "plane.h"
#include "point.h"
#include "vector.h"
#include "xform.h"
#include "knot.h"
#include "tolerance.h"
#include "intersection.h"
#include <cmath>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>

namespace session_cpp {

Mesh chevron_mesh(const NurbsSurface& surface,
                              int u_divisions, double v_division_dist,
                              double shift, double scale) {
    NurbsSurface srf = surface;

    // Measure arc lengths to determine long direction
    auto du0 = srf.domain(0);
    auto dv0 = srf.domain(1);
    int nsamp = 50;
    double u_arc = 0, v_arc = 0;
    {
        double v_mid = (dv0.first + dv0.second) / 2.0;
        Point prev = srf.point_at(du0.first, v_mid);
        for (int i = 1; i <= nsamp; i++) {
            Point curr = srf.point_at(du0.first + (du0.second - du0.first) * i / nsamp, v_mid);
            u_arc += prev.distance(curr);
            prev = curr;
        }
    }
    {
        double u_mid = (du0.first + du0.second) / 2.0;
        Point prev = srf.point_at(u_mid, dv0.first);
        for (int i = 1; i <= nsamp; i++) {
            Point curr = srf.point_at(u_mid, dv0.first + (dv0.second - dv0.first) * i / nsamp);
            v_arc += prev.distance(curr);
            prev = curr;
        }
    }

    // Transpose so v is the long direction (march direction)
    if (u_arc > v_arc) {
        srf.transpose();
        std::swap(u_arc, v_arc);
    }

    auto du = srf.domain(0);
    auto dv = srf.domain(1);

    // Scale v_division_dist from physical to parametric units
    double param_v = dv.second - dv.first;
    double half_v = dv.second * 0.5;
    double StepU = (du.second - du.first) / u_divisions;
    double totalV = param_v;
    double baseStepV = (v_arc > 1e-10) ? v_division_dist * param_v / v_arc : v_division_dist;

    std::vector<std::vector<Point>> polygons;

    double ctU = 0;
    for (int j = 0; j < u_divisions; j++) {
        double ctV = 0;
        double thresh = totalV / 2.0;
        double StepV1 = baseStepV;
        bool running = true;
        std::vector<double> ListV;

        Point p0, p1, p2, p6, p7, p8;
        Point savept6, savept7, savept8;
        int iterations = 0;

        while (running && iterations < 1000) {
            iterations++;
            ListV.push_back(StepV1);

            if (iterations == 1) {
                p0 = srf.point_at(ctU, ctV);
                p1 = srf.point_at(ctU + StepU * 0.5, ctV);
                p2 = srf.point_at(ctU + StepU, ctV);
                p6 = srf.point_at(ctU, ctV + StepV1 * (1.0 - shift / 2.0));
                p7 = srf.point_at(ctU + StepU * 0.5, ctV + StepV1 * (1.0 + shift / 2.0));
                p8 = srf.point_at(ctU + StepU, ctV + StepV1 * (1.0 - shift / 2.0));
                savept6 = p6; savept7 = p7; savept8 = p8;
            } else {
                p0 = savept6; p1 = savept7; p2 = savept8;
                p6 = srf.point_at(ctU, ctV + StepV1 * (1.0 - shift / 2.0));
                p7 = srf.point_at(ctU + StepU * 0.5, ctV + StepV1 * (1.0 + shift / 2.0));
                p8 = srf.point_at(ctU + StepU, ctV + StepV1 * (1.0 - shift / 2.0));
                savept6 = p6; savept7 = p7; savept8 = p8;
            }

            polygons.push_back({p0, p6, p7, p1});
            polygons.push_back({p1, p7, p8, p2});

            ctV += StepV1;
            thresh -= StepV1;
            StepV1 += StepV1 * scale;

            if (ctV + StepV1 > half_v) {
                ListV.push_back(thresh);
                std::reverse(ListV.begin(), ListV.end());
                double revCt = totalV / 2.0;

                for (size_t i = 0; i < ListV.size() - 1; i++) {
                    revCt += ListV[i];

                    if (i == 0) {
                        p0 = srf.point_at(ctU, revCt - ListV[i + 1] * shift / 2.0);
                        p1 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1] * shift / 2.0);
                        p2 = srf.point_at(ctU + StepU, revCt - ListV[i + 1] * shift / 2.0);

                        polygons.push_back({p6, p0, p1, p7});
                        polygons.push_back({p7, p1, p2, p8});

                        p6 = srf.point_at(ctU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        p7 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1] * (1.0 + shift / 2.0));
                        p8 = srf.point_at(ctU + StepU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        savept6 = p6; savept7 = p7; savept8 = p8;
                    } else if (i == ListV.size() - 2) {
                        p0 = savept6; p1 = savept7; p2 = savept8;
                        p6 = srf.point_at(ctU, revCt + ListV[i + 1]);
                        p7 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1]);
                        p8 = srf.point_at(ctU + StepU, revCt + ListV[i + 1]);
                    } else {
                        p0 = savept6; p1 = savept7; p2 = savept8;
                        p6 = srf.point_at(ctU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        p7 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1] * (1.0 + shift / 2.0));
                        p8 = srf.point_at(ctU + StepU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
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

    return Mesh::from_polylines(polygons, 0.01);
}

std::vector<NurbsSurface> annen_surfaces() {
    std::vector<NurbsSurface> surfaces;
    std::vector<std::string> prefixes = {
        "data/annen_surfaces/",
        "../data/annen_surfaces/"
    };
    for (int i = 0; i < 23; i++) {
        std::string fname = "surface_" + std::to_string(i) + ".json";
        for (auto& prefix : prefixes) {
            std::string path = prefix + fname;
            if (!std::filesystem::exists(path)) continue;
            NurbsSurface srf = NurbsSurface::json_load(path);
            if (srf.is_valid()) { surfaces.push_back(std::move(srf)); break; }
        }
    }
    return surfaces;
}

///////////////////////////////////////////////////////////////////////////////////////////
// FoldedPlates
///////////////////////////////////////////////////////////////////////////////////////////

FoldedPlates::FoldedPlates(const NurbsSurface& surface, int u_divisions, int v_divisions,
                           double thickness, double chamfer,
                           const std::vector<Plane>& base_planes,
                           const std::vector<double>& face_positions)
    : _srf(surface), _udiv(std::max(1, u_divisions)), _vdiv(std::max(1, v_divisions)),
      _thick(thickness), _cham(chamfer), _base_planes(base_planes), _face_pos(face_positions), _f(0)
{
    diamond_subdivision();
    build_topology();
    compute_face_planes();
    compute_edge_planes();
    compute_face_edge_planes();
    compute_face_polylines();
    compute_insertion_vectors();
}

Point FoldedPlates::closest_on_line(const Point& pt, const Point& a, const Point& b) {
    double dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
    double len2 = dx*dx + dy*dy + dz*dz;
    if (len2 < 1e-20) return a;
    double t = ((pt[0]-a[0])*dx + (pt[1]-a[1])*dy + (pt[2]-a[2])*dz) / len2;
    return Point(a[0]+t*dx, a[1]+t*dy, a[2]+t*dz);
}

bool FoldedPlates::line_plane_t(const Point& a, const Point& b, const Plane& pl, double& t) {
    Vector n = pl.z_axis();
    const Point& o = pl.origin();
    double dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
    double denom = n[0]*dx + n[1]*dy + n[2]*dz;
    if (std::abs(denom) < 1e-12) return false;
    t = (n[0]*(o[0]-a[0]) + n[1]*(o[1]-a[1]) + n[2]*(o[2]-a[2])) / denom;
    return true;
}

bool FoldedPlates::intersect_3_planes(const Plane& p0, const Plane& p1, const Plane& p2, Point& result) {
    Vector n0 = p0.z_axis(), n1 = p1.z_axis(), n2 = p2.z_axis();
    double d0 = n0[0]*p0.origin()[0] + n0[1]*p0.origin()[1] + n0[2]*p0.origin()[2];
    double d1 = n1[0]*p1.origin()[0] + n1[1]*p1.origin()[1] + n1[2]*p1.origin()[2];
    double d2 = n2[0]*p2.origin()[0] + n2[1]*p2.origin()[1] + n2[2]*p2.origin()[2];
    Vector c12 = n1.cross(n2), c20 = n2.cross(n0), c01 = n0.cross(n1);
    double det = n0.dot(c12);
    if (std::abs(det) < 1e-12) return false;
    double inv = 1.0 / det;
    result = Point((d0*c12[0] + d1*c20[0] + d2*c01[0]) * inv,
                   (d0*c12[1] + d1*c20[1] + d2*c01[1]) * inv,
                   (d0*c12[2] + d1*c20[2] + d2*c01[2]) * inv);
    return true;
}

Polyline FoldedPlates::chamfer_polyline(const Polyline& pl, double value) {
    if (std::abs(value) < 1e-10) return pl;
    size_t n = pl.point_count();
    if (n < 2) return pl;
    size_t segs = n - 1;
    Polyline result;
    for (size_t i = 0; i < segs; i++) {
        Point a = pl.get_point(i), b = pl.get_point(i + 1);
        double dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
        double len = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (len < 1e-10) continue;
        if (value < 0) {
            double r = std::abs(value) / len;
            result.add_point(Point(a[0]+r*dx, a[1]+r*dy, a[2]+r*dz));
            result.add_point(Point(b[0]-r*dx, b[1]-r*dy, b[2]-r*dz));
        } else {
            result.add_point(Point(a[0]+value*dx, a[1]+value*dy, a[2]+value*dz));
            double omt = 1.0 - value;
            result.add_point(Point(a[0]+omt*dx, a[1]+omt*dy, a[2]+omt*dz));
        }
    }
    if (result.point_count() > 0) result.add_point(result.get_point(0));
    return result;
}

void FoldedPlates::diamond_subdivision() {
    auto du = _srf.domain(0);
    auto dv = _srf.domain(1);
    double su = (du.second - du.first) / _udiv;
    double sv = (dv.second - dv.first) / _vdiv;
    std::vector<std::vector<Point>> tris;
    int fc = 0;

    auto plane_valid = [](const Plane& p) {
        Vector z = p.z_axis();
        return std::abs(z[0]) + std::abs(z[1]) + std::abs(z[2]) > 0.01;
    };

    double uu = du.first;
    for (int i = 0; i < _udiv; i++) {
        int a = (i % 2 == 0) ? 1 : 0;
        int b = (i % 2 == 0) ? 2 : 3;

        auto add_tri = [&](const Point& pa, const Point& pb, const Point& pc) {
            flags.push_back(fc % 4 == a || fc % 4 == b);
            tris.push_back({pa, pb, pc});
            fc++;
        };

        double vv = dv.first;
        for (int j = 0; j < _vdiv; j++) {
            Point p0 = _srf.point_at(uu, vv);
            Point p1 = _srf.point_at(uu + su, vv);
            Point p2 = _srf.point_at(uu, vv + sv);
            Point p3 = _srf.point_at(uu + su, vv + sv);
            Point p4 = _srf.point_at(uu + su * 0.5, vv + sv * 1.5);
            Point p5 = _srf.point_at(uu + su * 0.5, vv + sv * 0.5);
            Point p6 = _srf.point_at(uu + su * 0.5, vv - sv * 0.5);

            if (j == 0) {
                Point p9 = _srf.point_at(uu + su * 0.5, dv.first);
                Point cp = closest_on_line(p9, p5, p6);
                if (!_base_planes.empty() && plane_valid(_base_planes.front())) {
                    double t;
                    if (line_plane_t(p5, p6, _base_planes.front(), t))
                        cp = Point(p5[0]+t*(p6[0]-p5[0]), p5[1]+t*(p6[1]-p5[1]), p5[2]+t*(p6[2]-p5[2]));
                }
                add_tri(p5, cp, p1);
                add_tri(cp, p5, p0);
            }

            add_tri(p1, p3, p5);
            add_tri(p2, p0, p5);

            if (j < _vdiv - 1) {
                add_tri(p4, p5, p3);
                add_tri(p5, p4, p2);
            } else {
                Point p9 = _srf.point_at(uu + su * 0.5, dv.second);
                Point cp = closest_on_line(p9, p4, p5);
                if (!_base_planes.empty() && plane_valid(_base_planes.back())) {
                    double t;
                    if (line_plane_t(p4, p5, _base_planes.back(), t))
                        cp = Point(p4[0]+t*(p5[0]-p4[0]), p4[1]+t*(p5[1]-p4[1]), p4[2]+t*(p5[2]-p4[2]));
                }
                add_tri(cp, p5, p3);
                add_tri(p5, cp, p2);
            }
            vv += sv;
        }
        uu += su;
    }

    mesh = Mesh::from_polylines(tris, 1e-6);
}

void FoldedPlates::build_topology() {
    _fkeys.clear();
    for (auto& [fk, _] : mesh.face) _fkeys.push_back(fk);
    std::sort(_fkeys.begin(), _fkeys.end());
    _f = (int)_fkeys.size();

    _fv.resize(_f);
    for (int i = 0; i < _f; i++) _fv[i] = mesh.face[_fkeys[i]];

    std::map<std::pair<size_t,size_t>, std::vector<int>> ef_map;
    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        for (int j = 0; j < n; j++) {
            auto key = std::make_pair(std::min(v[j], v[(j+1)%n]), std::max(v[j], v[(j+1)%n]));
            ef_map[key].push_back(i);
        }
    }

    int ei = 0;
    for (auto& [ep, fl] : ef_map) {
        _eidx[ep] = ei;
        _ef.push_back(fl);
        ei++;
    }

    adjacency.clear();
    for (int i = 0; i < _f; i++) {
        if (!flags[i]) continue;
        auto& v = _fv[i];
        int n = (int)v.size();
        std::set<int> neighbors;
        for (int j = 0; j < n; j++) {
            auto key = std::make_pair(std::min(v[j], v[(j+1)%n]), std::max(v[j], v[(j+1)%n]));
            for (int fi : _ef[_eidx[key]])
                if (fi != i) neighbors.insert(fi);
        }
        for (int ni : neighbors)
            adjacency.push_back({i, ni, -1, -1});
    }
}

void FoldedPlates::compute_face_planes() {
    _fplanes.resize(_f);
    for (int i = 0; i < _f; i++) {
        auto& verts = _fv[i];
        int n = (int)verts.size();
        double cx = 0, cy = 0, cz = 0, w = 0;
        for (int j = 0; j < n; j++) {
            Point pa = mesh.vertex_position(verts[j]).value();
            Point pb = mesh.vertex_position(verts[(j+1)%n]).value();
            double d = std::sqrt((pb[0]-pa[0])*(pb[0]-pa[0])+(pb[1]-pa[1])*(pb[1]-pa[1])+(pb[2]-pa[2])*(pb[2]-pa[2]));
            cx += d * (pa[0]+pb[0]) * 0.5;
            cy += d * (pa[1]+pb[1]) * 0.5;
            cz += d * (pa[2]+pb[2]) * 0.5;
            w += d;
        }
        if (w > 1e-10) { cx /= w; cy /= w; cz /= w; }
        Point center(cx, cy, cz);
        Vector normal = mesh.face_normal(_fkeys[i]).value_or(Vector(0, 0, 1));
        _fplanes[i] = Plane::from_point_normal(center, normal);
    }
}

void FoldedPlates::compute_edge_planes() {
    _eplanes.resize(_ef.size());
    for (auto& [ep, ei] : _eidx) {
        Point v1 = mesh.vertex_position(ep.first).value();
        Point v2 = mesh.vertex_position(ep.second).value();
        Point mid((v1[0]+v2[0])*0.5, (v1[1]+v2[1])*0.5, (v1[2]+v2[2])*0.5);
        Vector edir(v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2]);
        edir.normalize_self();

        auto& cf = _ef[ei];
        Vector z(0,0,1);
        if (cf.size() == 2) {
            Vector fn0 = mesh.face_normal(_fkeys[cf[0]]).value();
            Vector fn1 = mesh.face_normal(_fkeys[cf[1]]).value();
            Vector avg((fn0[0]+fn1[0])*0.5, (fn0[1]+fn1[1])*0.5, (fn0[2]+fn1[2])*0.5);
            avg.normalize_self();
            z = edir.cross(avg);
        } else {
            Vector fn0 = mesh.face_normal(_fkeys[cf[0]]).value();
            z = fn0.cross(edir);
        }

        _eplanes[ei] = Plane::from_point_normal(mid, z);
    }
}

void FoldedPlates::compute_face_edge_planes() {
    _fe_planes.resize(_f);
    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        _fe_planes[i].resize(n);
        for (int j = 0; j < n; j++) {
            size_t v1 = v[j], v2 = v[(j+1)%n];
            auto key = std::make_pair(std::min(v1,v2), std::max(v1,v2));
            auto& cf = _ef[_eidx[key]];

            if (cf.size() == 2) {
                _fe_planes[i][j] = _eplanes[_eidx[key]];
            } else {
                Point p1 = mesh.vertex_position(v1).value();
                Point p2 = mesh.vertex_position(v2).value();
                Point mid((p1[0]+p2[0])*0.5, (p1[1]+p2[1])*0.5, (p1[2]+p2[2])*0.5);
                Vector sum(0, 0, 0);
                for (int fi : cf) {
                    Vector fn = mesh.face_normal(_fkeys[fi]).value();
                    sum = Vector(sum[0]+fn[0], sum[1]+fn[1], sum[2]+fn[2]);
                }
                double inv = 1.0 / cf.size();
                sum = Vector(sum[0]*inv, sum[1]*inv, sum[2]*inv);
                Vector xdir(p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]);
                _fe_planes[i][j] = Plane(mid, xdir, sum);
            }
        }
    }
}

void FoldedPlates::compute_face_polylines() {
    polylines.resize(_f);
    for (int i = 0; i < _f; i++) {
        auto& sides = _fe_planes[i];
        int n = (int)sides.size();

        for (size_t j = 0; j < _face_pos.size(); j++) {
            Plane base0 = _fplanes[i].translate_by_normal(-_face_pos[j]);
            Plane base1 = _fplanes[i].translate_by_normal(-(_face_pos[j] + _thick));

            Polyline pl0, pl1;
            for (int k = 0; k < n; k++) {
                Point pt;
                if (intersect_3_planes(base0, sides[k], sides[(k+1)%n], pt))
                    pl0.add_point(pt);
                if (intersect_3_planes(base1, sides[k], sides[(k+1)%n], pt))
                    pl1.add_point(pt);
            }
            if (pl0.point_count() > 0) pl0.add_point(pl0.get_point(0));
            if (pl1.point_count() > 0) pl1.add_point(pl1.get_point(0));

            polylines[i].push_back(pl0);
            polylines[i].push_back(pl1);
        }
    }
}

void FoldedPlates::compute_insertion_vectors() {
    for (int i = 0; i < _f; i++) {
        if (flags[i]) continue;
        if (polylines[i].empty() || polylines[i][0].point_count() < 4) continue;

        const Polyline& pl = polylines[i][0];
        Vector vec(pl.get_point(2)[0] - pl.get_point(0)[0],
                   pl.get_point(2)[1] - pl.get_point(0)[1],
                   pl.get_point(2)[2] - pl.get_point(0)[2]);
        vec.normalize_self();
        vec = Vector(vec[0]*0.5, vec[1]*0.5, vec[2]*0.5);

        for (int j = 0; j < 3; j++) {
            Point a = pl.get_point(j);
            Point b = pl.get_point((j + 1) % 3);
            Point mid((a[0]+b[0])*0.5, (a[1]+b[1])*0.5, (a[2]+b[2])*0.5);
            insertion_lines.push_back(Line::from_point_and_vector(mid, vec));
        }
    }

    if (_cham > 1e-6) {
        for (int i = 0; i < _f; i++)
            for (size_t j = 0; j < polylines[i].size(); j++)
                polylines[i][j] = chamfer_polyline(polylines[i][j], -_cham);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// CrossConnectors
///////////////////////////////////////////////////////////////////////////////////////////

CrossConnectors::CrossConnectors(const Mesh& m,
                                 double face_thickness,
                                 const std::vector<double>& face_positions,
                                 int edge_divisions,
                                 double rect_width,
                                 double rect_height,
                                 double rect_thickness,
                                 double chamfer)
    : mesh(m), _f(0), _thick(face_thickness), _rect_w(rect_width), _rect_h(rect_height),
      _rect_t(rect_thickness), _cham(chamfer), _edge_div(std::max(1, edge_divisions)),
      _face_pos(face_positions)
{
    if (_face_pos.empty()) _face_pos.push_back(0);
    std::sort(_face_pos.begin(), _face_pos.end());

    build_topology();
    compute_face_planes();
    compute_face_edge_planes();
    compute_bisector_planes();
    compute_face_polylines();
    compute_edges();
    compute_edge_faces();
    compute_edge_planes_method();
    compute_connectors();
}

void CrossConnectors::build_topology() {
    _fkeys.clear();
    for (auto& [fk, _] : mesh.face) _fkeys.push_back(fk);
    std::sort(_fkeys.begin(), _fkeys.end());
    _f = (int)_fkeys.size();

    _fv.resize(_f);
    for (int i = 0; i < _f; i++) _fv[i] = mesh.face[_fkeys[i]];

    _eidx.clear();
    int ei = 0;
    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        for (int j = 0; j < n; j++) {
            auto key = std::make_pair(std::min(v[j], v[(j+1)%n]), std::max(v[j], v[(j+1)%n]));
            if (_eidx.find(key) == _eidx.end()) {
                _eidx[key] = ei++;
            }
        }
    }
}

void CrossConnectors::compute_face_planes() {
    face_planes.resize(_f);
    for (int i = 0; i < _f; i++) {
        auto& verts = _fv[i];
        int n = (int)verts.size();
        double cx = 0, cy = 0, cz = 0, w = 0;
        for (int j = 0; j < n; j++) {
            Point pa = mesh.vertex_position(verts[j]).value();
            Point pb = mesh.vertex_position(verts[(j+1)%n]).value();
            double d = pa.distance(pb);
            cx += d * (pa[0]+pb[0]) * 0.5;
            cy += d * (pa[1]+pb[1]) * 0.5;
            cz += d * (pa[2]+pb[2]) * 0.5;
            w += d;
        }
        if (w > 1e-10) { cx /= w; cy /= w; cz /= w; }
        Point center(cx, cy, cz);
        Vector normal = mesh.face_normal(_fkeys[i]).value_or(Vector(0, 0, 1));
        face_planes[i] = Plane::from_point_normal(center, normal);
    }
}

void CrossConnectors::compute_face_edge_planes() {
    // Build edge -> averaged normal: sum each adjacent face's normal, then normalise.
    std::map<std::pair<size_t,size_t>, Vector> edge_avg_normal;
    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        Vector ni = face_planes[i].z_axis();
        for (int j = 0; j < n; j++) {
            auto key = std::make_pair(std::min(v[j], v[(j+1)%n]), std::max(v[j], v[(j+1)%n]));
            auto it = edge_avg_normal.find(key);
            if (it == edge_avg_normal.end())
                edge_avg_normal[key] = ni;
            else
                it->second = Vector(it->second[0]+ni[0], it->second[1]+ni[1], it->second[2]+ni[2]);
        }
    }
    for (auto& [k, v] : edge_avg_normal) v.normalize_self();

    fe_planes.resize(_f);
    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        fe_planes[i].resize(n);
        for (int j = 0; j < n; j++) {
            size_t v1 = v[j], v2 = v[(j+1)%n];
            Point p1 = mesh.vertex_position(v1).value();
            Point p2 = mesh.vertex_position(v2).value();
            Point mid((p1[0]+p2[0])*0.5, (p1[1]+p2[1])*0.5, (p1[2]+p2[2])*0.5);

            auto key = std::make_pair(std::min(v1,v2), std::max(v1,v2));
            Vector sum = edge_avg_normal.at(key);

            Vector xaxis(p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]);
            xaxis.normalize_self();
            Vector zaxis = xaxis.cross(sum);
            zaxis.normalize_self();
            fe_planes[i][j] = Plane(mid, xaxis, sum, zaxis);
        }
    }
}

Plane CrossConnectors::dihedral_plane(const Plane& p0, const Plane& p1) {
    Line isect_line;
    if (!Intersection::plane_plane(p0, p1, isect_line))
        return p0;

    Point centerDihedral;
    if (!Intersection::line_plane(isect_line, p0, centerDihedral, false)) {
        Point a = isect_line.start(), b = isect_line.end();
        centerDihedral = Point((a[0]+b[0])*0.5, (a[1]+b[1])*0.5, (a[2]+b[2])*0.5);
    }

    Vector line_dir(isect_line.end()[0]-isect_line.start()[0],
                    isect_line.end()[1]-isect_line.start()[1],
                    isect_line.end()[2]-isect_line.start()[2]);
    line_dir.normalize_self();

    Vector z0 = p0.z_axis(), z1 = p1.z_axis();
    Point o0 = p0.origin(), o1 = p1.origin();
    Line ray0(o0[0], o0[1], o0[2], o0[0]+z0[0], o0[1]+z0[1], o0[2]+z0[2]);
    Line ray1(o1[0], o1[1], o1[2], o1[0]+z1[0], o1[1]+z1[1], o1[2]+z1[2]);

    bool parallel = std::abs(z0.dot(z1)) > 0.9999;
    double dist2 = (o0[0]-o1[0])*(o0[0]-o1[0]) + (o0[1]-o1[1])*(o0[1]-o1[1]) + (o0[2]-o1[2])*(o0[2]-o1[2]);

    if (!parallel && dist2 > 0.001) {
        double t0, t1;
        if (Intersection::line_line_parameters(ray0, ray1, t0, t1, 0.01, false, false)) {
            Point center(o0[0]+t0*z0[0], o0[1]+t0*z0[1], o0[2]+t0*z0[2]);
            Vector v0(o0[0]-center[0], o0[1]-center[1], o0[2]-center[2]);
            Vector v1(o1[0]-center[0], o1[1]-center[1], o1[2]-center[2]);
            v0.normalize_self();
            v1.normalize_self();
            Vector bisector(v0[0]+v1[0], v0[1]+v1[1], v0[2]+v1[2]);
            return Plane(centerDihedral, line_dir, bisector);
        }
    }

    Vector bisector(z0[0]+z1[0], z0[1]+z1[1], z0[2]+z1[2]);
    return Plane(centerDihedral, line_dir, bisector);
}

void CrossConnectors::compute_bisector_planes() {
    bisector_planes.resize(_f);
    for (int i = 0; i < _f; i++) {
        int n = (int)fe_planes[i].size();
        bisector_planes[i].resize(n);
        for (int j = 0; j < n; j++) {
            bisector_planes[i][j] = dihedral_plane(fe_planes[i][(j+1)%n], fe_planes[i][j]);
        }
    }
}

Plane CrossConnectors::move_plane_by_axis(const Plane& pl, double dist, int axis) {
    Vector dir;
    if (axis == 0) dir = pl.x_axis();
    else if (axis == 1) dir = pl.y_axis();
    else dir = pl.z_axis();
    Point o = pl.origin();
    Point new_o(o[0]+dir[0]*dist, o[1]+dir[1]*dist, o[2]+dir[2]*dist);
    return Plane(new_o, pl.x_axis(), pl.y_axis(), pl.z_axis());
}

Polyline CrossConnectors::outline_from_planes(const Plane& face_plane,
                                               const std::vector<Plane>& edge_planes,
                                               const std::vector<Plane>& /*bise_planes*/) {
    Polyline pl;
    int n = (int)edge_planes.size();
    for (int i = 0; i < n; i++) {
        const Plane& plane = edge_planes[i];
        const Plane& plane1 = edge_planes[(i+1)%n];

        double angle = std::acos(std::min(1.0, std::max(-1.0, plane.z_axis().dot(plane1.z_axis()))));
        if (angle < 0.01 && angle > -0.01) continue;

        Line line;
        if (!Intersection::plane_plane(plane, plane1, line)) continue;
        Point pt;
        if (!Intersection::line_plane(line, face_plane, pt, false)) continue;
        pl.add_point(pt);
    }
    if (pl.point_count() > 0) pl.add_point(pl.get_point(0));
    return pl;
}

void CrossConnectors::compute_face_polylines() {
    face_polylines.resize(_f);
    for (int i = 0; i < _f; i++) {
        for (size_t j = 0; j < _face_pos.size(); j++) {
            double off_bot = _face_pos[j] - _thick * 0.5;
            double off_top = _face_pos[j] + _thick * 0.5;
            Plane bot_plane = move_plane_by_axis(face_planes[i], off_bot);
            Plane top_plane = move_plane_by_axis(face_planes[i], off_top);
            Polyline pline0 = outline_from_planes(bot_plane, fe_planes[i], bisector_planes[i]);
            Polyline pline1 = outline_from_planes(top_plane, fe_planes[i], bisector_planes[i]);
            face_polylines[i].push_back(pline0);
            face_polylines[i].push_back(pline1);
        }
    }
}

void CrossConnectors::compute_edges() {
    edges.clear();
    std::set<std::pair<size_t,size_t>> seen;
    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        for (int j = 0; j < n; j++) {
            auto key = std::make_pair(std::min(v[j], v[(j+1)%n]), std::max(v[j], v[(j+1)%n]));
            if (seen.insert(key).second)
                edges.push_back(key);
        }
    }
}

void CrossConnectors::compute_edge_faces() {
    edge_faces.resize(edges.size());
    std::map<std::pair<size_t,size_t>, int> edge_idx;
    for (size_t i = 0; i < edges.size(); i++) edge_idx[edges[i]] = (int)i;

    for (int i = 0; i < _f; i++) {
        auto& v = _fv[i];
        int n = (int)v.size();
        for (int j = 0; j < n; j++) {
            auto key = std::make_pair(std::min(v[j], v[(j+1)%n]), std::max(v[j], v[(j+1)%n]));
            auto it = edge_idx.find(key);
            if (it != edge_idx.end())
                edge_faces[it->second].push_back(i);
        }
    }
}

void CrossConnectors::compute_edge_planes_method() {
    edge_planes.resize(edges.size());
    _e90_multiple_planes.resize(edges.size());

    for (size_t i = 0; i < edges.size(); i++) {
        if (edge_faces[i].size() < 2) {
            edge_planes[i] = Plane();
            _e90_multiple_planes[i] = {Plane()};
            continue;
        }

        Point v1 = mesh.vertex_position(edges[i].first).value();
        Point v2 = mesh.vertex_position(edges[i].second).value();
        Point mid((v1[0]+v2[0])*0.5, (v1[1]+v2[1])*0.5, (v1[2]+v2[2])*0.5);
        Vector zaxis(v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2]);
        zaxis.normalize_self();

        Vector yaxis(face_planes[edge_faces[i][0]].z_axis()[0] + face_planes[edge_faces[i][1]].z_axis()[0],
                     face_planes[edge_faces[i][0]].z_axis()[1] + face_planes[edge_faces[i][1]].z_axis()[1],
                     face_planes[edge_faces[i][0]].z_axis()[2] + face_planes[edge_faces[i][1]].z_axis()[2]);
        yaxis = Vector(yaxis[0]*0.5, yaxis[1]*0.5, yaxis[2]*0.5);
        yaxis.normalize_self();

        Vector xaxis(zaxis[1]*yaxis[2]-zaxis[2]*yaxis[1],
                     zaxis[2]*yaxis[0]-zaxis[0]*yaxis[2],
                     zaxis[0]*yaxis[1]-zaxis[1]*yaxis[0]);
        xaxis.normalize_self();

        Point f0_o = face_planes[edge_faces[i][0]].origin();
        if ((mid[0]+xaxis[0]-f0_o[0])*(mid[0]+xaxis[0]-f0_o[0])
          + (mid[1]+xaxis[1]-f0_o[1])*(mid[1]+xaxis[1]-f0_o[1])
          + (mid[2]+xaxis[2]-f0_o[2])*(mid[2]+xaxis[2]-f0_o[2])
          < (mid[0]-xaxis[0]-f0_o[0])*(mid[0]-xaxis[0]-f0_o[0])
          + (mid[1]-xaxis[1]-f0_o[1])*(mid[1]-xaxis[1]-f0_o[1])
          + (mid[2]-xaxis[2]-f0_o[2])*(mid[2]-xaxis[2]-f0_o[2]))
            xaxis = Vector(-xaxis[0], -xaxis[1], -xaxis[2]);

        edge_planes[i] = Plane(mid, xaxis, yaxis, zaxis);

        _e90_multiple_planes[i].clear();
        for (int d = 0; d < _edge_div; d++) {
            double t = (d + 1.0) / (_edge_div + 1.0);
            Point pt(v1[0]+t*(v2[0]-v1[0]), v1[1]+t*(v2[1]-v1[1]), v1[2]+t*(v2[2]-v1[2]));
            _e90_multiple_planes[i].push_back(Plane(pt, xaxis, yaxis, zaxis));
        }
    }
}

Polyline CrossConnectors::make_rectangle(const Plane& pl, double w, double h) {
    Vector x = pl.x_axis();
    Vector y = pl.y_axis();
    Point o = pl.origin();
    double hw = w * 0.5, hh = h * 0.5;
    Polyline rect;
    rect.add_point(Point(o[0]-hw*x[0]-hh*y[0], o[1]-hw*x[1]-hh*y[1], o[2]-hw*x[2]-hh*y[2]));
    rect.add_point(Point(o[0]+hw*x[0]-hh*y[0], o[1]+hw*x[1]-hh*y[1], o[2]+hw*x[2]-hh*y[2]));
    rect.add_point(Point(o[0]+hw*x[0]+hh*y[0], o[1]+hw*x[1]+hh*y[1], o[2]+hw*x[2]+hh*y[2]));
    rect.add_point(Point(o[0]-hw*x[0]+hh*y[0], o[1]-hw*x[1]+hh*y[1], o[2]-hw*x[2]+hh*y[2]));
    rect.add_point(rect.get_point(0));
    return rect;
}

void CrossConnectors::compute_connectors() {
    edge_polylines.resize(edges.size());
    for (size_t i = 0; i < edges.size(); i++) {
        if (edge_faces[i].size() < 2) continue;

        for (size_t j = 0; j < _e90_multiple_planes[i].size(); j++) {
            const Plane& epl = _e90_multiple_planes[i][j];
            if (_rect_w > 0 && _rect_h > 0) {
                Plane pl0 = move_plane_by_axis(epl, _rect_t *  0.5);
                Plane pl1 = move_plane_by_axis(epl, _rect_t * -0.5);
                edge_polylines[i].push_back(make_rectangle(pl0, _rect_w, _rect_h));
                edge_polylines[i].push_back(make_rectangle(pl1, _rect_w, _rect_h));
            } else {
                double w = std::abs(_rect_h);
                double h = std::abs(_rect_w);
                Plane e_plane(epl);
                Vector ey = epl.y_axis();
                Vector new_x = epl.z_axis();
                Vector new_z(-(epl.x_axis()[0]), -(epl.x_axis()[1]), -(epl.x_axis()[2]));
                e_plane = Plane(epl.origin(), new_x, ey, new_z);

                Plane top0 = move_plane_by_axis(
                    move_plane_by_axis(face_planes[edge_faces[i][0]], _face_pos.back()),
                    _thick * 0.5 + h);
                Plane top1 = move_plane_by_axis(
                    move_plane_by_axis(face_planes[edge_faces[i][1]], _face_pos.back()),
                    _thick * 0.5 + h);
                Plane bot0 = move_plane_by_axis(
                    move_plane_by_axis(face_planes[edge_faces[i][0]], _face_pos.front()),
                    _thick * -0.5 - h);
                Plane bot1 = move_plane_by_axis(
                    move_plane_by_axis(face_planes[edge_faces[i][1]], _face_pos.front()),
                    _thick * -0.5 - h);

                Plane e_plane_main(edge_planes[i]);
                Vector emx = e_plane_main.z_axis();
                Vector emy = e_plane_main.y_axis();
                Vector emz(-(e_plane_main.x_axis()[0]), -(e_plane_main.x_axis()[1]), -(e_plane_main.x_axis()[2]));
                e_plane_main = Plane(e_plane_main.origin(), emx, emy, emz);

                std::vector<Plane> sides = {
                    top0, e_plane, top1,
                    move_plane_by_axis(e_plane_main, w * 0.5),
                    bot1, e_plane, bot0,
                    move_plane_by_axis(e_plane_main, w * -0.5)
                };

                Plane base0 = move_plane_by_axis(epl, -_rect_t * 0.5);
                Polyline type1_0;
                int ns = (int)sides.size();
                for (int s = 0; s < ns; s++) {
                    Line line;
                    if (!Intersection::plane_plane(sides[s], sides[(s+1)%ns], line)) continue;
                    Point pt;
                    if (!Intersection::line_plane(line, base0, pt, false)) continue;
                    type1_0.add_point(pt);
                }
                if (type1_0.point_count() > 0) type1_0.add_point(type1_0.get_point(0));

                Polyline type1_1;
                Vector zdir = epl.z_axis();
                for (int p = 0; p < type1_0.point_count(); p++) {
                    Point pp = type1_0.get_point(p);
                    type1_1.add_point(Point(pp[0]+zdir[0]*_rect_t, pp[1]+zdir[1]*_rect_t, pp[2]+zdir[2]*_rect_t));
                }

                edge_polylines[i].push_back(type1_0);
                edge_polylines[i].push_back(type1_1);
            }
        }
    }
}

} // namespace session_cpp
