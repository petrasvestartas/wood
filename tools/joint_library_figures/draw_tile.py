"""The four views of one design's tiles: python draw_tile.py <tiles_dir> <out_dir> <name> [elev azim]"""
import sys, os, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from scene import read, arrays, best_view, fit, solid_pieces
from zbuffer import render

src, out, name = sys.argv[1], sys.argv[2], sys.argv[3]
view = (float(sys.argv[4]), float(sys.argv[5])) if len(sys.argv) > 5 else None
parts, _, _ = read(f"{src}/{name}.txt")

parts = [(kind, solid_pieces(faces) if kind in ("male", "female") else faces) for kind, faces in parts]
head = [l.split() for l in open(f"{src}/{name}.txt") if l.startswith("N ")][0]
WHITE, YELLOW, KEY = (0.97, 0.97, 0.97), (1.0, 0.86, 0.12), (0.9, 0.45, 0.2)
color = lambda kind: WHITE if kind.startswith("male") else KEY if kind == "key" else YELLOW
pick = lambda kinds: [p for p in parts if p[0] in kinds]
size = (560, 560)
panels = [("male tile", pick(["male"])), ("female tile", pick(["female"])), ("both tiles", pick(["male", "female"])), ("on the plates", pick(["male_plate", "female_plate", "key"]))]
tile_pts = arrays(pick(["male", "female"]) or parts, lambda i, k: (1, 1, 1), False)[0].reshape(-1, 3)
if view is None: view = best_view(tile_pts, (25, 35))
c, r = fit(tile_pts, view, size)
fig, axs = plt.subplots(1, 4, figsize=(22, 6.2))
for ax, (title, ps) in zip(axs, panels):
    ax.set_axis_off(); ax.set_title(title, fontsize=12)
    if not ps: continue
    t, col, ln = arrays(ps, lambda i, k, ps=ps: color(ps[i][0]))
    frame = (c, r) if title != "on the plates" else fit(t.reshape(-1, 3), view, size)
    ax.imshow(render(t, col, ln, view, *frame, size, thick=1, ink=0.05)[0])
fig.suptitle(f"{head[1]}: {head[2]}   divisions {head[4]}, shift {float(head[6]):.2f}", fontsize=14)
plt.tight_layout(); plt.savefig(f"{out}/{name}.png", dpi=80)
