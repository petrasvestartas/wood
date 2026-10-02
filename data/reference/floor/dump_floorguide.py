"""Dump compas_tf FloorGuide geometry as `name x y z x y z ...` lines, the parity reference for wood_floor.

    python dump_floorguide.py <output> [size_grid_x size_grid_y]

The half spans default to example 1's 3000 / 3000; any other pair is one quarter view of a rectangle
for gate G8 R1 (3000 2400 for quarters 0 and 2 of the 3000 x 2400 bay, 2400 3000 for quarters 1 and 3).
"""

import sys

import compas_tf.floor_guide as fg


class Recorder:
    def __init__(self, top_polyline=None, bottom_polyline=None):
        self.top = top_polyline
        self.bottom = bottom_polyline


fg.PlateElement = Recorder

guide = fg.FloorGuide(
    size_grid_x=float(sys.argv[2]) if len(sys.argv) > 3 else 3000,
    size_grid_y=float(sys.argv[3]) if len(sys.argv) > 3 else 3000,
    size_column_head=220,
    size_column_head_chamfer=120,
    size_outer_ribs=100,
    size_inner_ribs=60,
    size_inner_beams=60,
    size_wedge=240,
    height=650,
    rise=453,
    size_oculus=1000,
)

lines = []


def points(name, pts):
    values = []
    for p in pts:
        values.extend("%.9f" % c for c in (p[0], p[1], p[2]))
    lines.append(name + " " + " ".join(values))


def scalar(name, value):
    lines.append(name + " %.9f" % value)


points("quarter_polygon", guide.quarter_polygon.points)
points("quarter_column_polygon", guide.quarter_column_polygon.points)
points("oculus_points", guide.oculus_points)

for key, pairs in guide.construction_planes.items():
    for i, pair in enumerate(pairs):
        for j, plane in enumerate(pair):
            points(f"planes/{key}/{i}/{j}", [plane.point, plane.normal])

for key, quads in guide.construction_quads.items():
    for i, quad in enumerate(quads):
        points(f"quads/{key}/{i}", quad.points)

for i, offsets in enumerate(guide.boundary_parabolas):
    for j, polyline in enumerate(offsets):
        points(f"parabolas/{i}/{j}", polyline.points)

scalar("block_level_bottom", guide.block_level_bottom)
scalar("block_level_top", guide.block_level_top)

for i, plane in enumerate(guide.bed_top_planes):
    normal = plane.normal if plane.normal[2] >= 0 else plane.normal * -1
    points(f"bed_top_planes/{i}", [plane.point, normal])

for group in ("outer_ribs", "inner_ribs", "inner_beams", "wedges_inner_beams", "tsections", "beds", "oculus", "column_cutters"):
    for i, plate in enumerate(getattr(guide, group)):
        points(f"{group}/{i}/top", plate.top.points)
        points(f"{group}/{i}/bottom", plate.bottom.points)
        if hasattr(plate, "bed_row"):
            scalar(f"{group}/{i}/row", plate.bed_row)

with open(sys.argv[1], "w") as f:
    f.write("\n".join(lines) + "\n")
print(len(lines), "records ->", sys.argv[1])
