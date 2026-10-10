"""A parameter sweep as a grid of tile pairs: python draw_sweep.py src out.png "title" ncols label=id ..."""
import sys, os, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from scene import read, arrays, best_view, fit, solid_pieces
from zbuffer import render

src, out, title, ncols = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
cells = [(a.split("|", 1)[0].replace("_", " "), a.split("|", 1)[1]) for a in sys.argv[5:]]
WHITE, YELLOW = (0.97, 0.97, 0.97), (1.0, 0.86, 0.12)
rows = (len(cells) + ncols - 1) // ncols
fig, axs = plt.subplots(rows, ncols, figsize=(4.6 * ncols, 4.0 * rows + 0.5))
axs = np.atleast_1d(axs).reshape(rows, ncols)
view = None
for ax, (label, name) in zip(axs.flat, cells):
    ax.set_axis_off(); ax.set_title(label, fontsize=12)
    path = f"{src}/{name.replace('/', '_')}.txt"
    if not os.path.exists(path):
        ax.text(0.5, 0.5, "not built\n(the outline crosses itself)", ha="center", va="center", transform=ax.transAxes); continue
    parts, _, _ = read(path)
    kinds = {"male": ("male",), "key": ("male", "key")}.get(os.environ.get("MODE", ""), ("male", "female"))
    ps = [(k, solid_pieces(f) if k != "key" else f) for k, f in parts if k in kinds]
    if not any(f for _, f in ps):
        ps = [(k, f) for k, f in parts if k in ("male_plate", "female_plate")]
    t, c, l = arrays(ps, lambda i, k: WHITE if k.startswith("male") else (0.9, 0.45, 0.2) if k == "key" else YELLOW)
    pts = t.reshape(-1, 3)
    if not len(pts):
        ax.text(0.5, 0.5, "empty tile", ha="center", va="center", transform=ax.transAxes); continue
    if view is None: view = best_view(pts, (25, 35))
    ax.imshow(render(t, c, l, view, *fit(pts, view, (420, 380)), (420, 380), thick=1, ink=0.05)[0])
for ax in axs.flat[len(cells):]: ax.set_axis_off()
fig.suptitle(title + {"male": "   (male tile)", "key": "   (male tile white, the key orange)"}.get(os.environ.get("MODE", ""), "   (male tile white, female tile yellow)"), fontsize=14)
plt.tight_layout(); plt.savefig(out, dpi=80)
