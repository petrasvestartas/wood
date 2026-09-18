#pragma once
#include <iostream>
#include <cstdio>
#include "session.h"
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
/// Usage:
///   ReciprocalMove rm;                    // default 12×10 sinusoidal dome
///   ReciprocalMove rm(mesh, 50.0, 100.0); // user mesh, 50 mm offset, 100 mm beam
class ReciprocalMove {
public:
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
                   double angle             = 0.05,
                   double beam_w            = 0.10,
                   double beam_h            = 0.0,
                   double extend_factor     = 5.0,
                   double cut_offset_factor = 1.0)
    {

        if (nx < 1 || ny < 1)
            throw std::invalid_argument("ReciprocalMove: nx and ny must be >= 1");

        dome_mesh = _make_dome(nx, ny, W, D, h);
        _build(dome_mesh, angle, beam_w, beam_h, extend_factor, cut_offset_factor);
    }

    /// External mesh constructor — use any mesh as the base.
    explicit ReciprocalMove(Mesh ext_mesh,
                            double angle             = 0.05,
                            double beam_w            = 0.10,
                            double beam_h            = 0.0,
                            double extend_factor     = 5.0,
                            double cut_offset_factor = 1.0)
    {
        dome_mesh = std::move(ext_mesh);
        _build(dome_mesh, angle, beam_w, beam_h, extend_factor, cut_offset_factor);
    }

private:
    // ── helpers ──────────────────────────────────────────────────────────────

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
        if (len < 1e-12) return Vector(0, 0, 1);

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
        if (std::abs(denom) < 1e-12) return P;  // parallel lines

        double t = (b * e_ - c * d_) / denom;
        return Point(P[0] + t*D[0], P[1] + t*D[1], P[2] + t*D[2]);
    }

    struct BeamGeom {
        Mesh               mesh;
        std::vector<Point> side0;
        std::vector<Point> side1;
        std::vector<Point> beam_bottom;  // bottom face corners (-up): sc[0],sc[1],ec[1],ec[0]
        std::vector<Point> beam_top;     // top face corners    (+up): sc[3],sc[2],ec[2],ec[3], each above beam_bottom[i]
    };

    struct CutPlane { Point org; Vector n; };
    struct FELine { Point from, to; Vector dir; };  // one face-edge centerline
    using EdgeKey = std::pair<size_t, size_t>;  // sorted vertex pair of a mesh edge
    using EdgeToFaceEdges = std::map<EdgeKey, std::vector<std::pair<int,int>>>;  // edge → [(face_idx, local_edge_idx), …]

    /// The corners as a closed outline: the first corner repeated at the end.
    static Polyline closed_outline(const std::vector<Point>& corners)
    {

        std::vector<Point> closed = corners;
        closed.push_back(corners[0]);
        return Polyline(closed);
    }

    /// Appends one beam's mesh and its four closed outlines to the given lists.
    static void store_beam(BeamGeom& bg, std::vector<Mesh>& meshes,
                           std::vector<Polyline>& right_faces, std::vector<Polyline>& left_faces,
                           std::vector<Polyline>& bottom_faces, std::vector<Polyline>& top_faces)
    {

        meshes.push_back(std::move(bg.mesh));
        right_faces.push_back(closed_outline(bg.side0));
        left_faces.push_back(closed_outline(bg.side1));
        bottom_faces.push_back(closed_outline(bg.beam_bottom));
        top_faces.push_back(closed_outline(bg.beam_top));
    }

    /// One box corner: p offset by sr times r and by sn times nn.
    static Point corner_point(const Point& p, const Vector& r, int sr,
                              const Vector& nn, int sn)
    {
        return Point(p[0] + sr*r[0] + sn*nn[0],
                     p[1] + sr*r[1] + sn*nn[1],
                     p[2] + sr*r[2] + sn*nn[2]);
    }

    /// Where the ray from origin along direction meets the cut plane; origin itself when they are parallel.
    static Point ray_plane_intersection(const Point& origin, const Vector& direction, const CutPlane& cut)
    {

        double denom = cut.n[0]*direction[0] + cut.n[1]*direction[1] + cut.n[2]*direction[2];
        if (std::abs(denom) < 1e-10) return origin;  // parallel → no cut

        double t = ((cut.org[0]-origin[0])*cut.n[0] +
                    (cut.org[1]-origin[1])*cut.n[1] +
                    (cut.org[2]-origin[2])*cut.n[2]) / denom;
        return Point(origin[0]+t*direction[0], origin[1]+t*direction[1], origin[2]+t*direction[2]);
    }

    // Build a box beam whose ends are cut by arbitrary planes (not flat perpendicular caps).
    // Each of the 4 corner rays (parallel to bdir, offset by ±right, ±up) is intersected
    // with cp_from and cp_to independently, producing angled end faces.
    // ref = any point on the beam axis (used as ray origin offset).
    static BeamGeom _make_beam_cut(const Point& ref, const Vector& bdir,
                                   const Vector& up, double w, double h,
                                   const CutPlane& cp_from, const CutPlane& cp_to)
    {

        Vector right_raw = bdir.cross(up);
        if (right_raw.is_zero()) right_raw = bdir.cross(Vector(0, 0, 1));
        Vector right = right_raw.normalized() * (w * 0.5);
        Vector n_up  = up * (h * 0.5);

        // sr[k], sn[k] = signs for right and up at corner k
        const int sr[4] = {-1, +1, +1, -1};
        const int sn[4] = {-1, -1, +1, +1};

        std::array<Point, 4> sc, ec;
        for (int k = 0; k < 4; k++) {
            // Ray: (ref + sr*right + sn*n_up) + t*bdir
            Point ro = corner_point(ref, right, sr[k], n_up, sn[k]);
            sc[k] = ray_plane_intersection(ro, bdir, cp_from);
            ec[k] = ray_plane_intersection(ro, bdir, cp_to);
        }

        std::vector<Point> pts = {
            sc[0], sc[1], sc[2], sc[3],
            ec[0], ec[1], ec[2], ec[3],
        };
        std::vector<std::vector<size_t>> faces = {
            {0, 1, 2, 3},  // start cap
            {4, 7, 6, 5},  // end cap
            {0, 4, 5, 1},  // bottom
            {1, 5, 6, 2},  // right
            {2, 6, 7, 3},  // top
            {3, 7, 4, 0},  // left
        };

        BeamGeom bg;
        bg.mesh        = Mesh::from_vertices_and_faces(pts, faces);
        bg.side0       = {sc[1], sc[2], ec[2], ec[1]};
        bg.side1       = {sc[0], sc[3], ec[3], ec[0]};
        bg.beam_bottom = {sc[0], sc[1], ec[1], ec[0]};  // bottom face (-up, for joinery)
        bg.beam_top    = {sc[3], sc[2], ec[2], ec[3]};  // top face (+up), corner i above beam_bottom[i] so the loft pairs them

        return bg;
    }

    static BeamGeom _make_beam(const Point& from, const Point& to,
                                const Vector& up, double w, double h)
    {

        double dx = to[0]-from[0], dy = to[1]-from[1], dz = to[2]-from[2];
        double len = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (len < 1e-12) return {};

        Vector dir(dx/len, dy/len, dz/len);

        Vector right = dir.cross(up);
        if (right.is_zero()) right = dir.cross(Vector(0, 0, 1));
        right = right.normalized() * (w * 0.5);
        Vector n = up * (h * 0.5);

        std::array<Point, 4> sc = {
            corner_point(from, right, -1, n, -1),
            corner_point(from, right, +1, n, -1),
            corner_point(from, right, +1, n, +1),
            corner_point(from, right, -1, n, +1),
        };
        std::array<Point, 4> ec = {
            corner_point(to, right, -1, n, -1),
            corner_point(to, right, +1, n, -1),
            corner_point(to, right, +1, n, +1),
            corner_point(to, right, -1, n, +1),
        };

        std::vector<Point> pts = {
            sc[0], sc[1], sc[2], sc[3],
            ec[0], ec[1], ec[2], ec[3],
        };
        std::vector<std::vector<size_t>> faces = {
            {0, 1, 2, 3},  // start cap
            {4, 7, 6, 5},  // end cap
            {0, 4, 5, 1},  // bottom
            {1, 5, 6, 2},  // right
            {2, 6, 7, 3},  // top
            {3, 7, 4, 0},  // left
        };

        BeamGeom bg;
        bg.mesh        = Mesh::from_vertices_and_faces(pts, faces);
        bg.side0       = {sc[1], sc[2], ec[2], ec[1]};  // right face
        bg.side1       = {sc[0], sc[3], ec[3], ec[0]};  // left  face
        bg.beam_bottom = {sc[0], sc[1], ec[1], ec[0]};  // bottom face (-up, for joinery)
        bg.beam_top    = {sc[3], sc[2], ec[2], ec[3]};  // top face (+up), corner i above beam_bottom[i] so the loft pairs them

        return bg;
    }

    /// Opposite half-edge: (fi, fj) → (fi2, fj2) in the adjacent face, nullopt on a boundary edge.
    static std::optional<std::pair<int,int>> opposite_face(const std::vector<std::vector<size_t>>& faces,
                                                           const EdgeToFaceEdges& edge_to_fe,
                                                           int fi, int fj)
    {

        const std::vector<size_t>& fv = faces[fi];
        int n = (int)fv.size();
        size_t u = fv[fj], v = fv[(fj+1)%n];
        auto it = edge_to_fe.find({std::min(u,v), std::max(u,v)});
        if (it == edge_to_fe.end()) return std::nullopt;

        for (auto& [fi2, fj2] : it->second)
            if (fi2 != fi) return std::make_pair(fi2, fj2);

        return std::nullopt;  // boundary edge
    }

    /// Interior cut: the side face of the crossing beam (ci, cj), offset by half the beam width and
    /// signed towards our_center. Falls back to a flat perpendicular cap at `endpoint` when the computed
    /// normal is ⊥ to our beam (degenerate intersection — happens at cone apex where cutting beam is parallel to ours).
    static CutPlane cut_plane(const std::vector<std::vector<FELine>>& EF,
                              const std::vector<Vector>& face_norms,
                              int i, const Vector& bdir, const Point& our_center,
                              double beam_w, double cut_offset_factor,
                              int ci, int cj, const Point& endpoint)
    {

        Vector cup = face_norms[ci];
        if (cup[2] < 0) cup = Vector(-cup[0], -cup[1], -cup[2]);
        Vector raw = EF[ci][cj].dir.cross(cup);
        double len = std::sqrt(raw[0]*raw[0]+raw[1]*raw[1]+raw[2]*raw[2]);
        if (len > 1e-12) raw = Vector(raw[0]/len, raw[1]/len, raw[2]/len);
        else             raw = EF[ci][cj].dir.cross(face_norms[i]);
        // Degenerate: cut plane normal ~perpendicular to beam
        // direction -> flat cap fallback. NOTE the breadth of
        // this net: cos(angle) < 0.15 means any crossing beam
        // within ~8.6 deg of perpendicular to our cut normal
        // (i.e. up to ~81 deg from the beam axis) gets a flat
        // cap instead of the angled interlock. Kept at the
        // historical value for output stability; tighten
        // deliberately if flat caps appear where interlocks are
        // expected.
        constexpr double FLAT_CAP_ALIGNMENT_THRESHOLD = 0.15;
        double alignment = std::abs(raw[0]*bdir[0]+raw[1]*bdir[1]+raw[2]*bdir[2]);
        if (alignment < FLAT_CAP_ALIGNMENT_THRESHOLD) return {endpoint, bdir};

        Point org = Point::mid_point(EF[ci][cj].from, EF[ci][cj].to);
        double dot = (our_center[0]-org[0])*raw[0]
                   + (our_center[1]-org[1])*raw[1]
                   + (our_center[2]-org[2])*raw[2];
        if (dot < 0) raw = Vector(-raw[0], -raw[1], -raw[2]);

        // cut_offset_factor was accepted by both constructors
        // and silently dropped; ReciprocalRotation applies it to
        // the face offset, so mirror that here. Default 1.0
        // preserves all existing output.
        const double face_off = beam_w * 0.5 * cut_offset_factor;
        Point face_org(org[0] + face_off*raw[0],
                       org[1] + face_off*raw[1],
                       org[2] + face_off*raw[2]);

        return {face_org, raw};
    }

    /// The naked half-edges as (face, local edge) in boundary-loop order, each walked in its owning face's winding.
    static std::vector<std::pair<int,int>> naked_half_edges(const std::vector<std::vector<size_t>>& faces,
                                                            const EdgeToFaceEdges& edge_to_fe)
    {

        std::vector<std::pair<int,int>> naked;
        std::map<size_t, size_t> leaving_vertex;  // boundary vertex → the naked half-edge starting there
        for (const auto& [key, owners] : edge_to_fe) {
            if (owners.size() != 1) continue;

            const auto& [fi, fj] = owners[0];
            if (!leaving_vertex.emplace(faces[fi][fj], naked.size()).second)
                std::cerr << fmt::format("  WARNING: ReciprocalMove: two naked edges leave vertex {} - the boundary is not a simple loop there.\n", faces[fi][fj]);
            naked.push_back(owners[0]);
        }

        std::vector<std::pair<int,int>> ordered;
        std::vector<bool> visited(naked.size(), false);
        for (size_t seed = 0; seed < naked.size(); seed++) {
            size_t current = seed;
            while (!visited[current]) {
                visited[current] = true;
                ordered.push_back(naked[current]);

                const auto& [fi, fj] = naked[current];
                std::map<size_t, size_t>::const_iterator next = leaving_vertex.find(faces[fi][(fj + 1) % faces[fi].size()]);
                if (next == leaving_vertex.end()) break;

                current = next->second;
            }
        }

        return ordered;
    }

    /// One up per boundary loop: the average of the owning faces' normals around the loop, sign-matched, returned per naked half-edge and flipped to agree with that edge's own face.
    static std::vector<Vector> loop_up_directions(const std::vector<std::pair<int,int>>& naked,
                                                  const std::vector<std::vector<size_t>>& faces,
                                                  const std::vector<Vector>& owner_normals)
    {

        std::vector<Vector> ups(naked.size());
        size_t loop_start = 0;
        while (loop_start < naked.size()) {
            size_t loop_end = loop_start + 1;  // one past the last half-edge of this loop: the walk keeps a loop's half-edges consecutive
            while (loop_end < naked.size() && faces[naked[loop_end].first][naked[loop_end].second]
                                              == faces[naked[loop_end - 1].first][(naked[loop_end - 1].second + 1) % faces[naked[loop_end - 1].first].size()])
                loop_end++;

            Vector sum(0, 0, 0);
            for (size_t k = loop_start; k < loop_end; k++)
                sum += owner_normals[k].dot(sum) < 0.0 ? -owner_normals[k] : owner_normals[k];

            Vector average = sum.is_zero() ? Vector(0, 0, 1) : sum.normalized();
            for (size_t k = loop_start; k < loop_end; k++)
                ups[k] = owner_normals[k].dot(average) < 0.0 ? -average : average;

            loop_start = loop_end;
        }

        return ups;
    }

    /// The mitre at a boundary vertex: the bisector plane whose normal is the sum of the arriving and leaving unit edge directions, the arriving one alone when they fold back.
    static CutPlane mitre_plane(const Point& vertex, const Vector& arriving, const Vector& leaving)
    {

        Vector normal = arriving + leaving;
        if (normal.is_zero()) return {vertex, arriving};

        return {vertex, normal.normalized()};
    }

    /// The boundary beam's side plane that looks into the shell: half the beam width from the edge axis towards the owning face, which lies to the left of its own half-edge.
    static CutPlane boundary_inner_plane(const Point& from, const Point& to, const Vector& dir,
                                         const Vector& up, double beam_w)
    {

        Vector inner = up.cross(dir).normalized();
        return {Point::mid_point(from, to) + inner * (beam_w * 0.5), inner};
    }

    /// Boundary cut: the inner face plane of the boundary beam on the naked edge (i, adj_j), so the interior beam stops flush against it. Same degenerate fallback as cut_plane.
    static CutPlane boundary_cut_plane(const std::map<EdgeKey, CutPlane>& inner_planes,
                                       const std::vector<std::vector<size_t>>& faces,
                                       int i, const Vector& bdir, int adj_j, const Point& endpoint)
    {

        const std::vector<size_t>& fv = faces[i];
        size_t u = fv[adj_j], v = fv[(adj_j + 1) % fv.size()];
        std::map<EdgeKey, CutPlane>::const_iterator it = inner_planes.find({std::min(u, v), std::max(u, v)});
        if (it == inner_planes.end()) return {endpoint, bdir};

        double alignment = std::abs(it->second.n.dot(bdir));
        if (alignment < 0.15) return {endpoint, bdir};  // see FLAT_CAP note in cut_plane

        return it->second;
    }

    // ── main build ───────────────────────────────────────────────────────────

    void _build(const Mesh& m, double angle, double beam_w, double beam_h,
                double extend_factor, double cut_offset_factor = 1.0)
    {

        if (beam_w <= 0.0)
            throw std::invalid_argument("ReciprocalMove: beam_w must be positive");

        if (beam_h <= 0.0) beam_h = beam_w * 2.0;
        (void)extend_factor;  // raw mesh edges — no extension (C# NexorTranslateLines)

        auto [pts, faces] = m.to_vertices_and_faces();
        int nf = (int)faces.size();

        // ── edge → [(face_idx, local_edge_idx), …] ──
        EdgeToFaceEdges edge_to_fe;
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {
                size_t u = faces[i][j], v = faces[i][(j+1)%n];
                edge_to_fe[{std::min(u,v), std::max(u,v)}].emplace_back(i, j);
            }
        }

        // ── FEFlatten: sequential half-edge IDs ──
        // FEFlat[i][j] = global half-edge index (same as C# FEFlatten)
        int total_he = 0;
        std::vector<std::vector<int>> FEFlat(nf);

        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            FEFlat[i].resize(n);
            for (int j = 0; j < n; j++)
                FEFlat[i][j] = total_he++;
        }

        // ── BFS 2-coloring of half-edges (C# _GraphColorHalfEdgesDict) ──
        // Adjacency: same-face next/prev + opposite half-edge across shared mesh edge.
        // This produces the checker pattern: shared edges get opposite colors in their two faces,
        // so each physical edge is translated from exactly one face (never both).
        std::vector<std::vector<int>> he_adj(total_he);
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {
                int id = FEFlat[i][j];
                he_adj[id].push_back(FEFlat[i][(j+1)%n]);
                he_adj[id].push_back(FEFlat[i][(j-1+n)%n]);
                std::optional<std::pair<int,int>> opp = opposite_face(faces, edge_to_fe, i, j);
                if (opp) he_adj[id].push_back(FEFlat[opp->first][opp->second]);
            }
        }
        std::vector<int> edgeColors(total_he, -1);
        {
            // total_he == 0 (a mesh with no faces, reachable through the
            // any-mesh constructor) used to write edgeColors[0] into an
            // empty vector - UB straight from the Python binding.
            if (total_he == 0) { return; }

            // Seed a BFS in EVERY unvisited component: the old single seed
            // at index 0 left all other components at -1, which the `!= 0`
            // consumer below treated as color 1 - disconnected mesh parts
            // silently produced no beams at all. Odd face cycles (triangles,
            // pentagons) cannot be 2-colored; detect the conflict instead of
            // breaking the one-beam-per-physical-edge invariant silently.
            std::queue<int> q;
            bool conflict = false;

            for (int seed = 0; seed < total_he; ++seed) {

                if (edgeColors[seed] != -1) continue;

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
                std::cerr << "  WARNING: ReciprocalMove half-edge 2-coloring found an odd cycle (non-quad face) - beam selection may be inconsistent. Use an even-gon (quad) mesh." << std::endl;
        }

        // ── build face-edge centerlines (raw mesh vertices, no extension) ──
        std::vector<std::vector<FELine>> EF(nf);

        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            EF[i].resize(n);
            for (int j = 0; j < n; j++) {
                const Point& a = pts[faces[i][j]];
                const Point& b = pts[faces[i][(j+1)%n]];
                double dx = b[0]-a[0], dy = b[1]-a[1], dz = b[2]-a[2];
                double len = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (len < 1e-12) len = 1.0;
                Vector unit(dx/len, dy/len, dz/len);
                EF[i][j].dir  = unit;
                EF[i][j].from = a;
                EF[i][j].to   = b;
            }
        }

        // ── translation phase: move color-0 face-edges in face plane ──
        // bisector = (next_unit - prev_unit) * 0.5  (C# v = (v0+v1)*0.5, v1 = -prev.Unit)
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {

                if (edgeColors[FEFlat[i][j]] != 0) continue;

                const Vector& d_next = EF[i][(j+1)%n].dir;
                const Vector& d_prev = EF[i][(j-1+n)%n].dir;
                double vx = (d_next[0] - d_prev[0]) * 0.5;
                double vy = (d_next[1] - d_prev[1]) * 0.5;
                double vz = (d_next[2] - d_prev[2]) * 0.5;
                EF[i][j].from = Point(EF[i][j].from[0]+vx*angle,
                                      EF[i][j].from[1]+vy*angle,
                                      EF[i][j].from[2]+vz*angle);
                EF[i][j].to   = Point(EF[i][j].to[0]+vx*angle,
                                      EF[i][j].to[1]+vy*angle,
                                      EF[i][j].to[2]+vz*angle);
            }
        }

        // ── trimming phase: in-place on EF, boundary fallback to same-face neighbor ──
        // Matches C# NexorTranslateLines exactly:
        //   line = EFLinesCopy[i][j] (local copy); trim TO via opp-of-next; trim FROM via opp-of-prev
        //   if opp is null (boundary), fall back to same-face next/prev edge
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {

                if (edgeColors[FEFlat[i][j]] != 0) continue;

                const FELine cur = EF[i][j];  // local copy before in-place modification

                // Trim TO: opposite of next edge, fallback to same-face next
                std::optional<std::pair<int,int>> opp_next = opposite_face(faces, edge_to_fe, i, (j+1)%n);
                int tni = opp_next ? opp_next->first  : i;
                int tnj = opp_next ? opp_next->second : (j+1)%n;
                EF[i][j].to = _lcp1(cur.from, cur.dir, EF[tni][tnj].from, EF[tni][tnj].dir);

                // Trim FROM: opposite of prev edge, fallback to same-face prev
                std::optional<std::pair<int,int>> opp_prev = opposite_face(faces, edge_to_fe, i, (j-1+n)%n);
                int tpi = opp_prev ? opp_prev->first  : i;
                int tpj = opp_prev ? opp_prev->second : (j-1+n)%n;
                EF[i][j].from = _lcp1(cur.from, cur.dir, EF[tpi][tpj].from, EF[tpi][tpj].dir);
            }
        }

        // ── precompute per-face normals (needed for cutting beam planes) ──
        std::vector<Vector> face_norms(nf);
        for (int i = 0; i < nf; i++)
            face_norms[i] = _face_normal(pts, faces[i]);

        // ── boundary beams: a straight beam on every naked edge, none of the reciprocal treatment ──
        // Axis = the mesh edge, up = one direction per boundary loop (the average of the owning faces'
        // normals, so consecutive prisms share their mitred end face), section centred on the axis like
        // the interior beams; consecutive beams are mitred with the bisector plane at their shared vertex.
        std::vector<std::pair<int,int>> naked = naked_half_edges(faces, edge_to_fe);
        std::vector<Vector> owner_normals;  // the owning face's normal per naked half-edge, flipped to +z like the interior beams' up
        for (const auto& [fi, fj] : naked)
            owner_normals.push_back(face_norms[fi][2] < 0 ? -face_norms[fi] : face_norms[fi]);

        std::vector<Vector> loop_ups = loop_up_directions(naked, faces, owner_normals);
        std::map<size_t, Vector> arriving, leaving;  // boundary vertex → unit direction of the naked edge ending / starting there
        for (const auto& [fi, fj] : naked) {
            size_t u = faces[fi][fj], v = faces[fi][(fj + 1) % faces[fi].size()];
            if ((pts[v] - pts[u]).is_zero()) continue;

            Vector dir = (pts[v] - pts[u]).normalized();
            arriving[v] = dir;
            leaving[u]  = dir;
        }

        std::map<EdgeKey, CutPlane> boundary_inner;  // naked edge → its boundary beam's face plane that looks into the shell
        for (size_t k = 0; k < naked.size(); k++) {
            const auto& [fi, fj] = naked[k];
            size_t u = faces[fi][fj], v = faces[fi][(fj + 1) % faces[fi].size()];
            if ((pts[v] - pts[u]).is_zero()) continue;

            Vector dir = (pts[v] - pts[u]).normalized();
            const Vector& up = loop_ups[k];

            CutPlane cp_from = arriving.count(u) ? mitre_plane(pts[u], arriving[u], dir) : CutPlane{pts[u], dir};
            CutPlane cp_to   = leaving.count(v)  ? mitre_plane(pts[v], dir, leaving[v])  : CutPlane{pts[v], dir};
            BeamGeom bg = _make_beam_cut(pts[u], dir, up, beam_w, beam_h, cp_from, cp_to);
            store_beam(bg, boundary_beams, boundary_side0, boundary_side1, boundary_beam_bottom, boundary_beam_top);
            boundary_inner[{std::min(u, v), std::max(u, v)}] = boundary_inner_plane(pts[u], pts[v], dir, up, beam_w);
        }

        // ── build box beams: interior color-0 edges, ends cut by crossing beam planes ──
        // Matches C# Beams2:
        //   op = _OppositeFE(i, j, -1) → side beam crossing our beam at "To"  end
        //   on = _OppositeFE(i, j, +1) → side beam crossing our beam at "From" end
        //   cut plane origin = center of that crossing beam (trimmed EF)
        //   cut plane normal = crossing_beam_dir × crossing_face_normal  (C# ePl.ZAxis)
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            Vector up = face_norms[i];
            if (up[2] < 0) up = Vector(-up[0], -up[1], -up[2]);

            for (int j = 0; j < n; j++) {

                if (edgeColors[FEFlat[i][j]] != 0) continue;

                // Skip boundary beams (C# Beams2: if (!p.M.IsNaked(fe[j])))
                if (!opposite_face(faces, edge_to_fe, i, j)) continue;

                // Cutting beams come from the prev/next edges of OUR face:
                //   FROM end cut: prev edge (j-1) → its opposite face beam
                //   TO   end cut: next edge (j+1) → its opposite face beam
                std::optional<std::pair<int,int>> op_to   = opposite_face(faces, edge_to_fe, i, (j + 1) % n);
                std::optional<std::pair<int,int>> op_from = opposite_face(faces, edge_to_fe, i, (j - 1 + n) % n);

                // Interior cut: crossing beam's side face (normal = crossing_dir × crossing_up, signed).
                // Use OUR beam's center (not endpoint) for sign selection — more stable.
                Point our_center = Point::mid_point(EF[i][j].from, EF[i][j].to);
                const Vector& bdir = EF[i][j].dir;

                CutPlane cp_to   = op_to
                    ? cut_plane(EF, face_norms, i, bdir, our_center, beam_w, cut_offset_factor,
                                op_to->first, op_to->second, EF[i][j].to)
                    : boundary_cut_plane(boundary_inner, faces, i, bdir, (j + 1) % n, EF[i][j].to);
                CutPlane cp_from = op_from
                    ? cut_plane(EF, face_norms, i, bdir, our_center, beam_w, cut_offset_factor,
                                op_from->first, op_from->second, EF[i][j].from)
                    : boundary_cut_plane(boundary_inner, faces, i, bdir, (j - 1 + n) % n, EF[i][j].from);
                BeamGeom bg = _make_beam_cut(EF[i][j].from, bdir, up,
                                             beam_w, beam_h, cp_from, cp_to);
                if (bg.mesh.number_of_vertices() == 0) continue;

                store_beam(bg, beams, side0, side1, beam_bottom, beam_top);
                beam_dirs.push_back({bdir[0], bdir[1], bdir[2]});
                beam_ups.push_back({up[0], up[1], up[2]});
            }
        }
    }
};
