import math
import numpy as np

def axes(elev, azim):
    e, a = math.radians(elev), math.radians(azim)
    sx = np.array([-math.sin(a), math.cos(a), 0.0])
    sy = np.array([-math.cos(a) * math.sin(e), -math.sin(a) * math.sin(e), math.cos(e)])
    sd = np.array([math.cos(a) * math.cos(e), math.sin(a) * math.cos(e), math.sin(e)])
    return sx, sy, sd

def render(tris, colors, lines, view, center, radius, size=(800, 800), thick=0, ink=0.35):
    """Orthographic z-buffer image of triangles (N,3,3) with rgb colors (N,3) and crease segments (M,2,3), seen from view (elev, azim)."""
    W, H = size
    sx, sy, sd = axes(*view)
    scale = min(W, H) / (2.0 * radius)
    c = np.asarray(center, float)
    def project(p):
        q = p - c
        return np.stack([W / 2 + (q @ sx) * scale, H / 2 - (q @ sy) * scale, q @ sd], axis=-1)
    image = np.ones((H, W, 3))
    depth = np.full((H, W), -np.inf)
    if len(tris):
        P = project(tris)
        n = np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0])
        n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
        light = 0.55 * sd + 0.45 * (0.6 * sy - 0.8 * sx); light /= np.linalg.norm(light)
        shade = 0.38 + 0.62 * np.abs(n @ light)
        rgb = np.clip(colors * shade[:, None], 0, 1)
        for (a, b, d), col in zip(P, rgb):
            x0, x1 = int(max(min(a[0], b[0], d[0]), 0)), int(min(max(a[0], b[0], d[0]) + 1, W - 1))
            y0, y1 = int(max(min(a[1], b[1], d[1]), 0)), int(min(max(a[1], b[1], d[1]) + 1, H - 1))
            if x0 > x1 or y0 > y1: continue
            area = (b[0] - a[0]) * (d[1] - a[1]) - (b[1] - a[1]) * (d[0] - a[0])
            if abs(area) < 1e-12: continue
            X, Y = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            w0 = ((b[0] - X) * (d[1] - Y) - (b[1] - Y) * (d[0] - X)) / area
            w1 = ((d[0] - X) * (a[1] - Y) - (d[1] - Y) * (a[0] - X)) / area
            w2 = 1.0 - w0 - w1
            inside = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
            if not inside.any(): continue
            z = w0 * a[2] + w1 * b[2] + w2 * d[2]
            region = depth[y0:y1 + 1, x0:x1 + 1]
            win = inside & (z > region)
            region[win] = z[win]
            image[y0:y1 + 1, x0:x1 + 1][win] = col
    visible = 0
    if len(lines):
        L = project(np.asarray(lines, float))
        eps = 0.004 * radius
        for p, q in L:
            steps = int(max(abs(q[0] - p[0]), abs(q[1] - p[1])) * 1.5) + 2
            t = np.linspace(0, 1, steps)[:, None]
            s = p + (q - p) * t
            xi, yi = s[:, 0].astype(int), s[:, 1].astype(int)
            ok = (xi >= 0) & (xi < W) & (yi >= 0) & (yi < H)
            xi, yi, zs = xi[ok], yi[ok], s[ok, 2]
            seen = zs >= depth[yi, xi] - eps
            for dx in range(-thick, thick + 1):
                for dy in range(-thick, thick + 1):
                    ax, ay = np.clip(xi[seen] + dx, 0, W - 1), np.clip(yi[seen] + dy, 0, H - 1)
                    image[ay, ax] = np.minimum(image[ay, ax], ink)
            visible += int(seen.sum())
    return image, visible
