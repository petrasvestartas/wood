#pragma once
#include <iostream>
#include <cstdio>
#include "session.h"
#include "reciprocal_boundary.h"
#include "polyline.h"
#include "tolerance.h"

#include <cmath>
#include <map>
#include <optional>
#include <queue>
#include <stdexcept>

using namespace session_cpp;

/// A translation-based reciprocal frame (nexorade) from any mesh.
///
/// Each mesh edge is treated as a beam centerline. Even-indexed edges in each
/// face (color-0) are translated in the face plane by a bisector direction and
/// then trimmed against neighboring edges via line-line closest-point queries.
/// The result is a set of box beams whose ends naturally interlock at the
/// reciprocal offset without explicit cut planes.
///
/// Algorithm ported from NexorTranslateLines (C#/Grasshopper):
///   Reciprocal frame via in-plane edge translation + trim (Rizzuto / Larsen).
///
/// shift is the sideways move of every beam in model units, not an angle. Each beam end bears on the
/// side of the beam it is shifted against over a length of 2 * shift - beam_w, so shift must be at least
/// beam_w / 2 for the end to land on its neighbour at all, and about beam_w for a real bearing length.
///
/// Usage:
///   ReciprocalMove rm;                     // default 12×10 sinusoidal dome
///   ReciprocalMove rm(mesh, 100.0, 100.0); // user mesh, 100 mm shift, 100 mm beam
class ReciprocalMove {
public:
    /// Where a beam's up comes from: the normal of the face its half-edge was taken from, or the average of the normals of the faces sharing its edge, one orientation per edge whichever face it was taken from.
    enum class BeamUp {
        OwningFace, // The normal of the face the half-edge was taken from.
        EdgeAverage, // The average of the normals of the faces sharing the edge.
    };

    Mesh dome_mesh;
    std::vector<Mesh>     beams;
    std::vector<Polyline> side0;        // right-face outlines (+right of beam direction)
    std::vector<Polyline> side1;        // left-face outlines  (-right of beam direction)
    std::vector<Polyline> beam_bottom;  // bottom-face outlines (-up direction, for joinery)
    std::vector<Polyline> beam_top;     // top-face outlines    (+up direction, for joinery)
    std::vector<std::array<double,3>> beam_dirs;  // unit axis direction per beam
    std::vector<std::array<double,3>> beam_ups;   // unit up (face normal) per beam
    std::vector<Mesh>     boundary_beams;        // straight beams on the naked edges, in boundary-loop order, mitred at the shared vertices
    std::vector<Polyline> boundary_side0;        // right-face outlines of the boundary beams
    std::vector<Polyline> boundary_side1;        // left-face outlines of the boundary beams
    std::vector<Polyline> boundary_beam_bottom;  // bottom-face outlines of the boundary beams
    std::vector<Polyline> boundary_beam_top;     // top-face outlines of the boundary beams, corner i above boundary_beam_bottom[i]

    /// Parametric sinusoidal dome constructor.
    ReciprocalMove(int    nx                = 12,
                   int    ny                = 10,
                   double W                 = 12.0,
                   double D                 = 10.0,
                   double h                 = 3.0,
                   double shift             = 0.05,
                   double beam_w            = 0.10,
                   double beam_h            = 0.0,
                   double extend_factor     = 5.0,
                   double cut_offset_factor = 1.0,
                   wood_reciprocal::BoundaryTwist boundary_twist = wood_reciprocal::BoundaryTwist::AtBends,
                   const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups = {},
                   BeamUp beam_up = BeamUp::OwningFace,
                   wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
                       const std::map<std::pair<size_t, size_t>, int>& through_priority = {})
    {

        if (nx < 1 || ny < 1)
            throw std::invalid_argument("ReciprocalMove: nx and ny must be >= 1");

        dome_mesh = _make_dome(nx, ny, W, D, h);
        _build(dome_mesh, shift, beam_w, beam_h, extend_factor, cut_offset_factor, boundary_twist, boundary_ups, beam_up, corner_joint, through_priority);
    }

    /// External mesh constructor — use any mesh as the base.
    explicit ReciprocalMove(Mesh ext_mesh,
                            double shift             = 0.05,
                            double beam_w            = 0.10,
                            double beam_h            = 0.0,
                            double extend_factor     = 5.0,
                            double cut_offset_factor = 1.0,
                            wood_reciprocal::BoundaryTwist boundary_twist = wood_reciprocal::BoundaryTwist::AtBends,
                            const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups = {},
                            BeamUp beam_up = BeamUp::OwningFace,
                   wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
                       const std::map<std::pair<size_t, size_t>, int>& through_priority = {})
    {
        dome_mesh = std::move(ext_mesh);
        _build(dome_mesh, shift, beam_w, beam_h, extend_factor, cut_offset_factor, boundary_twist, boundary_ups, beam_up, corner_joint, through_priority);
    }

private:

    static Mesh _make_dome(int nx, int ny, double W, double D, double h)
    {

        std::vector<Point> pts;
        pts.reserve((nx + 1) * (ny + 1));
        for (int j = 0; j <= ny; j++) {
            for (int i = 0; i <= nx; i++) {
                double x = W * i / nx;
                double y = D * j / ny;
                double z = h * std::sin(Tolerance::PI * x / W)
                             * std::sin(Tolerance::PI * y / D);
                pts.push_back(Point(x, y, z));
            }
        }

        std::vector<std::vector<size_t>> faces;
        faces.reserve(nx * ny);
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                faces.push_back({
                    (size_t)( j      * (nx + 1) + i    ),
                    (size_t)( j      * (nx + 1) + i + 1),
                    (size_t)((j + 1) * (nx + 1) + i + 1),
                    (size_t)((j + 1) * (nx + 1) + i    ),
                });
            }
        }

        return Mesh::from_vertices_and_faces(pts, faces);
    }

    /// Newell-method face normal (normalized, guaranteed non-zero).
    static Vector _face_normal(const std::vector<Point>& pts,
                               const std::vector<size_t>& face)
    {

        int n = (int)face.size();
        double nx = 0, ny = 0, nz = 0;

        for (int i = 0; i < n; i++) {
            const Point& a = pts[face[i]];
            const Point& b = pts[face[(i + 1) % n]];
            nx += (a[1] - b[1]) * (a[2] + b[2]);
            ny += (a[2] - b[2]) * (a[0] + b[0]);
            nz += (a[0] - b[0]) * (a[1] + b[1]);
        }

        double len = std::sqrt(nx*nx + ny*ny + nz*nz);
        if (len < 1e-12)
            return Vector(0, 0, 1);

        return Vector(nx / len, ny / len, nz / len);
    }

    /// Closest point on line1 (P + t*D) to line2 (Q + s*E).
    /// Returns the point on line1 that is nearest to line2.
    static Point _lcp1(const Point& P, const Vector& D,
                       const Point& Q, const Vector& E)
    {

        double a  = D[0]*D[0] + D[1]*D[1] + D[2]*D[2];
        double b  = D[0]*E[0] + D[1]*E[1] + D[2]*E[2];
        double c  = E[0]*E[0] + E[1]*E[1] + E[2]*E[2];
        double d_ = (P[0]-Q[0])*D[0] + (P[1]-Q[1])*D[1] + (P[2]-Q[2])*D[2];
        double e_ = (P[0]-Q[0])*E[0] + (P[1]-Q[1])*E[1] + (P[2]-Q[2])*E[2];
        double denom = a * c - b * b;
        if (std::abs(denom) < 1e-12)
            return P;  // parallel lines

        double t = (b * e_ - c * d_) / denom;
        return Point(P[0] + t*D[0], P[1] + t*D[1], P[2] + t*D[2]);
    }

    struct FELine { Point from, to; Vector dir; };  // one face-edge centerline

    /// Opposite half-edge: (fi, fj) → (fi2, fj2) in the adjacent face, nullopt on a boundary edge.
    static std::optional<std::pair<int,int>> opposite_face(const std::vector<std::vector<size_t>>& faces,
                                                           const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& edge_to_fe,
                                                           int fi, int fj)
    {

        const std::vector<size_t>& fv = faces[fi];
        int n = (int)fv.size();
        size_t u = fv[fj], v = fv[(fj+1)%n];
        std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>::const_iterator it = edge_to_fe.find(wood_reciprocal::edge_key(u, v));
        if (it == edge_to_fe.end())
            return std::nullopt;

        for (auto& [fi2, fj2] : it->second)
            if (fi2 != fi)
                return std::make_pair(fi2, fj2);

        return std::nullopt;  // boundary edge
    }

    /// Interior cut: the side face of the crossing beam (ci, cj), offset by half the beam width and
    /// signed towards our_center. Falls back to a flat perpendicular cap at `endpoint` when the computed
    /// normal is ⊥ to our beam (degenerate intersection — happens at cone apex where cutting beam is parallel to ours).
    static Plane cut_plane(const std::vector<std::vector<FELine>>& EF,
                           const std::vector<std::vector<Vector>>& beam_ups,
                           int i, const Vector& bdir, const Point& our_center,
                           double beam_w, double cut_offset_factor,
                           int ci, int cj, const Point& endpoint)
    {

        const Vector& cup = beam_ups[ci][cj];  // the crossing beam's own up, so the plane is its real side face
        Vector raw = EF[ci][cj].dir.cross(cup);
        double len = std::sqrt(raw[0]*raw[0]+raw[1]*raw[1]+raw[2]*raw[2]);
        if (len > 1e-12)
            raw = Vector(raw[0]/len, raw[1]/len, raw[2]/len);
        else
            raw = EF[ci][cj].dir.cross(beam_ups[i][0]);
        constexpr double FLAT_CAP_ALIGNMENT_THRESHOLD = 0.15;
        double alignment = std::abs(raw[0]*bdir[0]+raw[1]*bdir[1]+raw[2]*bdir[2]);
        if (alignment < FLAT_CAP_ALIGNMENT_THRESHOLD)
            return Plane::from_point_normal(endpoint, bdir);

        Point org = Point::mid_point(EF[ci][cj].from, EF[ci][cj].to);
        double dot = (our_center[0]-org[0])*raw[0]
                   + (our_center[1]-org[1])*raw[1]
                   + (our_center[2]-org[2])*raw[2];
        if (dot < 0)
            raw = Vector(-raw[0], -raw[1], -raw[2]);

        const double face_off = beam_w * 0.5 * cut_offset_factor;
        Point face_org(org[0] + face_off*raw[0],
                       org[1] + face_off*raw[1],
                       org[2] + face_off*raw[2]);

        return Plane::from_point_normal(face_org, raw);
    }
    void _build(const Mesh& m, double shift, double beam_w, double beam_h,
                double extend_factor, double cut_offset_factor,
                wood_reciprocal::BoundaryTwist boundary_twist,
                const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups,
                BeamUp beam_up,
                wood_reciprocal::CornerJoint corner_joint,
                const std::map<std::pair<size_t, size_t>, int>& through_priority)
    {

        if (beam_w <= 0.0)
            throw std::invalid_argument("ReciprocalMove: beam_w must be positive");

        if (beam_h <= 0.0)
            beam_h = beam_w * 2.0;
        if (shift < beam_w * 0.5)
            std::cerr << fmt::format("  WARNING: ReciprocalMove: shift {} is below half the beam width {}: every beam end overhangs the neighbour it should bear on by {} and leaves a gap at the node.\n", shift, beam_w, beam_w * 0.5 - shift);

        (void)extend_factor;  // raw mesh edges — no extension (C# NexorTranslateLines)

        auto [pts, faces] = m.to_vertices_and_faces();
        int nf = (int)faces.size();
        std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>> edge_to_fe = wood_reciprocal::edge_owners(faces);
        int total_he = 0;
        std::vector<std::vector<int>> FEFlat(nf);

        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            FEFlat[i].resize(n);
            for (int j = 0; j < n; j++)
                FEFlat[i][j] = total_he++;
        }
        std::vector<std::vector<int>> he_adj(total_he);
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {
                int id = FEFlat[i][j];
                std::optional<std::pair<int,int>> opp = opposite_face(faces, edge_to_fe, i, j);
                if (!opp)
                    continue;

                he_adj[id].push_back(FEFlat[opp->first][opp->second]);
                if (opposite_face(faces, edge_to_fe, i, (j+1)%n))
                    he_adj[id].push_back(FEFlat[i][(j+1)%n]);
                if (opposite_face(faces, edge_to_fe, i, (j-1+n)%n))
                    he_adj[id].push_back(FEFlat[i][(j-1+n)%n]);
            }
        }
        std::vector<int> edgeColors(total_he, -1);
        {
            if (total_he == 0)
                return;

            std::queue<int> q;
            bool conflict = false;

            for (int seed = 0; seed < total_he; ++seed) {

                if (edgeColors[seed] != -1)
                    continue;

                edgeColors[seed] = 0;
                q.push(seed);
                while (!q.empty()) {
                    int v = q.front(); q.pop();
                    for (int nb : he_adj[v]) {
                        if (edgeColors[nb] == -1) {
                            edgeColors[nb] = 1 - edgeColors[v];
                            q.push(nb);
                        } else if (edgeColors[nb] == edgeColors[v]) {
                            conflict = true;
                        }
                    }
                }
            }

            if (conflict)
                std::cerr << "  WARNING: ReciprocalMove half-edge 2-coloring found an odd cycle (an interior face with an odd number of sides) - beam selection may be inconsistent." << std::endl;
        }
        std::vector<std::vector<FELine>> EF(nf);

        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            EF[i].resize(n);
            for (int j = 0; j < n; j++) {
                const Point& a = pts[faces[i][j]];
                const Point& b = pts[faces[i][(j+1)%n]];
                double dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
                double len = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (len < 1e-12)
                    len = 1.0;
                Vector unit(dx/len, dy/len, dz/len);
                EF[i][j].dir  = unit;
                EF[i][j].from = a;
                EF[i][j].to   = b;
            }
        }
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {

                if (edgeColors[FEFlat[i][j]] != 0)
                    continue;

                const Vector& d_next = EF[i][(j+1)%n].dir;
                const Vector& d_prev = EF[i][(j-1+n)%n].dir;
                double vx = (d_next[0] - d_prev[0]) * 0.5;
                double vy = (d_next[1] - d_prev[1]) * 0.5;
                double vz = (d_next[2] - d_prev[2]) * 0.5;
                EF[i][j].from = Point(EF[i][j].from[0]+vx*shift,
                                      EF[i][j].from[1]+vy*shift,
                                      EF[i][j].from[2]+vz*shift);
                EF[i][j].to   = Point(EF[i][j].to[0]+vx*shift,
                                      EF[i][j].to[1]+vy*shift,
                                      EF[i][j].to[2]+vz*shift);
            }
        }
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {

                if (edgeColors[FEFlat[i][j]] != 0)
                    continue;

                const FELine cur = EF[i][j];  // local copy before in-place modification

                std::optional<std::pair<int,int>> opp_next = opposite_face(faces, edge_to_fe, i, (j+1)%n);
                int tni = opp_next ? opp_next->first  : i;
                int tnj = opp_next ? opp_next->second : (j+1)%n;
                EF[i][j].to = _lcp1(cur.from, cur.dir, EF[tni][tnj].from, EF[tni][tnj].dir);

                std::optional<std::pair<int,int>> opp_prev = opposite_face(faces, edge_to_fe, i, (j-1+n)%n);
                int tpi = opp_prev ? opp_prev->first  : i;
                int tpj = opp_prev ? opp_prev->second : (j-1+n)%n;
                EF[i][j].from = _lcp1(cur.from, cur.dir, EF[tpi][tpj].from, EF[tpi][tpj].dir);
            }
        }
        std::vector<Vector> face_norms(nf);
        for (int i = 0; i < nf; i++)
            face_norms[i] = _face_normal(pts, faces[i]);
        std::vector<Vector> upward_norms(nf);  // the face normals flipped to +z like the interior beams' up
        for (int i = 0; i < nf; i++)
            upward_norms[i] = face_norms[i][2] < 0 ? -face_norms[i] : face_norms[i];
        std::vector<std::vector<Vector>> half_edge_ups(nf);
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            half_edge_ups[i].assign(n, upward_norms[i]);
            if (beam_up != BeamUp::EdgeAverage)
                continue;

            for (int j = 0; j < n; j++) {
                Vector sum(0, 0, 0);
                for (const auto& [fi, fj] : edge_to_fe.at(wood_reciprocal::edge_key(faces[i][j], faces[i][(j + 1) % n])))
                    sum += upward_norms[fi];

                if (!sum.is_zero())
                    half_edge_ups[i][j] = sum.normalized();
            }
        }

        std::map<size_t, Point> vertex_points;
        for (size_t v = 0; v < pts.size(); v++)
            vertex_points.emplace(v, pts[v]);

        wood_reciprocal::BoundaryFrame frame = wood_reciprocal::boundary_frame(faces, edge_to_fe, vertex_points, upward_norms, beam_w, beam_h, boundary_twist, boundary_ups, corner_joint, through_priority);
        for (size_t k = 0; k < frame.naked.size(); k++) {
            const Vector& dir = frame.directions[k];
            if (dir.is_zero())
                continue;

            const Point& pu = pts[wood_reciprocal::half_edge_start(faces, frame.naked[k])];
            const Point& pv = pts[wood_reciprocal::half_edge_end(faces, frame.naked[k])];
            wood_reciprocal::BeamGeom bg = wood_reciprocal::cut_beam(pu, pv, dir, frame.ups[k], beam_w, beam_h, wood_reciprocal::unbounded(frame.cut_from[k]), wood_reciprocal::unbounded(frame.cut_to[k]));
            wood_reciprocal::store_beam(bg, boundary_beams, boundary_side0, boundary_side1, boundary_beam_bottom, boundary_beam_top);
        }
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();

            for (int j = 0; j < n; j++) {

                if (edgeColors[FEFlat[i][j]] != 0)
                    continue;

                const Vector& up = half_edge_ups[i][j];

                if (!opposite_face(faces, edge_to_fe, i, j))
                    continue;

                std::optional<std::pair<int,int>> op_to   = opposite_face(faces, edge_to_fe, i, (j + 1) % n);
                std::optional<std::pair<int,int>> op_from = opposite_face(faces, edge_to_fe, i, (j - 1 + n) % n);

                Point our_center = Point::mid_point(EF[i][j].from, EF[i][j].to);
                const Vector& bdir = EF[i][j].dir;

                std::vector<wood_reciprocal::CutFace> cuts_to = op_to
                    ? wood_reciprocal::unbounded(cut_plane(EF, half_edge_ups, i, bdir, our_center, beam_w, cut_offset_factor,
                                                           op_to->first, op_to->second, EF[i][j].to))
                    : wood_reciprocal::boundary_cuts_or(frame, faces[i][(j + 1) % n], bdir, Plane::from_point_normal(EF[i][j].to, bdir));
                std::vector<wood_reciprocal::CutFace> cuts_from = op_from
                    ? wood_reciprocal::unbounded(cut_plane(EF, half_edge_ups, i, bdir, our_center, beam_w, cut_offset_factor,
                                                           op_from->first, op_from->second, EF[i][j].from))
                    : wood_reciprocal::boundary_cuts_or(frame, faces[i][j], bdir, Plane::from_point_normal(EF[i][j].from, bdir));
                wood_reciprocal::BeamGeom bg = wood_reciprocal::cut_beam(EF[i][j].from, EF[i][j].to, bdir, up, beam_w, beam_h, cuts_from, cuts_to);
                if (bg.mesh.number_of_vertices() == 0)
                    continue;

                wood_reciprocal::store_beam(bg, beams, side0, side1, beam_bottom, beam_top);
                beam_dirs.push_back({bdir[0], bdir[1], bdir[2]});
                beam_ups.push_back({up[0], up[1], up[2]});
            }
        }
    }
};
