#pragma once
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
            const auto& a = pts[face[i]];
            const auto& b = pts[face[(i + 1) % n]];
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
        std::vector<Point> beam_top;     // top face corners    (+up): sc[2],sc[3],ec[3],ec[2]
    };

    struct CutPlane { Point org; Vector n; };

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

        auto isect = [&](const CutPlane& cp, int k) -> Point {
            // Ray: (ref + sr*right + sn*n_up) + t*bdir
            Point ro(ref[0] + sr[k]*right[0] + sn[k]*n_up[0],
                     ref[1] + sr[k]*right[1] + sn[k]*n_up[1],
                     ref[2] + sr[k]*right[2] + sn[k]*n_up[2]);
            double denom = cp.n[0]*bdir[0] + cp.n[1]*bdir[1] + cp.n[2]*bdir[2];
            if (std::abs(denom) < 1e-10) return ro;  // parallel → no cut
            double t = ((cp.org[0]-ro[0])*cp.n[0] +
                        (cp.org[1]-ro[1])*cp.n[1] +
                        (cp.org[2]-ro[2])*cp.n[2]) / denom;
            return Point(ro[0]+t*bdir[0], ro[1]+t*bdir[1], ro[2]+t*bdir[2]);
        };

        std::array<Point, 4> sc, ec;
        for (int k = 0; k < 4; k++) {
            sc[k] = isect(cp_from, k);
            ec[k] = isect(cp_to,   k);
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
        bg.beam_top    = {sc[2], sc[3], ec[3], ec[2]};  // top face    (+up, for joinery)
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

        auto corner = [](const Point& p, const Vector& r, int sr,
                         const Vector& nn, int sn) {
            return Point(p[0] + sr*r[0] + sn*nn[0],
                         p[1] + sr*r[1] + sn*nn[1],
                         p[2] + sr*r[2] + sn*nn[2]);
        };

        std::array<Point, 4> sc = {
            corner(from, right, -1, n, -1),
            corner(from, right, +1, n, -1),
            corner(from, right, +1, n, +1),
            corner(from, right, -1, n, +1),
        };
        std::array<Point, 4> ec = {
            corner(to, right, -1, n, -1),
            corner(to, right, +1, n, -1),
            corner(to, right, +1, n, +1),
            corner(to, right, -1, n, +1),
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
        bg.beam_top    = {sc[2], sc[3], ec[3], ec[2]};  // top face    (+up, for joinery)
        return bg;
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
        using EdgeKey = std::pair<size_t, size_t>;
        std::map<EdgeKey, std::vector<std::pair<int,int>>> edge_to_fe;
        for (int i = 0; i < nf; i++) {
            int n = (int)faces[i].size();
            for (int j = 0; j < n; j++) {
                size_t u = faces[i][j], v = faces[i][(j+1)%n];
                edge_to_fe[{std::min(u,v), std::max(u,v)}].emplace_back(i, j);
            }
        }

        // Opposite half-edge: (fi, fj) → (fi2, fj2) in the adjacent face.
        auto get_opp = [&](int fi, int fj)
            -> std::optional<std::pair<int,int>>
        {
            const auto& fv = faces[fi];
            int n = (int)fv.size();
            size_t u = fv[fj], v = fv[(fj+1)%n];
            auto it = edge_to_fe.find({std::min(u,v), std::max(u,v)});
            if (it == edge_to_fe.end()) return std::nullopt;
            for (auto& [fi2, fj2] : it->second)
                if (fi2 != fi) return std::make_pair(fi2, fj2);
            return std::nullopt;  // boundary edge
        };

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
                auto opp = get_opp(i, j);
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
            if (conflict) {
                fprintf(stderr,
                        "  WARNING: ReciprocalMove half-edge 2-coloring found an odd "
                        "cycle (non-quad face) - beam selection may be inconsistent. "
                        "Use an even-gon (quad) mesh.\n");
                fflush(stderr);
            }
        }

        // ── build face-edge centerlines (raw mesh vertices, no extension) ──
        struct FELine { Point from, to; Vector dir; };
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
                auto opp_next = get_opp(i, (j+1)%n);
                int tni = opp_next ? opp_next->first  : i;
                int tnj = opp_next ? opp_next->second : (j+1)%n;
                EF[i][j].to = _lcp1(cur.from, cur.dir, EF[tni][tnj].from, EF[tni][tnj].dir);

                // Trim FROM: opposite of prev edge, fallback to same-face prev
                auto opp_prev = get_opp(i, (j-1+n)%n);
                int tpi = opp_prev ? opp_prev->first  : i;
                int tpj = opp_prev ? opp_prev->second : (j-1+n)%n;
                EF[i][j].from = _lcp1(cur.from, cur.dir, EF[tpi][tpj].from, EF[tpi][tpj].dir);
            }
        }

        // ── precompute per-face normals (needed for cutting beam planes) ──
        std::vector<Vector> face_norms(nf);
        for (int i = 0; i < nf; i++)
            face_norms[i] = _face_normal(pts, faces[i]);

        // ── helpers for plane-based end cutting ──
        auto cross3 = [](const Vector& a, const Vector& b) -> Vector {
            return Vector(a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]);
        };
        auto midpt = [](const Point& a, const Point& b) -> Point {
            return Point((a[0]+b[0])*0.5, (a[1]+b[1])*0.5, (a[2]+b[2])*0.5);
        };

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
                if (!get_opp(i, j)) continue;

                // Cutting beams come from the prev/next edges of OUR face:
                //   FROM end cut: prev edge (j-1) → its opposite face beam
                //   TO   end cut: next edge (j+1) → its opposite face beam
                auto op_to   = get_opp(i, (j + 1) % n);
                auto op_from = get_opp(i, (j - 1 + n) % n);

                // Interior cut: crossing beam's side face (normal = crossing_dir × crossing_up, signed).
                // Use OUR beam's center (not endpoint) for sign selection — more stable.
                Point our_center = midpt(EF[i][j].from, EF[i][j].to);
                const Vector& bdir = EF[i][j].dir;

                // Interior cut: crossing beam's side face. Falls back to flat perpendicular
                // cap at `endpoint` when the computed normal is ⊥ to our beam (degenerate
                // intersection — happens at cone apex where cutting beam is parallel to ours).
                auto make_cut_plane = [&](int ci, int cj, const Point& endpoint) -> CutPlane {
                    Vector cup = face_norms[ci];
                    if (cup[2] < 0) cup = Vector(-cup[0], -cup[1], -cup[2]);
                    Vector raw = cross3(EF[ci][cj].dir, cup);
                    double len = std::sqrt(raw[0]*raw[0]+raw[1]*raw[1]+raw[2]*raw[2]);
                    if (len > 1e-12) raw = Vector(raw[0]/len, raw[1]/len, raw[2]/len);
                    else             raw = cross3(EF[ci][cj].dir, face_norms[i]);
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
                    Point org = midpt(EF[ci][cj].from, EF[ci][cj].to);
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
                };

                // Boundary cut: plane at beam endpoint, normal = cross(boundary_edge_dir, face_normal).
                // Same degenerate fallback as above.
                auto make_boundary_cut_plane = [&](int adj_j, const Point& endpoint) -> CutPlane {
                    const Vector& edge_dir = EF[i][adj_j].dir;
                    Vector raw = cross3(edge_dir, face_norms[i]);
                    double len = std::sqrt(raw[0]*raw[0]+raw[1]*raw[1]+raw[2]*raw[2]);
                    if (len > 1e-12) raw = Vector(raw[0]/len, raw[1]/len, raw[2]/len);
                    double alignment = std::abs(raw[0]*bdir[0]+raw[1]*bdir[1]+raw[2]*bdir[2]);
                    if (alignment < 0.15) return {endpoint, bdir};  // see FLAT_CAP note above
                    return {endpoint, raw};
                };

                CutPlane cp_to   = op_to
                    ? make_cut_plane(op_to->first,   op_to->second,   EF[i][j].to)
                    : make_boundary_cut_plane((j + 1) % n,             EF[i][j].to);
                CutPlane cp_from = op_from
                    ? make_cut_plane(op_from->first, op_from->second, EF[i][j].from)
                    : make_boundary_cut_plane((j - 1 + n) % n,        EF[i][j].from);
                BeamGeom bg = _make_beam_cut(EF[i][j].from, bdir, up,
                                             beam_w, beam_h, cp_from, cp_to);
                if (bg.mesh.number_of_vertices() == 0) continue;

                beams.push_back(std::move(bg.mesh));

                std::vector<Point> s0 = bg.side0; s0.push_back(s0[0]);
                side0.emplace_back(s0);

                std::vector<Point> s1 = bg.side1; s1.push_back(s1[0]);
                side1.emplace_back(s1);

                std::vector<Point> bb = bg.beam_bottom; bb.push_back(bb[0]);
                beam_bottom.emplace_back(bb);

                std::vector<Point> bt = bg.beam_top; bt.push_back(bt[0]);
                beam_top.emplace_back(bt);

                beam_dirs.push_back({bdir[0], bdir[1], bdir[2]});
                beam_ups.push_back({up[0], up[1], up[2]});
            }
        }
    }
};
