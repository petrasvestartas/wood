import sys, os, math, colorsys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from creases import creases

def read(path):
    """Parts (kind, faces), joint centres (point, name) and the uncut plate indices of an export."""
    parts, joints, uncut = [], [], set()
    for line in open(path):
        t = line.split()
        if not t: continue
        if t[0] == "P": parts.append((t[2] if len(t) > 2 else "", []))
        elif t[0] == "F":
            v = list(map(float, t[1:])); parts[-1][1].append([v[i:i + 3] for i in range(0, len(v), 3)])
        elif t[0] == "J": joints.append(([float(t[1]), float(t[2]), float(t[3])], t[4] if len(t) > 4 else ""))
        elif t[0] == "X": uncut.add(int(t[1]))
    return parts, joints, uncut

def ear_clip(f):
    """Triangles of a simple polygon face, clipped ear by ear in its own plane."""
    if len(f) == 3: return [tuple(f)]
    P = np.asarray(f, float)
    n = np.zeros(3)
    for i in range(len(P)): n += np.cross(P[i], P[(i + 1) % len(P)])
    if np.linalg.norm(n) < 1e-12: return []
    n /= np.linalg.norm(n)
    u = np.cross(n, [1.0, 0, 0] if abs(n[0]) < 0.9 else [0, 1.0, 0]); u /= np.linalg.norm(u); v = np.cross(n, u)
    q = [(p @ u, p @ v) for p in P]
    idx = list(range(len(f))); out = []
    cross = lambda a, b, c: (q[b][0] - q[a][0]) * (q[c][1] - q[a][1]) - (q[b][1] - q[a][1]) * (q[c][0] - q[a][0])
    guard = 0
    while len(idx) > 3 and guard < 4 * len(f) ** 2:
        guard += 1
        for k in range(len(idx)):
            a, b, c = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            if cross(a, b, c) <= 1e-12: continue
            if any(cross(a, b, m) >= 0 and cross(b, c, m) >= 0 and cross(c, a, m) >= 0 for m in idx if m not in (a, b, c) and q[m] not in (q[a], q[b], q[c])): continue
            out.append((f[a], f[b], f[c])); idx.pop(k); break
        else:
            break
    if len(idx) == 3: out.append(tuple(f[i] for i in idx))
    return out

def arrays(parts, color, with_lines=True):
    tris, cols, lines = [], [], []
    for i, (kind, faces) in enumerate(parts):
        c = color(i, kind)
        for f in faces:
            for t in ear_clip(f):
                tris.append(t); cols.append(c)
        if with_lines: lines += creases(faces)
    return np.array(tris, float).reshape(-1, 3, 3), np.array(cols, float).reshape(-1, 3), lines

def pastel(i):
    return colorsys.hsv_to_rgb((i * 0.618033 + 0.55) % 1.0, 0.28, 0.93)

def best_view(points, elevs=(20, 35, 55)):
    pts = np.asarray(points, float)[:: max(1, len(points) // 4000)]
    def area(e, a):
        e, a = math.radians(e), math.radians(a)
        x = -pts[:, 0] * math.sin(a) + pts[:, 1] * math.cos(a)
        y = -pts[:, 0] * math.cos(a) * math.sin(e) - pts[:, 1] * math.sin(a) * math.sin(e) + pts[:, 2] * math.cos(e)
        return np.ptp(x) * np.ptp(y)
    return max(((e, a) for e in elevs for a in range(-180, 180, 15)), key=lambda v: area(*v))

def fit(points, view, size):
    """Centre and radius that frame the points in an image of size, seen from view."""
    from zbuffer import axes
    sx, sy, _ = axes(*view)
    pts = np.asarray(points, float)
    x, y = pts @ sx, pts @ sy
    c = pts.mean(axis=0)
    c = c + sx * ((x.max() + x.min()) / 2 - c @ sx) + sy * ((y.max() + y.min()) / 2 - c @ sy)
    W, H = size
    return c, 1.06 * max(np.ptp(x) / 2 * H / W, np.ptp(y) / 2) if W >= H else 1.06 * max(np.ptp(x) / 2, np.ptp(y) / 2 * W / H)

def most_visible(tris, cols, lines, views, frame):
    """The view of views that shows the most crease length, frame(view) giving its centre and radius."""
    from zbuffer import render
    return max(views, key=lambda v: render(tris, cols, lines, v, *frame(v), (220, 210))[1])

def around(view):
    e, a = view
    return [(e, a), (e, a + 180), (-e, a), (-e, a + 180), (e, a + 90), (e, a - 90)]

def solid_pieces(faces):
    """The faces of the connected pieces that enclose volume: the boolean leaves flat slivers where a plate face lies on a box face."""
    key = lambda q: tuple(round(c, 4) for c in q)
    parent = {}
    def find(a):
        while parent.setdefault(a, a) != a: a = parent[a]
        return a
    for f in faces:
        for q in f[1:]: parent[find(key(q))] = find(key(f[0]))
    groups = {}
    for f in faces: groups.setdefault(find(key(f[0])), []).append(f)
    def volume(fs):
        v = 0.0
        for f in fs:
            a = np.array(f[0])
            for i in range(1, len(f) - 1): v += np.dot(a, np.cross(np.array(f[i]), np.array(f[i + 1]))) / 6.0
        return abs(v)
    sizes = {k: volume(fs) for k, fs in groups.items()}
    top = max(sizes.values()) if sizes else 0.0
    return [f for k, fs in groups.items() if sizes[k] > 1e-3 * top for f in fs]
