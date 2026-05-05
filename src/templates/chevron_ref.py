"""
Pure-numpy reference implementation of the Chevron plate algorithm.
No Rhino dependency. Used to generate ground-truth plate coordinates
for comparison against the C++ implementation.

Usage:
    python chevron_ref.py > chevron_ref.json
"""

import numpy as np
import json
import math
from collections import OrderedDict

# ─────────────────────────────────────────────────────────────
# Plane helpers
# ─────────────────────────────────────────────────────────────

class Plane:
    def __init__(self, origin, x_axis, y_axis):
        o = np.asarray(origin, float)
        x = np.asarray(x_axis, float)
        y = np.asarray(y_axis, float)
        z = np.cross(x, y)
        zn = np.linalg.norm(z)
        if zn > 1e-12:
            z = z / zn
        else:
            z = np.array([0.0, 0.0, 1.0])
        xn = np.linalg.norm(x)
        x = x / xn if xn > 1e-12 else np.array([1.0, 0.0, 0.0])
        self.o = o
        self.x = x
        self.y = y / np.linalg.norm(y) if np.linalg.norm(y) > 1e-12 else np.array([0.0, 1.0, 0.0])
        self.z = z

    def translate_by_normal(self, d):
        return Plane(self.o + self.z * d, self.x, self.y)

    def offset_along_normal(self, d):
        """alias"""
        return self.translate_by_normal(d)

    def move_along_axis(self, d, axis=2):
        """move along x(0), y(1), or z(2) axis"""
        if axis == 0:   return Plane(self.o + self.x * d, self.x, self.y)
        elif axis == 1: return Plane(self.o + self.y * d, self.x, self.y)
        else:           return self.translate_by_normal(d)

    def rotate_around_y(self, angle_rad):
        """Rotate plane around its own Y-axis."""
        c, s = math.cos(angle_rad), math.sin(angle_rad)
        new_x = c * self.x + s * self.z
        new_z = -s * self.x + c * self.z
        new_x /= np.linalg.norm(new_x)
        new_z /= np.linalg.norm(new_z)
        # rebuild y from z × x
        new_y = np.cross(new_z, new_x)
        return Plane(self.o, new_x, new_y)

    def flip_x(self):
        """Mirror plane: negate x_axis (for neighbor update)."""
        return Plane(self.o, -self.x, self.y)

    def __repr__(self):
        return f"Plane(o={self.o.tolist()}, x={self.x.tolist()}, z={self.z.tolist()})"


def orthogonalize_plane(plane):
    """Snap z-axis to the dominant world axis."""
    z = plane.z
    az = np.abs(z)
    idx = int(np.argmax(az))
    sign = math.copysign(1.0, z[idx])
    new_z = np.zeros(3)
    new_z[idx] = sign
    # rebuild plane from new normal
    ref = np.array([1.0, 0.0, 0.0]) if idx != 0 else np.array([0.0, 1.0, 0.0])
    new_x = np.cross(ref, new_z)
    nxn = np.linalg.norm(new_x)
    if nxn < 1e-12:
        new_x = np.cross([0.0, 1.0, 0.0], new_z)
    new_x /= np.linalg.norm(new_x)
    new_y = np.cross(new_z, new_x)
    return Plane(plane.o, new_x, new_y)


def plane_plane_plane(p0, p1, p2):
    """Intersect 3 planes → point. Returns None if degenerate."""
    A = np.array([p0.z, p1.z, p2.z])
    b = np.array([np.dot(p0.z, p0.o),
                  np.dot(p1.z, p1.o),
                  np.dot(p2.z, p2.o)])
    try:
        return np.linalg.solve(A, b)
    except np.linalg.LinAlgError:
        return None


def plane_plane_line(p0, p1):
    """Intersect 2 planes → (point_on_line, direction). Returns None if parallel."""
    d = np.cross(p0.z, p1.z)
    dn = np.linalg.norm(d)
    if dn < 1e-10:
        return None, None
    d = d / dn
    # anchor: find point closest to both planes via least-squares
    A = np.array([p0.z, p1.z, d])
    b = np.array([np.dot(p0.z, p0.o), np.dot(p1.z, p1.o), 0.0])
    try:
        anchor = np.linalg.solve(A, b)
    except np.linalg.LinAlgError:
        return None, None
    return anchor, d


def line_line_closest(o0, d0, o1, d1):
    """Closest point on line0 to line1 (infinite lines). Returns (t0, t1, point_on_line0)."""
    w = o0 - o1
    a = np.dot(d0, d0)
    b = np.dot(d0, d1)
    c = np.dot(d1, d1)
    d = np.dot(d0, w)
    e = np.dot(d1, w)
    denom = a * c - b * b
    if abs(denom) < 1e-12:
        return 0.0, 0.0, o0
    t0 = (b * e - c * d) / denom
    t1 = (a * e - b * d) / denom
    return t0, t1, o0 + d0 * t0


def dihedral_plane(p0, p1):
    """Bisector plane between two edge planes, matching the Rhino Python algorithm."""
    anchor, line_dir = plane_plane_line(p0, p1)
    if anchor is None:
        return None

    # lines along each plane's normal from its origin
    o0, n0 = p0.o, p0.z
    o1, n1 = p1.o, p1.z

    if p0.z @ p1.z > 1.0 - 0.01 or np.linalg.norm(o0 - o1) < 0.001:
        return None  # parallel or coincident

    t0, t1, center = line_line_closest(o0, n0, o1, n1)

    v0 = o0 - center
    v1 = o1 - center
    n0v = np.linalg.norm(v0)
    n1v = np.linalg.norm(v1)
    if n0v < 1e-12 or n1v < 1e-12:
        return None
    v0 /= n0v
    v1 /= n1v

    bisector = v0 + v1
    bn = np.linalg.norm(bisector)
    if bn < 1e-12:
        return None
    bisector /= bn

    return Plane(anchor, line_dir, bisector)


def polyline_from_planes(base_plane, side_planes):
    """
    Intersect base_plane with each consecutive pair of side_planes.
    Returns closed list of points (last == first).
    """
    n = len(side_planes)
    pts = []
    for i in range(n):
        pt = plane_plane_plane(base_plane, side_planes[i], side_planes[(i + 1) % n])
        pts.append(pt)
    pts.append(pts[0].copy())  # close
    return pts

# ─────────────────────────────────────────────────────────────
# Mesh data structure
# ─────────────────────────────────────────────────────────────

class CMesh:
    def __init__(self, vertices, quads):
        """
        vertices: list of [x,y,z]
        quads:    list of [a,b,c,d] vertex indices (quad faces)
        """
        self.v = [np.array(p, float) for p in vertices]
        self.f = list(quads)   # list of [a,b,c,d]
        self.n_faces = len(self.f)

    def face_normal(self, fi):
        a, b, c, d = [self.v[i] for i in self.f[fi]]
        n = np.cross(b - a, d - a)
        nn = np.linalg.norm(n)
        return n / nn if nn > 1e-12 else np.array([0., 0., 1.])

    def face_center(self, fi):
        pts = [self.v[i] for i in self.f[fi]]
        return sum(pts) / len(pts)

    def edge_normal(self, vi0, vi1):
        """Average face normal for faces sharing edge vi0-vi1."""
        normals = []
        for fi in range(self.n_faces):
            verts = self.f[fi]
            n = len(verts)
            for j in range(n):
                u, v = verts[j], verts[(j+1) % n]
                if (u == vi0 and v == vi1) or (u == vi1 and v == vi0):
                    normals.append(self.face_normal(fi))
                    break
        if not normals:
            return np.array([0., 0., 1.])
        avg = sum(normals) / len(normals)
        nn = np.linalg.norm(avg)
        return avg / nn if nn > 1e-12 else np.array([0., 0., 1.])

    def edge_faces(self, vi0, vi1):
        result = []
        for fi in range(self.n_faces):
            verts = self.f[fi]
            n = len(verts)
            for j in range(n):
                u, v = verts[j], verts[(j+1) % n]
                if (u == vi0 and v == vi1) or (u == vi1 and v == vi0):
                    result.append(fi)
                    break
        return result

# ─────────────────────────────────────────────────────────────
# Chevron plate algorithm
# ─────────────────────────────────────────────────────────────

class ChevronPlates:
    def __init__(self, mesh, edge_rotation=1.0, edge_offset=0.5,
                 box_height=760.0, top_plate_inlet=80.0,
                 plate_thickness=40.0, ortho=True):
        # Flip mesh winding — equivalent to Rhino mesh.Flip(True, True, True)
        # Reverses face vertex order, which flips face normals.
        self.mesh = CMesh(mesh.v, [list(reversed(f)) for f in mesh.f])
        self.edge_rotation = max(min(edge_rotation, 30), -30)
        self.edge_offset    = max(min(edge_offset, 2), -2)
        self.box_height     = max(box_height, 1)
        self.top_plate_inlet = max(min(top_plate_inlet, box_height * 0.333), 1)
        self.plate_thickness = max(min(plate_thickness, box_height * 0.333), 1)
        self.ortho = ortho

        self.f_e             = OrderedDict()   # face_idx → [local_edge_j0, local_edge_j1]
        self.f_rotation_flip = OrderedDict()   # face_idx → bool
        self.e_planes        = []              # [face_idx][local_edge] = Plane
        self.f_planes        = []              # [face_idx] = Plane
        self.f_me            = []              # [face_idx][local_edge] = (vi0, vi1) topology key
        self.bi_planes       = []             # [face_idx][corner_j] = Plane | None
        self.plines          = []              # flat list of polylines

    # ── Phase 1: stripper ────────────────────────────────────

    def _get_mesh_face_edges(self, fi, flag):
        return [0, 1] if flag else [1, 2]

    def _faces_share_strip_edge(self, fi, fj):
        """True if fi and fj share an edge at positions A-B, C-D, B-C, or D-A."""
        a, b, c, d   = self.mesh.f[fi]
        a1, b1, c1, d1 = self.mesh.f[fj]
        return ((a == b1 and b == a1) or
                (c == d1 and d == c1) or
                (c == b1 and d == a1) or
                (b == c1 and a == d1))

    def stripper(self):
        n = self.mesh.n_faces
        flagged = [False] * n
        strip_idx = 0
        f_e_temp = OrderedDict()
        f_rf_temp = OrderedDict()

        while True:
            # find next seed
            seed = next((i for i in range(n) if not flagged[i]), None)
            if seed is None:
                break

            flag = (strip_idx % 2 == 0)
            f_e_temp[seed]  = self._get_mesh_face_edges(seed, flag)
            f_rf_temp[seed] = flag
            flagged[seed] = True
            frontier = [seed]
            flags_done = [False]
            strip_faces = [seed]

            # BFS expansion: keep adding adjacent strip-connected faces
            changed = True
            while changed:
                changed = False
                for qi, fi in enumerate(strip_faces):
                    if flags_done[qi]:
                        continue
                    for fj in range(n):
                        if flagged[fj]:
                            continue
                        if self._faces_share_strip_edge(fi, fj):
                            f_e_temp[fj]  = self._get_mesh_face_edges(fj, flag)
                            f_rf_temp[fj] = flag
                            flagged[fj] = True
                            strip_faces.append(fj)
                            flags_done.append(False)
                            changed = True
                    flags_done[qi] = True

            # for even strips reverse the order
            keys = list(f_e_temp.keys())
            if strip_idx % 2 == 0:
                keys.reverse()

            for k in keys:
                self.f_e[k]             = f_e_temp[k]
                self.f_rotation_flip[k] = f_rf_temp[k]

            f_e_temp.clear()
            f_rf_temp.clear()
            strip_idx += 1

    # ── Phase 2: edge planes ─────────────────────────────────

    def get_edge_planes(self):
        n = self.mesh.n_faces
        self.e_planes = [None] * n
        self.f_planes = [None] * n
        self.f_me     = [None] * n

        for fi in range(n):
            verts = self.mesh.f[fi]
            nv = len(verts)
            self.e_planes[fi] = [None] * nv
            self.f_me[fi]     = [None] * nv

            fn = self.mesh.face_normal(fi)
            fc = self.mesh.face_center(fi)
            ref = np.array([1., 0., 0.]) if abs(fn[0]) < 0.9 else np.array([0., 1., 0.])
            fx = np.cross(ref, fn); fx /= np.linalg.norm(fx)
            self.f_planes[fi] = Plane(fc, fx, np.cross(fn, fx))

            for j in range(nv):
                vi0 = verts[j]
                vi1 = verts[(j + 1) % nv]
                self.f_me[fi][j] = (min(vi0, vi1), max(vi0, vi1))

                mid = (self.mesh.v[vi0] + self.mesh.v[vi1]) * 0.5
                ex  = self.mesh.v[vi0] - self.mesh.v[vi1]
                avg_n = self.mesh.edge_normal(vi0, vi1)
                self.e_planes[fi][j] = Plane(mid, ex, avg_n)

    # ── Phase 3: rotate/offset edge planes ───────────────────

    def _update_neighbor_plane(self, updated_plane, curr_fi, curr_j):
        vi0, vi1 = self.mesh.f[curr_fi][curr_j], self.mesh.f[curr_fi][(curr_j + 1) % 4]
        neighbors = self.mesh.edge_faces(vi0, vi1)
        if len(neighbors) < 2:
            return
        nb = neighbors[0] if neighbors[0] != curr_fi else neighbors[1]
        # find which local edge of nb corresponds to this edge
        nb_verts = self.mesh.f[nb]
        for k in range(4):
            u, v = nb_verts[k], nb_verts[(k + 1) % 4]
            if (min(u,v), max(u,v)) == (min(vi0,vi1), max(vi0,vi1)):
                self.e_planes[nb][k] = updated_plane.flip_x()
                break

    def rotate_planes(self):
        for fi in range(self.mesh.n_faces):
            for j in range(4):
                vi0 = self.mesh.f[fi][j]
                vi1 = self.mesh.f[fi][(j + 1) % 4]
                nb = self.mesh.edge_faces(vi0, vi1)

                if len(nb) == 2:
                    if j in self.f_e[fi]:
                        if j % 2 == 0:
                            self.e_planes[fi][j] = self.e_planes[fi][j].move_along_axis(
                                self.plate_thickness * self.edge_offset)
                            self._update_neighbor_plane(self.e_planes[fi][j], fi, j)
                        else:
                            sign = -1.0 if self.f_rotation_flip[fi] else 1.0
                            sign *= -1.0
                            self.e_planes[fi][j] = self.e_planes[fi][j].rotate_around_y(
                                math.radians(self.edge_rotation * sign))
                            self._update_neighbor_plane(self.e_planes[fi][j], fi, j)
                elif self.ortho:
                    self.e_planes[fi][j] = orthogonalize_plane(self.e_planes[fi][j])

    # ── Phase 4: bisector planes ─────────────────────────────

    def get_bisector_planes(self):
        n = self.mesh.n_faces
        self.bi_planes = [None] * n
        for fi in range(n):
            bi = []
            for j in range(4):
                pl1 = self.e_planes[fi][j]
                pl0 = self.e_planes[fi][(j + 1) % 4]
                bi.append(dihedral_plane(pl0, pl1))
            self.bi_planes[fi] = bi

    # ── Phase 5: get plates ───────────────────────────────────

    def get_plates(self):
        H   = self.box_height
        inp = self.top_plate_inlet
        t   = self.plate_thickness

        for fi, e in self.f_e.items():
            # edge planes with chevron edges offset by +t
            ep_local = []
            for j in range(4):
                pl = self.e_planes[fi][j]
                if j in e:
                    pl = pl.move_along_axis(t)
                ep_local.append(pl)

            fp = self.f_planes[fi]

            # top face plate pair
            self.plines.append(polyline_from_planes(fp.translate_by_normal( H*0.5 - inp - t*0.5), ep_local))
            self.plines.append(polyline_from_planes(fp.translate_by_normal( H*0.5 - inp + t*0.5), ep_local))
            # bottom face plate pair
            self.plines.append(polyline_from_planes(fp.translate_by_normal(-H*0.5 + inp - t*0.5), ep_local))
            self.plines.append(polyline_from_planes(fp.translate_by_normal(-H*0.5 + inp + t*0.5), ep_local))

            # side plates for each chevron edge
            e_sorted = sorted(e)
            if e_sorted[0] == 0 and e_sorted[1] == 3:
                e_sorted.reverse()

            for idx_in_e, curr in enumerate(e_sorted):
                prev = (curr - 1) % 4
                nxt  = (curr + 1) % 4

                s0 = fp.translate_by_normal( H * 0.5)   # top
                s1 = self.e_planes[fi][prev] if idx_in_e == 0 else self.bi_planes[fi][prev]
                s2 = fp.translate_by_normal(-H * 0.5)   # bottom
                s3 = self.e_planes[fi][nxt]  if idx_in_e != 0 else self.bi_planes[fi][curr]

                side_planes = [s0, s1, s2, s3]
                base0 = self.e_planes[fi][curr]
                base1 = base0.move_along_axis(t)

                self.plines.append(polyline_from_planes(base0, side_planes))
                self.plines.append(polyline_from_planes(base1, side_planes))

    def run(self):
        self.stripper()
        self.get_edge_planes()
        self.rotate_planes()
        self.get_bisector_planes()
        self.get_plates()

# ─────────────────────────────────────────────────────────────
# Synthetic test mesh — flat 2-strip × 4-row chevron
# Vertices on a flat z=0 plane, matching a chevron_mesh output
# ─────────────────────────────────────────────────────────────

def make_test_mesh():
    """
    Simple flat quad mesh: 2 u-strips × 4 v-rows = 8 faces.
    Vertex layout:
      Row i, col j -> index i*(cols+1) + j
      cols=2 (u_divisions=2, one midpoint per strip), rows=5
    """
    # 3 columns (u=0, u=0.5, u=1), 5 rows (v=0..4)
    verts = []
    for row in range(5):
        for col in range(3):
            verts.append([col * 100.0, row * 200.0, 0.0])
    # faces: each strip row has 2 faces (left: col 0-1, right: col 1-2)
    # But chevron_mesh produces zigzag: peak at midpoint
    # Simplify: just use a regular 2x4 grid, zigzag the mid column
    # Actually let's match what chevron_mesh would produce for u=2 strips:
    # left quad: [v(r,0), v(r+1,0), v(r+1,1), v(r,1)]   — quads going up
    # right quad: [v(r,1), v(r+1,1), v(r+1,2), v(r,2)]
    quads = []
    for row in range(4):
        def vi(r, c): return r * 3 + c
        # left strip (col 0-1)
        quads.append([vi(row,0), vi(row+1,0), vi(row+1,1), vi(row,1)])
        # right strip (col 1-2)
        quads.append([vi(row,1), vi(row+1,1), vi(row+1,2), vi(row,2)])
    return CMesh(verts, quads)


def pts_to_list(pts):
    return [[round(float(p[0]),6), round(float(p[1]),6), round(float(p[2]),6)]
            for p in pts if p is not None]


if __name__ == "__main__":
    mesh = make_test_mesh()

    cp = ChevronPlates(mesh,
                       edge_rotation=1.0,
                       edge_offset=0.5,
                       box_height=760.0,
                       top_plate_inlet=80.0,
                       plate_thickness=40.0,
                       ortho=True)
    cp.run()

    out = {
        "n_faces": mesh.n_faces,
        "f_e": {str(k): v for k, v in cp.f_e.items()},
        "f_rotation_flip": {str(k): v for k, v in cp.f_rotation_flip.items()},
        "n_plines": len(cp.plines),
        "plines": [pts_to_list(pl) for pl in cp.plines]
    }

    print(json.dumps(out, indent=2))
