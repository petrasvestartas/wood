import math
from collections import defaultdict

def normal(f):
    n = [0.0, 0.0, 0.0]
    for i in range(len(f)):
        a, b = f[i], f[(i + 1) % len(f)]
        n[0] += (a[1] - b[1]) * (a[2] + b[2]); n[1] += (a[2] - b[2]) * (a[0] + b[0]); n[2] += (a[0] - b[0]) * (a[1] + b[1])
    m = math.sqrt(sum(c * c for c in n)) or 1.0
    return [c / m for c in n]

def creases(faces, angle=15.0):
    """Segments of one part's crease edges: where its two faces turn by more than angle, or an edge with one face."""
    key = lambda p: tuple(round(c, 4) for c in p)
    edge_normals = defaultdict(list); ends = {}
    for f in faces:
        n = normal(f)
        for i in range(len(f)):
            a, b = key(f[i]), key(f[(i + 1) % len(f)])
            e = (a, b) if a < b else (b, a)
            edge_normals[e].append(n); ends[e] = (f[i], f[(i + 1) % len(f)])
    limit = math.cos(math.radians(angle))
    out = []
    for e, ns in edge_normals.items():
        if len(ns) == 1 or any(sum(x * y for x, y in zip(ns[0], m)) < limit for m in ns[1:]):
            out.append(ends[e])
    return out
