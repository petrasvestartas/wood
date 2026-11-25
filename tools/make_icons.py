"""The icons of the docs, docs/doxygen/icons.svg, one symbol each: the Rhino plugin's own (wood_nano/plugin_rhino/plugin/icons),
their points as they are, and the ones drawn after them for the pages without one: a 24 grid, a black outline one unit wide with
round joins, flat faces in three browns. The fills are custom properties, so wood.css colours them.

    python3 tools/make_icons.py"""

import math
import pathlib

WOOD = pathlib.Path(__file__).resolve().parents[1]
FILLS = {"a": "var(--icon-a, #d09a67)", "b": "var(--icon-b, #b27c4a)", "c": "var(--icon-c, #8a5a31)", "g": "#dbdbdb", "n": "none", "w": "#ffffff"}
ISO = (math.cos(math.radians(30.0)), 0.5)


# ------------------------------------------------------------------ shapes: a fill, then 2D points; "line" is open, "dot" black


def at(x: float, y: float, z: float) -> tuple:
    """A 3D point on the paper, seen from (+, +, +)."""
    return ((x - y) * ISO[0], (x + y) * ISO[1] - z)


def face(fill: str, *points: tuple) -> tuple:
    """A face of 3D points."""
    return (fill, [at(*point) for point in points])


def line(*points: tuple) -> tuple:
    """An open line of 3D points."""
    return ("line", [at(*point) for point in points])


def dot(x: float, y: float, z: float, radius: float) -> tuple:
    """A black dot at a 3D point, its radius on the 24 grid."""
    return ("dot", [at(x, y, z)], radius)


def box(x0: float, x1: float, y0: float, y1: float, z0: float, z1: float) -> list:
    """The three faces seen of a box: the top light, the one towards y mid, the one towards x dark."""
    return [
        face("c", (x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)),
        face("b", (x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)),
        face("a", (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)),
    ]


def extruded(loop: list, y0: float, y1: float, fill: str = "a", sides: tuple = ()) -> list:
    """A loop of (x, z) drawn at y1, with the sides between the given consecutive corners running back to y0."""
    shapes = []
    for i, shade in sides:
        (xa, za), (xb, zb) = loop[i], loop[(i + 1) % len(loop)]
        shapes.append(face(shade, (xa, y0, za), (xb, y0, zb), (xb, y1, zb), (xa, y1, za)))
    return shapes + [face(fill, *((x, y1, z) for x, z in loop))]


def arc(x: float, z: float, radius: float, start: float, end: float, count: int) -> list:
    """Points of an arc in (x, z), degrees."""
    angles = [math.radians(start + (end - start) * k / count) for k in range(count + 1)]
    return [(x + radius * math.cos(angle), z + radius * math.sin(angle)) for angle in angles]


def strip(outer: list, y0: float, y1: float, shades: str) -> list:
    """A strip of faces over a polyline in (x, z), from y0 to y1, its shades in turn."""
    return [
        face(shades[k % len(shades)], (xa, y0, za), (xb, y0, zb), (xb, y1, zb), (xa, y1, za))
        for k, ((xa, za), (xb, zb)) in enumerate(zip(outer, outer[1:]))
    ]


# ------------------------------------------------------------------ the plugin's icons, their points as they are


PLUGIN = {
    "chevron": [("b", "10.2 22.24 2.91 17.57 3.99 2.43 11.86 5.01"), ("a", "19.48 19.89 10.2 22.24 11.86 5.01 21.09 4.08")],
    "connectors": [
        ("line", "2.25 4.52 21 21.86"), ("a", "11.15 16.31 11.72 22.72 18.65 15.06 18.63 7.86"), ("a", "4.52 10.76 5.09 17.17 12.36 9.18 12.14 2.13"),
        ("line", "20.32 21.23 15.08 15.99"), ("line", "11.63 13.19 8.76 10.32"),
    ],
    "diamond_mesh": [
        ("c", "11.98 11.88 12.06 3.06 22.6 1.06"), ("a", "12.07 11.9 11.99 3.09 1.45 1.09"), ("a", "12.1 12.02 11.94 20.84 1.4 22.76"),
        ("c", "12 12 12.01 20.81 22.55 22.89"), ("c", "1.45 1.09 1.4 22.76 11.98 11.88"), ("a", "22.6 1.06 22.55 22.89 12.1 12.02"),
    ],
    "insertion_direction": [
        ("thick", "21.41 8.5 16.42 8.5"), ("thick", "7.91 17.94 2.91 17.94"), ("g", "12.3 4.4 21.06 13.16 12.3 21.92 3.54 13.16"),
        ("dot", "15.83 8.5", 2.19), ("dot", "8.3 17.94", 2.19),
    ],
    "install": [
        ("line", "12.11 3.4 12.11 17.93"), ("line", "7.69 12.08 12.11 17.93 16.59 12.08"),
        ("line", "21.35 15.71 21.35 17.74 20.9 19.3 19.9 20.2 18.5 20.59 5.5 20.59 4.1 20.2 3.1 19.3 2.65 17.74 2.65 15.71"),
    ],
    "joint_type": [
        ("g", "12 3.24 20.76 12 12 20.76 3.24 12"), ("dot", "16.66 7.34", 2.19), ("dot", "7.34 7.34", 2.19), ("dot", "7.34 16.78", 2.19), ("dot", "16.66 16.78", 2.19),
    ],
    "reciprocal_move": [
        ("a", "19.92 3.99 19.96 9.54 2.72 11.86 2.53 6.24"), ("a", "18.75 11.91 18.78 19.37 15.14 9.07 14.96 1.56"),
        ("a", "10.65 12.83 10.68 20.28 7.05 9.99 6.86 2.47"), ("a", "22.17 14.68 22.2 20.23 4.97 22.55 4.78 16.93"),
    ],
    "reciprocal_rotation": [
        ("b", "14.52 8.56 14.4 15.65 3.78 8.23 3.75 1.07"), ("a", "1.39 15.83 1.26 22.93 9.15 12.41 9.12 5.25"), ("a", "22.79 7.29 22.67 14.38 5.51 16.93 5.48 9.77"),
    ],
    "reflex_fold": [("c", "14.38 4.02 8.33 2.06 15.99 16.79 22.5 18.24"), ("a", "9.85 21.94 3.19 20.25 8.11 2.06 14.38 4.02")],
    "solver": [("solid", "0 6.63 6.01 12.73 11.94 6.77 18.01 12.73 24 6.61 24 0 0 0"), ("solid", "0 12.74 6 18.72 12.07 12.78 18 18.71 24 12.73 24 24 0 24")],
    "translation_shell": [
        ("b", "10.91 11.57 2.76 11.09 5.91 2.18 13.9 2.49"), ("a", "20.71 13.14 10.91 11.57 13.9 2.49 23.14 4.39"),
        ("c", "9.36 21.21 1.49 19.83 2.76 11.09 10.91 11.35"), ("a", "19.32 21.71 9.36 21.21 10.91 11.57 20.71 13.14"),
    ],
}


# ------------------------------------------------------------------ the new ones


def flat(*shapes: tuple) -> list:
    """Shapes given in 2D."""
    return list(shapes)


def new_icons() -> dict:
    """Every icon drawn after the plugin's, back to front."""
    icons = {}

    # chapters
    icons["architecture"] = flat(
        ("line", [(12, 8), (12, 12), (5.5, 12), (5.5, 15)]), ("line", [(12, 12), (18.5, 12), (18.5, 15)]),
        ("a", [(8.5, 2), (15.5, 2), (15.5, 8), (8.5, 8)]), ("g", [(2, 15), (9, 15), (9, 21), (2, 21)]), ("g", [(15, 15), (22, 15), (22, 21), (15, 21)]),
    )
    icons["elements"] = box(0, 10, 0, 10, 0, 3) + box(1, 4, 6, 9, 3, 11) + box(5.5, 9, 1, 4.5, 3, 7)
    icons["classes"] = flat(
        ("line", [(8.5, 3), (6.5, 3.4), (5.8, 5), (5.8, 9.6), (5, 11.2), (2.8, 12), (5, 12.8), (5.8, 14.4), (5.8, 19), (6.5, 20.6), (8.5, 21)]),
        ("line", [(15.5, 3), (17.5, 3.4), (18.2, 5), (18.2, 9.6), (19, 11.2), (21.2, 12), (19, 12.8), (18.2, 14.4), (18.2, 19), (17.5, 20.6), (15.5, 21)]),
        ("a", [(12, 8.6), (15.4, 12), (12, 15.4), (8.6, 12)]),
    )

    # nodes, edges, faces
    icons["block"] = box(0, 10, 0, 10, 0, 9)
    icons["support"] = box(0, 10, 0, 10, 0, 1.6) + box(3.8, 6.2, 3.8, 6.2, 1.6, 8) + box(1.5, 8.5, 1.5, 8.5, 8, 9.8)
    icons["edge_straight"] = box(0, 18, 0, 4, 0, 5)
    outer, inner = arc(0, 0, 13, 165, 15, 5), arc(0, 0, 8.6, 15, 165, 5)
    icons["edge_curved"] = [face("c", *((x, z_, y) for x, z_, y in ((inner[0][0], 0, inner[0][1]), (outer[-1][0], 0, outer[-1][1]), (outer[-1][0], 5, outer[-1][1]), (inner[0][0], 5, inner[0][1]))))]
    icons["edge_curved"] += strip(outer, 0, 5, "aaabb") + [face("b", *((x, 5, z) for x, z in outer + inner))]
    icons["edge_variable"] = extruded([(0, 0), (18, 3.5), (18, 7), (0, 9)], 0, 3.5, "b", ((1, "c"), (2, "a")))
    icons["face_planar"] = box(0, 14, 0, 14, 0, 1.6)
    icons["face_curved"] = strip(arc(0, 0, 12, 180, 20, 4), 0, 11, "ccab")
    icons["cut_plane"] = box(0, 12, 0, 4, 0, 5) + [face("g", (9, -4, -4), (9, 8, -4), (15, 8, 9), (15, -4, 9))]
    icons["pin"] = flat(
        ("a", [(3.2, 15.6), (15.4, 3.4), (20.6, 8.6), (8.4, 20.8)]), ("c", [(3.2, 15.6), (2.6, 17.6), (3.6, 19.8), (5.8, 21.2), (8.4, 20.8), (8.8, 19), (7.6, 17), (5.4, 15.6)]),
    )

    # joints of plates
    tenon = [(0, 0), (2.5, 0), (2.5, -2.6), (5, -2.6), (5, 0), (8, 0), (8, -2.6), (10.5, -2.6), (10.5, 0), (13, 0), (13, 9), (0, 9)]
    icons["plate_top_side"] = box(-2, 15, -4, 8, -8.5, -6.2) + [
        face("g", (2.5, 0.6, -6.2), (5, 0.6, -6.2), (5, 3, -6.2), (2.5, 3, -6.2)), face("g", (8, 0.6, -6.2), (10.5, 0.6, -6.2), (10.5, 3, -6.2), (8, 3, -6.2)),
    ] + extruded(tenon, 0.6, 3, "a", ((9, "c"), (10, "b")))
    icons["plate_side_side_inplane"] = flat(
        ("b", [(2, 4), (10, 4), (10, 7.2), (14, 7.2), (14, 10.4), (10, 10.4), (10, 13.6), (14, 13.6), (14, 16.8), (10, 16.8), (10, 20), (2, 20)]),
        ("a", [(22, 4), (10, 4), (10, 7.2), (14, 7.2), (14, 10.4), (10, 10.4), (10, 13.6), (14, 13.6), (14, 16.8), (10, 16.8), (10, 20), (22, 20)]),
    )
    icons["plate_side_side_outplane"] = flat(
        ("a", [(2, 6), (12, 9.5), (12, 12), (14.4, 11.2), (14.4, 14), (12, 14.8), (12, 17.2), (14.4, 16.4), (14.4, 19.2), (12, 22), (2, 18)]),
        ("c", [(22, 6), (12, 9.5), (12, 12), (14.4, 11.2), (14.4, 14), (12, 14.8), (12, 17.2), (14.4, 16.4), (14.4, 19.2), (12, 22), (22, 18)]),
        ("b", [(2, 6), (12, 2), (22, 6), (12, 9.5)]),
    )
    icons["plate_side_side_rotated"] = flat(("b", [(2, 9), (12, 12.6), (12, 21.6), (2, 18)]), ("a", [(8, 3), (21, 2), (22, 12), (12, 12.6)]), ("line", [(12, 12.6), (12, 21.6)]), ("g", [(10.4, 10.2), (13.6, 10.2), (13.6, 15), (10.4, 15)]))
    icons["plate_cross"] = [face("c", (7, -1, 0), (7, 15, 0), (7, 15, 11), (7, -1, 11)), face("a", (0, 7, 0), (7, 7, 0), (7, 7, 11), (0, 7, 11)), face("a", (7, 7, 0), (14, 7, 0), (14, 7, 11), (7, 7, 11)), face("c", (7, 7, 0), (7, 15, 0), (7, 15, 11), (7, 7, 11))]
    icons["plate_top_top"] = box(0, 12, 0, 12, 0, 2) + [line((6, 6, 2), (6, 6, 7))] + box(0, 12, 0, 12, 7, 9) + [dot(3.5, 3.5, 9, 1.1), dot(8.5, 3.5, 9, 1.1), dot(3.5, 8.5, 9, 1.1), dot(8.5, 8.5, 9, 1.1)]
    icons["plate_border"] = [face("c", (0, 0, 0), (13, 0, 0), (13, 13, 0), (0, 13, 0)), face("a", (3, 3, 0), (10, 3, 0), (10, 10, 0), (3, 10, 0)), line((0, 0, 0), (3, 3, 0)), line((13, 0, 0), (10, 3, 0)), line((13, 13, 0), (10, 10, 0)), line((0, 13, 0), (3, 10, 0))]

    # joints of beams
    icons["beam"] = box(0, 4, -9, 4, 0, 4.5) + box(4, 13, 0, 4, 0, 4.5)
    icons["beam_cross"] = box(6, 10, -6, 6, 0, 4) + box(-2, 18, 6, 10, 0, 4) + box(6, 10, 10, 18, 0, 4)
    icons["beam_cross"] = box(-2, 6, 6, 10, 0, 4) + box(6, 10, -2, 18, 0, 4) + box(10, 18, 6, 10, 0, 4)
    icons["beam_top_side"] = box(0, 17, 0, 4.5, 0, 4.5) + box(6, 10, 0.2, 4.3, 4.5, 14)
    icons["beam_bridle"] = box(0, 17, 0, 4.5, 0, 4.5) + box(6, 10, 0.2, 1.4, 4.5, 14) + box(6, 10, 3.1, 4.4, 1.5, 14)
    icons["beam_column_head"] = box(4, 8, 4, 8, -13, -3) + box(-3, 6, 4, 8, -3, 1.5) + box(4, 8, -3, 15, -3, 1.5) + box(8, 15, 4, 8, -3, 1.5)
    icons["beam_compendium"] = flat(
        ("b", [(12, 6), (9, 4), (3, 4), (2, 5), (2, 19.5), (9, 19.5), (12, 21.5)]), ("a", [(12, 6), (15, 4), (21, 4), (22, 5), (22, 19.5), (15, 19.5), (12, 21.5)]),
        ("line", [(5, 8.4), (9, 8.4)]), ("line", [(5, 11.6), (9, 11.6)]), ("line", [(5, 14.8), (9, 14.8)]), ("line", [(14.4, 9.2), (15.9, 13), (17.1, 10.4), (18.3, 13), (19.8, 9.2)]),
    )
    icons["beam_todo"] = flat(("a", [(4, 2.5), (20, 2.5), (20, 21.5), (4, 21.5)]), ("line", [(7, 8), (8.2, 9.3), (10.2, 6.6)]), ("line", [(12.6, 8), (17, 8)]), ("line", [(7, 15.5), (8.2, 16.8), (10.2, 14.1)]), ("line", [(12.6, 15.5), (17, 15.5)]))
    icons["beam_keyed_box"] = box(0, 2, -2, 12, 0, 13) + box(2, 9, 3, 8, 4, 9) + box(9, 11, -2, 12, 0, 13)
    icons["beam_keyed_box"] = box(-5, -3, -1, 11, 0, 12) + box(-3, 3, 3, 8, 3.5, 8.5) + [face("g", (3, 4.4, 8.5), (3, 6.6, 8.5), (3, 6.6, 10.2), (3, 4.4, 10.2))]
    icons["beam_end_to_end"] = extruded([(0, 0), (6.5, 0), (12.5, 5), (0, 5)], 0, 4.5, "b", ((2, "a"),)) + extruded([(7.8, 0), (19, 0), (19, 5), (13.8, 5)], 0, 4.5, "b", ((1, "c"), (2, "a")))
    icons["beam_butt"] = box(0, 4, -7, 11, 0, 5) + box(4, 15, 0, 4, 0, 5)
    icons["beam_pins"] = box(0, 17, 0, 5, 0, 5) + [dot(4, 5, 2.5, 1.2), dot(8.5, 5, 2.5, 1.2), dot(13, 5, 2.5, 1.2)]

    # templates
    icons["cylindrical_folding"] = strip([(-11, 0), (-9, 6), (-4.5, 10), (0, 11), (4.5, 10), (9, 6), (11, 0)], 0, 5.5, "cb") + strip([(-11, 0), (-9, 6), (-4.5, 10), (0, 11), (4.5, 10), (9, 6), (11, 0)], 5.5, 11, "ba")
    icons["developable_folding"] = flat(
        ("c", [(12, 2.5), (14.5, 7), (15, 12), (14, 17), (12, 21.5), (21.5, 18), (22.5, 12), (21, 6)]), ("a", [(12, 2.5), (14.5, 7), (15, 12), (14, 17), (12, 21.5), (3, 19.5), (5.6, 15), (6.4, 11), (5.4, 7.6), (3, 4.5)]),
    )
    icons["cross"] = flat(
        ("c", [(3, 6), (9, 2.5), (21.5, 16.4), (15.5, 20)]), ("a", [(15, 2), (21, 5.6), (9, 21.6), (3, 18)]), ("b", [(12.2, 5.8), (17.4, 10.4), (12, 17.6), (6.8, 12.9)]),
    )
    icons["module"] = [face("b", (0, 12, 0), (12, 12, 0), (12, 12, 8), (6, 12, 13.5), (0, 12, 8)), face("c", (12, 0, 0), (12, 12, 0), (12, 12, 8), (12, 0, 8)), face("a", (6, 0, 13.5), (6, 12, 13.5), (12, 12, 8), (12, 0, 8)), face("g", (4.2, 12, 0), (7.8, 12, 0), (7.8, 12, 5.6), (4.2, 12, 5.6))]
    frame = []
    for y in (0, 10):
        for x in (0, 10):
            frame += box(x, x + 1.8, y, y + 1.8, 0, 8)
    icons["grid"] = frame[:3] + box(0, 1.8, 1.8, 10, 8, 10) + frame[3:6] + frame[6:] + box(0, 11.8, 0, 1.8, 8, 10)[2:] + box(0, 11.8, 10, 11.8, 8, 10) + box(10, 11.8, 0, 10, 8, 10)
    legs = box(0, 1.8, 0, 1.8, 0, 9) + box(12, 13.8, 0, 1.8, 0, 9) + box(0, 1.8, 12, 13.8, 0, 9) + box(12, 13.8, 12, 13.8, 0, 9)
    icons["floor"] = legs + box(0, 13.8, 0, 13.8, 9, 11) + [face("c", (6.9, 3.9, 11), (9.9, 6.9, 11), (6.9, 9.9, 11), (3.9, 6.9, 11)), line((0, 0, 11), (5.4, 5.4, 11)), line((13.8, 13.8, 11), (8.4, 8.4, 11)), line((13.8, 0, 11), (8.4, 5.4, 11)), line((0, 13.8, 11), (5.4, 8.4, 11))]
    icons["truss"] = flat(
        ("a", [(1.5, 18), (22.5, 18), (22.5, 20.8), (1.5, 20.8)]), ("b", [(1.5, 18), (12, 4.2), (12, 8.2), (4.6, 18)]), ("c", [(22.5, 18), (12, 4.2), (12, 8.2), (19.4, 18)]),
        ("a", [(10.9, 8.6), (13.1, 8.6), (13.1, 18), (10.9, 18)]), ("line", [(11, 18), (7.6, 13.6)]), ("line", [(13, 18), (16.4, 13.6)]),
    )
    ring, hole = arc(12, -20, 10.5, 0, 180, 5), arc(12, -20, 5.6, 0, 180, 5)
    stones = [("abcba"[k], [(ring[k][0], -ring[k][1]), (ring[k + 1][0], -ring[k + 1][1]), (hole[k + 1][0], -hole[k + 1][1]), (hole[k][0], -hole[k][1])]) for k in range(5)]
    icons["vault"] = stones + [("c", [(1.5, 20), (6.4, 20), (6.4, 27), (1.5, 27)]), ("b", [(17.6, 20), (22.5, 20), (22.5, 27), (17.6, 27)])]
    icons["plaited"] = flat(
        ("b", [(2, 5), (22, 5), (22, 9.4), (2, 9.4)]), ("b", [(2, 14.6), (22, 14.6), (22, 19), (2, 19)]), ("a", [(5, 2), (9.4, 2), (9.4, 22), (5, 22)]), ("a", [(14.6, 2), (19, 2), (19, 22), (14.6, 22)]),
        ("b", [(12, 5), (22, 5), (22, 9.4), (12, 9.4)]), ("b", [(2, 14.6), (12, 14.6), (12, 19), (2, 19)]),
    )
    icons["vidy"] = strip([(-9, 0), (-10, 7), (0, 12), (10, 7), (9, 0)], 0, 4, "cbac") + strip([(-9, 0), (-10, 7), (0, 12), (10, 7), (9, 0)], 4, 8, "cbab") + [face("a", (-9, 8, 0), (-10, 8, 7), (0, 8, 12), (10, 8, 7), (9, 8, 0), (7, 8, 0), (7.8, 8, 5.8), (0, 8, 9.6), (-7.8, 8, 5.8), (-7, 8, 0))]
    return icons


# ------------------------------------------------------------------ the sprite


def fitted(shapes: list) -> list:
    """Shapes scaled and centred into the grid, a unit and a half clear of its edge."""
    points = [point for shape in shapes for point in shape[1]]
    low = [min(point[k] for point in points) for k in (0, 1)]
    high = [max(point[k] for point in points) for k in (0, 1)]
    scale = 21.0 / max(high[0] - low[0], high[1] - low[1])
    return [(shape[0], [tuple(12.0 + (point[k] - (low[k] + high[k]) / 2.0) * scale for k in (0, 1)) for point in shape[1]], *shape[2:]) for shape in shapes]


def drawn(shape: tuple) -> str:
    """A shape as SVG."""
    points = shape[1]
    if isinstance(points, str):
        numbers = [float(value) for value in points.split()]
        points = list(zip(numbers[::2], numbers[1::2]))
    if shape[0] == "dot":
        return f'<circle cx="{points[0][0]:.2f}" cy="{points[0][1]:.2f}" r="{shape[2]}" fill="#000" stroke="none"/>'
    listed = " ".join(f"{x:.2f} {y:.2f}" for x, y in points)
    if shape[0] == "line":
        return f'<polyline fill="none" points="{listed}"/>'
    if shape[0] == "thick":
        return f'<polyline fill="none" stroke-width="2" points="{listed}"/>'
    if shape[0] == "solid":
        return f'<polygon fill="#000" stroke="none" points="{listed}"/>'
    return f'<polygon fill="{FILLS[shape[0]]}" points="{listed}"/>'


def main() -> None:
    """Writes the sprite."""
    icons = {**PLUGIN, **{name: fitted(shapes) for name, shapes in new_icons().items()}}
    symbols = []
    for name, shapes in sorted(icons.items()):
        inside = "".join(drawn(shape) for shape in shapes)
        symbols.append(f'<symbol id="{name}" viewBox="0 0 24 24"><g stroke="#000" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">{inside}</g></symbol>')
    (WOOD / "docs/doxygen/icons.svg").write_text('<svg xmlns="http://www.w3.org/2000/svg">\n' + "\n".join(symbols) + "\n</svg>\n")
    print(len(icons), "icons")


if __name__ == "__main__":
    main()
